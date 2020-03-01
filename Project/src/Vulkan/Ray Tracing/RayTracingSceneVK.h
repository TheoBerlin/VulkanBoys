#pragma once

#include "../VulkanCommon.h"
#include "Vulkan/ProfilerVK.h"

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

struct TempMaterial
{
	~TempMaterial()
	{
		SAFEDELETE(pAlbedo);
		SAFEDELETE(pNormalMap);
		SAFEDELETE(pRoughnessMap);
	}

	Texture2DVK* pAlbedo = nullptr;
	Texture2DVK* pNormalMap = nullptr;
	Texture2DVK* pRoughnessMap = nullptr;
};

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
		uint32_t textureIndex = 0;
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

	uint32_t addGraphicsObjectInstance(IMesh* pMesh, TempMaterial* pMaterial, const glm::mat3x4& transform = glm::mat3x4(1.0f), uint8_t customMask = 0x80);
	void updateGraphicsObject(uint32_t index, const glm::mat3x4& transform);
	bool finalize();

	void update();

	void generateLightProbeGeometry(uint32_t worldSizeX, uint32_t worldSizeY, uint32_t worldSizeZ, uint32_t samplesPerProbe, uint32_t numProbesPerDimension);

	BufferVK* getCombinedVertexBuffer() { return m_pCombinedVertexBuffer; }
	BufferVK* getCombinedIndexBuffer() { return m_pCombinedIndexBuffer; }
	BufferVK* getMeshIndexBuffer() { return m_pMeshIndexBuffer; }
	const std::vector<TempMaterial*>& getAllMaterials() { return m_AllMaterials; }
	const TopLevelAccelerationStructure& getTLAS() { return m_TopLevelAccelerationStructure; }
	ProfilerVK* getProfiler() { return m_pProfiler; }
	
private:
	bool initBLAS(MeshVK* pMesh, TempMaterial* pMaterial);
	bool initTLAS();
	bool buildAccelerationTable();
	void createProfiler();

	void cleanGarbage();
	void updateScratchBuffer();
	void updateInstanceBuffer();
	void createCombinedGraphicsObjectData();
	VkDeviceSize findMaxMemReqBLAS();

private:
	GraphicsContextVK* m_pContext;
	DeviceVK* m_pDevice;
	ProfilerVK* m_pProfiler;
	Timestamp m_TimestampBuildAccelStruct;

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
	std::vector<TempMaterial*> m_AllMaterials;

	TopLevelAccelerationStructure m_TopLevelAccelerationStructure;
	std::unordered_map<MeshVK*, std::unordered_map<TempMaterial*, BottomLevelAccelerationStructure>> m_BottomLevelAccelerationStructures;
	uint32_t m_NumBottomLevelAccelerationStructures;

	std::vector<GeometryInstance> m_AllGeometryInstances;
	
	CommandPoolVK* m_pTempCommandPool;
	CommandBufferVK* m_pTempCommandBuffer;

	bool m_Finalized;

	MeshVK* m_pLightProbeMesh;
	TempMaterial* m_pVeryTempMaterial;
};
