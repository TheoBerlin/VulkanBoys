#include "Application.h"
#include "Camera.h"
#include "Input.h"
#include "TaskDispatcher.h"
#include "Transform.h"

#include "Common/Profiler.h"
#include "Common/RenderingHandler.hpp"
#include "Common/IImgui.h"
#include "Common/IWindow.h"
#include "Common/IShader.h"
#include "Common/ISampler.h"
#include "Common/IScene.h"
#include "Common/IMesh.h"
#include "Common/IRenderer.h"
#include "Common/IShader.h"
#include "Common/ITexture2D.h"
#include "Common/ITextureCube.h"
#include "Common/IInputHandler.h"
#include "Common/IGraphicsContext.h"

#include <thread>
#include <chrono>
#include <fstream>

#include <imgui/imgui.h>

#include <glm/gtc/type_ptr.hpp>

#include "LightSetup.h"
#include "Vulkan/RenderPassVK.h"
#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/GraphicsContextVK.h"
#include "Vulkan/DescriptorSetLayoutVK.h"

#ifdef max
	#undef max
#endif

#ifdef min
	#undef min
#endif

Application* Application::s_pInstance = nullptr;

constexpr bool	FORCE_RAY_TRACING_OFF	= false;
constexpr bool	HIGH_RESOLUTION_SPHERE	= false;
constexpr float CAMERA_PAN_LENGTH		= 10.0f;

