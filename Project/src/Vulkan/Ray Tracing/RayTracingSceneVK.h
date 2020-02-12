#pragma once
#include "../VulkanCommon.h"

#include <vector>
#include <unordered_map>

class IGraphicsContext;
class GraphicsContextVK;
class DeviceVK;

class IMesh;
class ITexture2D;
class BufferVK;
class MeshVK;
class Texture2DVK;

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

	uint32_t addGraphicsObjectInstance(IMesh* pMesh, ITexture2D* pTexture2D, const glm::mat3x4& transform = glm::mat3x4(1.0f));
	void updateMeshInstance(uint32_t index, const glm::mat3x4& transform);
	bool finalize();

	void update();

	BufferVK* getCombinedVertexBuffer() { return m_pCombinedVertexBuffer; }
	BufferVK* getCombinedIndexBuffer() { return m_pCombinedIndexBuffer; }
	BufferVK* getMeshIndexBuffer() { return m_pMeshIndexBuffer; }
	const TopLevelAccelerationStructure& getTLAS() { return m_TopLevelAccelerationStructure; }
	
private:
	bool initBLAS(MeshVK* pMesh, Texture2DVK* pTexture2D);
	bool initTLAS();
	bool buildAccelerationTable();

	void cleanGarbage();
	void updateScratchBuffer();
	void updateInstanceBuffer();
	void createCombinedMeshData();
	VkDeviceSize findMaxMemReqBLAS();
	
private:
	GraphicsContextVK* m_pContext;
	DeviceVK* m_pDevice;

	BufferVK* m_pScratchBuffer;
	BufferVK* m_pInstanceBuffer;
	BufferVK* m_pGarbageScratchBuffer;
	BufferVK* m_pGarbageInstanceBuffer;

	std::vector<MeshVK*> m_AllMeshes;
	uint32_t m_TotalNumberOfVertices;
	uint32_t m_TotalNumberOfIndices;

	BufferVK* m_pCombinedVertexBuffer;
	BufferVK* m_pCombinedIndexBuffer;
	BufferVK* m_pMeshIndexBuffer;

	TopLevelAccelerationStructure m_TopLevelAccelerationStructure;
	std::unordered_map<MeshVK*, std::unordered_map<Texture2DVK*, BottomLevelAccelerationStructure>> m_BottomLevelAccelerationStructures;
	uint32_t m_NumBottomLevelAccelerationStructures;

	std::vector<GeometryInstance> m_AllGeometryInstances;
	
	CommandPoolVK* m_pTempCommandPool;
	CommandBufferVK* m_pTempCommandBuffer;

	bool m_Finalized;
};
