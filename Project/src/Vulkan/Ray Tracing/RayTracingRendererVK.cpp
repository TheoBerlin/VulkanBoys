#include "RayTracingRendererVK.h"

#include "Vulkan/GraphicsContextVK.h"
#include "Vulkan/SwapChainVK.h"
#include "Vulkan/RenderingHandlerVK.h"
#include "Vulkan/PipelineLayoutVK.h"
#include "Vulkan/DescriptorSetLayoutVK.h"
#include "Vulkan/DescriptorPoolVK.h"
#include "Vulkan/DescriptorSetVK.h"
#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/CommandBufferVK.h"
#include "Vulkan/ImageViewVK.h"
#include "Vulkan/ImageVK.h"
#include "Vulkan/SamplerVK.h"
#include "Vulkan/MeshVK.h"
#include "Vulkan/ShaderVK.h"
#include "Vulkan/BufferVK.h"
#include "Vulkan/FrameBufferVK.h"
#include "Vulkan/RenderPassVK.h"

#include "ShaderBindingTableVK.h"
#include "RayTracingPipelineVK.h"
#include "RayTracingSceneVK.h"

RayTracingRendererVK::RayTracingRendererVK(GraphicsContextVK* pContext, RenderingHandlerVK* pRenderingHandler) :
	m_pContext(pContext),
	m_pRenderingHandler(pRenderingHandler),
	m_pRayTracingPipeline(nullptr),
	m_pRayTracingScene(nullptr),
	m_pRayTracingStorageImage(nullptr),
	m_pRayTracingStorageImageView(nullptr),
	m_pRayTracingDescriptorSet(nullptr),
	m_pRayTracingDescriptorPool(nullptr),
	m_pRayTracingDescriptorSetLayout(nullptr)
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppComputeCommandPools[i] = nullptr;
		m_ppGraphicsCommandPools[i] = nullptr;
	}
}

RayTracingRendererVK::~RayTracingRendererVK()
{
	SAFEDELETE(m_pProfiler);

	//Ray Tracing Stuff
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		SAFEDELETE(m_ppComputeCommandPools[i]);
		SAFEDELETE(m_ppGraphicsCommandPools[i]);
	}

	SAFEDELETE(m_pRayTracingScene);
	SAFEDELETE(m_pRayTracingPipeline);
	SAFEDELETE(m_pRayTracingPipelineLayout);
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
}

