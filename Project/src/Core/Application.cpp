#include "Application.h"
#include "Common/IWindow.h"
#include "Common/IShader.h"
#include "Common/IContext.h"
#include "Common/IRenderer.h"

#include "Vulkan/ContextVK.h"
#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/RenderPassVK.h"
#include "Vulkan/DescriptorSetLayoutVK.h"

Application g_Application;

Application::Application()
	: m_pWindow(nullptr),
	m_IsRunning(false)
{
}

void Application::init()
{
	m_pWindow = IWindow::create("Hello Vulkan", 800, 600);
	if (m_pWindow)
	{
		m_pWindow->setEventHandler(this);
	}

	m_pIContext = IContext::create(m_pWindow, API::VULKAN);
	m_pRenderer = m_pIContext->createRenderer();
	m_pRenderer->init();
	m_pRenderer->setClearColor(0.0f, 0.0f, 0.0f);
	m_pRenderer->setViewport(m_pWindow->getWidth(), m_pWindow->getHeight(), 0.0f, 1.0f, 0.0f, 0.0f);

	//Should we have ICommandBuffer? Or is commandbuffers internal i.e belongs in the renderer?
	DeviceVK* pDevice = reinterpret_cast<ContextVK*>(m_pIContext)->getDevice();

	DescriptorSetLayoutVK* pDescriptorLayout = new DescriptorSetLayoutVK(pDevice);
	pDescriptorLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, 0, 1); //Vertex
	pDescriptorLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 1, 1); //Transform
	pDescriptorLayout->addBindingSampledImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, 2, 1); //texture
	pDescriptorLayout->finalizeLayout();

	SAFEDELETE(pDescriptorLayout);
}

void Application::run()
{
	m_IsRunning = true;
	
	while (m_IsRunning)
	{
		m_pWindow->peekEvents();

		update();
		render();
	}
}

void Application::release()
{
	SAFEDELETE(m_pWindow);
	SAFEDELETE(m_pRenderer);
	SAFEDELETE(m_pIContext);
}

void Application::OnWindowResize(uint32_t width, uint32_t height)
{
	D_LOG("Resize w=%d h%d", width , height);

	if (m_pRenderer)
	{
		m_pRenderer->setViewport(width, height, 0.0f, 1.0f, 0.0f, 0.0f);
	}
}

void Application::OnWindowClose()
{
	D_LOG("Window Closed");
	m_IsRunning = false;
}

Application& Application::getInstance()
{
	return g_Application;
}

void Application::update()
{
}

void Application::render()
{
	m_pRenderer->beginFrame();
	m_pRenderer->drawTriangle();
	m_pRenderer->endFrame();

	m_pRenderer->swapBuffers();
}
