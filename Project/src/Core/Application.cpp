#include "Application.h"
#include "IWindow.h"
#include "Common/IContext.h"
#include "Common/IShader.h"

#include "Vulkan/ContextVK.h"
#include "Vulkan/CommandPoolVK.h"

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

	m_pIContext = IContext::create(API::VULKAN);

	IShader* pVertexShader = m_pIContext->createShader();
	pVertexShader->loadFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/vertex.spv");
	pVertexShader->finalize();

	delete pVertexShader;

	//Should we have ICommandBuffer? Or is commandbuffers internal i.e belongs in the renderer?
	DeviceVK* pDevice = reinterpret_cast<ContextVK*>(m_pIContext)->getDevice();
	uint32_t queueFamilyIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();

	CommandPoolVK* pCommandPool = new CommandPoolVK(pDevice, queueFamilyIndex);
	pCommandPool->init();

	CommandBufferVK* pCommandBuffer = pCommandPool->allocateCommandBuffer();
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
}

void Application::OnWindowResize(uint32_t width, uint32_t height)
{
	D_LOG("Resize w=%d h%d", width , height);
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
}
