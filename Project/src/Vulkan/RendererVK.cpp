#include "RendererVK.h"
#include "MeshVK.h"
#include "ImguiVK.h"
#include "BufferVK.h"
#include "PipelineVK.h"
#include "SwapChainVK.h"
#include "RenderPassVK.h"
#include "CommandPoolVK.h"
#include "FrameBufferVK.h"
#include "DescriptorSetVK.h"
#include "CommandBufferVK.h"
#include "DescriptorPoolVK.h"
#include "PipelineLayoutVK.h"
#include "GraphicsContextVK.h"
#include "SamplerVK.h"
#include "Texture2DVK.h"

#include "Core/Camera.h"

#include <glm/gtc/type_ptr.hpp>

//TEMP
#include "Ray Tracing/ShaderBindingTableVK.h"
#include "Ray Tracing/RayTracingPipelineVK.h"
#include "Ray Tracing/RayTracingSceneVK.h"
#include "ShaderVK.h"
#include "Scene.h"
#include "ImageVK.h"
#include "ImageViewVK.h"

RendererVK::RendererVK(GraphicsContextVK* pContext)
	: m_pContext(pContext),
	m_ppCommandPools(),
	m_ppCommandBuffers(),
	m_pRenderPass(nullptr),
	m_ppBackbuffers(),
	m_pPipeline(nullptr),
	m_pDescriptorSet(nullptr),
	m_pDescriptorPool(nullptr),
	m_pDescriptorSetLayout(nullptr),
	m_ClearColor(),
	m_ClearDepth(),
	m_Viewport(),
	m_ScissorRect(),
	m_CurrentFrame(0),
	m_BackBufferIndex(0)
{
	m_ClearDepth.depthStencil.depth = 1.0f;
	m_ClearDepth.depthStencil.stencil = 0;
}

RendererVK::~RendererVK()
{
	m_pContext->getDevice()->wait();

	releaseFramebuffers();

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		SAFEDELETE(m_ppCommandPools[i]);
		if (m_ImageAvailableSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(m_pContext->getDevice()->getDevice(), m_ImageAvailableSemaphores[i], nullptr);
		}
		if (m_RenderFinishedSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(m_pContext->getDevice()->getDevice(), m_RenderFinishedSemaphores[i], nullptr);
		}
	}

	SAFEDELETE(m_pRenderPass);
	SAFEDELETE(m_pPipelineLayout);
	SAFEDELETE(m_pPipeline);
	SAFEDELETE(m_pDescriptorPool);
	SAFEDELETE(m_pDescriptorSetLayout);
	SAFEDELETE(m_pCameraBuffer);

	//Ray Tracing Stuff
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		SAFEDELETE(m_ppComputeCommandPools[i]);
	}

	SAFEDELETE(m_pRayTracingScene);
	SAFEDELETE(m_pRayTracingPipeline);
	SAFEDELETE(m_pRayTracingPipelineLayout);
	SAFEDELETE(m_pSBT);
	SAFEDELETE(m_pRayTracingStorageImage);
	SAFEDELETE(m_pRayTracingStorageImageView);

	SAFEDELETE(m_pRayTracingDescriptorPool);
	SAFEDELETE(m_pRayTracingDescriptorSetLayout);

	SAFEDELETE(m_pRayTracingUniformBuffer);

	SAFEDELETE(m_pMeshCube);
	SAFEDELETE(m_pMeshGun);
	SAFEDELETE(m_pMeshSphere);
	SAFEDELETE(m_pMeshPlane);

	SAFEDELETE(m_pRaygenShader);
	SAFEDELETE(m_pClosestHitShader);
	SAFEDELETE(m_pClosestHitShadowShader);
	SAFEDELETE(m_pMissShader);
	SAFEDELETE(m_pMissShadowShader);

	SAFEDELETE(m_pSampler);

	SAFEDELETE(m_pGunMaterial);
	SAFEDELETE(m_pCubeMaterial);
	SAFEDELETE(m_pSphereMaterial);
	SAFEDELETE(m_pPlaneMaterial);

	m_pContext = nullptr;
}

