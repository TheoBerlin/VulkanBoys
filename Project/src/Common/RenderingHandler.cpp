#include "RenderingHandler.hpp"
#include "IGraphicsContext.h"
#include "IRenderer.h"

RenderingHandler::RenderingHandler()
    : m_pGraphicsContext(nullptr),
    m_pParticleEmitterHandler(nullptr),
    m_pScene(nullptr)
{
}

RenderingHandler::~RenderingHandler()
{
}
