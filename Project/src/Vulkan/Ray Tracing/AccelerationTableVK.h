#pragma once
#include "../VulkanCommon.h"
#include "../BufferVK.h"
#include "../MeshVK.h"

#include <vector>
#include <unordered_map>

class IGraphicsContext;
class GraphicsContextVK;
class DeviceVK;

//Todo: Remove these
class CommandPoolVK;
class CommandBufferVK;

class AccelerationTableVK
{
	struct GeometryInstance
	{
		glm::mat3x4 transform;
		uint32_t instanceId : 24;
		uint32_t mask : 8;
		uint32_t instanceOffset : 24;
		uint32_t flags : 8;
		uint64_t accelerationStructureHandle;
	};
	
	struct BottomLevelAccelerationStructure
	{
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkAccelerationStructureNV accelerationStructure = VK_NULL_HANDLE;
		uint64_t handle = 0;
		VkGeometryNV geometry = {};
	};

	struct TopLevelAccelerationStructure
	{
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkAccelerationStructureNV accelerationStructure = VK_NULL_HANDLE;
		uint64_t handle = 0;
	};

	
public:
	DECL_NO_COPY(AccelerationTableVK);

	AccelerationTableVK(IGraphicsContext* pContext);
	~AccelerationTableVK();

	uint32_t addMeshInstance(MeshVK* pMesh, const glm::mat3x4& transform = glm::mat3x4());
	bool finalize();

private:
	bool initTLAS();
	bool buildAccelerationTable();
	
private:
	GraphicsContextVK* m_pContext;
	DeviceVK* m_pDevice;

	TopLevelAccelerationStructure m_TopLevelAccelerationStructure;
	std::unordered_map<MeshVK*, BottomLevelAccelerationStructure> m_BottomLevelAccelerationStructures;

	std::vector<GeometryInstance> m_AllGeometryInstances;

	VkDeviceSize m_MaxMemReqBLAS;
	
	CommandPoolVK* m_pTempCommandPool;
	CommandBufferVK* m_pTempCommandBuffer;
};