bool RendererVK::init()
{
	DeviceVK* pDevice = m_pContext->getDevice();
	SwapChainVK* pSwapChain = m_pContext->getSwapChain();

	if (!createRenderPass())
	{
		return false;
	}

	createFramebuffers();

	if (!createCommandPoolAndBuffers())
	{
		return false;
	}

	if (!createPipelineLayouts())
	{
		return false;
	}

	if (!createPipelines())
	{
		return false;
	}

	if (!createSemaphores())
	{
		return false;
	}

	if (!createBuffers())
	{
		return false;
	}

	//Last thing is to write to the descriptor set
	//m_pDescriptorSet->writeUniformBufferDescriptor(m_pCameraBuffer->getBuffer(), 0);

	//Testing
	//Vertex vertices[] = 
	//{
	//	{ glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(1.0f,  0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
	//	{ glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(1.0f,  0.0f, 0.0f), glm::vec2(1.0f, 0.0f) },
	//	{ glm::vec3( 0.0f, -1.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(1.0f,  0.0f, 0.0f), glm::vec2(0.0f, 1.0f) },
	//};

	//// Setup indices
	//uint32_t indices[] = { 0, 1, 2 };

	//IMesh* pMesh = m_pContext->createMesh();
	//pMesh->initFromMemory(vertices, 3, indices, 3);

	Vertex cubeVertices[] =
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

	uint32_t cubeIndices[] =
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

	Vertex planeVertices[] =
	{
		//TOP FACE
		{ glm::vec4(-0.5, 0.5, 0.5, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) },
		{ glm::vec4(0.5,  0.5,  0.5, 1.0f),  glm::vec4(0.0f,  1.0f,  0.0f, 0.0f), glm::vec4(1.0f,  0.0f, 0.0f,  0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f) },
		{ glm::vec4(-0.5,  0.5, -0.5, 1.0f), glm::vec4(0.0f,  1.0f,  0.0f, 0.0f), glm::vec4(1.0f,  0.0f, 0.0f,  0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) },
		{ glm::vec4(0.5,  0.5, -0.5, 1.0f),  glm::vec4(0.0f,  1.0f,  0.0f, 0.0f), glm::vec4(1.0f,  0.0f, 0.0f,  0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 0.0f) },
	};

	uint32_t planeIndices[] =
	{
		//TOP FACE
		2, 1, 0,
		2, 3, 1,
	};

	m_pGunMaterial = new TempMaterial();
	m_pGunMaterial->pAlbedo = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pGunMaterial->pAlbedo->initFromFile("assets/textures/gunAlbedo.tga");

	m_pGunMaterial->pNormalMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pGunMaterial->pNormalMap->initFromFile("assets/textures/gunNormal.tga");

	m_pGunMaterial->pRoughnessMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pGunMaterial->pRoughnessMap->initFromFile("assets/textures/gunRoughness.tga");

	m_pCubeMaterial = new TempMaterial();
	m_pCubeMaterial->pAlbedo = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pCubeMaterial->pAlbedo->initFromFile("assets/textures/cubeAlbedo.jpg");

	m_pCubeMaterial->pNormalMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pCubeMaterial->pNormalMap->initFromFile("assets/textures/cubeNormal.jpg");

	m_pCubeMaterial->pRoughnessMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pCubeMaterial->pRoughnessMap->initFromFile("assets/textures/cubeRoughness.jpg");

	m_pSphereMaterial = new TempMaterial();
	m_pSphereMaterial->pAlbedo = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pSphereMaterial->pAlbedo->initFromFile("assets/textures/whiteTransparent.png");

	m_pSphereMaterial->pNormalMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pSphereMaterial->pNormalMap->initFromFile("assets/textures/cubeNormal.jpg");

	m_pSphereMaterial->pRoughnessMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pSphereMaterial->pRoughnessMap->initFromFile("assets/textures/whiteOpaque.png");

	m_pPlaneMaterial = new TempMaterial();
	m_pPlaneMaterial->pAlbedo = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pPlaneMaterial->pAlbedo->initFromFile("assets/textures/woodAlbedo.png");

	m_pPlaneMaterial->pNormalMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pPlaneMaterial->pNormalMap->initFromFile("assets/textures/woodNormal.png");

	m_pPlaneMaterial->pRoughnessMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pPlaneMaterial->pRoughnessMap->initFromFile("assets/textures/woodRoughness.png");

	m_pMeshCube = m_pContext->createMesh();
	m_pMeshCube->initFromMemory(cubeVertices, 24, cubeIndices, 36);
	
	m_pMeshGun = m_pContext->createMesh();
	m_pMeshGun->initFromFile("assets/meshes/gun.obj");

	m_pMeshSphere = m_pContext->createMesh();
	m_pMeshSphere->initFromFile("assets/meshes/sphere.obj");

	m_pMeshPlane = m_pContext->createMesh();
	m_pMeshPlane->initFromMemory(planeVertices, 4, planeIndices, 6);

	m_Matrix0 = glm::transpose(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));
	m_Matrix1 = glm::transpose(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
	m_Matrix2 = glm::transpose(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f)));
	m_Matrix3 = glm::transpose(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 0.0f)));
	m_Matrix4 = glm::transpose(glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
	m_Matrix5 = glm::transpose(glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 1.0f, 20.0f)));
	m_Matrix5 = glm::transpose(glm::translate(m_Matrix5, glm::vec3(0.0f, -1.0f, 0.0f)));

	m_pRayTracingScene = new RayTracingSceneVK(m_pContext);
	m_InstanceIndex0 = m_pRayTracingScene->addGraphicsObjectInstance(m_pMeshGun, m_pGunMaterial, m_Matrix0);
	m_InstanceIndex1 = m_pRayTracingScene->addGraphicsObjectInstance(m_pMeshGun, m_pGunMaterial, m_Matrix1);
	m_InstanceIndex2 = m_pRayTracingScene->addGraphicsObjectInstance(m_pMeshGun, m_pGunMaterial, m_Matrix2);
	m_InstanceIndex3 = m_pRayTracingScene->addGraphicsObjectInstance(m_pMeshCube, m_pCubeMaterial, m_Matrix3);
	m_InstanceIndex4 = m_pRayTracingScene->addGraphicsObjectInstance(m_pMeshSphere, m_pSphereMaterial, m_Matrix4);
	m_InstanceIndex5 = m_pRayTracingScene->addGraphicsObjectInstance(m_pMeshPlane, m_pPlaneMaterial, m_Matrix5);
	m_pRayTracingScene->finalize();

	m_TempTimer = 0;

	RaygenGroupParams raygenGroupParams = {};
	HitGroupParams hitGroupParams = {};
	HitGroupParams hitGroupShadowParams = {};
	MissGroupParams missGroupParams = {};
	MissGroupParams missGroupShadowParams = {};

	{
		m_pRaygenShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		m_pRaygenShader->initFromFile(EShader::RAYGEN_SHADER, "main", "assets/shaders/raytracing/raygen.spv");
		m_pRaygenShader->finalize();
		raygenGroupParams.pRaygenShader = m_pRaygenShader;

		m_pClosestHitShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		m_pClosestHitShader->initFromFile(EShader::CLOSEST_HIT_SHADER, "main", "assets/shaders/raytracing/closesthit.spv");
		m_pClosestHitShader->finalize();
		m_pClosestHitShader->setSpecializationConstant<uint32_t>(0, 3);
		hitGroupParams.pClosestHitShader = m_pClosestHitShader;

		m_pClosestHitShadowShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		m_pClosestHitShadowShader->initFromFile(EShader::CLOSEST_HIT_SHADER, "main", "assets/shaders/raytracing/closesthitShadow.spv");
		m_pClosestHitShadowShader->finalize();
		m_pClosestHitShadowShader->setSpecializationConstant<uint32_t>(0, 3);
		hitGroupShadowParams.pClosestHitShader = m_pClosestHitShadowShader;

		m_pMissShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		m_pMissShader->initFromFile(EShader::MISS_SHADER, "main", "assets/shaders/raytracing/miss.spv");
		m_pMissShader->finalize();
		missGroupParams.pMissShader = m_pMissShader;

		m_pMissShadowShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		m_pMissShadowShader->initFromFile(EShader::MISS_SHADER, "main", "assets/shaders/raytracing/missShadow.spv");
		m_pMissShadowShader->finalize();
		missGroupShadowParams.pMissShader = m_pMissShadowShader;
	}

	std::cout << "Creating RTX Pipeline Layout" << std::endl;
	createRayTracingPipelineLayouts();

	m_pRayTracingPipeline = new RayTracingPipelineVK(m_pContext->getDevice());
	m_pRayTracingPipeline->addRaygenShaderGroup(raygenGroupParams);
	m_pRayTracingPipeline->addMissShaderGroup(missGroupParams);
	m_pRayTracingPipeline->addMissShaderGroup(missGroupShadowParams);
	m_pRayTracingPipeline->addHitShaderGroup(hitGroupParams);
	m_pRayTracingPipeline->addHitShaderGroup(hitGroupShadowParams);
	m_pRayTracingPipeline->finalize(m_pRayTracingPipelineLayout);
	
	m_pSBT = new ShaderBindingTableVK(m_pContext);
	m_pSBT->init(m_pRayTracingPipeline);

	ImageParams imageParams = {};
	imageParams.Type = VK_IMAGE_TYPE_2D;
	imageParams.Format = m_pContext->getSwapChain()->getFormat();
	imageParams.Extent.width = m_pContext->getSwapChain()->getExtent().width;
	imageParams.Extent.height = m_pContext->getSwapChain()->getExtent().height;
	imageParams.Extent.depth = 1;
	imageParams.MipLevels = 1;
	imageParams.ArrayLayers = 1;
	imageParams.Samples = VK_SAMPLE_COUNT_1_BIT;
	imageParams.Usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	imageParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	
	m_pRayTracingStorageImage = new ImageVK(m_pContext->getDevice());
	m_pRayTracingStorageImage->init(imageParams);

	ImageViewParams imageViewParams = {};
	imageViewParams.Type = VK_IMAGE_VIEW_TYPE_2D;
	imageViewParams.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewParams.FirstMipLevel = 0;
	imageViewParams.MipLevels = 1;
	imageViewParams.FirstLayer = 0;
	imageViewParams.LayerCount = 1;
	
	m_pRayTracingStorageImageView = new ImageViewVK(m_pContext->getDevice(), m_pRayTracingStorageImage);
	m_pRayTracingStorageImageView->init(imageViewParams);

	CommandBufferVK* pTempCommandBuffer = m_ppCommandPools[0]->allocateCommandBuffer();
	pTempCommandBuffer->reset();
	pTempCommandBuffer->begin();
	
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.image = m_pRayTracingStorageImage->getImage();
	imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	imageMemoryBarrier.srcAccessMask = 0;

	vkCmdPipelineBarrier(
		pTempCommandBuffer->getCommandBuffer(),
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);
	pTempCommandBuffer->end();
	
	m_pContext->getDevice()->executeCommandBuffer(m_pContext->getDevice()->getGraphicsQueue(), pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
	m_pContext->getDevice()->wait();

	m_ppCommandPools[0]->freeCommandBuffer(&pTempCommandBuffer);

	BufferParams rayTracingUniformBufferParams = {};
	rayTracingUniformBufferParams.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	rayTracingUniformBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	rayTracingUniformBufferParams.SizeInBytes = sizeof(CameraBuffer);
	
	m_pRayTracingUniformBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pRayTracingUniformBuffer->init(rayTracingUniformBufferParams);

	auto& allMaterials = m_pRayTracingScene->getAllMaterials();

	std::vector<VkImageView> albedoImageViews;
	albedoImageViews.reserve(MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	std::vector<VkImageView> normalImageViews;
	normalImageViews.reserve(MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	std::vector<VkImageView> roughnessImageViews;
	roughnessImageViews.reserve(MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);

	m_pSampler = new SamplerVK(m_pContext->getDevice());
	m_pSampler->init(VkFilter::VK_FILTER_LINEAR, VkFilter::VK_FILTER_LINEAR, VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT, VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT);
	std::vector<VkSampler> samplers;
	samplers.reserve(MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);

	for (uint32_t i = 0; i < MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES; i++)
	{
		samplers.push_back(m_pSampler->getSampler());

		if (i < allMaterials.size())
		{
			albedoImageViews.push_back(allMaterials[i]->pAlbedo->getImageView()->getImageView());
			normalImageViews.push_back(allMaterials[i]->pNormalMap->getImageView()->getImageView());
			roughnessImageViews.push_back(allMaterials[i]->pRoughnessMap->getImageView()->getImageView());
		}
		else
		{
			albedoImageViews.push_back(allMaterials[0]->pAlbedo->getImageView()->getImageView());
			normalImageViews.push_back(allMaterials[0]->pNormalMap->getImageView()->getImageView());
			roughnessImageViews.push_back(allMaterials[0]->pRoughnessMap->getImageView()->getImageView());
		}
	}

	m_pRayTracingDescriptorSet->writeAccelerationStructureDescriptor(m_pRayTracingScene->getTLAS().accelerationStructure, 0);
	m_pRayTracingDescriptorSet->writeStorageImageDescriptor(m_pRayTracingStorageImageView->getImageView(), 1);
	m_pRayTracingDescriptorSet->writeUniformBufferDescriptor(m_pRayTracingUniformBuffer->getBuffer(), 2);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(m_pRayTracingScene->getCombinedVertexBuffer()->getBuffer(), 3);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(m_pRayTracingScene->getCombinedIndexBuffer()->getBuffer(), 4);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(m_pRayTracingScene->getMeshIndexBuffer()->getBuffer(), 5);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(albedoImageViews.data(), samplers.data(), MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES, 6);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(normalImageViews.data(), samplers.data(), MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES, 7);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(roughnessImageViews.data(), samplers.data(), MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES, 8);
	
	return true;
}

void RendererVK::onWindowResize(uint32_t width, uint32_t height)
{
	m_pContext->getDevice()->wait();
	releaseFramebuffers();
	
	m_pContext->getSwapChain()->resize(width, height);

	createFramebuffers();
}

void RendererVK::beginFrame(const Camera& camera)
{
	//Prepare for frame
	m_pContext->getSwapChain()->acquireNextImage(m_ImageAvailableSemaphores[m_CurrentFrame]);
	uint32_t backBufferIndex = m_pContext->getSwapChain()->getImageIndex();

	m_ppCommandBuffers[m_CurrentFrame]->reset();
	m_ppCommandPools[m_CurrentFrame]->reset();

	m_ppCommandBuffers[m_CurrentFrame]->begin();

	//Update camera
	CameraBuffer cameraBuffer = {};
	cameraBuffer.Projection = camera.getProjectionMat();
	cameraBuffer.View = camera.getViewMat();
	m_ppCommandBuffers[m_CurrentFrame]->updateBuffer(m_pCameraBuffer, 0, (const void*)&cameraBuffer, sizeof(CameraBuffer));

	//Start renderpass
	VkClearValue clearValues[] = { m_ClearColor, m_ClearDepth };
	m_ppCommandBuffers[m_CurrentFrame]->beginRenderPass(m_pRenderPass, m_ppBackbuffers[backBufferIndex], m_Viewport.width, m_Viewport.height, clearValues, 2);

	m_ppCommandBuffers[m_CurrentFrame]->setViewports(&m_Viewport, 1);
	m_ppCommandBuffers[m_CurrentFrame]->setScissorRects(&m_ScissorRect, 1);

	m_ppCommandBuffers[m_CurrentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &m_pDescriptorSet, 0, nullptr);
}

void RendererVK::endFrame()
{
	m_ppCommandBuffers[m_CurrentFrame]->endRenderPass();
	m_ppCommandBuffers[m_CurrentFrame]->end();

	//Execute commandbuffer
	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	DeviceVK* pDevice = m_pContext->getDevice();
	pDevice->executeCommandBuffer(pDevice->getGraphicsQueue(), m_ppCommandBuffers[m_CurrentFrame], waitSemaphores, waitStages, 1, signalSemaphores, 1);
}

void RendererVK::beginRayTraceFrame(const Camera& camera)
{
	m_TempTimer += 0.001f;
	glm::mat4 matrix1 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_Matrix1 = glm::transpose(glm::scale(matrix1, glm::vec3(glm::sin(2.0f * m_TempTimer) * 0.5f + 0.5f)));
	glm::mat4 matrix2 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));
	m_Matrix2 = glm::transpose(glm::rotate(matrix2, m_TempTimer, glm::vec3(0.0f, 1.0f, 0.0f)));
	m_Matrix3 = glm::transpose(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f + glm::sin(m_TempTimer), 0.0)));
	glm::mat4 matrix4 = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	m_Matrix4 = glm::transpose(glm::rotate(matrix4, m_TempTimer, glm::vec3(0.0f, 1.0f, 0.0f)));
	m_pRayTracingScene->updateGraphicsObject(m_InstanceIndex1, m_Matrix1);
	m_pRayTracingScene->updateGraphicsObject(m_InstanceIndex2, m_Matrix2);
	m_pRayTracingScene->updateGraphicsObject(m_InstanceIndex3, m_Matrix3);
	m_pRayTracingScene->updateGraphicsObject(m_InstanceIndex4, m_Matrix4);
	m_pRayTracingScene->update();

	m_ppComputeCommandBuffers[m_CurrentFrame]->reset();
	m_ppComputeCommandPools[m_CurrentFrame]->reset();

	m_ppComputeCommandBuffers[m_CurrentFrame]->begin();

	CameraBuffer cameraBuffer = {};
	cameraBuffer.Projection = glm::inverse(camera.getProjectionMat());
	cameraBuffer.View = glm::inverse(camera.getViewMat());
	m_ppComputeCommandBuffers[m_CurrentFrame]->updateBuffer(m_pRayTracingUniformBuffer, 0, (const void*)&cameraBuffer, sizeof(CameraBuffer));

	m_ppComputeCommandBuffers[m_CurrentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipelineLayout, 0, 1, &m_pRayTracingDescriptorSet, 0, nullptr);
}

