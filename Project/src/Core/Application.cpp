#include "Application.h"
#include "Camera.h"
#include "Input.h"
#include "TaskDispatcher.h"
#include "Transform.h"

#include "Common/IGraphicsContext.h"
#include "Common/IImgui.h"
#include "Common/IInputHandler.h"
#include "Common/IMesh.h"
#include "Common/IProfiler.h"
#include "Common/IRenderer.h"
#include "Common/IRenderingHandler.hpp"
#include "Common/IShader.h"
#include "Common/ITexture2D.h"
#include "Common/IWindow.h"

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
	m_pRenderingHandler(nullptr),
	m_pMeshRenderer(nullptr),
	m_pRayTracingRenderer(nullptr),
	m_pImgui(nullptr),
	m_pMesh(nullptr),
	m_pAlbedo(nullptr),
	m_pInputHandler(nullptr),
	m_Camera(),
	m_IsRunning(false),
	m_UpdateCamera(false),
	m_pParticleTexture(nullptr),
	m_pParticleEmitterHandler(nullptr),
	m_CurrentEmitterIdx(0)
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
	LOG("Starting application");

	TaskDispatcher::init();

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

	const bool forceRayTracingOff = true;
	m_EnableRayTracing = m_pContext->supportsRayTracing() && !forceRayTracingOff;

	//Setup Imgui
	m_pImgui = m_pContext->createImgui();
	m_pImgui->init();
	m_pWindow->addEventHandler(m_pImgui);

	//Setup camera
	m_Camera.setDirection(glm::vec3(0.0f, 0.0f, 1.0f));
	m_Camera.setPosition(glm::vec3(0.0f, 0.5f, -2.0f));
	m_Camera.setProjection(90.0f, m_pWindow->getWidth(), m_pWindow->getHeight(), 0.1f, 100.0f);
	m_Camera.update();

	// Setup particles
	m_pParticleEmitterHandler = m_pContext->createParticleEmitterHandler();
	m_pParticleEmitterHandler->initialize(m_pContext, &m_Camera);

	m_pParticleTexture = m_pContext->createTexture2D();
	if (!m_pParticleTexture->initFromFile("assets/textures/sun.png")) {
		LOG("Failed to create particle texture");
		SAFEDELETE(m_pParticleTexture);
	}

	ParticleEmitterInfo emitterInfo = {};
	emitterInfo.position = glm::vec3(0.0f, 0.0f, 0.0f);
	emitterInfo.direction = glm::normalize(glm::vec3(0.0f, 0.9f, 0.1f));
	emitterInfo.particleSize = glm::vec2(0.2f, 0.2f);
	emitterInfo.initialSpeed = 5.5f;
	emitterInfo.particleDuration = 3.0f;
	emitterInfo.particlesPerSecond = 40.0f;
	emitterInfo.spread = glm::quarter_pi<float>() / 1.3f;
	emitterInfo.pTexture = m_pParticleTexture;
	m_pParticleEmitterHandler->createEmitter(emitterInfo);

	for (size_t i = 0; i < 2; i++) {
		emitterInfo.position.x = 1.0f + (float)i;
		m_pParticleEmitterHandler->createEmitter(emitterInfo);
	}

	// Setup rendering handler
	m_pRenderingHandler = m_pContext->createRenderingHandler();
	m_pRenderingHandler->initialize();
	m_pRenderingHandler->setClearColor(0.0f, 0.0f, 0.0f);
	m_pRenderingHandler->setParticleEmitterHandler(m_pParticleEmitterHandler);

	// Setup renderers
	m_pMeshRenderer = m_pContext->createMeshRenderer(m_pRenderingHandler);
	m_pParticleRenderer = m_pContext->createParticleRenderer(m_pRenderingHandler);
	if (m_EnableRayTracing) {
		m_pRayTracingRenderer = m_pContext->createRayTracingRenderer(m_pRenderingHandler);
		m_pRayTracingRenderer->init();
	}
	m_pMeshRenderer->init();
	m_pParticleRenderer->init();

	// TODO: Should the renderers themselves call these instead?
	m_pRenderingHandler->setMeshRenderer(m_pMeshRenderer);
	m_pRenderingHandler->setParticleRenderer(m_pParticleRenderer);
	if (m_EnableRayTracing) {
		m_pRenderingHandler->setRayTracer(m_pRayTracingRenderer);
	}

	m_pRenderingHandler->setViewport(m_pWindow->getWidth(), m_pWindow->getHeight(), 0.0f, 1.0f, 0.0f, 0.0f);

	//Load mesh
	{
		using namespace glm;

		Vertex vertices[] =
		{
			//FRONT FACE
			{ glm::vec4(-0.5,  0.5, -0.5, 1.0f), glm::vec4(0.0f,  0.0f, -1.0f, 0.0f), glm::vec4(1.0f,  0.0f, 0.0f,  0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) },
			{ glm::vec4(0.5,  0.5, -0.5, 1.0f),  glm::vec4(0.0f,  0.0f, -1.0f, 0.0f), glm::vec4(1.0f,  0.0f, 0.0f,  0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f) },
			{ glm::vec4(-0.5, -0.5, -0.5, 1.0f), glm::vec4(0.0f,  0.0f, -1.0f, 0.0f), glm::vec4(1.0f,  0.0f, 0.0f,  0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) },
			{ glm::vec4(0.5, -0.5, -0.5, 1.0f),  glm::vec4(0.0f,  0.0f, -1.0f, 0.0f), glm::vec4(1.0f,  0.0f, 0.0f,  0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 0.0f) },

			//BACK FACE
			{ glm::vec4(0.5,  0.5,  0.5, 1.0f),  glm::vec4(0.0f,  0.0f,  1.0f, 0.0f), glm::vec4(-1.0f,  0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) },
			{ glm::vec4(-0.5,  0.5,  0.5, 1.0f), glm::vec4(0.0f,  0.0f,  1.0f, 0.0f), glm::vec4(-1.0f,  0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f) },
			{ glm::vec4(0.5, -0.5,  0.5, 1.0f),  glm::vec4(0.0f,  0.0f,  1.0f, 0.0f), glm::vec4(-1.0f,  0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) },
			{ glm::vec4(-0.5, -0.5,  0.5, 1.0f), glm::vec4(0.0f,  0.0f,  1.0f, 0.0f), glm::vec4(-1.0f,  0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 0.0f) },

			//RIGHT FACE
			{ glm::vec4(0.5,  0.5, -0.5, 1.0f),  glm::vec4(1.0f,  0.0f,  0.0f, 0.0f), glm::vec4(0.0f,  0.0f, 1.0f,  0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) },
			{ glm::vec4(0.5,  0.5,  0.5, 1.0f),  glm::vec4(1.0f,  0.0f,  0.0f, 0.0f), glm::vec4(0.0f,  0.0f, 1.0f,  0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f) },
			{ glm::vec4(0.5, -0.5, -0.5, 1.0f),  glm::vec4(1.0f,  0.0f,  0.0f, 0.0f), glm::vec4(0.0f,  0.0f, 1.0f,  0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) },
			{ glm::vec4(0.5, -0.5,  0.5, 1.0f),  glm::vec4(1.0f,  0.0f,  0.0f, 0.0f), glm::vec4(0.0f,  0.0f, 1.0f,  0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 0.0f) },

			//LEFT FACE
			{ glm::vec4(-0.5,  0.5, -0.5, 1.0f), glm::vec4(-1.0f,  0.0f, 0.0f, 0.0f), glm::vec4(0.0f,  0.0f, -1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) },
			{ glm::vec4(-0.5,  0.5,  0.5, 1.0f), glm::vec4(-1.0f,  0.0f, 0.0f, 0.0f), glm::vec4(0.0f,  0.0f, -1.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f) },
			{ glm::vec4(-0.5, -0.5, -0.5, 1.0f), glm::vec4(-1.0f,  0.0f, 0.0f, 0.0f), glm::vec4(0.0f,  0.0f, -1.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) },
			{ glm::vec4(-0.5, -0.5,  0.5, 1.0f), glm::vec4(-1.0f,  0.0f, 0.0f, 0.0f), glm::vec4(0.0f,  0.0f, -1.0f, 0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 0.0f) },

			//TOP FACE
			{ glm::vec4(-0.5,  0.5,  0.5, 1.0f), glm::vec4(0.0f,  1.0f,  0.0f, 0.0f), glm::vec4(1.0f,  0.0f, 0.0f,  0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) },
			{ glm::vec4(0.5,  0.5,  0.5, 1.0f),  glm::vec4(0.0f,  1.0f,  0.0f, 0.0f), glm::vec4(1.0f,  0.0f, 0.0f,  0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f) },
			{ glm::vec4(-0.5,  0.5, -0.5, 1.0f), glm::vec4(0.0f,  1.0f,  0.0f, 0.0f), glm::vec4(1.0f,  0.0f, 0.0f,  0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) },
			{ glm::vec4(0.5,  0.5, -0.5, 1.0f),  glm::vec4(0.0f,  1.0f,  0.0f, 0.0f), glm::vec4(1.0f,  0.0f, 0.0f,  0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 0.0f) },

			//BOTTOM FACE
			{ glm::vec4(-0.5, -0.5, -0.5, 1.0f), glm::vec4(0.0f, -1.0f,  0.0f, 0.0f), glm::vec4(-1.0f,  0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) },
			{ glm::vec4(0.5, -0.5, -0.5, 1.0f),  glm::vec4(0.0f, -1.0f,  0.0f, 0.0f), glm::vec4(-1.0f,  0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f) },
			{ glm::vec4(-0.5, -0.5,  0.5, 1.0f), glm::vec4(0.0f, -1.0f,  0.0f, 0.0f), glm::vec4(-1.0f,  0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) },
			{ glm::vec4(0.5, -0.5,  0.5, 1.0f),  glm::vec4(0.0f, -1.0f,  0.0f, 0.0f), glm::vec4(-1.0f,  0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 0.0f) },
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
		TaskDispatcher::execute([this] 
			{ 
				m_pMesh->initFromFile("assets/meshes/gun.obj"); 
			});

		m_pAlbedo = m_pContext->createTexture2D();
		TaskDispatcher::execute([this]
			{
				m_pAlbedo->initFromFile("assets/textures/albedo.tga");
			});

		TaskDispatcher::waitForTasks();
		
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

	SAFEDELETE(m_pAlbedo);
	SAFEDELETE(m_pMesh);
	SAFEDELETE(m_pRenderingHandler);
	SAFEDELETE(m_pMeshRenderer);
	SAFEDELETE(m_pRayTracingRenderer);
	SAFEDELETE(m_pParticleRenderer);
	SAFEDELETE(m_pParticleTexture);
	SAFEDELETE(m_pParticleEmitterHandler);
	SAFEDELETE(m_pImgui);
	SAFEDELETE(m_pContext);

	SAFEDELETE(m_pInputHandler);
	Input::setInputHandler(nullptr);

	SAFEDELETE(m_pWindow);

	TaskDispatcher::release();

	LOG("Exiting Application");
}

void Application::onWindowResize(uint32_t width, uint32_t height)
{
	D_LOG("Resize w=%d h%d", width , height);

	if (width != 0 && height != 0)
	{
		if (m_pRenderingHandler) {
			m_pRenderingHandler->setViewport(width, height, 0.0f, 1.0f, 0.0f, 0.0f);
			m_pRenderingHandler->onWindowResize(width, height);
		}

		m_Camera.setProjection(90.0f, float(width), float(height), 0.1f, 100.0f);
	}
}

void Application::onMouseMove(uint32_t x, uint32_t y)
{
	if (m_UpdateCamera)
	{
		glm::vec2 middlePos = middlePos = glm::vec2(m_pWindow->getClientWidth() / 2.0f, m_pWindow->getClientHeight() / 2.0f);

		float xoffset = middlePos.x - x;
		float yoffset = y - middlePos.y;

		constexpr float sensitivity = 0.25f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		glm::vec3 rotation = m_Camera.getRotation();
		rotation += glm::vec3(yoffset, -xoffset, 0.0f);

		m_Camera.setRotation(rotation);
	}
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
	IProfiler::progressTimer(dt);

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
		m_Camera.rotate(glm::vec3(0.0f, -rotationSpeed * dt, 0.0f));
	}
	else if (Input::isKeyPressed(EKey::KEY_RIGHT))
	{
		m_Camera.rotate(glm::vec3(0.0f, rotationSpeed * dt, 0.0f));
	}

	if (Input::isKeyPressed(EKey::KEY_UP))
	{
		m_Camera.rotate(glm::vec3(-rotationSpeed * dt, 0.0f, 0.0f));
	}
	else if (Input::isKeyPressed(EKey::KEY_DOWN))
	{
		m_Camera.rotate(glm::vec3(rotationSpeed * dt, 0.0f, 0.0f));
	}

	m_Camera.update();

	if (m_UpdateCamera)
	{
		Input::setMousePosition(m_pWindow, glm::vec2(m_pWindow->getClientWidth() / 2.0f, m_pWindow->getClientHeight() / 2.0f));
	}

	m_pParticleEmitterHandler->update(dt);
}

