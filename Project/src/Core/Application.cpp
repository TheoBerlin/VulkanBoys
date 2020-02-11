#include "Application.h"
#include "Camera.h"
#include "Input.h"

#include "Common/IMesh.h"
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

#include <thread>
#include <chrono>
#include <imgui/imgui.h>

#include <glm/gtc/type_ptr.hpp>

#ifdef max
	#undef max
#endif

#ifdef min
	#undef min
#endif

Application* Application::s_pInstance = nullptr;

Application::Application()
	: m_pWindow(nullptr),
	m_pContext(nullptr),
	m_pRenderer(nullptr),
	m_pImgui(nullptr),
	m_pMesh(nullptr),
	m_pInputHandler(nullptr),
	m_Camera(),
	m_IsRunning(false),
	m_UpdateCamera(false)
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
	m_Camera.setPosition(glm::vec3(0.0f, -1.0f, -1.0f));
	//m_Camera.setRotation(glm::vec3(0.0f, 45.0f, 0.0f));
	m_Camera.setProjection(90.0f, m_pWindow->getWidth(), m_pWindow->getHeight(), 0.1f, 100.0f);
	m_Camera.update();

	//Load mesh
	{
		using namespace glm;

		Vertex vertices[] =
		{
			//FRONT FACE
			{ vec3(-0.5,  0.5, -0.5), vec3(0.0f,  0.0f, -1.0f), vec3(1.0f,  0.0f, 0.0f), vec2(0.0f, 0.0f) },
			{ vec3( 0.5,  0.5, -0.5), vec3(0.0f,  0.0f, -1.0f), vec3(1.0f,  0.0f, 0.0f), vec2(1.0f, 0.0f) },
			{ vec3(-0.5, -0.5, -0.5), vec3(0.0f,  0.0f, -1.0f), vec3(1.0f,  0.0f, 0.0f), vec2(0.0f, 1.0f) },
			{ vec3( 0.5, -0.5, -0.5), vec3(0.0f,  0.0f, -1.0f), vec3(1.0f,  0.0f, 0.0f), vec2(1.0f, 1.0f) },

			//BACK FACE
			{ vec3( 0.5,  0.5,  0.5), vec3(0.0f,  0.0f,  1.0f), vec3(-1.0f,  0.0f, 0.0f), vec2(0.0f, 0.0f) },
			{ vec3(-0.5,  0.5,  0.5), vec3(0.0f,  0.0f,  1.0f), vec3(-1.0f,  0.0f, 0.0f), vec2(1.0f, 0.0f) },
			{ vec3( 0.5, -0.5,  0.5), vec3(0.0f,  0.0f,  1.0f), vec3(-1.0f,  0.0f, 0.0f), vec2(0.0f, 1.0f) },
			{ vec3(-0.5, -0.5,  0.5), vec3(0.0f,  0.0f,  1.0f), vec3(-1.0f,  0.0f, 0.0f), vec2(1.0f, 1.0f) },

			//RIGHT FACE
			{ vec3(0.5,  0.5, -0.5), vec3(1.0f,  0.0f,  0.0f), vec3(0.0f,  0.0f, 1.0f), vec2(0.0f, 0.0f) },
			{ vec3(0.5,  0.5,  0.5), vec3(1.0f,  0.0f,  0.0f), vec3(0.0f,  0.0f, 1.0f), vec2(1.0f, 0.0f) },
			{ vec3(0.5, -0.5, -0.5), vec3(1.0f,  0.0f,  0.0f), vec3(0.0f,  0.0f, 1.0f), vec2(0.0f, 1.0f) },
			{ vec3(0.5, -0.5,  0.5), vec3(1.0f,  0.0f,  0.0f), vec3(0.0f,  0.0f, 1.0f), vec2(1.0f, 1.0f) },

			//LEFT FACE
			{ vec3(-0.5,  0.5, -0.5), vec3(-1.0f,  0.0f,  0.0f), vec3(0.0f,  0.0f, -1.0f), vec2(0.0f, 0.0f) },
			{ vec3(-0.5,  0.5,  0.5), vec3(-1.0f,  0.0f,  0.0f), vec3(0.0f,  0.0f, -1.0f), vec2(1.0f, 0.0f) },
			{ vec3(-0.5, -0.5, -0.5), vec3(-1.0f,  0.0f,  0.0f), vec3(0.0f,  0.0f, -1.0f), vec2(0.0f, 1.0f) },
			{ vec3(-0.5, -0.5,  0.5), vec3(-1.0f,  0.0f,  0.0f), vec3(0.0f,  0.0f, -1.0f), vec2(1.0f, 1.0f) },

			//TOP FACE
			{ vec3(-0.5,  0.5,  0.5), vec3(0.0f,  1.0f,  0.0f), vec3(1.0f,  0.0f, 0.0f), vec2(0.0f, 0.0f) },
			{ vec3( 0.5,  0.5,  0.5), vec3(0.0f,  1.0f,  0.0f), vec3(1.0f,  0.0f, 0.0f), vec2(1.0f, 0.0f) },
			{ vec3(-0.5,  0.5, -0.5), vec3(0.0f,  1.0f,  0.0f), vec3(1.0f,  0.0f, 0.0f), vec2(0.0f, 1.0f) },
			{ vec3( 0.5,  0.5, -0.5), vec3(0.0f,  1.0f,  0.0f), vec3(1.0f,  0.0f, 0.0f), vec2(1.0f, 1.0f) },

			//BOTTOM FACE
			{ vec3(-0.5, -0.5, -0.5), vec3(0.0f, -1.0f,  0.0f), vec3(-1.0f,  0.0f, 0.0f), vec2(0.0f, 0.0f) },
			{ vec3( 0.5, -0.5, -0.5), vec3(0.0f, -1.0f,  0.0f), vec3(-1.0f,  0.0f, 0.0f), vec2(1.0f, 0.0f) },
			{ vec3(-0.5, -0.5,  0.5), vec3(0.0f, -1.0f,  0.0f), vec3(-1.0f,  0.0f, 0.0f), vec2(0.0f, 1.0f) },
			{ vec3( 0.5, -0.5,  0.5), vec3(0.0f, -1.0f,  0.0f), vec3(-1.0f,  0.0f, 0.0f), vec2(1.0f, 1.0f) },
		};

		uint32_t indices[] =
		{
			//FRONT FACE
			2, 1, 0,
			2, 3, 1,

			//BACK FACE
			6, 5, 4,
			6, 7, 5,

			//RIGHT FACE
			10, 9, 8,
			10, 11, 9,

			//LEFT FACE
			12, 13, 14,
			13, 15, 14,

			//TOP FACE
			18, 17, 16,
			18, 19, 17,

			//BOTTOM FACE
			22, 21, 20,
			22, 23, 21
		};

		m_pMesh = m_pContext->createMesh();
		m_pMesh->initFromFile("assets/meshes/gun.obj");
		//m_pMesh->initFromMemory(vertices, 24, indices, 36);
	}
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

	m_pContext->sync();
	SAFEDELETE(m_pMesh);
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
	if (m_UpdateCamera)
	{
		glm::vec2 middlePos = middlePos = glm::vec2(m_pWindow->getClientWidth() / 2.0f, m_pWindow->getClientHeight() / 2.0f);

		float xoffset = middlePos.x - x;
		float yoffset = middlePos.y - y;

		constexpr float sensitivity = 0.5f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		glm::vec3 rotation = m_Camera.getRotation();
		rotation += glm::vec3(yoffset, xoffset, 0.0f);

		m_Camera.setRotation(rotation);
	}
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
	else if (key == EKey::KEY_1)
	{
		m_pWindow->toggleFullscreenState();
	}
	else if (key == EKey::KEY_2)
	{
		m_UpdateCamera = !m_UpdateCamera;
        if (m_UpdateCamera)
        {
            Input::captureMouse(m_pWindow);
        }
        else
        {
            Input::releaseMouse(m_pWindow);
        }
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
	constexpr float speed = 0.75f;
	if (Input::isKeyPressed(EKey::KEY_A))
	{
		m_Camera.translate(glm::vec3(-speed * dt, 0.0f, 0.0f));
	}
	else if (Input::isKeyPressed(EKey::KEY_D))
	{
		m_Camera.translate(glm::vec3(speed * dt, 0.0f, 0.0f));
	}

	if (Input::isKeyPressed(EKey::KEY_W))
	{
		m_Camera.translate(glm::vec3(0.0f, 0.0f, speed * dt));
	}
	else if (Input::isKeyPressed(EKey::KEY_S))
	{
		m_Camera.translate(glm::vec3(0.0f, 0.0f, -speed * dt));
	}

	if (Input::isKeyPressed(EKey::KEY_Q))
	{
		m_Camera.translate(glm::vec3(0.0f, speed * dt, 0.0f));
	}
	else if (Input::isKeyPressed(EKey::KEY_E))
	{
		m_Camera.translate(glm::vec3(0.0f, -speed * dt, 0.0f));
	}

	if (Input::isKeyPressed(EKey::KEY_A))
	{
		m_Camera.translate(glm::vec3(-speed * dt, 0.0f, 0.0f));
	}
	else if (Input::isKeyPressed(EKey::KEY_D))
	{
		m_Camera.translate(glm::vec3(speed * dt, 0.0f, 0.0f));
	}

	constexpr float rotationSpeed = 30.0f;
	if (Input::isKeyPressed(EKey::KEY_LEFT))
	{
		m_Camera.rotate(glm::vec3(0.0f, rotationSpeed * dt, 0.0f));
	}
	else if (Input::isKeyPressed(EKey::KEY_RIGHT))
	{
		m_Camera.rotate(glm::vec3(0.0f, -rotationSpeed * dt, 0.0f));
	}

	if (Input::isKeyPressed(EKey::KEY_UP))
	{
		m_Camera.rotate(glm::vec3(rotationSpeed * dt, 0.0f, 0.0f));
	}
	else if (Input::isKeyPressed(EKey::KEY_DOWN))
	{
		m_Camera.rotate(glm::vec3(-rotationSpeed * dt, 0.0f, 0.0f));
	}

	m_Camera.update();

	if (m_UpdateCamera)
	{
		Input::setMousePosition(m_pWindow, glm::vec2(m_pWindow->getClientWidth() / 2.0f, m_pWindow->getClientHeight() / 2.0f));
	}
}

static glm::vec4 g_Color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

void Application::renderUI(double dt)
{
	m_pImgui->begin(dt);

	ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Color", NULL, ImGuiWindowFlags_NoResize))
	{
		ImGui::ColorPicker4("##picker", glm::value_ptr(g_Color), ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
	}
	ImGui::End();

	m_pImgui->end();
}

void Application::render(double dt)
{
	m_pRenderer->beginFrame(m_Camera);

	//g_Rotation = glm::rotate(g_Rotation, glm::radians(15.0f * float(dt)), glm::vec3(0.0f, 1.0f, 0.0f));

	m_pRenderer->submitMesh(m_pMesh, g_Color, glm::mat4(1.0f) * g_Rotation);
	m_pRenderer->drawImgui(m_pImgui);

	m_pRenderer->endFrame();

	m_pRenderer->swapBuffers();
}