Application::Application()
	: m_pWindow(nullptr),
	m_pContext(nullptr),
	m_pRenderingHandler(nullptr),
	m_pMeshRenderer(nullptr),
	m_pRayTracingRenderer(nullptr),
	m_pImgui(nullptr),
	m_pScene(nullptr),
	m_pGunMesh(nullptr),
	m_pGunAlbedo(nullptr),
	m_pInputHandler(nullptr),
	m_Camera(),
	m_IsRunning(false),
	m_UpdateCamera(false),
	m_pParticleTexture(nullptr),
	m_pParticleEmitterHandler(nullptr),
	m_NewEmitterInfo(),
	m_CurrentEmitterIdx(0),
	m_CreatingEmitter(false),
	m_pCameraPositionSpline(nullptr),
	m_pCameraDirectionSpline(nullptr),
	m_CameraSplineTimer(0.0f),
	m_CameraSplineEnabled(false),
	m_KeyInputEnabled(false)
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

	m_pContext->setRayTracingEnabled(!FORCE_RAY_TRACING_OFF);

	// Create and setup rendering handler
	m_pRenderingHandler = m_pContext->createRenderingHandler();
	m_pRenderingHandler->initialize();
	m_pRenderingHandler->setClearColor(0.0f, 0.0f, 0.0f);
	m_pRenderingHandler->setViewport((float)m_pWindow->getWidth(), (float)m_pWindow->getHeight(), 0.0f, 1.0f, 0.0f, 0.0f);

	//Create Scene
	m_pScene = m_pContext->createScene(m_pRenderingHandler);
	m_pScene->init();

	m_pRenderingHandler->setScene(m_pScene);

	TaskDispatcher::execute([this]
		{
			m_pScene->loadFromFile("assets/sponza/", "sponza.obj");
		});

	//Setup lights
	LightSetup& lightSetup = m_pScene->getLightSetup();
	lightSetup.addPointLight(PointLight(glm::vec3( 0.0f, 4.0f, 0.0f), glm::vec4(100.0f)));
	lightSetup.addPointLight(PointLight(glm::vec3( 0.0f, 4.0f, 0.0f), glm::vec4(100.0f)));
	lightSetup.addPointLight(PointLight(glm::vec3( 0.0f, 4.0f, 0.0f), glm::vec4(100.0f)));
	lightSetup.addPointLight(PointLight(glm::vec3( 0.0f, 4.0f, 0.0f), glm::vec4(100.0f)));

	VolumetricLightSettings volumetricLightSettings = {
		0.8f, 	// Scatter amount
		0.2f 	// Particle G
	};
	glm::vec3 sunDirection = glm::normalize(glm::vec3(-1.0f, -1.0f, 0.0f));
	lightSetup.setDirectionalLight(DirectionalLight(volumetricLightSettings, sunDirection, {0.6f, 0.45f, 0.2f, 1.0f}));


	// Setup renderers
	m_pMeshRenderer = m_pContext->createMeshRenderer(m_pRenderingHandler);
	m_pMeshRenderer->init();

	m_pParticleRenderer = m_pContext->createParticleRenderer(m_pRenderingHandler);
	m_pParticleRenderer->init();

	if (m_pContext->isRayTracingEnabled())
	{
		m_pRayTracingRenderer = m_pContext->createRayTracingRenderer(m_pRenderingHandler);
		m_pRayTracingRenderer->init();
	}

	//Create particlehandler
	m_pParticleEmitterHandler = m_pContext->createParticleEmitterHandler();
	m_pParticleEmitterHandler->initialize(m_pContext, &m_Camera);

	//Setup Imgui
	m_pImgui = m_pContext->createImgui();
	m_pImgui->init();
	m_pWindow->addEventHandler(m_pImgui);

	//Set renderers to renderhandler
	m_pRenderingHandler->setMeshRenderer(m_pMeshRenderer);
	m_pRenderingHandler->setParticleEmitterHandler(m_pParticleEmitterHandler);
	m_pRenderingHandler->setParticleRenderer(m_pParticleRenderer);
	m_pRenderingHandler->setImguiRenderer(m_pImgui);

	if (m_pContext->isRayTracingEnabled())
	{
		m_pRenderingHandler->setRayTracer(m_pRayTracingRenderer);
	}

	m_pRenderingHandler->setClearColor(0.0f, 0.0f, 0.0f);

	//Load resources
	ITexture2D* pPanorama = m_pContext->createTexture2D();
	TaskDispatcher::execute([&]
		{
			pPanorama->initFromFile("assets/textures/arches.hdr", ETextureFormat::FORMAT_R32G32B32A32_FLOAT, false);
			m_pSkybox = m_pRenderingHandler->generateTextureCube(pPanorama, ETextureFormat::FORMAT_R16G16B16A16_FLOAT, 2048, 1);
		});

	m_pGunMesh = m_pContext->createMesh();
	TaskDispatcher::execute([&]
		{
			m_pGunMesh->initFromFile("assets/meshes/gun.obj");
		});

	m_pGunAlbedo = m_pContext->createTexture2D();
	TaskDispatcher::execute([this]
		{
			m_pGunAlbedo->initFromFile("assets/textures/gunAlbedo.tga", ETextureFormat::FORMAT_R8G8B8A8_UNORM);
		});

	m_pGunNormal = m_pContext->createTexture2D();
	TaskDispatcher::execute([this]
		{
			m_pGunNormal->initFromFile("assets/textures/gunNormal.tga", ETextureFormat::FORMAT_R8G8B8A8_UNORM);
		});

	m_pGunMetallic = m_pContext->createTexture2D();
	TaskDispatcher::execute([this]
		{
			m_pGunMetallic->initFromFile("assets/textures/gunMetallic.tga", ETextureFormat::FORMAT_R8G8B8A8_UNORM);
		});

	m_pGunRoughness = m_pContext->createTexture2D();
	TaskDispatcher::execute([this]
		{
			m_pGunRoughness->initFromFile("assets/textures/gunRoughness.tga", ETextureFormat::FORMAT_R8G8B8A8_UNORM);
		});

	// Setup particles
	m_pParticleTexture = m_pContext->createTexture2D();
	if (!m_pParticleTexture->initFromFile("assets/textures/sun.png", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true))
	{
		LOG("Failed to create particle texture");
		SAFEDELETE(m_pParticleTexture);
	}

	ParticleEmitterInfo emitterInfo = {};
	emitterInfo.position			= glm::vec3(6.0f, 0.0f, 0.0f);
	emitterInfo.direction			= glm::normalize(glm::vec3(0.0f, 0.9f, 0.1f));
	emitterInfo.particleSize		= glm::vec2(0.2f, 0.2f);
	emitterInfo.initialSpeed		= 5.5f;
	emitterInfo.particleDuration	= 3.0f;
	emitterInfo.particlesPerSecond	= 200.0f;
	emitterInfo.spread				= glm::quarter_pi<float>() / 1.3f;
	emitterInfo.pTexture			= m_pParticleTexture;
	m_pParticleEmitterHandler->createEmitter(emitterInfo);

	// Second emitter
	emitterInfo.position.x = -emitterInfo.position.x;
	m_pParticleEmitterHandler->createEmitter(emitterInfo);

	//We can set the pointer to the material even if loading happens on another thread
	m_GunMaterial.setAlbedo(glm::vec4(1.0f));
	m_GunMaterial.setAmbientOcclusion(1.0f);
	m_GunMaterial.setMetallic(1.0f);
	m_GunMaterial.setRoughness(1.0f);
	m_GunMaterial.setAlbedoMap(m_pGunAlbedo);
	m_GunMaterial.setNormalMap(m_pGunNormal);
	m_GunMaterial.setMetallicMap(m_pGunMetallic);
	m_GunMaterial.setRoughnessMap(m_pGunRoughness);

	SamplerParams samplerParams = {};
	samplerParams.MinFilter = VK_FILTER_LINEAR;
	samplerParams.MagFilter = VK_FILTER_LINEAR;
	samplerParams.WrapModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerParams.WrapModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	m_GunMaterial.createSampler(m_pContext, samplerParams);

	//Setup camera
	m_Camera.setDirection(glm::vec3(0.0f, 0.0f, 1.0f));
	m_Camera.setPosition(glm::vec3(0.0f, 1.0f, -3.0f));
	m_Camera.setProjection(90.0f, (float)m_pWindow->getWidth(), (float)m_pWindow->getHeight(), 0.0001f, 50.0f);
	m_Camera.update();

	TaskDispatcher::waitForTasks();
	
	glm::mat4 scale = glm::scale(glm::vec3(0.75f));
	m_GraphicsIndex0 = m_pScene->submitGraphicsObject(m_pGunMesh, &m_GunMaterial, glm::translate(glm::mat4(1.0f), glm::vec3( 0.0f, 1.0f, 0.1f)) * scale);
	m_GraphicsIndex1 = m_pScene->submitGraphicsObject(m_pGunMesh, &m_GunMaterial, glm::translate(glm::mat4(1.0f), glm::vec3( 1.5f, 1.0f, 0.1f)) * scale);
	m_GraphicsIndex2 = m_pScene->submitGraphicsObject(m_pGunMesh, &m_GunMaterial, glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 1.0f, 0.1f)) * scale);

	m_pRenderingHandler->setSkybox(m_pSkybox);
	m_pWindow->show();

	SAFEDELETE(pPanorama);

	m_pScene->finalize();
	m_pRenderingHandler->onSceneUpdated(m_pScene);

	std::vector<glm::vec3> positionControlPoints
	{
		glm::vec3(-4.0f,  1.0f,  0.45f),
		glm::vec3( 5.0f,  1.0f,  0.0f),

		glm::vec3( 5.0f,  1.0f,  2.0f),
		glm::vec3(-5.0f,  1.0f,  2.0f),

		glm::vec3(-5.0f,  1.5f, -0.75f),

		glm::vec3( 5.0f,  3.0f,  0.55f),

		glm::vec3( 5.0f,  3.0f,  -2.5f),
		glm::vec3(-5.0f,  3.0f,  -2.5f),

		glm::vec3(-5.0f,  3.0f,  0.35f),
		glm::vec3( 3.0f,  5.0f,  0.35f),

		glm::vec3( 3.0f,  5.0f, -0.85f),

		glm::vec3(-4.0f,  1.0f, -0.85f)
	};


	m_pCameraPositionSpline = DBG_NEW LoopingUniformCRSpline<glm::vec3, float>(positionControlPoints);

	std::vector<glm::vec3> directionControlPoints
	{
		glm::vec3(0.0f),
		glm::vec3(0.0f),
		glm::vec3(0.0f),
		glm::vec3(0.0f),
		glm::vec3(0.0f),
		glm::vec3(0.0f),
		glm::vec3(0.0f),
		glm::vec3(0.0f),
		glm::vec3(0.0f, -2.0f,  0.0f),
		glm::vec3(0.0f, -2.0f,  0.0f),
		glm::vec3(0.0f, -1.0f,  0.0f),
		glm::vec3(0.0f)
	};

	m_pCameraDirectionSpline = DBG_NEW LoopingUniformCRSpline<glm::vec3, float>(directionControlPoints);
	m_CameraSplineTimer = 0.0f;
	m_CameraSplineEnabled = false;
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

	m_GunMaterial.release();

	SAFEDELETE(m_pSkybox);
	SAFEDELETE(m_pGunRoughness);
	SAFEDELETE(m_pGunMetallic);
	SAFEDELETE(m_pGunNormal);
	SAFEDELETE(m_pGunAlbedo);
	SAFEDELETE(m_pGunMesh);
	SAFEDELETE(m_pRenderingHandler);
	SAFEDELETE(m_pMeshRenderer);
	SAFEDELETE(m_pRayTracingRenderer);
	SAFEDELETE(m_pParticleRenderer);
	SAFEDELETE(m_pParticleTexture);
	SAFEDELETE(m_pParticleEmitterHandler);
	SAFEDELETE(m_pImgui);
	SAFEDELETE(m_pScene);

	SAFEDELETE(m_pContext);

	SAFEDELETE(m_pInputHandler);
	Input::setInputHandler(nullptr);

	SAFEDELETE(m_pWindow);

	SAFEDELETE(m_pCameraDirectionSpline);
	SAFEDELETE(m_pCameraPositionSpline);

	TaskDispatcher::release();

	LOG("Exiting Application");
}

