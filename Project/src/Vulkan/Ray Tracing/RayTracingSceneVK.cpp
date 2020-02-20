#include "RayTracingSceneVK.h"

#include "Vulkan/GraphicsContextVK.h"
#include "Vulkan/DeviceVK.h"
#include "Vulkan/BufferVK.h"
#include "Vulkan/MeshVK.h"
#include "Vulkan/Texture2DVK.h"

#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/CommandBufferVK.h"

#include <algorithm>

#ifdef max
    #undef max
#endif

RayTracingSceneVK::RayTracingSceneVK(IGraphicsContext* pContext) :
	m_pContext(reinterpret_cast<GraphicsContextVK*>(pContext)),
	m_pScratchBuffer(nullptr),
	m_pInstanceBuffer(nullptr),
	m_pGarbageScratchBuffer(nullptr),
	m_TotalNumberOfVertices(0),
	m_TotalNumberOfIndices(0),
	m_pGarbageInstanceBuffer(nullptr),
	m_pCombinedVertexBuffer(nullptr),
	m_pCombinedIndexBuffer(nullptr),
	m_pMeshIndexBuffer(nullptr),
	m_NumBottomLevelAccelerationStructures(0),
	m_pTempCommandPool(nullptr),
	m_pTempCommandBuffer(nullptr),
	m_Finalized(false),
	m_pVeryTempMaterial(nullptr),
	m_pLightProbeMesh(nullptr)
{
	m_pDevice = reinterpret_cast<DeviceVK*>(m_pContext->getDevice());
}

RayTracingSceneVK::~RayTracingSceneVK()
{
	if (m_pTempCommandBuffer != nullptr)
	{
		m_pTempCommandPool->freeCommandBuffer(&m_pTempCommandBuffer);
		m_pTempCommandBuffer = nullptr;
	}

	SAFEDELETE(m_pTempCommandPool);
	SAFEDELETE(m_pScratchBuffer);
	SAFEDELETE(m_pInstanceBuffer);
	SAFEDELETE(m_pGarbageScratchBuffer);
	SAFEDELETE(m_pGarbageInstanceBuffer);
	SAFEDELETE(m_pCombinedVertexBuffer);
	SAFEDELETE(m_pCombinedIndexBuffer);
	SAFEDELETE(m_pMeshIndexBuffer);
	SAFEDELETE(m_pLightProbeMesh);

	for (auto& bottomLevelAccelerationStructurePerMesh : m_BottomLevelAccelerationStructures)
	{
		for (auto& bottomLevelAccelerationStructure : bottomLevelAccelerationStructurePerMesh.second)
		{
			vkFreeMemory(m_pDevice->getDevice(), bottomLevelAccelerationStructure.second.memory, nullptr);
			m_pDevice->vkDestroyAccelerationStructureNV(m_pDevice->getDevice(), bottomLevelAccelerationStructure.second.accelerationStructure, nullptr);
		}
	}
	m_BottomLevelAccelerationStructures.clear();

	if (m_TopLevelAccelerationStructure.memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(m_pDevice->getDevice(), m_TopLevelAccelerationStructure.memory, nullptr);
		m_TopLevelAccelerationStructure.memory = VK_NULL_HANDLE;
	}

	if (m_TopLevelAccelerationStructure.accelerationStructure != VK_NULL_HANDLE)
	{
		m_pDevice->vkDestroyAccelerationStructureNV(m_pDevice->getDevice(), m_TopLevelAccelerationStructure.accelerationStructure, nullptr);
		m_TopLevelAccelerationStructure.accelerationStructure = VK_NULL_HANDLE;
	}

	m_TopLevelAccelerationStructure.handle = 0;
}

bool RayTracingSceneVK::finalize()
{
	if (m_Finalized)
	{
		LOG("--- RayTracingScene: Finalize failed, already finalized!");
		return false;
	}

	m_Finalized = true;
	m_pTempCommandPool = DBG_NEW CommandPoolVK(m_pContext->getDevice(), m_pContext->getDevice()->getQueueFamilyIndices().computeFamily.value());
	m_pTempCommandPool->init();

	m_pTempCommandBuffer = m_pTempCommandPool->allocateCommandBuffer();
	
	createCombinedGraphicsObjectData();

	//Create TLAS
	if (!initTLAS())
	{
		LOG("--- RayTracingScene: Failed to initialize TLAS!");
		return false;
	}

	if (!buildAccelerationTable())
	{
		LOG("--- RayTracingScene: Failed to build Acceleration Table!");
		return false;
	}

	LOG("--- RayTracingScene: Successfully initialized Acceleration Table!");
	return true;
}

