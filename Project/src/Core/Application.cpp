#include "Application.h"
#include "Common/IImgui.h"
#include "Common/IWindow.h"
#include "Common/IShader.h"
#include "Common/IRenderer.h"
#include "Common/IGraphicsContext.h"

#include "Vulkan/RenderPassVK.h"
#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/GraphicsContextVK.h"
#include "Vulkan/DescriptorSetLayoutVK.h"

#include <thread>
#include <chrono>
#include <imgui/imgui.h>

Application g_Application;

Application::Application()
	: m_pWindow(nullptr),
	m_pContext(nullptr),
	m_pRenderer(nullptr),
	m_pImgui(nullptr),
	m_IsRunning(false)
{
}

void Application::init()
{
	m_pWindow = IWindow::create("Hello Vulkan", 800, 600);
	if (m_pWindow)
	{
		m_pWindow->addEventHandler(this);
		m_pWindow->setFullscreenState(false);
	}

	m_pContext = IGraphicsContext::create(m_pWindow, API::VULKAN);
	m_pImgui = m_pContext->createImgui();
	m_pImgui->init(m_pWindow->getWidth(), m_pWindow->getHeight());

	m_pWindow->addEventHandler(m_pImgui);

	m_pRenderer = m_pContext->createRenderer();
	m_pRenderer->init();
	m_pRenderer->setClearColor(0.0f, 0.0f, 0.0f);
	m_pRenderer->setViewport(m_pWindow->getWidth(), m_pWindow->getHeight(), 0.0f, 1.0f, 0.0f, 0.0f);
}

void Application::run()
{
	m_IsRunning = true;
	
	while (m_IsRunning)
	{
		m_pWindow->peekEvents();
		if (m_pWindow->hasFocus())
		{
			update();
			renderUI();
			render();
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
	}
}

void Application::release()
{
	m_pWindow->removeEventHandler(m_pImgui);
	m_pWindow->removeEventHandler(this);

	SAFEDELETE(m_pRenderer);
	SAFEDELETE(m_pImgui);
	SAFEDELETE(m_pContext);
	SAFEDELETE(m_pWindow);
}

void Application::onWindowResize(uint32_t width, uint32_t height)
{
	D_LOG("Resize w=%d h%d", width , height);

	if (width != 0 && height != 0)
	{
		if (m_pRenderer)
		{
			m_pRenderer->setViewport(width, height, 0.0f, 1.0f, 0.0f, 0.0f);
			m_pRenderer->onWindowResize(width, height);
		}
	}
}

void Application::onWindowFocusChanged(IWindow* pWindow, bool hasFocus)
{
}

void Application::onWindowClose()
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

void Application::renderUI()
{
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
	ImGui::EndFrame();

	ImGui::Render();
}

void Application::render()
{
	m_pRenderer->beginFrame();
	m_pRenderer->drawTriangle();
	m_pRenderer->drawImgui(m_pImgui);
	m_pRenderer->endFrame();

	m_pRenderer->swapBuffers();
}