void RendererVK::endRayTraceFrame()
{
	//m_ppCommandBuffers[m_CurrentFrame]->endRenderPass();
	m_ppComputeCommandBuffers[m_CurrentFrame]->end();

	DeviceVK* pDevice = m_pContext->getDevice();
	pDevice->executeCommandBuffer(pDevice->getComputeQueue(), m_ppComputeCommandBuffers[m_CurrentFrame], nullptr, nullptr, 0, nullptr, 0);

	m_pContext->getDevice()->wait();

	//Prepare for frame
	m_pContext->getSwapChain()->acquireNextImage(m_ImageAvailableSemaphores[m_CurrentFrame]);

	m_ppCommandBuffers[m_CurrentFrame]->reset();
	m_ppCommandPools[m_CurrentFrame]->reset();

	m_ppCommandBuffers[m_CurrentFrame]->begin();

	ImageVK* pCurrentImage = m_pContext->getSwapChain()->getImage(m_pContext->getSwapChain()->getImageIndex());
	m_ppCommandBuffers[m_CurrentFrame]->transitionImageLayout(pCurrentImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_ppCommandBuffers[m_CurrentFrame]->transitionImageLayout(m_pRayTracingStorageImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	VkImageCopy copyRegion{};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = { 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = { 0, 0, 0 };
	copyRegion.extent = { m_pContext->getSwapChain()->getExtent().width, m_pContext->getSwapChain()->getExtent().height, 1 };
	vkCmdCopyImage(m_ppCommandBuffers[m_CurrentFrame]->getCommandBuffer(), m_pRayTracingStorageImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pCurrentImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	m_ppCommandBuffers[m_CurrentFrame]->transitionImageLayout(pCurrentImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	m_ppCommandBuffers[m_CurrentFrame]->transitionImageLayout(m_pRayTracingStorageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

	m_ppCommandBuffers[m_CurrentFrame]->end();

	//Execute commandbuffer
	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	pDevice->executeCommandBuffer(pDevice->getGraphicsQueue(), m_ppCommandBuffers[m_CurrentFrame], waitSemaphores, waitStages, 1, signalSemaphores, 1);
}

void RendererVK::setClearColor(float r, float g, float b)
{
	setClearColor(glm::vec3(r, g, b));
}

void RendererVK::setClearColor(const glm::vec3& color)
{
	m_ClearColor.color.float32[0] = color.r;
	m_ClearColor.color.float32[1] = color.g;
	m_ClearColor.color.float32[2] = color.b;
	m_ClearColor.color.float32[3] = 1.0f;
}

void RendererVK::setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY)
{
	m_Viewport.x = topX;
	m_Viewport.y = topY;
	m_Viewport.width	= width;
	m_Viewport.height	= height;
	m_Viewport.minDepth = minDepth;
	m_Viewport.maxDepth = maxDepth;

	m_ScissorRect.extent.width = width;
	m_ScissorRect.extent.height = height;
	m_ScissorRect.offset.x = 0;
	m_ScissorRect.offset.y = 0;
}

void RendererVK::swapBuffers()
{
	m_pContext->swapBuffers(m_RenderFinishedSemaphores[m_CurrentFrame]);
	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RendererVK::submitMesh(IMesh* pMesh, const glm::vec4& color, const glm::mat4& transform)
{
	ASSERT(pMesh != nullptr);

	m_ppCommandBuffers[m_CurrentFrame]->bindGraphicsPipeline(m_pPipeline);

	m_ppCommandBuffers[m_CurrentFrame]->pushConstants(m_pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,				   sizeof(glm::mat4), (const void*)glm::value_ptr(transform));
	m_ppCommandBuffers[m_CurrentFrame]->pushConstants(m_pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), (const void*)glm::value_ptr(color));

	BufferVK* pIndexBuffer = reinterpret_cast<BufferVK*>(pMesh->getIndexBuffer());
	m_ppCommandBuffers[m_CurrentFrame]->bindIndexBuffer(pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

    static bool submit = true;
	if (submit)
	{
		BufferVK* pVertBuffer = reinterpret_cast<BufferVK*>(pMesh->getVertexBuffer());
		m_pDescriptorSet->writeStorageBufferDescriptor(pVertBuffer->getBuffer(), 1);

		submit = false;
	}

	m_ppCommandBuffers[m_CurrentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &m_pDescriptorSet, 0, nullptr);

	m_ppCommandBuffers[m_CurrentFrame]->drawIndexInstanced(pMesh->getIndexCount(), 1, 0, 0, 0);
}

void RendererVK::traceRays()
{
	vkCmdBindPipeline(m_ppComputeCommandBuffers[m_CurrentFrame]->getCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipeline->getPipeline());

	m_ppComputeCommandBuffers[m_CurrentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipelineLayout, 0, 1, &m_pRayTracingDescriptorSet, 0, nullptr);

	m_ppComputeCommandBuffers[m_CurrentFrame]->traceRays(m_pSBT, m_pContext->getSwapChain()->getExtent().width, m_pContext->getSwapChain()->getExtent().height, 0);
}

void RendererVK::drawImgui(IImgui* pImgui)
{
	pImgui->render(m_ppCommandBuffers[m_CurrentFrame]);
}

void RendererVK::drawTriangle(const glm::vec4& color, const glm::mat4& transform)
{
	m_ppCommandBuffers[m_CurrentFrame]->bindGraphicsPipeline(m_pPipeline);

	m_ppCommandBuffers[m_CurrentFrame]->pushConstants(m_pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,					sizeof(glm::mat4), (const void*)glm::value_ptr(transform));
	m_ppCommandBuffers[m_CurrentFrame]->pushConstants(m_pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4),	sizeof(glm::vec4), (const void*)glm::value_ptr(color));
	
	m_ppCommandBuffers[m_CurrentFrame]->drawInstanced(3, 1, 0, 0);
}

bool RendererVK::createSemaphores()
{
	//Create semaphores
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(m_pContext->getDevice()->getDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]), "Failed to create semaphores for a Frame");
		VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(m_pContext->getDevice()->getDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]), "Failed to create semaphores for a Frame");
	}

	return true;
}