static glm::vec4 g_Color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

void Application::renderUI(double dt)
{
	m_pImgui->begin(dt);

	// Color picker for mesh
	ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Color", NULL, ImGuiWindowFlags_NoResize))
	{
		ImGui::ColorPicker4("##picker", glm::value_ptr(g_Color), ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
	}
	ImGui::End();

	// Particle control panel
	ImGui::SetNextWindowSize(ImVec2(320, 210), ImGuiCond_Always);
	if (ImGui::Begin("Particles", NULL, ImGuiWindowFlags_NoResize))
	{
		ImGui::Text("Toggle Computation Device");
		const char* btnLabel = m_pParticleEmitterHandler->gpuComputed() ? "GPU" : "CPU";
		if (ImGui::Button(btnLabel, ImVec2(40, 25))) {
			m_pParticleEmitterHandler->toggleComputationDevice();
		}

		// Get current emitter data
		ParticleEmitter* pEmitter = m_pParticleEmitterHandler->getParticleEmitter(m_CurrentEmitterIdx);
		glm::vec3 emitterPos = pEmitter->getPosition();

		glm::vec3 emitterDirection = pEmitter->getDirection();
		float yaw = getYaw(emitterDirection);
		float oldYaw = yaw;
		float pitch = getPitch(emitterDirection);
		float oldPitch = pitch;

		float emitterSpeed = pEmitter->getInitialSpeed();

		if (ImGui::SliderFloat3("Position", glm::value_ptr(emitterPos), -10.0f, 10.0f)) {
			pEmitter->setPosition(emitterPos);
		}
		if (ImGui::SliderFloat("Yaw", &yaw, 0.01f - glm::pi<float>(), glm::pi<float>() - 0.01f)) {
			applyYaw(emitterDirection, yaw - oldYaw);
			pEmitter->setDirection(emitterDirection);
		}
		if (ImGui::SliderFloat("Pitch", &pitch, 0.01f - glm::half_pi<float>(), glm::half_pi<float>() - 0.01f)) {
			applyPitch(emitterDirection, pitch - oldPitch);
			pEmitter->setDirection(emitterDirection);
		}
		if (ImGui::SliderFloat3("Direction", glm::value_ptr(emitterDirection), -1.0f, 1.0f)) {
			pEmitter->setDirection(glm::normalize(emitterDirection));
		}
		if (ImGui::SliderFloat("Speed", &emitterSpeed, 0.0f, 20.0f)) {
			pEmitter->setInitialSpeed(emitterSpeed);
		}

		ImGui::Text("Particles: %d", pEmitter->getParticleCount());
		float particlesPerSecond = pEmitter->getParticlesPerSecond();
		float particleDuration = pEmitter->getParticleDuration();

		if (ImGui::SliderFloat("Particles per second", &particlesPerSecond, 0.0f, 1000.0f)) {
		}
	}
	ImGui::End();

	// Draw profiler UI
	ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Profiler", NULL, ImGuiWindowFlags_NoResize)) {
		m_pParticleEmitterHandler->drawProfilerUI();
		m_pRenderingHandler->drawProfilerUI();
	}
	ImGui::End();

	m_pImgui->end();
}

void Application::render(double dt)
{
	m_pRenderingHandler->beginFrame(m_Camera);

	if (!m_EnableRayTracing) {
		g_Rotation = glm::rotate(g_Rotation, glm::radians(30.0f * float(dt)), glm::vec3(0.0f, 1.0f, 0.0f));
		m_pRenderingHandler->submitMesh(m_pMesh, g_Color, glm::mat4(1.0f) * g_Rotation);
	}

	m_pRenderingHandler->drawImgui(m_pImgui);
	m_pRenderingHandler->endFrame();
	m_pRenderingHandler->swapBuffers();
}
