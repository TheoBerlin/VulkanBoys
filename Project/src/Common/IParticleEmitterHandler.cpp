#include "IParticleEmitterHandler.h"

#include "Common/Debug.h"
#include "Common/IGraphicsContext.h"
#include "Common/IShader.h"

IParticleEmitterHandler::IParticleEmitterHandler()
    :m_pGraphicsContext(nullptr),
    m_GPUComputed(false)
{}

IParticleEmitterHandler::~IParticleEmitterHandler()
{
	for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
		SAFEDELETE(pEmitter);
	}
}

void IParticleEmitterHandler::initialize(IGraphicsContext* pGraphicsContext, const Camera* pCamera)
{
    m_pGraphicsContext = pGraphicsContext;

    if (!initializeGPUCompute()) {
        LOG("Failed to initialize Particle Emitter Handler GPU compute resources");
        return;
    }

    // Initialize all emitters
    for (ParticleEmitter* particleEmitter : m_ParticleEmitters) {
        initializeEmitter(particleEmitter);
    }
}

ParticleEmitter* IParticleEmitterHandler::createEmitter(const ParticleEmitterInfo& emitterInfo)
{
	ParticleEmitter* pNewEmitter = DBG_NEW ParticleEmitter(emitterInfo);
    m_ParticleEmitters.push_back(pNewEmitter);
    if (m_pGraphicsContext != nullptr) {
        initializeEmitter(pNewEmitter);
    }

	return pNewEmitter;
}