bool RayTracingRendererVK::init()
{
	if (!createCommandPoolAndBuffers())
	{
		return false;
	}

	if (!createPipelineLayouts())
	{
		return false;
	}

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
	m_pGunMaterial->pAlbedo->initFromFile("assets/textures/gunAlbedo.tga", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true);

	m_pGunMaterial->pNormalMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pGunMaterial->pNormalMap->initFromFile("assets/textures/gunNormal.tga", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true);

	m_pGunMaterial->pRoughnessMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pGunMaterial->pRoughnessMap->initFromFile("assets/textures/gunRoughness.tga", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true);

	m_pCubeMaterial = new TempMaterial();
	m_pCubeMaterial->pAlbedo = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pCubeMaterial->pAlbedo->initFromFile("assets/textures/cubeAlbedo.jpg", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true);

	m_pCubeMaterial->pNormalMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pCubeMaterial->pNormalMap->initFromFile("assets/textures/cubeNormal.jpg", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true);

	m_pCubeMaterial->pRoughnessMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pCubeMaterial->pRoughnessMap->initFromFile("assets/textures/cubeRoughness.jpg", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true);

	m_pSphereMaterial = new TempMaterial();
	m_pSphereMaterial->pAlbedo = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pSphereMaterial->pAlbedo->initFromFile("assets/textures/whiteTransparent.png", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true);

	m_pSphereMaterial->pNormalMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pSphereMaterial->pNormalMap->initFromFile("assets/textures/cubeNormal.jpg", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true);

	m_pSphereMaterial->pRoughnessMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pSphereMaterial->pRoughnessMap->initFromFile("assets/textures/whiteOpaque.png", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true);

	m_pPlaneMaterial = new TempMaterial();
	m_pPlaneMaterial->pAlbedo = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pPlaneMaterial->pAlbedo->initFromFile("assets/textures/woodAlbedo.png", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true);

	m_pPlaneMaterial->pNormalMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pPlaneMaterial->pNormalMap->initFromFile("assets/textures/woodNormal.png", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true);

	m_pPlaneMaterial->pRoughnessMap = reinterpret_cast<Texture2DVK*>(m_pContext->createTexture2D());
	m_pPlaneMaterial->pRoughnessMap->initFromFile("assets/textures/woodRoughness.png", ETextureFormat::FORMAT_R8G8B8A8_UNORM, true);

	m_pMeshCube = m_pContext->createMesh();
	m_pMeshCube->initFromMemory(cubeVertices, sizeof(Vertex), 24, cubeIndices, 36);

	m_pMeshGun = m_pContext->createMesh();
	m_pMeshGun->initFromFile("assets/meshes/gun.obj");

	m_pMeshSphere = m_pContext->createMesh();
	m_pMeshSphere->initFromFile("assets/meshes/sphere.obj");

	m_pMeshPlane = m_pContext->createMesh();
	m_pMeshPlane->initFromMemory(planeVertices, sizeof(Vertex), 4, planeIndices, 6);

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

	m_pRayTracingPipeline = new RayTracingPipelineVK(m_pContext);
	m_pRayTracingPipeline->addRaygenShaderGroup(raygenGroupParams);
	m_pRayTracingPipeline->addMissShaderGroup(missGroupParams);
	m_pRayTracingPipeline->addMissShaderGroup(missGroupShadowParams);
	m_pRayTracingPipeline->addHitShaderGroup(hitGroupParams);
	m_pRayTracingPipeline->addHitShaderGroup(hitGroupShadowParams);
	m_pRayTracingPipeline->finalize(m_pRayTracingPipelineLayout);

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

	CommandBufferVK* pTempCommandBuffer = m_ppGraphicsCommandPools[0]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	pTempCommandBuffer->reset(true);
	pTempCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	pTempCommandBuffer->transitionImageLayout(m_pRayTracingStorageImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1);
	pTempCommandBuffer->end();

	m_pContext->getDevice()->executeCommandBuffer(m_pContext->getDevice()->getGraphicsQueue(), pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
	m_pContext->getDevice()->wait();

	m_ppGraphicsCommandPools[0]->freeCommandBuffer(&pTempCommandBuffer);

	BufferParams rayTracingUniformBufferParams = {};
	rayTracingUniformBufferParams.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	rayTracingUniformBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	rayTracingUniformBufferParams.SizeInBytes = sizeof(CameraMatricesBuffer);

	m_pRayTracingUniformBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pRayTracingUniformBuffer->init(rayTracingUniformBufferParams);

	auto& allMaterials = m_pRayTracingScene->getAllMaterials();

	std::vector<const ImageViewVK*> albedoImageViews;
	albedoImageViews.reserve(MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	std::vector<const ImageViewVK*> normalImageViews;
	normalImageViews.reserve(MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	std::vector<const ImageViewVK*> roughnessImageViews;
	roughnessImageViews.reserve(MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);

	m_pSampler = new SamplerVK(m_pContext->getDevice());
	
	SamplerParams samplerParams = {};
	samplerParams.MinFilter = VkFilter::VK_FILTER_LINEAR;
	samplerParams.MagFilter = VkFilter::VK_FILTER_LINEAR;
	samplerParams.WrapModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerParams.WrapModeV = samplerParams.WrapModeU;
	samplerParams.WrapModeW = samplerParams.WrapModeU;
	m_pSampler->init(samplerParams);

	std::vector<const SamplerVK*> samplers;
	samplers.reserve(MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);

	for (uint32_t i = 0; i < MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES; i++)
	{
		samplers.push_back(m_pSampler);

		if (i < allMaterials.size())
		{
			albedoImageViews.push_back(allMaterials[i]->pAlbedo->getImageView());
			normalImageViews.push_back(allMaterials[i]->pNormalMap->getImageView());
			roughnessImageViews.push_back(allMaterials[i]->pRoughnessMap->getImageView());
		}
		else
		{
			albedoImageViews.push_back(allMaterials[0]->pAlbedo->getImageView());
			normalImageViews.push_back(allMaterials[0]->pNormalMap->getImageView());
			roughnessImageViews.push_back(allMaterials[0]->pRoughnessMap->getImageView());
		}
	}

	m_pRayTracingDescriptorSet->writeAccelerationStructureDescriptor(m_pRayTracingScene->getTLAS().accelerationStructure, 0);
	m_pRayTracingDescriptorSet->writeStorageImageDescriptor(m_pRayTracingStorageImageView, 1);
	m_pRayTracingDescriptorSet->writeUniformBufferDescriptor(m_pRayTracingUniformBuffer, 2);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(m_pRayTracingScene->getCombinedVertexBuffer(), 3);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(m_pRayTracingScene->getCombinedIndexBuffer(), 4);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(m_pRayTracingScene->getMeshIndexBuffer(), 5);

	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(albedoImageViews.data(), samplers.data(), MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES, 6);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(normalImageViews.data(), samplers.data(), MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES, 7);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(roughnessImageViews.data(), samplers.data(), MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES, 8);

	createProfiler();

	return true;
}

void RayTracingRendererVK::beginFrame(const Camera& camera, const LightSetup& lightSetup)
{
	{
		//Temp Update Stuff
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
	}

	uint32_t currentFrame = m_pRenderingHandler->getCurrentFrameIndex();

	m_ppComputeCommandBuffers[currentFrame]->reset(false);
	m_ppComputeCommandPools[currentFrame]->reset();

	// Needed to begin a secondary buffer
	VkCommandBufferInheritanceInfo inheritanceInfo = {};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.pNext = nullptr;
	inheritanceInfo.renderPass = VK_NULL_HANDLE;
	inheritanceInfo.subpass = 0;
	inheritanceInfo.framebuffer = VK_NULL_HANDLE;

	m_ppComputeCommandBuffers[currentFrame]->begin(&inheritanceInfo, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	
	m_pProfiler->reset(currentFrame, m_ppComputeCommandBuffers[currentFrame]);
	m_pProfiler->beginFrame(m_ppComputeCommandBuffers[currentFrame]);

	CameraMatricesBuffer cameraMatricesBuffer = {};
	cameraMatricesBuffer.Projection = glm::inverse(camera.getProjectionMat());
	cameraMatricesBuffer.View = glm::inverse(camera.getViewMat());
	m_ppComputeCommandBuffers[currentFrame]->updateBuffer(m_pRayTracingUniformBuffer, 0, (const void*)&cameraMatricesBuffer, sizeof(CameraMatricesBuffer));

	m_ppComputeCommandBuffers[currentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipelineLayout, 0, 1, &m_pRayTracingDescriptorSet, 0, nullptr);
}

void RayTracingRendererVK::endFrame()
{
	uint32_t currentFrame = m_pRenderingHandler->getCurrentFrameIndex();

	vkCmdBindPipeline(m_ppComputeCommandBuffers[currentFrame]->getCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipeline->getPipeline());

	m_ppComputeCommandBuffers[currentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipelineLayout, 0, 1, &m_pRayTracingDescriptorSet, 0, nullptr);
	m_pProfiler->beginTimestamp(&m_TimestampTraceRays);
	m_ppComputeCommandBuffers[currentFrame]->traceRays(m_pRayTracingPipeline->getSBT(), m_pContext->getSwapChain()->getExtent().width, m_pContext->getSwapChain()->getExtent().height, 0);
	m_pProfiler->endTimestamp(&m_TimestampTraceRays);

	m_pProfiler->endFrame();
	m_ppComputeCommandBuffers[currentFrame]->end();

	//DeviceVK* pDevice = m_pContext->getDevice();
	//pDevice->executeSecondaryCommandBuffer(m_pRenderingHandler->getCurrentComputeCommandBuffer(), m_ppComputeCommandBuffers[currentFrame]);

	//Prepare for frame
	//pDevice->wait(); //Todo: Remove this

	m_ppGraphicsCommandBuffers[currentFrame]->reset(false);
	m_ppGraphicsCommandPools[currentFrame]->reset();

	// Needed to begin a secondary buffer
	VkCommandBufferInheritanceInfo inheritanceInfo = {};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.pNext = nullptr;
	inheritanceInfo.renderPass = VK_NULL_HANDLE;
	inheritanceInfo.subpass = 0;
	inheritanceInfo.framebuffer = VK_NULL_HANDLE;

	m_ppGraphicsCommandBuffers[currentFrame]->begin(&inheritanceInfo, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	ImageVK* pCurrentImage = m_pContext->getSwapChain()->getImage(m_pContext->getSwapChain()->getImageIndex());
	m_ppGraphicsCommandBuffers[currentFrame]->transitionImageLayout(pCurrentImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 1, 0, 1);
	m_ppGraphicsCommandBuffers[currentFrame]->transitionImageLayout(m_pRayTracingStorageImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 1, 0, 1);

	VkImageCopy copyRegion = {};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = { 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = { 0, 0, 0 };
	copyRegion.extent = { m_pContext->getSwapChain()->getExtent().width, m_pContext->getSwapChain()->getExtent().height, 1 };
	vkCmdCopyImage(m_ppGraphicsCommandBuffers[currentFrame]->getCommandBuffer(), m_pRayTracingStorageImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pCurrentImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	m_ppGraphicsCommandBuffers[currentFrame]->transitionImageLayout(pCurrentImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, 1, 0, 1);
	m_ppGraphicsCommandBuffers[currentFrame]->transitionImageLayout(m_pRayTracingStorageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1);

	m_ppGraphicsCommandBuffers[currentFrame]->end();

	//Execute commandbuffer
	//pDevice->executeSecondaryCommandBuffer(m_pRenderingHandler->getCurrentGraphicsCommandBuffer(), m_ppGraphicsCommandBuffers[currentFrame]);
}

void RayTracingRendererVK::setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY)
{
}

void RayTracingRendererVK::submitMesh(IMesh* pMesh, TempMaterial* pMaterial, const glm::mat4& transform)
{
	//Todo: Implement
}

bool RayTracingRendererVK::createCommandPoolAndBuffers()
{
	DeviceVK* pDevice = m_pContext->getDevice();

	const uint32_t graphicsQueueFamilyIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppGraphicsCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, graphicsQueueFamilyIndex);

		if (!m_ppGraphicsCommandPools[i]->init())
		{
			return false;
		}

		m_ppGraphicsCommandBuffers[i] = m_ppGraphicsCommandPools[i]->allocateCommandBuffer(VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_SECONDARY);
		if (m_ppGraphicsCommandBuffers[i] == nullptr)
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

		m_ppComputeCommandBuffers[i] = m_ppComputeCommandPools[i]->allocateCommandBuffer(VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_SECONDARY);
		if (m_ppComputeCommandBuffers[i] == nullptr)
		{
			return false;
		}
	}

	return true;
}

bool RayTracingRendererVK::createPipelineLayouts()
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

void RayTracingRendererVK::createProfiler()
{
	m_pProfiler = DBG_NEW ProfilerVK("Raytracer", m_pContext->getDevice());
	m_pProfiler->initTimestamp(&m_TimestampTraceRays, "Trace Rays");

	ProfilerVK* pSceneProfiler = m_pRayTracingScene->getProfiler();
	pSceneProfiler->setParentProfiler(m_pProfiler);
}
