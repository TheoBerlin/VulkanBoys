#include "Application.h"
#include "Common/IImgui.h"
#include "Common/IWindow.h"
#include "Common/IShader.h"
#include "Common/IRenderer.h"
#include "Common/IGraphicsContext.h"
#include "Common/IInputHandler.h"

#include "Vulkan/RenderPassVK.h"
#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/GraphicsContextVK.h"
#include "Vulkan/DescriptorSetLayoutVK.h"

#include "Camera.h"
#include "Input.h"

#include <thread>
#include <chrono>
#include <imgui/imgui.h>
#include <glm/gtc/type_ptr.hpp>

Application* Application::s_pInstance = nullptr;

Application::Application()
	: m_pWindow(nullptr),
	m_pContext(nullptr),
	m_pRenderer(nullptr),
	m_pImgui(nullptr),
	m_IsRunning(false)
{
	ASSERT(s_pInstance == nullptr);
	s_pInstance = this;
}

Application::~Application()
{
	s_pInstance = nullptr;
}

void Application::init()
{
	//Create window
	m_pWindow = IWindow::create("Hello Vulkan", 1440, 900);
	if (m_pWindow)
	{
		m_pWindow->addEventHandler(this);
		m_pWindow->setFullscreenState(false);
	}

	//Create input
	m_pInputHandler = IInputHandler::create();
	Input::setInputHandler(m_pInputHandler);
	m_pWindow->addEventHandler(m_pInputHandler);

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

	//Setup camera
	m_Camera.setDirection(glm::vec3(0.0f, 0.0f, 1.0f));
	m_Camera.setPosition(glm::vec3(0.0f, 0.0f, -1.0f));
	m_Camera.setProjection(90.0f, m_pWindow->getWidth(), m_pWindow->getHeight(), 0.1f, 100.0f);
	m_Camera.update();
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
		double seconds = deltatime.count() / 1000.0;

		m_pWindow->peekEvents();
		if (m_pWindow->hasFocus())
		{
			update(seconds);
			renderUI(seconds);
			render(seconds);
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
	}
}

void Application::release()
{
	m_pWindow->removeEventHandler(m_pInputHandler);
	m_pWindow->removeEventHandler(m_pImgui);
	m_pWindow->removeEventHandler(this);

	SAFEDELETE(m_pRenderer);
	SAFEDELETE(m_pImgui);
	SAFEDELETE(m_pContext);

	SAFEDELETE(m_pInputHandler);
	Input::setInputHandler(nullptr);

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

		m_Camera.setProjection(90.0f, float(width), float(height), 0.1f, 100.0f);
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

Application* Application::get()
{
	return s_pInstance;
}

static glm::mat4 g_Rotation = glm::mat4(1.0f);

void Application::update(double dt)
{
	if (Input::isKeyPressed(EKey::KEY_A))
	{
		m_Camera.translate(glm::vec3(0.2f * dt, 0.0f, 0.0f));
	}
	else if (Input::isKeyPressed(EKey::KEY_D))
	{
		m_Camera.translate(glm::vec3(-0.2f * dt, 0.0f, 0.0f));
	}

	if (Input::isKeyPressed(EKey::KEY_W))
	{
		m_Camera.translate(glm::vec3(0.0f, 0.0f, 0.2f * dt));
	}
	else if (Input::isKeyPressed(EKey::KEY_S))
	{
		m_Camera.translate(glm::vec3(0.0f, 0.0f, -0.2f * dt));
	}

	m_Camera.update();
}

static glm::vec4 g_TriangleColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

void Application::renderUI(double dt)
{
	m_pImgui->begin(dt);

	ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Color", NULL, ImGuiWindowFlags_NoResize))
	{
		ImGui::ColorPicker4("##picker", glm::value_ptr(g_TriangleColor), ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
	}
	ImGui::End();

	m_pImgui->end();
}

void Application::render(double dt)
{
	m_pRenderer->beginFrame(m_Camera);

	g_Rotation = glm::rotate(g_Rotation, glm::radians(15.0f * float(dt)), glm::vec3(0.0f, 0.0f, 1.0f));

	m_pRenderer->drawTriangle(g_TriangleColor, glm::translate(glm::mat4(1.0f), glm::vec3( 0.5f, 0.5f, 0.0f)) * g_Rotation);
	m_pRenderer->drawTriangle(g_TriangleColor, glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.5f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)));
	m_pRenderer->drawTriangle(g_TriangleColor, glm::translate(glm::mat4(1.0f), glm::vec3( 0.5f, -0.5f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)));
	m_pRenderer->drawTriangle(g_TriangleColor, glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, -0.5f, 0.0f)));

	m_pRenderer->drawImgui(m_pImgui);

	m_pRenderer->endFrame();

	m_pRenderer->swapBuffers();
}
