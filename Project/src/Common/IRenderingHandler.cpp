#include "IRenderingHandler.hpp"

#include "Common/IGraphicsContext.h"
#include "Common/IRenderer.h"

IRenderingHandler::IRenderingHandler()
    :m_pGraphicsContext(nullptr),
    m_pMeshRenderer(nullptr),
	m_pRayTracer(nullptr),
	m_pParticleRenderer(nullptr)
{}

IRenderingHandler::~IRenderingHandler()
{}