bool RendererVK::createCommandPoolAndBuffers()
{
	DeviceVK* pDevice = m_pContext->getDevice();

	const uint32_t graphicsQueueFamilyIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, graphicsQueueFamilyIndex);
		
		if (!m_ppCommandPools[i]->init())
		{
			return false;
		}

		m_ppCommandBuffers[i] = m_ppCommandPools[i]->allocateCommandBuffer();
		if (m_ppCommandBuffers[i] == nullptr)
		{
			return false;
		}
	}

	const uint32_t computeQueueFamilyIndex = pDevice->getQueueFamilyIndices().computeFamily.value();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppComputeCommandPools[i] = new CommandPoolVK(pDevice, computeQueueFamilyIndex);

		if (!m_ppComputeCommandPools[i]->init())
		{
			return false;
		}

		m_ppComputeCommandBuffers[i] = m_ppComputeCommandPools[i]->allocateCommandBuffer();
		if (m_ppComputeCommandBuffers[i] == nullptr)
		{
			return false;
		}
	}

	return true;
}

void RendererVK::createFramebuffers()
{
	SwapChainVK* pSwapChain = m_pContext->getSwapChain();
	DeviceVK* pDevice = m_pContext->getDevice();

	VkExtent2D extent = pSwapChain->getExtent();
	ImageViewVK* pDepthStencilView = pSwapChain->getDepthStencilView();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppBackbuffers[i] = DBG_NEW FrameBufferVK(pDevice);
		m_ppBackbuffers[i]->addColorAttachment(pSwapChain->getImageView(i));
		m_ppBackbuffers[i]->setDepthStencilAttachment(pDepthStencilView);
		m_ppBackbuffers[i]->finalize(m_pRenderPass, extent.width, extent.height);
	}
}

