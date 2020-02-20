#include "ParticleEmitter.h"

#include "glm/gtx/vector_angle.hpp"

#include "Common/IBuffer.h"
#include "Common/IMesh.h"

#include <math.h>

ParticleEmitter::ParticleEmitter(const ParticleEmitterInfo& emitterInfo)
    :m_Position(glm::vec4(emitterInfo.position, 0.0f)),
    m_Direction(glm::normalize(glm::vec4(emitterInfo.direction, 0.0f))),
    m_ParticleSize(emitterInfo.particleSize),
    m_ParticleDuration(emitterInfo.particleDuration),
    m_InitialSpeed(emitterInfo.initialSpeed),
    m_ParticlesPerSecond(emitterInfo.particlesPerSecond),
    m_Spread(emitterInfo.spread),
    m_pTexture(emitterInfo.pTexture),
    m_EmitterUpdated(false),
    m_EmitterAge(0.0f),
    m_RandEngine(std::random_device()()),
    m_ZRandomizer(std::cos(m_Spread), 1.0f),
    m_PhiRandomizer(0.0f, glm::two_pi<float>()),
    m_pParticleBuffer(nullptr),
    m_pEmitterBuffer(nullptr)
{
    // Calculate rotation quaternion
    glm::vec3 axis = glm::normalize(glm::cross(m_ZVec, m_Direction));

    float angle = glm::angle(m_ZVec, m_Direction);
    m_CenteringRotQuat = glm::angleAxis(angle, axis);
}

ParticleEmitter::~ParticleEmitter()
{
    SAFEDELETE(m_pParticleBuffer);
    SAFEDELETE(m_pEmitterBuffer);
}

bool ParticleEmitter::initializeCPU(IGraphicsContext* pGraphicsContext, const Camera* pCamera)
{
    m_pCamera = pCamera;

    size_t particleCount = m_ParticlesPerSecond * m_ParticleDuration;
    m_ParticleStorage.positions.reserve(particleCount);
    m_ParticleStorage.velocities.reserve(particleCount);
    m_ParticleStorage.ages.reserve(particleCount);

    return createBuffers(pGraphicsContext);
}

void ParticleEmitter::updateCPU(float dt)
{
    size_t maxParticleCount = m_ParticlesPerSecond * m_ParticleDuration;
    if (m_ParticleStorage.positions.size() < maxParticleCount) {
        m_EmitterAge += dt;
        spawnNewParticlesCPU();
    }

    moveParticlesCPU(dt);

    respawnOldParticlesCPU();
}

bool ParticleEmitter::createBuffers(IGraphicsContext* pGraphicsContext)
{
    size_t particleCount = m_ParticlesPerSecond * m_ParticleDuration;

    // Create particle buffer
    BufferParams bufferParams = {};
    bufferParams.IsExclusive = true;
    bufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bufferParams.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferParams.SizeInBytes = particleCount * sizeof(glm::vec4);

    m_pParticleBuffer = pGraphicsContext->createBuffer();
    if (!m_pParticleBuffer->init(bufferParams)) {
        LOG("Failed to create particle buffer");
        return false;
    }

    // Create emitter buffer
    bufferParams.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferParams.SizeInBytes = sizeof(glm::vec2);

    m_pEmitterBuffer = pGraphicsContext->createBuffer();
    if (!m_pEmitterBuffer->init(bufferParams)) {
        LOG("Failed to create emitter buffer");
        return false;
    }

    // Put initial data into emitter buffer
    pGraphicsContext->updateBuffer(m_pEmitterBuffer, 0, &m_ParticleSize, sizeof(glm::vec2));
}

void ParticleEmitter::spawnNewParticlesCPU()
{
    size_t particleCount = m_ParticleStorage.positions.size();
    size_t newParticleCount = m_EmitterAge * m_ParticlesPerSecond;
    size_t particlesToSpawn = newParticleCount - particleCount;

    if (particlesToSpawn == 0) {
        return;
    }

    // Create uninitialized particles where createParticle() will fill in the data
    m_ParticleStorage.positions.resize(newParticleCount);
    m_ParticleStorage.velocities.resize(newParticleCount);
    m_ParticleStorage.ages.resize(newParticleCount);

    for (size_t i = 0; i < particlesToSpawn; i++) {
        // Exact time when the particle should have spawned, relative to the creation of the emitter
        float spawnTime = (particleCount + 1 + i) / m_ParticlesPerSecond;
        float particleAge = m_EmitterAge - spawnTime;

        size_t newParticleIdx = particleCount + i;
        createParticleCPU(newParticleIdx, particleAge);
    }
}

void ParticleEmitter::moveParticlesCPU(float dt)
{
    std::vector<glm::vec4>& positions = m_ParticleStorage.positions;
    std::vector<glm::vec4>& velocities = m_ParticleStorage.velocities;
    std::vector<float>& ages = m_ParticleStorage.ages;

    for (size_t particleIdx = 0; particleIdx < positions.size(); particleIdx++) {
        positions[particleIdx] += velocities[particleIdx] * dt;
        velocities[particleIdx].y -= 9.82f * dt;
        ages[particleIdx] += dt;
    }
}

void ParticleEmitter::respawnOldParticlesCPU()
{
    std::vector<glm::vec4>& positions = m_ParticleStorage.positions;
    std::vector<glm::vec4>& velocities = m_ParticleStorage.velocities;
    std::vector<float>& ages = m_ParticleStorage.ages;

    /*
        TODO: Optimize respawning, dead pixels are grouped up, no need to iterate through the entire particle storage
        Three cases of particles grouping up: (d=dead, a=alive)
        (front) daaaaaadd (back)
        (front) aaaaaaddd (back)
        (front) dddaaaaaa (back)
    */
    for (size_t particleIdx = 0; particleIdx < m_ParticleStorage.positions.size(); particleIdx++) {
        float newParticleAge = ages[particleIdx] - m_ParticleDuration;
        if (newParticleAge < 0.0f) {
            continue;
        }

        createParticleCPU(particleIdx, newParticleAge);
    }
}

void ParticleEmitter::createParticleCPU(size_t particleIdx, float particleAge)
{
    glm::vec3 particleDirection = m_Direction;

    // Randomized unit vector within a cone based on https://math.stackexchange.com/a/205589
    float minZ = std::cos(m_Spread);
    float z = m_ZRandomizer(m_RandEngine);
    float phi = m_PhiRandomizer(m_RandEngine);

    float sqrtZInv = std::sqrt(1.0f - z * z);

    // Randomized vector given that the cone is centered around (0,0,1)
    glm::vec3 randVec(sqrtZInv * std::cos(phi), sqrtZInv * std::sin(phi), z);
    const glm::vec3 zVec(0.0f, 0.0f, 1.0f);

    if (m_Direction == zVec) {
        particleDirection = randVec;
    } else {
        // Rotate the random vector so that the center of the cone is aligned with the emitter
        particleDirection = m_CenteringRotQuat * randVec;
    }

    /*
        a = (0, -g, 0)
        v = (0, -gt, 0) + V0
        p = (0, -(gt^2)/2, 0) + V0*t + P0
    */

    float gt = -9.82f * particleAge;
    glm::vec3 V0 = particleDirection * m_InitialSpeed;
    m_ParticleStorage.velocities[particleIdx] = glm::vec4(glm::vec3(0.0f, gt, 0.0f) + V0, 0.0f);
    m_ParticleStorage.positions[particleIdx] = glm::vec4(glm::vec3(0.0f, gt * particleAge / 2.0f, 0.0f) + V0 * particleAge + m_Position, 1.0f);
    m_ParticleStorage.ages[particleIdx] = particleAge;
}
