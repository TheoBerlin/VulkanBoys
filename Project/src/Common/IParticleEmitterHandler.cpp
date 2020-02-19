#include "IParticleEmitterHandler.h"

#include "Common/Debug.h"
#include "Common/IGraphicsContext.h"
#include "Common/IShader.h"

IParticleEmitterHandler::IParticleEmitterHandler()
    :m_pGraphicsContext(nullptr)
{}

IParticleEmitterHandler::~IParticleEmitterHandler()
{
	for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
		SAFEDELETE(pEmitter);
	}
}

void IParticleEmitterHandler::update(float dt)
{
    for (ParticleEmitter* particleEmitter : m_ParticleEmitters) {
        particleEmitter->update(dt);
    }
}

void IParticleEmitterHandler::initialize(IGraphicsContext* pGraphicsContext, const Camera* pCamera)
{
    m_pGraphicsContext = pGraphicsContext;

    // Initialize all emitters
    for (ParticleEmitter* particleEmitter : m_ParticleEmitters) {
        particleEmitter->initialize(m_pGraphicsContext, pCamera);
    }
}

ParticleEmitter* IParticleEmitterHandler::createEmitter(const ParticleEmitterInfo& emitterInfo)
{
	ParticleEmitter* pNewEmitter = DBG_NEW ParticleEmitter(emitterInfo);
    m_ParticleEmitters.push_back(pNewEmitter);
    if (m_pGraphicsContext != nullptr) {
        pNewEmitter->initialize(m_pGraphicsContext, m_pCamera);
    }

	return pNewEmitter;
}
