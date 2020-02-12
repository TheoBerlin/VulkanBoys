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

class RayTracingSceneVK
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
		uint32_t index = 0;
	};

	struct TopLevelAccelerationStructure
	{
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkAccelerationStructureNV accelerationStructure = VK_NULL_HANDLE;
		uint64_t handle = 0;
	};

	
public:
	DECL_NO_COPY(RayTracingSceneVK);

	RayTracingSceneVK(IGraphicsContext* pContext);
	~RayTracingSceneVK();

	uint32_t addMeshInstance(IMesh* pMesh, const glm::mat3x4& transform = glm::mat3x4(1.0f));
	void updateMeshInstance(uint32_t index, const glm::mat3x4& transform);
	bool finalize();

	void update();

	BufferVK* getCombinedVertexBuffer() { return m_pCombinedVertexBuffer; }
	BufferVK* getCombinedIndexBuffer() { return m_pCombinedIndexBuffer; }
	const TopLevelAccelerationStructure& getTLAS() { return m_TopLevelAccelerationStructure; }
	
private:
	bool initBLAS(MeshVK* pMesh);
	bool initTLAS();
	bool buildAccelerationTable();

	void cleanGarbage();
	void updateScratchBuffer();
	void updateInstanceBuffer();
	VkDeviceSize findMaxMemReqBLAS();
	
private:
	GraphicsContextVK* m_pContext;
	DeviceVK* m_pDevice;

	BufferVK* m_pScratchBuffer;
	BufferVK* m_pInstanceBuffer;
	BufferVK* m_pGarbageScratchBuffer;
	BufferVK* m_pGarbageInstanceBuffer;

	BufferVK* m_pCombinedVertexBuffer;
	BufferVK* m_pCombinedIndexBuffer;

	TopLevelAccelerationStructure m_TopLevelAccelerationStructure;
	std::unordered_map<MeshVK*, BottomLevelAccelerationStructure> m_BottomLevelAccelerationStructures;

	std::vector<GeometryInstance> m_AllGeometryInstances;
	
	CommandPoolVK* m_pTempCommandPool;
	CommandBufferVK* m_pTempCommandBuffer;

	bool m_Finalized;
};
