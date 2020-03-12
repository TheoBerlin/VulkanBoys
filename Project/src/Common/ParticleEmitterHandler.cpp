#include "ParticleEmitterHandler.h"

#include "Common/Debug.h"
#include "Common/IGraphicsContext.h"
#include "Common/IShader.h"

ParticleEmitterHandler::ParticleEmitterHandler()
    : m_pGraphicsContext(nullptr),
    m_GPUComputed(false),
    m_CollisionsEnabled(false)
{
}

ParticleEmitterHandler::~ParticleEmitterHandler()
{
	for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
		SAFEDELETE(pEmitter);
	}
}

void ParticleEmitterHandler::initialize(IGraphicsContext* pGraphicsContext, RenderingHandler* pRenderingHandler, const Camera* pCamera)
{
    UNREFERENCED_PARAMETER(pCamera);

    m_pCamera = pCamera;
    m_pGraphicsContext = pGraphicsContext;
    m_pRenderingHandler = pRenderingHandler;

    if (!initializeGPUCompute()) {
        LOG("Failed to initialize Particle Emitter Handler GPU compute resources");
        return;
    }

    // Initialize all emitters
    for (ParticleEmitter* particleEmitter : m_ParticleEmitters) {
        initializeEmitter(particleEmitter);
    }
}

ParticleEmitter* ParticleEmitterHandler::createEmitter(const ParticleEmitterInfo& emitterInfo)
{
	ParticleEmitter* pNewEmitter = DBG_NEW ParticleEmitter(emitterInfo);
    m_ParticleEmitters.push_back(pNewEmitter);
    if (m_pGraphicsContext != nullptr) {
        initializeEmitter(pNewEmitter);
    }

	return pNewEmitter;
}
