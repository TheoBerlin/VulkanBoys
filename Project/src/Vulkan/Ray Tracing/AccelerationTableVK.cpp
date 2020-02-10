#include "AccelerationTableVK.h"

#include "../GraphicsContextVK.h"
#include "../DeviceVK.h"
#include "../BufferVK.h"

#include "../CommandPoolVK.h"
#include "../CommandBufferVK.h"

#include <algorithm>

#ifdef max
    #undef max
#endif

AccelerationTableVK::AccelerationTableVK(IGraphicsContext* pContext) :
	m_pContext(reinterpret_cast<GraphicsContextVK*>(pContext)),
	m_MaxMemReqBLAS(0),
	m_pTempCommandPool(nullptr),
	m_pTempCommandBuffer(nullptr)
{
	m_pDevice = reinterpret_cast<DeviceVK*>(m_pContext->getDevice());
}

AccelerationTableVK::~AccelerationTableVK()
{
	if (m_pTempCommandPool != nullptr)
	{
		delete m_pTempCommandPool;
		m_pTempCommandPool = nullptr;
	}
	
	for (auto& bottomLevelAccelerationStructure : m_BottomLevelAccelerationStructures)
	{
		vkFreeMemory(m_pDevice->getDevice(), bottomLevelAccelerationStructure.second.memory, nullptr);
		m_pDevice->vkDestroyAccelerationStructureNV(m_pDevice->getDevice(), bottomLevelAccelerationStructure.second.accelerationStructure, nullptr);
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

bool AccelerationTableVK::finalize()
{
	m_pTempCommandPool = new CommandPoolVK(m_pContext->getDevice(), m_pContext->getDevice()->getQueueFamilyIndices().transferFamily.value());
	m_pTempCommandPool->init();

	m_pTempCommandBuffer = m_pTempCommandPool->allocateCommandBuffer();
	
	if (!buildAccelerationTable())
	{
		std::cerr << "--- AccelerationTable: Failed to build Acceleration Table!" << std::endl;
		return false;
	}

	std::cout << "--- AccelerationTable: Successfully initialized Acceleration Table!" << std::endl;
	return true;
}

uint32_t AccelerationTableVK::addMeshInstance(MeshVK* pMesh, const glm::mat3x4& transform)
{
	BottomLevelAccelerationStructure bottomLevelAccelerationStructure = {};
	
	if (m_BottomLevelAccelerationStructures.find(pMesh) == m_BottomLevelAccelerationStructures.end())
	{
		bottomLevelAccelerationStructure.geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
		bottomLevelAccelerationStructure.geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
		bottomLevelAccelerationStructure.geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
		bottomLevelAccelerationStructure.geometry.geometry.triangles.vertexData = ((BufferVK*)pMesh->getVertexBuffer())->getBuffer();
		bottomLevelAccelerationStructure.geometry.geometry.triangles.vertexOffset = 0;
		bottomLevelAccelerationStructure.geometry.geometry.triangles.vertexCount = static_cast<uint32_t>(pMesh->getVertexBuffer()->getSizeInBytes() / sizeof(Vertex));
		bottomLevelAccelerationStructure.geometry.geometry.triangles.vertexStride = sizeof(Vertex);
		bottomLevelAccelerationStructure.geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		bottomLevelAccelerationStructure.geometry.geometry.triangles.indexData = ((BufferVK*)pMesh->getIndexBuffer())->getBuffer();
		bottomLevelAccelerationStructure.geometry.geometry.triangles.indexOffset = 0;
		bottomLevelAccelerationStructure.geometry.geometry.triangles.indexCount = pMesh->getIndexBuffer()->getSizeInBytes() / sizeof(uint32_t);
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
		VK_CHECK_RESULT(m_pDevice->vkCreateAccelerationStructureNV(m_pDevice->getDevice(), &accelerationStructureCreateInfo, nullptr, &bottomLevelAccelerationStructure.accelerationStructure), "--- AccelerationTable: Could not create BLAS!");

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
		VK_CHECK_RESULT(vkAllocateMemory(m_pDevice->getDevice(), &memoryAllocateInfo, nullptr, &bottomLevelAccelerationStructure.memory), "--- AccelerationTable: Could not allocate memory for BLAS!");

		VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
		accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
		accelerationStructureMemoryInfo.accelerationStructure = bottomLevelAccelerationStructure.accelerationStructure;
		accelerationStructureMemoryInfo.memory = bottomLevelAccelerationStructure.memory;
		VK_CHECK_RESULT(m_pDevice->vkBindAccelerationStructureMemoryNV(m_pDevice->getDevice(), 1, &accelerationStructureMemoryInfo), "--- AccelerationTable: Could not bind memory for BLAS!");

		VK_CHECK_RESULT(m_pDevice->vkGetAccelerationStructureHandleNV(m_pDevice->getDevice(), bottomLevelAccelerationStructure.accelerationStructure, sizeof(uint64_t), &bottomLevelAccelerationStructure.handle), "--- AccelerationTable: Could not get handle for BLAS!");

		m_BottomLevelAccelerationStructures[pMesh] = bottomLevelAccelerationStructure;

		VkMemoryRequirements2 memReqBLAS;
		memoryRequirementsInfo.accelerationStructure = bottomLevelAccelerationStructure.accelerationStructure;
		m_pDevice->vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->getDevice(), &memoryRequirementsInfo, &memReqBLAS);

		if (memReqBLAS.memoryRequirements.size > m_MaxMemReqBLAS)
			m_MaxMemReqBLAS = memReqBLAS.memoryRequirements.size;
		
	}
	else
	{
		bottomLevelAccelerationStructure = m_BottomLevelAccelerationStructures[pMesh];
	}

	GeometryInstance geometryInstance = {};
	geometryInstance.transform = transform;
	geometryInstance.instanceId = 0;
	geometryInstance.mask = 0xff;
	geometryInstance.instanceOffset = 0;
	geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
	geometryInstance.accelerationStructureHandle = bottomLevelAccelerationStructure.handle;
	m_AllGeometryInstances.push_back(geometryInstance);
	
	return m_AllGeometryInstances.size() - 1;
}

bool AccelerationTableVK::initTLAS()
{
	VkAccelerationStructureInfoNV accelerationStructureInfo = {};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	accelerationStructureInfo.instanceCount = m_AllGeometryInstances.size();
	accelerationStructureInfo.geometryCount = 0;

	VkAccelerationStructureCreateInfoNV accelerationStructureCreateInfo = {};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCreateInfo.info = accelerationStructureInfo;
	VK_CHECK_RESULT_RETURN_FALSE(m_pDevice->vkCreateAccelerationStructureNV(m_pDevice->getDevice(), &accelerationStructureCreateInfo, nullptr, &m_TopLevelAccelerationStructure.accelerationStructure), "--- AccelerationTable: Could not create TLAS!");

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
	VK_CHECK_RESULT_RETURN_FALSE(vkAllocateMemory(m_pDevice->getDevice(), &memoryAllocateInfo, nullptr, &m_TopLevelAccelerationStructure.memory), "--- AccelerationTable: Could not allocate memory for TLAS!");

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
	accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accelerationStructureMemoryInfo.accelerationStructure = m_TopLevelAccelerationStructure.accelerationStructure;
	accelerationStructureMemoryInfo.memory = m_TopLevelAccelerationStructure.memory;
	VK_CHECK_RESULT_RETURN_FALSE(m_pDevice->vkBindAccelerationStructureMemoryNV(m_pDevice->getDevice(), 1, &accelerationStructureMemoryInfo), "--- AccelerationTable: Could not allocate bind memory for TLAS!");

	VK_CHECK_RESULT_RETURN_FALSE(m_pDevice->vkGetAccelerationStructureHandleNV(m_pDevice->getDevice(), m_TopLevelAccelerationStructure.accelerationStructure, sizeof(uint64_t), &m_TopLevelAccelerationStructure.handle), "--- AccelerationTable: Could not get handle for TLAS!");
	return true;
}

bool AccelerationTableVK::buildAccelerationTable()
{
	//Create Buffers

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
	scratchBufferParams.SizeInBytes = std::max(m_MaxMemReqBLAS, memReqTLAS.memoryRequirements.size);

	BufferVK* pScratchBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	pScratchBuffer->init(scratchBufferParams);

	BufferParams instanceBufferParmas = {};
	instanceBufferParmas.Usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
	instanceBufferParmas.MemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	instanceBufferParmas.SizeInBytes = sizeof(GeometryInstance) * m_AllGeometryInstances.size();

	BufferVK* pInstanceBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	pInstanceBuffer->init(instanceBufferParmas);

	void* pData;
	pInstanceBuffer->map(&pData);
	memcpy(pData, m_AllGeometryInstances.data(), sizeof(GeometryInstance) * m_AllGeometryInstances.size());
	pInstanceBuffer->unmap();

	//Create Memory Barrier
	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	
	//Create TLAS
	
	if (!initTLAS())
		return false;
	
	m_pTempCommandBuffer->begin();
	
	for (auto& bottomLevelAccelerationStructure : m_BottomLevelAccelerationStructures)
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
			pScratchBuffer->getBuffer(),
			0);

		vkCmdPipelineBarrier(m_pTempCommandBuffer->getCommandBuffer(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);
	}
	
	/*
		Build top-level acceleration structure
	*/
	VkAccelerationStructureInfoNV buildInfo = {};
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	buildInfo.pGeometries = nullptr;
	buildInfo.geometryCount = 0;
	buildInfo.instanceCount = m_AllGeometryInstances.size();

	m_pDevice->vkCmdBuildAccelerationStructureNV(
		m_pTempCommandBuffer->getCommandBuffer(),
		&buildInfo,
		pInstanceBuffer->getBuffer(),
		0,
		VK_FALSE,
		m_TopLevelAccelerationStructure.accelerationStructure,
		VK_NULL_HANDLE,
		pScratchBuffer->getBuffer(),
		0);

	vkCmdPipelineBarrier(m_pTempCommandBuffer->getCommandBuffer(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

	m_pTempCommandBuffer->end();

	delete pScratchBuffer;
	delete pInstanceBuffer;

	return true;
}