void RendererVK::releaseFramebuffers()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		SAFEDELETE(m_ppBackbuffers[i]);
	}
}

bool RendererVK::createRenderPass()
{
	//Create renderpass
	m_pRenderPass = DBG_NEW RenderPassVK(m_pContext->getDevice());
	VkAttachmentDescription description = {};
	description.format = VK_FORMAT_B8G8R8A8_UNORM;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				//Clear Before Rendering
	description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				//Store After Rendering
	description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	//Dont care about stencil before
	description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Still dont care about stencil
	description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			//Dont care about what the initial layout of the image is (We do not need to save this memory)
	description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		//The image will be used for presenting
	m_pRenderPass->addAttachment(description);

	description.format = VK_FORMAT_D24_UNORM_S8_UINT;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				//Clear Before Rendering
	description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				//Store After Rendering
	description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	//Dont care about stencil before (If we need the stencil this needs to change)
	description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Still dont care about stencil	 (If we need the stencil this needs to change)
	description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;							//Dont care about what the initial layout of the image is (We do not need to save this memory)
	description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		//The image will be used as depthstencil
	m_pRenderPass->addAttachment(description);

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment = 1;
	depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pRenderPass->addSubpass(&colorAttachmentRef, 1, &depthStencilAttachmentRef);
	m_pRenderPass->addSubpassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	return m_pRenderPass->finalize();
}

