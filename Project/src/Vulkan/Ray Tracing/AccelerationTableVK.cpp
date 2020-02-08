#include "AccelerationTableVK.h"

#include "../BufferVK.h"

AccelerationTableVK::AccelerationTableVK(DeviceVK* pDevice) :
	m_pDevice(pDevice)
{
}

AccelerationTableVK::~AccelerationTableVK()
{
	for (auto& bottomLevelAccelerationStructure : m_BottomLevelAccelerationStructures)
	{
		vkFreeMemory(m_pDevice->getDevice(), bottomLevelAccelerationStructure.memory, nullptr);
		m_pDevice->vkDestroyAccelerationStructureNV(m_pDevice->getDevice(), bottomLevelAccelerationStructure.accelerationStructure, nullptr);
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

uint32_t AccelerationTableVK::addBLAS(BufferVK* pVertexBuffer, BufferVK* pIndexBuffer)
{
	VkGeometryNV geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
	geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
	geometry.geometry.triangles.vertexData = pVertexBuffer->getBuffer();
	geometry.geometry.triangles.vertexOffset = 0;
	geometry.geometry.triangles.vertexCount = static_cast<uint32_t>(pVertexBuffer->getSizeInBytes() / sizeof(Vertex));
	geometry.geometry.triangles.vertexStride = sizeof(Vertex);
	geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	geometry.geometry.triangles.indexData = pIndexBuffer->getBuffer();
	geometry.geometry.triangles.indexOffset = 0;
	geometry.geometry.triangles.indexCount = pIndexBuffer->getSizeInBytes() / sizeof(uint32_t);
	geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	geometry.geometry.triangles.transformData = VK_NULL_HANDLE;
	geometry.geometry.triangles.transformOffset = 0;
	geometry.geometry.aabbs = {};
	geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

	VkAccelerationStructureInfoNV accelerationStructureInfo = {};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	accelerationStructureInfo.instanceCount = 0;
	accelerationStructureInfo.geometryCount = 1;
	accelerationStructureInfo.pGeometries = &geometry;

	AccelerationStructure bottomLevelAccelerationStructure = {};
	
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

	m_BottomLevelAccelerationStructures.push_back(bottomLevelAccelerationStructure);
	return m_BottomLevelAccelerationStructures.size() - 1;
}

bool AccelerationTableVK::finalize()
{
	VkAccelerationStructureInfoNV accelerationStructureInfo = {};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	accelerationStructureInfo.instanceCount = 1;
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

	//Todo: Link TLAS and BLAS

	
	return true;
}
