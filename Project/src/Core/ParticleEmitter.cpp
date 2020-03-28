#include "ParticleEmitter.h"

#include "glm/gtx/vector_angle.hpp"

#include "Common/IBuffer.h"
#include "Common/IMesh.h"

#include <algorithm>
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
    m_pDescriptorSetCompute(nullptr),
    m_pDescriptorSetRender(nullptr),
    m_pPositionsBuffer(nullptr),
    m_pVelocitiesBuffer(nullptr),
    m_pAgesBuffer(nullptr),
    m_pEmitterBuffer(nullptr)
{
    createCenteringQuaternion();
}

ParticleEmitter::~ParticleEmitter()
{
    SAFEDELETE(m_pPositionsBuffer);
    SAFEDELETE(m_pVelocitiesBuffer);
    SAFEDELETE(m_pAgesBuffer);
    SAFEDELETE(m_pEmitterBuffer);
}

bool ParticleEmitter::initialize(IGraphicsContext* pGraphicsContext)
{
    size_t particleCount = m_ParticlesPerSecond * m_ParticleDuration;
    resizeParticleStorage(particleCount);

    return createBuffers(pGraphicsContext);
}

void ParticleEmitter::update(float dt)
{
    ageEmitter(dt);
    moveParticles(dt);
    respawnOldParticles();
}

void ParticleEmitter::updateGPU(float dt)
{
    ageEmitter(dt);
    // The rest is performed by the particle emitter handler
}

void ParticleEmitter::createEmitterBuffer(EmitterBuffer& emitterBuffer)
{
    emitterBuffer.position = glm::vec4(m_Position, 1.0f);
    emitterBuffer.direction = glm::vec4(m_Direction, 0.0f);
    emitterBuffer.particleSize = m_ParticleSize;
    emitterBuffer.centeringRotMatrix = glm::mat4_cast(m_CenteringRotQuat);
    emitterBuffer.particleDuration = m_ParticleDuration;
    emitterBuffer.initialSpeed = m_InitialSpeed;
    emitterBuffer.spread = m_Spread;
    emitterBuffer.particleCount = getParticleCount();
}

uint32_t ParticleEmitter::getParticleCount() const
{
    return uint32_t(m_ParticlesPerSecond * std::min(m_ParticleDuration, m_EmitterAge));
}

void ParticleEmitter::setPosition(const glm::vec3& position)
{
    m_Position = position;
    m_EmitterUpdated = true;
}

void ParticleEmitter::setDirection(const glm::vec3& direction)
{
    m_Direction = direction;
    createCenteringQuaternion();
    m_EmitterUpdated = true;
}

void ParticleEmitter::setParticleSize(const glm::vec2& size)
{
    m_ParticleSize = size;
    m_EmitterUpdated = true;
}

void ParticleEmitter::setInitialSpeed(float initialSpeed)
{
    m_InitialSpeed = initialSpeed;
    m_EmitterUpdated = true;
}

void ParticleEmitter::setParticlesPerSecond(float particlesPerSecond)
{
    size_t newParticleCount = m_ParticlesPerSecond * m_ParticleDuration;
    resizeParticleStorage(newParticleCount);

    m_ParticlesPerSecond = particlesPerSecond;
    m_EmitterUpdated = true;
}

void ParticleEmitter::setParticleDuration(float particleDuration)
{
    size_t newParticleCount = m_ParticlesPerSecond * m_ParticleDuration;
    resizeParticleStorage(newParticleCount);

    m_ParticleDuration = particleDuration;
    m_EmitterUpdated = true;
}

void ParticleEmitter::setSpread(float spread)
{
    m_ZRandomizer = std::uniform_real_distribution<float>(std::cos(spread), 1.0f);
    m_Spread = spread;
    m_EmitterUpdated = true;
}