bool RendererVK::createPipelines()
{
	//Create pipelinestate
	IShader* pVertexShader = m_pContext->createShader();
	pVertexShader->initFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/vertex.spv");
	if (!pVertexShader->finalize())
	{
		return false;
	}

	IShader* pPixelShader = m_pContext->createShader();
	pPixelShader->initFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/fragment.spv");
	if (!pPixelShader->finalize())
	{
		return false;
	}

	std::vector<IShader*> shaders = { pVertexShader, pPixelShader };
	m_pPipeline = DBG_NEW PipelineVK(m_pContext->getDevice());
	m_pPipeline->addColorBlendAttachment(false, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
	m_pPipeline->setCulling(true);
	m_pPipeline->setDepthTest(true);
	m_pPipeline->setWireFrame(false);
	//TODO: Return bool
	m_pPipeline->finalize(shaders, m_pRenderPass, m_pPipelineLayout);

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);

	return true;
}

bool RendererVK::createPipelineLayouts()
{
	//DescriptorSetLayout
	m_pDescriptorSetLayout = DBG_NEW DescriptorSetLayoutVK(m_pContext->getDevice());
	//CameraBuffer
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 0, 1);
	//VertexBuffer
	m_pDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, 1, 1);
	m_pDescriptorSetLayout->finalize();
	std::vector<const DescriptorSetLayoutVK*> descriptorSetLayouts = { m_pDescriptorSetLayout };

	//Descriptorpool
	DescriptorCounts descriptorCounts = {};
	descriptorCounts.m_SampledImages	= 128;
	descriptorCounts.m_StorageBuffers	= 128;
	descriptorCounts.m_UniformBuffers	= 128;
	descriptorCounts.m_StorageImages = 1;
	descriptorCounts.m_AccelerationStructures = 1;

	m_pDescriptorPool = DBG_NEW DescriptorPoolVK(m_pContext->getDevice());
	m_pDescriptorPool->init(descriptorCounts, 16);
	m_pDescriptorSet = m_pDescriptorPool->allocDescriptorSet(m_pDescriptorSetLayout);
	if (m_pDescriptorSet == nullptr)
	{
		return false;
	}

	//PushConstant - Triangle color
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.size = sizeof(glm::mat4) + sizeof(glm::vec4);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	std::vector<VkPushConstantRange> pushConstantRanges = { pushConstantRange };

	m_pPipelineLayout = DBG_NEW PipelineLayoutVK(m_pContext->getDevice());
	
	//TODO: Return bool
	m_pPipelineLayout->init(descriptorSetLayouts, pushConstantRanges);

	return true;
}

