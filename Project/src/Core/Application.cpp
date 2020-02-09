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
#include <glm/gtc/type_ptr.hpp>

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
	//Create window
	m_pWindow = IWindow::create("Hello Vulkan", 800, 600);
	if (m_pWindow)
	{
		m_pWindow->addEventHandler(this);
		m_pWindow->setFullscreenState(false);
	}

	//Create context
	m_pContext = IGraphicsContext::create(m_pWindow, API::VULKAN);
	
	//Setup Imgui
	m_pImgui = m_pContext->createImgui();
	m_pImgui->init();
	m_pWindow->addEventHandler(m_pImgui);

	//Setup renderer
	m_pRenderer = m_pContext->createRenderer();
	m_pRenderer->init();
	m_pRenderer->setClearColor(0.0f, 0.0f, 0.0f);
	m_pRenderer->setViewport(m_pWindow->getWidth(), m_pWindow->getHeight(), 0.0f, 1.0f, 0.0f, 0.0f);
}

void Application::run()
{
	m_IsRunning = true;
	
	auto currentTime	= std::chrono::high_resolution_clock::now();
	auto lastTime		= currentTime;

	//HACK to get a non-null deltatime
	std::this_thread::sleep_for(std::chrono::milliseconds(16));

	while (m_IsRunning)
	{
		lastTime	= currentTime;
		currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> deltatime = currentTime - lastTime;

		m_pWindow->peekEvents();
		if (m_pWindow->hasFocus())
		{
			update(deltatime.count());
			renderUI(deltatime.count());
			render(deltatime.count());
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

void Application::onMouseMove(uint32_t x, uint32_t y)
{
}

void Application::onMousePressed(int32_t button)
{
}

void Application::onMouseScroll(double x, double y)
{
}

void Application::onMouseReleased(int32_t button)
{
}

void Application::onKeyTyped(uint32_t character)
{
}

void Application::onKeyPressed(EKey key)
{
	//Exit application by pressing escape
	if (key == EKey::KEY_ESCAPE)
	{
		m_IsRunning = false;
	}
}

void Application::onKeyReleased(EKey key)
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

void Application::update(double ms)
{
}

static glm::vec4 g_TriangleColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

void Application::renderUI(double ms)
{
	m_pImgui->begin(ms);

	ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Color", NULL, ImGuiWindowFlags_NoResize))
	{
		ImGui::ColorPicker4("##picker", glm::value_ptr(g_TriangleColor), ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
	}
	ImGui::End();

	m_pImgui->end();
}

void Application::render(double ms)
{
	m_pRenderer->beginFrame();
	m_pRenderer->drawTriangle(g_TriangleColor);
	m_pRenderer->drawImgui(m_pImgui);
	m_pRenderer->endFrame();

	m_pRenderer->swapBuffers();
}