bool ParticleEmitter::createBuffers(IGraphicsContext* pGraphicsContext)
{
    size_t particleCount = m_ParticlesPerSecond * m_ParticleDuration;

    // Create particle buffers
    BufferParams bufferParams = {};
    bufferParams.IsExclusive = true;
    bufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bufferParams.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferParams.SizeInBytes = particleCount * sizeof(glm::vec4);

    m_pPositionsBuffer = pGraphicsContext->createBuffer();
    if (!m_pPositionsBuffer->init(bufferParams)) {
        LOG("Failed to create particle positions buffer");
        return false;
    }
    else
    {
        m_pPositionsBuffer->setName("Position Buffer");
    }

    m_pVelocitiesBuffer = pGraphicsContext->createBuffer();
    if (!m_pVelocitiesBuffer->init(bufferParams)) {
        LOG("Failed to create particle velocities buffer");
        return false;
    }
    else
    {
        m_pVelocitiesBuffer->setName("Velocity Buffer");
    }

    bufferParams.SizeInBytes = particleCount * sizeof(float);

    m_pAgesBuffer = pGraphicsContext->createBuffer();
    if (!m_pAgesBuffer->init(bufferParams)) {
        LOG("Failed to create particle ages buffer");
        return false;
    }
    else
    {
        m_pAgesBuffer->setName("Age Buffer");
    }

    // Create emitter buffer
    bufferParams.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferParams.SizeInBytes = sizeof(EmitterBuffer);

    m_pEmitterBuffer = pGraphicsContext->createBuffer();
    if (!m_pEmitterBuffer->init(bufferParams)) {
        LOG("Failed to create emitter buffer");
        return false;
    }
    else
    {
        m_pEmitterBuffer->setName("Emitter Buffer");
    }

    // Put initial data into emitter buffer
    EmitterBuffer emitterBuffer = {};
    createEmitterBuffer(emitterBuffer);
    pGraphicsContext->updateBuffer(m_pEmitterBuffer, 0, &emitterBuffer, sizeof(EmitterBuffer));
}

void ParticleEmitter::ageEmitter(float dt)
{
    if (m_EmitterAge < m_ParticleDuration) {
        uint32_t oldParticleCount = getParticleCount();
        m_EmitterAge += dt;
        uint32_t newParticleCount = getParticleCount();

        if (newParticleCount > oldParticleCount) {
            m_EmitterUpdated = true;
        }
    }
}

void ParticleEmitter::moveParticles(float dt)
{
    std::vector<glm::vec4>& positions = m_ParticleStorage.positions;
    std::vector<glm::vec4>& velocities = m_ParticleStorage.velocities;
    std::vector<float>& ages = m_ParticleStorage.ages;

    size_t particleCount = getParticleCount();

    for (size_t particleIdx = 0; particleIdx < particleCount; particleIdx++) {
        positions[particleIdx] += velocities[particleIdx] * dt;
        velocities[particleIdx].y -= 9.82f * dt;
        ages[particleIdx] += dt;
    }
}

void ParticleEmitter::respawnOldParticles()
{
    std::vector<glm::vec4>& positions = m_ParticleStorage.positions;
    std::vector<glm::vec4>& velocities = m_ParticleStorage.velocities;
    std::vector<float>& ages = m_ParticleStorage.ages;

    size_t particleCount = getParticleCount();

    /*
        TODO: Optimize respawning, dead pixels are grouped up, no need to iterate through the entire particle storage
        Three cases of particles grouping up: (d=dead, a=alive)
        (front) daaaaaadd (back)
        (front) aaaaaaddd (back)
        (front) dddaaaaaa (back)
    */
    for (size_t particleIdx = 0; particleIdx < particleCount; particleIdx++) {
        float newParticleAge = ages[particleIdx] - m_ParticleDuration;
        if (newParticleAge < 0.0f) {
            continue;
        }

        createParticle(particleIdx, newParticleAge);
    }
}

void ParticleEmitter::createParticle(size_t particleIdx, float particleAge)
{
    glm::vec3 particleDirection = m_Direction;

    // Randomized unit vector within a cone based on https://math.stackexchange.com/a/205589
    float z = m_ZRandomizer(m_RandEngine);
    float phi = m_PhiRandomizer(m_RandEngine);

    float sqrtZInv = std::sqrt(1.0f - z * z);

    // Randomized vector given that the cone is centered around (0,0,1)
    glm::vec3 randVec(sqrtZInv * std::cos(phi), sqrtZInv * std::sin(phi), z);

    if (m_Direction == m_ZVec) {
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
    m_ParticleStorage.positions[particleIdx] = glm::vec4(glm::vec3(0.0f, gt * particleAge * 0.5f, 0.0f) + V0 * particleAge + m_Position, 1.0f);
    m_ParticleStorage.ages[particleIdx] = particleAge;
}

void ParticleEmitter::createCenteringQuaternion()
{
    glm::vec3 axis = glm::normalize(glm::cross(m_ZVec, m_Direction));

    float angle = glm::angle(m_ZVec, m_Direction);
    m_CenteringRotQuat = glm::angleAxis(angle, axis);
}

void ParticleEmitter::resizeParticleStorage(size_t newSize)
{
    size_t oldSize = m_ParticleStorage.positions.size();

    m_ParticleStorage.positions.resize(newSize);
    m_ParticleStorage.velocities.resize(newSize);
    m_ParticleStorage.ages.resize(newSize);

    // Set the particle ages so that the first particle will respawn after the first update
    float spawnRateReciprocal = 1.0f / m_ParticlesPerSecond;
    std::vector<float>& ages = m_ParticleStorage.ages;

    for (size_t i = oldSize; i < newSize; i++) {
        ages[i] = m_ParticleDuration - i * spawnRateReciprocal;
    }
}
