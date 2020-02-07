#include "AccelerationStructureVK.h"

#include "../BufferVK.h"

AccelerationStructureVK::AccelerationStructureVK()
{
}

AccelerationStructureVK::~AccelerationStructureVK()
{
}

bool AccelerationStructureVK::addBLAS(BufferVK* pVertexBuffer, BufferVK* pIndexBuffer)
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

	VkAccelerationStructureCreateInfoNV accelerationStructureCreateInfo = {};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCreateInfo.info = accelerationStructureInfo;
	VK_CHECK_RESULT_RETURN_FALSE(m_pVulkanDevice->vkCreateAccelerationStructureNV(m_pVulkanDevice->getDevice(), &accelerationStructureCreateInfo, nullptr, &m_BLAS.accelerationStructure));

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memoryRequirementsInfo.accelerationStructure = m_BLAS.accelerationStructure;

	VkMemoryRequirements2 memoryRequirements2 = {};
	m_pVulkanDevice->vkGetAccelerationStructureMemoryRequirementsNV(m_pVulkanDevice->getDevice(), &memoryRequirementsInfo, &memoryRequirements2);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(m_pVulkanDevice->getPhysicalDevice(), memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT_RETURN_FALSE(vkAllocateMemory(m_pVulkanDevice->getDevice(), &memoryAllocateInfo, nullptr, &m_BLAS.memory));

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
	accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accelerationStructureMemoryInfo.accelerationStructure = m_BLAS.accelerationStructure;
	accelerationStructureMemoryInfo.memory = m_BLAS.memory;
	VK_CHECK_RESULT_RETURN_FALSE(m_pVulkanDevice->vkBindAccelerationStructureMemoryNV(m_pVulkanDevice->getDevice(), 1, &accelerationStructureMemoryInfo));

	VK_CHECK_RESULT_RETURN_FALSE(m_pVulkanDevice->vkGetAccelerationStructureHandleNV(m_pVulkanDevice->getDevice(), m_BLAS.accelerationStructure, sizeof(uint64_t), &m_BLAS.handle));
}

void AccelerationStructureVK::finalize()
{
}
