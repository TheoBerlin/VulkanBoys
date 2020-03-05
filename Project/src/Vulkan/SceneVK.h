#pragma once
#include "Common/IScene.h"

#include "Vulkan/VulkanCommon.h"
#include "Vulkan/ProfilerVK.h"
#include "Vulkan/Texture2DVK.h"

#include <vector>
#include <unordered_map>

class IGraphicsContext;
class GraphicsContextVK;
class DeviceVK;

class IMesh;
class Material;
class ITexture2D;
class BufferVK;
class MeshVK;
class Texture2DVK;

//Todo: Remove these
class CommandPoolVK;
class CommandBufferVK;

class SceneVK : public IScene
{
	struct GraphicsObjectVK
	{
		const MeshVK* pMesh = nullptr;
		const Material* pMaterial = nullptr;
		glm::mat4 Transform;
	};

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
	DECL_NO_COPY(SceneVK);

	SceneVK(IGraphicsContext* pContext);
	~SceneVK();

	virtual bool finalize() override;
	virtual void update() override;

	virtual void updateCamera(const Camera& camera) override;
	virtual void updateLightSetup(const LightSetup& lightsetup) override;

	virtual uint32_t submitGraphicsObject(const IMesh* pMesh, const Material* pMaterial, const glm::mat4& transform = glm::mat4(1.0f), uint8_t customMask = 0x80) override;
	virtual void updateGraphicsObjectTransform(uint32_t index, const glm::mat4& transform) override;


	const Camera& getCamera() { return m_Camera; }

	const LightSetup& getLightSetup() { return m_LightSetup; }

	const std::vector<GraphicsObjectVK>& getGraphicsObjects() { return m_GraphicsObjects; }

	BufferVK* getCombinedVertexBuffer()		{ return m_pCombinedVertexBuffer; }
	BufferVK* getCombinedIndexBuffer()		{ return m_pCombinedIndexBuffer; }
	BufferVK* getMeshIndexBuffer()			{ return m_pMeshIndexBuffer; }

	const std::vector<const Material*>& getAllMaterials() { return m_AllMaterials; }

	const TopLevelAccelerationStructure& getTLAS() { return m_TopLevelAccelerationStructure; }

	ProfilerVK* getProfiler() { return m_pProfiler; }
	
	void generateLightProbeGeometry(float probeStepX, float probeStepY, float probeStepZ, uint32_t samplesPerProbe, uint32_t numProbesPerDimension);

private:
	void initBuildBuffers();

	BottomLevelAccelerationStructure* createBLAS(const MeshVK* pMesh, const Material* pMaterial);
	bool buildBLASs();

	bool createTLAS();
	bool buildTLAS();
	bool updateTLAS();

	void createProfiler();

	void cleanGarbage();
	void updateScratchBufferForBLAS();
	void updateScratchBufferForTLAS();
	void updateInstanceBuffer();
	bool createCombinedGraphicsObjectData();
	VkDeviceSize findMaxMemReqBLAS();
	VkDeviceSize findMaxMemReqTLAS();

private:
	GraphicsContextVK* m_pContext;
	DeviceVK* m_pDevice;
	ProfilerVK* m_pProfiler;
	Timestamp m_TimestampBuildAccelStruct; //Todo: create more of these

	Camera m_Camera;
	LightSetup m_LightSetup;

	std::vector<GraphicsObjectVK> m_GraphicsObjects;

	std::vector<GeometryInstance> m_GeometryInstances;

	std::vector<const MeshVK*> m_AllMeshes;
	uint32_t m_TotalNumberOfVertices;
	uint32_t m_TotalNumberOfIndices;

	BufferVK* m_pCombinedVertexBuffer;
	BufferVK* m_pCombinedIndexBuffer;
	BufferVK* m_pMeshIndexBuffer;
	std::vector<const Material*> m_AllMaterials;

	TopLevelAccelerationStructure m_OldTopLevelAccelerationStructure;
	TopLevelAccelerationStructure m_TopLevelAccelerationStructure;
	std::unordered_map<const MeshVK*, std::unordered_map<const Material*, BottomLevelAccelerationStructure>> m_NewBottomLevelAccelerationStructures;
	std::unordered_map<const MeshVK*, std::unordered_map<const Material*, BottomLevelAccelerationStructure>> m_FinalizedBottomLevelAccelerationStructures;
	uint32_t m_NumBottomLevelAccelerationStructures;

	BufferVK* m_pScratchBuffer;
	BufferVK* m_pInstanceBuffer;
	BufferVK* m_pGarbageScratchBuffer;
	BufferVK* m_pGarbageInstanceBuffer;

	bool m_BottomLevelIsDirty;
	bool m_TopLevelIsDirty;

	//Temp / Debug
	CommandPoolVK* m_pTempCommandPool;
	CommandBufferVK* m_pTempCommandBuffer;

	MeshVK* m_pLightProbeMesh;
	const Material* m_pVeryTempMaterial;
};