void RayTracingSceneVK::update()
{
	cleanGarbage();
	updateScratchBuffer();
	updateInstanceBuffer();

	VkAccelerationStructureInfoNV buildInfo = {};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV;
	buildInfo.pGeometries = 0;
	buildInfo.geometryCount = 0;
	buildInfo.instanceCount = m_AllGeometryInstances.size();

	//Create Memory Barrier
	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

	m_pTempCommandBuffer->reset();
	m_pTempCommandBuffer->begin();

	m_pDevice->vkCmdBuildAccelerationStructureNV(
		m_pTempCommandBuffer->getCommandBuffer(),
		&buildInfo,
		m_pInstanceBuffer->getBuffer(),
		0,
		VK_TRUE,
		m_TopLevelAccelerationStructure.accelerationStructure,
		m_TopLevelAccelerationStructure.accelerationStructure,
		m_pScratchBuffer->getBuffer(),
		0);

	vkCmdPipelineBarrier(m_pTempCommandBuffer->getCommandBuffer(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

	m_pTempCommandBuffer->end();
	m_pContext->getDevice()->executeCommandBuffer(m_pContext->getDevice()->getComputeQueue(), m_pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
	m_pContext->getDevice()->wait();
}

void RayTracingSceneVK::generateLightProbeGeometry(uint32_t worldSizeX, uint32_t worldSizeY, uint32_t worldSizeZ, uint32_t samplesPerProbe, uint32_t numProbesPerDimension)
{
	/*float sqrt5 = glm::sqrt(5.0f);

	std::vector<Vertex> lightProbeVertices;
	std::vector<uint32_t> lightProbeIndices;

	for (uint32_t n = 0; n < samplesPerProbe; n += 4)
	{
		float index0 = float(n) + 0.5f;
		float phi0 = glm::acos(1.0f - 2.0f * index0 / samplesPerProbe);
		float theta0 = glm::pi<float>() * index0 * (1.0f + sqrt5);

		float index1 = float(n + 1) + 0.5f;
		float phi1 = glm::acos(1.0f - 2.0f * index1 / samplesPerProbe);
		float theta1 = glm::pi<float>() * index1 * (1.0f + sqrt5);

		float index2 = float(n + 2) + 0.5f;
		float phi2 = glm::acos(1.0f - 2.0f * index2 / samplesPerProbe);
		float theta2 = glm::pi<float>() * index2 * (1.0f + sqrt5);

		float index3 = float(n + 3) + 0.5f;
		float phi3 = glm::acos(1.0f - 2.0f * index3 / samplesPerProbe);
		float theta3 = glm::pi<float>() * index3 * (1.0f + sqrt5);

		glm::vec3 posAndNormal0(glm::cos(theta0) * glm::sin(phi0), glm::sin(theta0) * glm::sin(phi0), glm::cos(phi0));
		glm::vec3 posAndNormal1(glm::cos(theta1) * glm::sin(phi1), glm::sin(theta1) * glm::sin(phi1), glm::cos(phi1));
		glm::vec3 posAndNormal2(glm::cos(theta2) * glm::sin(phi2), glm::sin(theta2) * glm::sin(phi2), glm::cos(phi2));
		glm::vec3 posAndNormal3(glm::cos(theta3) * glm::sin(phi3), glm::sin(theta3) * glm::sin(phi3), glm::cos(phi3));
		std::cout << "Vertex 0: " << glm::to_string(posAndNormal0) << std::endl;
		std::cout << "Vertex 1: " << glm::to_string(posAndNormal1) << std::endl;
		std::cout << "Vertex 2: " << glm::to_string(posAndNormal2) << std::endl;
		std::cout << "Vertex 3: " << glm::to_string(posAndNormal3) << std::endl << std::endl;


		lightProbeVertices.push_back({ glm::vec4(posAndNormal0, 1.0f), glm::vec4(posAndNormal0, 0.0f), glm::vec4(0.0f), glm::vec4(0.0f) });
		lightProbeVertices.push_back({ glm::vec4(posAndNormal1, 1.0f), glm::vec4(posAndNormal1, 0.0f), glm::vec4(0.0f), glm::vec4(0.0f) });
		lightProbeVertices.push_back({ glm::vec4(posAndNormal2, 1.0f), glm::vec4(posAndNormal2, 0.0f), glm::vec4(0.0f), glm::vec4(0.0f) });
		lightProbeVertices.push_back({ glm::vec4(posAndNormal3, 1.0f), glm::vec4(posAndNormal3, 0.0f), glm::vec4(0.0f), glm::vec4(0.0f) });

		lightProbeIndices.push_back(lightProbeVertices.size() - 4);
		lightProbeIndices.push_back(lightProbeVertices.size() - 3);
		lightProbeIndices.push_back(lightProbeVertices.size() - 2);

		lightProbeIndices.push_back(lightProbeVertices.size() - 3);
		lightProbeIndices.push_back(lightProbeVertices.size() - 2);
		lightProbeIndices.push_back(lightProbeVertices.size() - 1);
	}*/

	m_pLightProbeMesh = new MeshVK(m_pDevice);
	m_pLightProbeMesh->initAsSphere(3);

	for (uint32_t x = 0; x < numProbesPerDimension; x++)
	{
		for (uint32_t y = 0; y < numProbesPerDimension; y++)
		{
			for (uint32_t z = 0; z < numProbesPerDimension; z++)
			{
				float xPosition = (float(x) / float(numProbesPerDimension - 1)) * float(worldSizeX) - float(worldSizeX) / 2.0f;
				float yPosition = (float(y) / float(numProbesPerDimension - 1)) * float(worldSizeY) - float(worldSizeY) / 2.0f;
				float zPosition = (float(z) / float(numProbesPerDimension - 1)) * float(worldSizeZ) - float(worldSizeZ) / 2.0f;

				std::cout << "Position: " << glm::to_string(glm::vec3(xPosition, yPosition, zPosition)) << std::endl;

				glm::mat4 transform = glm::transpose(glm::translate(glm::mat4(1.0f), glm::vec3(xPosition, yPosition, zPosition)));
				addGraphicsObjectInstance(m_pLightProbeMesh, m_pVeryTempMaterial, transform, 0x40);
			}
		}
	}
}

uint32_t RayTracingSceneVK::addGraphicsObjectInstance(IMesh* pMesh, TempMaterial* pMaterial, const glm::mat3x4& transform, uint8_t customMask)
{
	if (m_pVeryTempMaterial == nullptr)
	{
		m_pVeryTempMaterial = pMaterial;
	}

	//Todo: Same mesh but different textures => different geometry instance instanceId

	MeshVK* pVulkanMesh = reinterpret_cast<MeshVK*>(pMesh);
	//Texture2DVK* pVulkanTexture = reinterpret_cast<Texture2DVK*>(pTexture2D);

	if (pVulkanMesh == nullptr || pMaterial == nullptr)
	{
		LOG("--- RayTracingScene: addGraphicsObjectInstance failed, Mesh or Texture2D is nullptr!");
		return false;
	}
	
	auto& blasPerMesh = m_BottomLevelAccelerationStructures.find(pVulkanMesh);

	if (blasPerMesh == m_BottomLevelAccelerationStructures.end())
	{
		if (m_Finalized)
		{
			LOG("--- RayTracingScene: addGraphicsObjectInstance failed, new mesh instance cannot be added to finalized RayTracingScene if the mesh has not been added before!");
			return false;
		}

		initBLAS(pVulkanMesh, pMaterial);
		m_AllMeshes.push_back(pVulkanMesh);
		m_TotalNumberOfVertices += static_cast<uint32_t>(pMesh->getVertexBuffer()->getSizeInBytes() / sizeof(Vertex));
		m_TotalNumberOfIndices += pMesh->getIndexBuffer()->getSizeInBytes() / sizeof(uint32_t);
	}
	else if (blasPerMesh->second.find(pMaterial) == blasPerMesh->second.end())
	{
		if (m_Finalized)
		{
			LOG("--- RayTracingScene: addGraphicsObjectInstance failed, new mesh instance cannot be added to finalized RayTracingScene if the texture has not been added before!");
			return false;
		}

		BottomLevelAccelerationStructure blasCopy = blasPerMesh->second.begin()->second;
		blasCopy.index = m_NumBottomLevelAccelerationStructures;
		m_NumBottomLevelAccelerationStructures++;
		blasCopy.textureIndex = m_AllMaterials.size();
		m_AllMaterials.push_back(pMaterial);
		blasPerMesh->second[pMaterial] = blasCopy;
	}

	const BottomLevelAccelerationStructure& bottomLevelAccelerationStructure = m_BottomLevelAccelerationStructures[pVulkanMesh][pMaterial];

	GeometryInstance geometryInstance = {};
	geometryInstance.transform = transform;
	geometryInstance.instanceId = bottomLevelAccelerationStructure.index; //This is not really used anymore, Todo: remove this
	geometryInstance.mask = customMask;
	geometryInstance.instanceOffset = 0;
	geometryInstance.flags = 0;// VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
	geometryInstance.accelerationStructureHandle = bottomLevelAccelerationStructure.handle;
	m_AllGeometryInstances.push_back(geometryInstance);
	
	return m_AllGeometryInstances.size() - 1;
}

void RayTracingSceneVK::updateGraphicsObject(uint32_t index, const glm::mat3x4& transform)
{
	m_AllGeometryInstances[index].transform = transform;
}

bool RayTracingSceneVK::initBLAS(MeshVK* pMesh, TempMaterial* pMaterial)
{
	BottomLevelAccelerationStructure bottomLevelAccelerationStructure = {};

	bottomLevelAccelerationStructure.geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
	bottomLevelAccelerationStructure.geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
	bottomLevelAccelerationStructure.geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
	bottomLevelAccelerationStructure.geometry.geometry.triangles.vertexData = ((BufferVK*)pMesh->getVertexBuffer())->getBuffer();
	bottomLevelAccelerationStructure.geometry.geometry.triangles.vertexOffset = 0;
	bottomLevelAccelerationStructure.geometry.geometry.triangles.vertexCount = pMesh->getVertexCount();
	bottomLevelAccelerationStructure.geometry.geometry.triangles.vertexStride = sizeof(Vertex);
	bottomLevelAccelerationStructure.geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	bottomLevelAccelerationStructure.geometry.geometry.triangles.indexData = ((BufferVK*)pMesh->getIndexBuffer())->getBuffer();
	bottomLevelAccelerationStructure.geometry.geometry.triangles.indexOffset = 0;
	bottomLevelAccelerationStructure.geometry.geometry.triangles.indexCount = pMesh->getIndexCount();
	bottomLevelAccelerationStructure.geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	bottomLevelAccelerationStructure.geometry.geometry.triangles.transformData = VK_NULL_HANDLE;
	bottomLevelAccelerationStructure.geometry.geometry.triangles.transformOffset = 0;
	bottomLevelAccelerationStructure.geometry.geometry.aabbs = {};
	bottomLevelAccelerationStructure.geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
	bottomLevelAccelerationStructure.geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

	VkAccelerationStructureInfoNV accelerationStructureInfo = {};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	accelerationStructureInfo.instanceCount = 0;
	accelerationStructureInfo.geometryCount = 1;
	accelerationStructureInfo.pGeometries = &bottomLevelAccelerationStructure.geometry;

	VkAccelerationStructureCreateInfoNV accelerationStructureCreateInfo = {};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCreateInfo.info = accelerationStructureInfo;
	VK_CHECK_RESULT(m_pDevice->vkCreateAccelerationStructureNV(m_pDevice->getDevice(), &accelerationStructureCreateInfo, nullptr, &bottomLevelAccelerationStructure.accelerationStructure), "--- RayTracingScene: Could not create BLAS!");

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memoryRequirementsInfo.accelerationStructure = bottomLevelAccelerationStructure.accelerationStructure;

	VkMemoryRequirements2 memoryRequirements2 = {};
	m_pDevice->vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->getDevice(), &memoryRequirementsInfo, &memoryRequirements2);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(m_pDevice->getPhysicalDevice(), memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(m_pDevice->getDevice(), &memoryAllocateInfo, nullptr, &bottomLevelAccelerationStructure.memory), "--- RayTracingScene: Could not allocate memory for BLAS!");

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo = {};
	accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accelerationStructureMemoryInfo.accelerationStructure = bottomLevelAccelerationStructure.accelerationStructure;
	accelerationStructureMemoryInfo.memory = bottomLevelAccelerationStructure.memory;
	VK_CHECK_RESULT(m_pDevice->vkBindAccelerationStructureMemoryNV(m_pDevice->getDevice(), 1, &accelerationStructureMemoryInfo), "--- RayTracingScene: Could not bind memory for BLAS!");

	VK_CHECK_RESULT(m_pDevice->vkGetAccelerationStructureHandleNV(m_pDevice->getDevice(), bottomLevelAccelerationStructure.accelerationStructure, sizeof(uint64_t), &bottomLevelAccelerationStructure.handle), "--- RayTracingScene: Could not get handle for BLAS!");

	bottomLevelAccelerationStructure.index = m_NumBottomLevelAccelerationStructures;
	m_NumBottomLevelAccelerationStructures++;

	bottomLevelAccelerationStructure.textureIndex = m_AllMaterials.size();
	m_AllMaterials.push_back(pMaterial);

	std::unordered_map<TempMaterial*, BottomLevelAccelerationStructure> newBLASPerMesh;
	newBLASPerMesh[pMaterial] = bottomLevelAccelerationStructure;
	m_BottomLevelAccelerationStructures[pMesh] = newBLASPerMesh;

	return true;
}

bool RayTracingSceneVK::initTLAS()
{
	VkAccelerationStructureInfoNV accelerationStructureInfo = {};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	accelerationStructureInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV;
	accelerationStructureInfo.instanceCount = m_AllGeometryInstances.size();
	accelerationStructureInfo.geometryCount = 0;

	VkAccelerationStructureCreateInfoNV accelerationStructureCreateInfo = {};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCreateInfo.info = accelerationStructureInfo;
	VK_CHECK_RESULT_RETURN_FALSE(m_pDevice->vkCreateAccelerationStructureNV(m_pDevice->getDevice(), &accelerationStructureCreateInfo, nullptr, &m_TopLevelAccelerationStructure.accelerationStructure), "--- RayTracingScene: Could not create TLAS!");

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memoryRequirementsInfo.accelerationStructure = m_TopLevelAccelerationStructure.accelerationStructure;

	VkMemoryRequirements2 memoryRequirements2 = {};
	m_pDevice->vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->getDevice(), &memoryRequirementsInfo, &memoryRequirements2);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(m_pDevice->getPhysicalDevice(), memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT_RETURN_FALSE(vkAllocateMemory(m_pDevice->getDevice(), &memoryAllocateInfo, nullptr, &m_TopLevelAccelerationStructure.memory), "--- RayTracingScene: Could not allocate memory for TLAS!");

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo = {};
	accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accelerationStructureMemoryInfo.accelerationStructure = m_TopLevelAccelerationStructure.accelerationStructure;
	accelerationStructureMemoryInfo.memory = m_TopLevelAccelerationStructure.memory;
	VK_CHECK_RESULT_RETURN_FALSE(m_pDevice->vkBindAccelerationStructureMemoryNV(m_pDevice->getDevice(), 1, &accelerationStructureMemoryInfo), "--- RayTracingScene: Could not allocate bind memory for TLAS!");

	VK_CHECK_RESULT_RETURN_FALSE(m_pDevice->vkGetAccelerationStructureHandleNV(m_pDevice->getDevice(), m_TopLevelAccelerationStructure.accelerationStructure, sizeof(uint64_t), &m_TopLevelAccelerationStructure.handle), "--- RayTracingScene: Could not get handle for TLAS!");
	return true;
}

bool RayTracingSceneVK::buildAccelerationTable()
{
	// Acceleration structure build requires some scratch space to store temporary information
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

	VkMemoryRequirements2 memReqTLAS;
	memoryRequirementsInfo.accelerationStructure = m_TopLevelAccelerationStructure.accelerationStructure;
	m_pDevice->vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->getDevice(), &memoryRequirementsInfo, &memReqTLAS);

	BufferParams scratchBufferParams = {};
	scratchBufferParams.Usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
	scratchBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	scratchBufferParams.SizeInBytes = std::max(findMaxMemReqBLAS(), memReqTLAS.memoryRequirements.size);

	m_pScratchBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pScratchBuffer->init(scratchBufferParams);

	BufferParams instanceBufferParmas = {};
	instanceBufferParmas.Usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
	instanceBufferParmas.MemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	instanceBufferParmas.SizeInBytes = sizeof(GeometryInstance) * m_AllGeometryInstances.size();

	m_pInstanceBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pInstanceBuffer->init(instanceBufferParmas);

	void* pData;
	m_pInstanceBuffer->map(&pData);
	memcpy(pData, m_AllGeometryInstances.data(), sizeof(GeometryInstance) * m_AllGeometryInstances.size());
	m_pInstanceBuffer->unmap();

	//Create Memory Barrier
	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	
	m_pTempCommandBuffer->reset();
	m_pTempCommandBuffer->begin();
	
	for (auto& bottomLevelAccelerationStructurePerMesh : m_BottomLevelAccelerationStructures)
	{
		for (auto& bottomLevelAccelerationStructure : bottomLevelAccelerationStructurePerMesh.second)
		{
			/*
				Build bottom level acceleration structure
			*/
			VkAccelerationStructureInfoNV buildInfo = {};
			buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
			buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
			buildInfo.geometryCount = 1;
			buildInfo.pGeometries = &bottomLevelAccelerationStructure.second.geometry;

			//Todo: Make this better?
			m_pDevice->vkCmdBuildAccelerationStructureNV(
				m_pTempCommandBuffer->getCommandBuffer(),
				&buildInfo,
				VK_NULL_HANDLE,
				0,
				VK_FALSE,
				bottomLevelAccelerationStructure.second.accelerationStructure,
				VK_NULL_HANDLE,
				m_pScratchBuffer->getBuffer(),
				0);

			vkCmdPipelineBarrier(m_pTempCommandBuffer->getCommandBuffer(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);
		}
	}
	
	/*
		Build top-level acceleration structure
	*/
	VkAccelerationStructureInfoNV buildInfo = {};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV;
	buildInfo.pGeometries = 0;
	buildInfo.geometryCount = 0;
	buildInfo.instanceCount = m_AllGeometryInstances.size();

	m_pDevice->vkCmdBuildAccelerationStructureNV(
		m_pTempCommandBuffer->getCommandBuffer(),
		&buildInfo,
		m_pInstanceBuffer->getBuffer(),
		0,
		VK_FALSE,
		m_TopLevelAccelerationStructure.accelerationStructure,
		VK_NULL_HANDLE,
		m_pScratchBuffer->getBuffer(),
		0);

	vkCmdPipelineBarrier(m_pTempCommandBuffer->getCommandBuffer(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

	m_pTempCommandBuffer->end();
	m_pContext->getDevice()->executeCommandBuffer(m_pContext->getDevice()->getComputeQueue(), m_pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
	m_pContext->getDevice()->wait();

	return true;
}

void RayTracingSceneVK::cleanGarbage()
{
	SAFEDELETE(m_pGarbageScratchBuffer);
	SAFEDELETE(m_pGarbageInstanceBuffer)
}

void RayTracingSceneVK::updateScratchBuffer()
{
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;

	VkMemoryRequirements2 memReqTLAS;
	memoryRequirementsInfo.accelerationStructure = m_TopLevelAccelerationStructure.accelerationStructure;
	m_pDevice->vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->getDevice(), &memoryRequirementsInfo, &memReqTLAS);

	if (m_pScratchBuffer->getSizeInBytes() < memReqTLAS.memoryRequirements.size)
	{
		m_pGarbageScratchBuffer = m_pScratchBuffer;

		BufferParams scratchBufferParams = {};
		scratchBufferParams.Usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		scratchBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		scratchBufferParams.SizeInBytes = memReqTLAS.memoryRequirements.size;

		m_pScratchBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
		m_pScratchBuffer->init(scratchBufferParams);
	}
}

void RayTracingSceneVK::updateInstanceBuffer()
{
	if (m_pInstanceBuffer->getSizeInBytes() < sizeof(GeometryInstance) * m_AllGeometryInstances.size())
	{
		m_pGarbageInstanceBuffer = m_pInstanceBuffer;

		BufferParams instanceBufferParmas = {};
		instanceBufferParmas.Usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		instanceBufferParmas.MemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		instanceBufferParmas.SizeInBytes = sizeof(GeometryInstance) * m_AllGeometryInstances.size();

		m_pInstanceBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
		m_pInstanceBuffer->init(instanceBufferParmas);
	}

	void* pData;
	m_pInstanceBuffer->map(&pData);
	memcpy(pData, m_AllGeometryInstances.data(), sizeof(GeometryInstance) * m_AllGeometryInstances.size());
	m_pInstanceBuffer->unmap();
}

void RayTracingSceneVK::createCombinedGraphicsObjectData()
{
	m_pCombinedVertexBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pCombinedIndexBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pMeshIndexBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());

	BufferParams vertexBufferParams = {};
	vertexBufferParams.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vertexBufferParams.SizeInBytes = sizeof(Vertex) * m_TotalNumberOfVertices;
	vertexBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	BufferParams indexBufferParams = {};
	indexBufferParams.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	indexBufferParams.SizeInBytes = sizeof(uint32_t) * m_TotalNumberOfIndices;
	indexBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	BufferParams meshIndexBufferParams = {};
	meshIndexBufferParams.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	meshIndexBufferParams.SizeInBytes = sizeof(uint32_t) * 3 * m_NumBottomLevelAccelerationStructures;
	meshIndexBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	m_pCombinedVertexBuffer->init(vertexBufferParams);
	m_pCombinedIndexBuffer->init(indexBufferParams);
	m_pMeshIndexBuffer->init(meshIndexBufferParams);

	m_pTempCommandBuffer->reset();
	m_pTempCommandBuffer->begin();

	uint32_t vertexBufferOffset = 0;
	uint32_t indexBufferOffset = 0;
	uint64_t meshIndexBufferOffset = 0;
	uint32_t currentCustomInstanceIndexNV = 0;

	void* pMeshIndexBufferMapped;
	m_pMeshIndexBuffer->map(&pMeshIndexBufferMapped);

	for (auto& pMesh : m_AllMeshes)
	{
		uint32_t numVertices = pMesh->getVertexCount();
		uint32_t numIndices = pMesh->getIndexCount();

		m_pTempCommandBuffer->copyBuffer(reinterpret_cast<BufferVK*>(pMesh->getVertexBuffer()), 0, m_pCombinedVertexBuffer, vertexBufferOffset * sizeof(Vertex), numVertices * sizeof(Vertex));
		m_pTempCommandBuffer->copyBuffer(reinterpret_cast<BufferVK*>(pMesh->getIndexBuffer()), 0, m_pCombinedIndexBuffer, indexBufferOffset * sizeof(uint32_t), numIndices * sizeof(uint32_t));

		for (auto& bottomLevelAccelerationStructure : m_BottomLevelAccelerationStructures[pMesh])
		{
			for (auto& geometryInstance : m_AllGeometryInstances)
			{
				if (geometryInstance.accelerationStructureHandle == bottomLevelAccelerationStructure.second.handle)
				{
					geometryInstance.instanceId = currentCustomInstanceIndexNV;
				}
			}

			memcpy((void*)((size_t)pMeshIndexBufferMapped +  meshIndexBufferOffset		* sizeof(uint32_t)), &vertexBufferOffset, sizeof(uint32_t));
			memcpy((void*)((size_t)pMeshIndexBufferMapped + (meshIndexBufferOffset + 1) * sizeof(uint32_t)), &indexBufferOffset, sizeof(uint32_t));
			memcpy((void*)((size_t)pMeshIndexBufferMapped + (meshIndexBufferOffset + 2) * sizeof(uint32_t)), &bottomLevelAccelerationStructure.second.textureIndex, sizeof(uint32_t));
			meshIndexBufferOffset += 3;
			currentCustomInstanceIndexNV++;
		}

		vertexBufferOffset += numVertices;
		indexBufferOffset += numIndices;
	}

	uint32_t test[8];
	memcpy(test, pMeshIndexBufferMapped, sizeof(uint32_t) * 6);
	m_pMeshIndexBuffer->unmap();

	m_pTempCommandBuffer->end();
	m_pDevice->executeCommandBuffer(m_pDevice->getComputeQueue(), m_pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
	m_pDevice->wait(); //Todo: Remove?
}

VkDeviceSize RayTracingSceneVK::findMaxMemReqBLAS()
{
	VkDeviceSize maxSize = 0;
	
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
	
	for (auto& bottomLevelAccelerationStructurePerMesh : m_BottomLevelAccelerationStructures)
	{
		for (auto& bottomLevelAccelerationStructure : bottomLevelAccelerationStructurePerMesh.second)
		{
			VkMemoryRequirements2 memReqBLAS;
			memoryRequirementsInfo.accelerationStructure = bottomLevelAccelerationStructure.second.accelerationStructure;
			m_pDevice->vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->getDevice(), &memoryRequirementsInfo, &memReqBLAS);

			if (memReqBLAS.memoryRequirements.size > maxSize)
				maxSize = memReqBLAS.memoryRequirements.size;
		}
	}

	return maxSize;
}