void Application::onWindowResize(uint32_t width, uint32_t height)
{
	D_LOG("Resize w=%d h%d", width , height);

	if (width != 0 && height != 0)
	{
		if (m_pRenderingHandler)
		{
			m_pRenderingHandler->setViewport((float)width, (float)height, 0.0f, 1.0f, 0.0f, 0.0f);
			m_pRenderingHandler->onWindowResize(width, height);
		}

		m_Camera.setProjection(90.0f, float(width), float(height), 0.01f, 100.0f);
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

	if (m_KeyInputEnabled)
	{
		if (key == EKey::KEY_1)
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

static glm::vec4 g_Color = glm::vec4(1.0f);

void Application::update(double dt)
{
	Profiler::progressTimer((float)dt);

	if (!m_TestParameters.Running)
	{
		if (m_KeyInputEnabled)
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
		}
	}
	else
	{
		float deltaTimeMS = (float)dt * 1000.0f;
		m_TestParameters.FrameTimeSum += deltaTimeMS;
		m_TestParameters.FrameCount += 1.0f;
		m_TestParameters.AverageFrametime = m_TestParameters.FrameTimeSum / m_TestParameters.FrameCount;
		m_TestParameters.WorstFrametime = deltaTimeMS > m_TestParameters.WorstFrametime ? deltaTimeMS : m_TestParameters.WorstFrametime;
		m_TestParameters.BestFrametime = deltaTimeMS < m_TestParameters.BestFrametime ? deltaTimeMS : m_TestParameters.BestFrametime;

		auto& interpolatedPositionPT = m_pCameraPositionSpline->getTangent(m_CameraSplineTimer);
		glm::vec3 position = interpolatedPositionPT.position;
		glm::vec3 heading = interpolatedPositionPT.tangent;
		glm::vec3 direction = m_pCameraDirectionSpline->getPosition(m_CameraSplineTimer);

		m_CameraSplineTimer += (float)dt / (glm::max(glm::length(heading), 0.0001f));

		m_Camera.setPosition(position);
		m_Camera.setDirection(normalize(glm::normalize(heading) + direction));

		if (m_CameraSplineTimer >= m_pCameraPositionSpline->getMaxT())
		{
			m_TestParameters.CurrentRound++;
			m_CameraSplineTimer = 0.0f;

			if (m_TestParameters.CurrentRound >= m_TestParameters.NumRounds)
			{
				testFinished();
			}
		}
	}

	m_Camera.update();

	if (m_UpdateCamera)
	{
		Input::setMousePosition(m_pWindow, glm::vec2(m_pWindow->getClientWidth() / 2.0f, m_pWindow->getClientHeight() / 2.0f));
	}

	m_pParticleEmitterHandler->update((float)dt);

	static glm::mat4 rotation = glm::mat4(1.0f);
	rotation = glm::rotate(rotation, glm::radians(30.0f * float(dt)), glm::vec3(0.0f, 1.0f, 0.0f));

	const glm::mat4 scale = glm::scale(glm::vec3(0.75f));
	m_pScene->updateGraphicsObjectTransform(m_GraphicsIndex0, glm::translate(glm::mat4(1.0f), glm::vec3( 0.0f, 1.0f, 0.1f)) * rotation * scale);

	m_pScene->updateCamera(m_Camera);
	m_pScene->updateDebugParameters();

	m_pScene->updateMeshesAndGraphicsObjects();
	
	m_pRenderingHandler->onSceneUpdated(m_pScene);
}

void Application::renderUI(double dt)
{
	m_pImgui->begin(dt);

	if (m_ApplicationParameters.IsDirty)
	{
		m_pRenderingHandler->setRayTracingResolutionDenominator(m_ApplicationParameters.RayTracingResolutionDenominator);
		m_ApplicationParameters.IsDirty = false;
	}

	if (!m_TestParameters.Running)
	{
		// Color picker for mesh
		ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Color", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::ColorPicker4("##picker", glm::value_ptr(g_Color), ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
		}
		ImGui::End();

		// Particle control panel
		ImGui::SetNextWindowSize(ImVec2(420, 210), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Particles", NULL)) {
			ImGui::Text("Toggle Computation Device");
			const char* btnLabel = m_pParticleEmitterHandler->gpuComputed() ? "GPU" : "CPU";
			if (ImGui::Button(btnLabel, ImVec2(40, 25))) {
				m_pParticleEmitterHandler->toggleComputationDevice();
			}

			std::vector<ParticleEmitter*> particleEmitters = m_pParticleEmitterHandler->getParticleEmitters();

			// Emitter creation
			if (ImGui::Button("New emitter")) {
				m_CreatingEmitter = true;
				ParticleEmitter* pLatestEmitter = particleEmitters.back();

				m_NewEmitterInfo = {};
				m_NewEmitterInfo.direction = pLatestEmitter->getDirection();
				m_NewEmitterInfo.initialSpeed = pLatestEmitter->getInitialSpeed();
				m_NewEmitterInfo.particleSize = pLatestEmitter->getParticleSize();
				m_NewEmitterInfo.spread = pLatestEmitter->getSpread();
				m_NewEmitterInfo.particlesPerSecond = pLatestEmitter->getParticlesPerSecond();
				m_NewEmitterInfo.particleDuration = pLatestEmitter->getParticleDuration();
			}

			// Emitter selection
			m_CurrentEmitterIdx = std::min(m_CurrentEmitterIdx, particleEmitters.size() - 1);
			int emitterIdxInt = (int)m_CurrentEmitterIdx;

			if (ImGui::SliderInt("Emitter selection", &emitterIdxInt, 0, int(particleEmitters.size() - 1))) {
				m_CurrentEmitterIdx = (size_t)emitterIdxInt;
			}

			// Get current emitter data
			ParticleEmitter* pEmitter = particleEmitters[m_CurrentEmitterIdx];
			glm::vec3 emitterPos = pEmitter->getPosition();
			glm::vec2 particleSize = pEmitter->getParticleSize();

			glm::vec3 emitterDirection = pEmitter->getDirection();
			float yaw = getYaw(emitterDirection);
			float oldYaw = yaw;
			float pitch = getPitch(emitterDirection);
			float oldPitch = pitch;

			float emitterSpeed = pEmitter->getInitialSpeed();
			float spread = pEmitter->getSpread();

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
			if (ImGui::SliderFloat2("Size", glm::value_ptr(particleSize), 0.0f, 1.0f)) {
				pEmitter->setParticleSize(particleSize);
			}
			if (ImGui::SliderFloat("Speed", &emitterSpeed, 0.0f, 20.0f)) {
				pEmitter->setInitialSpeed(emitterSpeed);
			}
			if (ImGui::SliderFloat("Spread", &spread, 0.0f, glm::pi<float>())) {
				pEmitter->setSpread(spread);
			}

			ImGui::Text("Particles: %d", pEmitter->getParticleCount());
		}
		ImGui::End();

		// Emitter creation window
		if (m_CreatingEmitter) {
			// Open a new window
			ImGui::SetNextWindowSize(ImVec2(420, 210), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Emitter Creation", NULL)) {
				float yaw = getYaw(m_NewEmitterInfo.direction);
				float oldYaw = yaw;
				float pitch = getPitch(m_NewEmitterInfo.direction);
				float oldPitch = pitch;

				ImGui::SliderFloat3("Position", glm::value_ptr(m_NewEmitterInfo.position), -10.0f, 10.0f);
				if (ImGui::SliderFloat("Yaw", &yaw, 0.01f - glm::pi<float>(), glm::pi<float>() - 0.01f)) {
					applyYaw(m_NewEmitterInfo.direction, yaw - oldYaw);
				}
				if (ImGui::SliderFloat("Pitch", &pitch, 0.01f - glm::half_pi<float>(), glm::half_pi<float>() - 0.01f)) {
					applyPitch(m_NewEmitterInfo.direction, pitch - oldPitch);
				}
				if (ImGui::SliderFloat3("Direction", glm::value_ptr(m_NewEmitterInfo.direction), -1.0f, 1.0f)) {
					m_NewEmitterInfo.direction = glm::normalize(m_NewEmitterInfo.direction);
				}
				ImGui::SliderFloat2("Particle Size", glm::value_ptr(m_NewEmitterInfo.particleSize), 0.0f, 1.0f);
				ImGui::SliderFloat("Speed", &m_NewEmitterInfo.initialSpeed, 0.0f, 20.0f);
				ImGui::SliderFloat("Spread", &m_NewEmitterInfo.spread, 0.0f, glm::pi<float>());
				ImGui::InputFloat("Particle duration", &m_NewEmitterInfo.particleDuration);
				ImGui::InputFloat("Particles per second", &m_NewEmitterInfo.particlesPerSecond);
				ImGui::Text("Emitted particles: %d", int(m_NewEmitterInfo.particlesPerSecond * m_NewEmitterInfo.particleDuration));

				if (ImGui::Button("Create")) {
					m_CreatingEmitter = false;
					m_NewEmitterInfo.pTexture = m_pParticleTexture;

					m_pParticleEmitterHandler->createEmitter(m_NewEmitterInfo);
				}
				if (ImGui::Button("Cancel")) {
					m_CreatingEmitter = false;
				}
				ImGui::End();
			}
		}

		// Draw profiler UI
		ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Profiler", NULL))
		{
			//TODO: If the renderinghandler have all renderers/handlers, why are we calling their draw UI functions should this not be done by the renderinghandler
			m_pParticleEmitterHandler->drawProfilerUI();
			m_pRenderingHandler->drawProfilerUI();
		}
		ImGui::End();

		if (m_pContext->isRayTracingEnabled())
		{
			// Draw Ray Tracing UI
			ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Ray Tracer", NULL))
			{
				m_pRayTracingRenderer->renderUI();
			}
			ImGui::End();
		}

		// Draw Scene UI
		m_pScene->renderUI();
	}

	// Draw Application UI
	ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Application", NULL))
	{
		if (!m_TestParameters.Running)
		{
			//ImGui::Checkbox("Camera Spline Enabled", &m_CameraSplineEnabled);
			//ImGui::SliderFloat("Camera Timer", &m_CameraSplineTimer, 0.0f, m_CameraPositionSpline->getMaxT());

			//Input Parameters
			if (ImGui::Button("Toggle Key Input"))
			{
				m_KeyInputEnabled = !m_KeyInputEnabled;
			}
			ImGui::SameLine();
			if (m_KeyInputEnabled)
				ImGui::Text("Key Input Enabled");
			else
				ImGui::Text("Key Input Disabled");

			ImGui::NewLine();

			//Graphical Parameters

			m_ApplicationParameters.IsDirty = m_ApplicationParameters.IsDirty || ImGui::SliderInt("Ray Tracing Res. Denom.", &m_ApplicationParameters.RayTracingResolutionDenominator, 1, 8);

			ImGui::NewLine();

			//Test Parameters
			ImGui::InputText("Test Name", m_TestParameters.TestName, 256);
			ImGui::SliderInt("Number of Test Rounds", &m_TestParameters.NumRounds, 1, 5);

			if (ImGui::Button("Start Test"))
			{
				m_CameraSplineTimer = 0.0f;

				m_TestParameters.Running = true;
				m_TestParameters.FrameTimeSum = 0.0f;
				m_TestParameters.FrameCount = 0.0f;
				m_TestParameters.AverageFrametime = 0.0f;
				m_TestParameters.WorstFrametime = std::numeric_limits<float>::min();
				m_TestParameters.BestFrametime = std::numeric_limits<float>::max();
				m_TestParameters.CurrentRound = 0;
			}

			ImGui::Text("Previous Test: %s : %d ", m_TestParameters.TestName, m_TestParameters.NumRounds);
			ImGui::Text("Average Frametime: %f", m_TestParameters.AverageFrametime);
			ImGui::Text("Worst Frametime: %f", m_TestParameters.WorstFrametime);
			ImGui::Text("Best Frametime: %f", m_TestParameters.BestFrametime);
			ImGui::Text("Frame count: %f", m_TestParameters.FrameCount);
		}
		else
		{
			if (ImGui::Button("Stop Test"))
			{
				m_TestParameters.Running = false;
			}

			ImGui::Text("Round: %d / %d", m_TestParameters.CurrentRound, m_TestParameters.NumRounds);
			ImGui::Text("Average Frametime: %f", m_TestParameters.AverageFrametime);
			ImGui::Text("Worst Frametime: %f", m_TestParameters.WorstFrametime);
			ImGui::Text("Best Frametime: %f", m_TestParameters.BestFrametime);
			ImGui::Text("Frame Count: %f", m_TestParameters.FrameCount);
		}

	}
	ImGui::End();

	m_pImgui->end();
}

void Application::render(double dt)
{
	UNREFERENCED_PARAMETER(dt);

	m_pRenderingHandler->render(m_pScene);
}

void Application::testFinished()
{
	m_TestParameters.Running = false;

	sanitizeString(m_TestParameters.TestName, 256);

	std::ofstream fileStream;
	fileStream.open("Results/test_" + std::string(m_TestParameters.TestName) + ".txt");

	if (fileStream.is_open())
	{
		fileStream << "Avg. FT\tWorst FT\tBest FT\tFrame Count" << std::endl;
		fileStream << m_TestParameters.AverageFrametime << "\t";
		fileStream << m_TestParameters.WorstFrametime << "\t";
		fileStream << m_TestParameters.BestFrametime << "\t";
		fileStream << (uint32_t)m_TestParameters.FrameCount;
	}

	fileStream.close();
}

void Application::sanitizeString(char string[], uint32_t numCharacters)
{
	static std::string illegalChars = "\\/:?\"<>|";
	for (uint32_t i = 0; i < numCharacters; i++)
	{
		if (illegalChars.find(string[i]) != std::string::npos)
		{
			string[i] = '_';
		}
	}
}
