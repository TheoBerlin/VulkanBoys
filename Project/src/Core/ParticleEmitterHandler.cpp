#include "ParticleEmitterHandler.hpp"

// DOESN'T EXIST: #include "Common/IPipeline.h"
#include "Common/Debug.h"
#include "Common/IGraphicsContext.h"
#include "Common/IShader.h"

ParticleEmitterHandler::ParticleEmitterHandler()
    :m_pGraphicsContext(nullptr)
{}

ParticleEmitterHandler::~ParticleEmitterHandler()
{}

void ParticleEmitterHandler::initialize(IGraphicsContext* pGraphicsContext)
{
    m_pGraphicsContext = pGraphicsContext;

    // TODO: Create quad vertex buffer, either here or in GraphicsContext

    // Create pipeline state
	IShader* pVertexShader = m_pGraphicsContext->createShader();
	pVertexShader->initFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/particles/vertex.spv");
	if (!pVertexShader->finalize()) {
        LOG("Failed to create vertex shader for particle emitter handler");
		return;
	}

	IShader* pPixelShader = m_pGraphicsContext->createShader();
	pPixelShader->initFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/particles/fragment.spv");
	if (!pPixelShader->finalize()) {
		LOG("Failed to create pixel shader for particle emitter handler");
		return;
	}

	std::vector<IShader*> shaders = { pVertexShader, pPixelShader };
	m_pPipeline = DBG_NEW PipelineVK(m_pGraphicsContext->getDevice());
	m_pPipeline->addColorBlendAttachment(true, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
	m_pPipeline->setCulling(false);
	m_pPipeline->setDepthTest(false);
	m_pPipeline->setWireFrame(false);
	m_pPipeline->finalize(shaders, m_pRenderPass, m_pPipelineLayout);

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);

    // Initialize all emitters
    for (ParticleEmitter& particleEmitter : m_ParticleEmitters) {
        particleEmitter.initialize(m_pGraphicsContext);
    }
}

void ParticleEmitterHandler::render()
{
    // TODO: Set pipeline
    m_pPipeline->bind();

    // TODO: Bind quad buffer

    for (const ParticleEmitter& particleEmitter : m_ParticleEmitters) {
        particleEmitter.render();
    }
}

void ParticleEmitterHandler::addEmitter(glm::vec3 position, glm::vec3 direction, float particleDuration, float initialSpeed, float particlesPerSecond)
{
    m_ParticleEmitters.push_back(ParticleEmitter(position, direction, particleDuration, initialSpeed, particlesPerSecond));
    if (m_pGraphicsContext != nullptr) {
        m_ParticleEmitters.back().initialize(m_pGraphicsContext);
    }
}
