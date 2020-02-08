#pragma once

#include "../VulkanCommon.h"
#include "Vulkan/DeviceVK.h"

class BufferVK;

class AccelerationTableVK
{
	struct AccelerationStructure
	{
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkAccelerationStructureNV accelerationStructure = VK_NULL_HANDLE;
		uint64_t handle = 0;
	};
	
public:
	DECL_NO_COPY(AccelerationTableVK);

	AccelerationTableVK(DeviceVK* pDevice);
	~AccelerationTableVK();

	uint32_t addBLAS(BufferVK* pVertexBuffer, BufferVK* pIndexBuffer);
	bool finalize();

private:
	DeviceVK* m_pDevice;

	AccelerationStructure m_TopLevelAccelerationStructure;
	std::vector<AccelerationStructure> m_BottomLevelAccelerationStructures;
};
