#include "ParticleEmitterHandler.hpp"

// DOESN'T EXIST: #include "Common/IPipeline.h"
#include "Common/Debug.h"
#include "Common/IGraphicsContext.h"
#include "Common/IShader.h"

#include "ParticleEmitter.hpp"

ParticleEmitterHandler::ParticleEmitterHandler()
    :m_pGraphicsContext(nullptr)
{}

ParticleEmitterHandler::~ParticleEmitterHandler()
{
	for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
		SAFEDELETE(pEmitter);
	}
}

void ParticleEmitterHandler::initialize(IGraphicsContext* pGraphicsContext)
{
    m_pGraphicsContext = pGraphicsContext;

    // TODO: Create quad vertex buffer, either here or in GraphicsContext

    // Initialize all emitters
    for (ParticleEmitter* particleEmitter : m_ParticleEmitters) {
        particleEmitter->initialize(m_pGraphicsContext);
    }
}

ParticleEmitter* ParticleEmitterHandler::createEmitter(const ParticleEmitterInfo& emitterInfo)
{
	ParticleEmitter* pNewEmitter = DBG_NEW ParticleEmitter(emitterInfo.position, emitterInfo.direction, emitterInfo.particleDuration, emitterInfo.initialSpeed, emitterInfo.particlesPerSecond);
    m_ParticleEmitters.push_back(pNewEmitter);
    if (m_pGraphicsContext != nullptr) {
        pNewEmitter->initialize(m_pGraphicsContext);
    }

	return pNewEmitter;
}