bool RendererVK::createRayTracingPipelineLayouts()
{
	m_pRayTracingDescriptorSetLayout = new DescriptorSetLayoutVK(m_pContext->getDevice());
	m_pRayTracingDescriptorSetLayout->addBindingAccelerationStructure(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 0, 1);
	m_pRayTracingDescriptorSetLayout->addBindingStorageImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, 1, 1);
	m_pRayTracingDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV, 2, 1);
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 3, 1);
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 4, 1);
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 5, 1);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, 6, MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, 7, MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, 8, MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	m_pRayTracingDescriptorSetLayout->finalize();

	std::vector<const DescriptorSetLayoutVK*> rayTracingDescriptorSetLayouts = { m_pRayTracingDescriptorSetLayout };
	std::vector<VkPushConstantRange> rayTracingPushConstantRanges;

	//Descriptorpool
	DescriptorCounts descriptorCounts = {};
	descriptorCounts.m_SampledImages = 256;
	descriptorCounts.m_StorageBuffers = 16;
	descriptorCounts.m_UniformBuffers = 16;
	descriptorCounts.m_StorageImages = 1;
	descriptorCounts.m_AccelerationStructures = 1;

	m_pRayTracingDescriptorPool = new DescriptorPoolVK(m_pContext->getDevice());
	m_pRayTracingDescriptorPool->init(descriptorCounts, 16);
	m_pRayTracingDescriptorSet = m_pRayTracingDescriptorPool->allocDescriptorSet(m_pRayTracingDescriptorSetLayout);
	if (m_pRayTracingDescriptorSet == nullptr)
	{
		return false;
	}
	
	m_pRayTracingPipelineLayout = new PipelineLayoutVK(m_pContext->getDevice());
	m_pRayTracingPipelineLayout->init(rayTracingDescriptorSetLayouts, rayTracingPushConstantRanges);
	
	return true;
}

bool RendererVK::createBuffers()
{
	BufferParams cameraBufferParams = {};
	cameraBufferParams.Usage			= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	cameraBufferParams.SizeInBytes		= sizeof(CameraBuffer);
	cameraBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	m_pCameraBuffer = DBG_NEW BufferVK(m_pContext->getDevice());
	return m_pCameraBuffer->init(cameraBufferParams);
}
