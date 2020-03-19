#pragma once
#include "Common/IScene.h"

#include "Vulkan/VulkanCommon.h"
#include "Vulkan/ProfilerVK.h"
#include "Vulkan/Texture2DVK.h"

#include <vector>
#include <map>

class IGraphicsContext;
class GraphicsContextVK;
class DeviceVK;

class IMesh;
class Material;
class ITexture2D;
class BufferVK;
class MeshVK;
class Texture2DVK;
class SamplerVK;

//Todo: Remove these
class CommandPoolVK;
class CommandBufferVK;

constexpr uint32_t NUM_INITIAL_GRAPHICS_OBJECTS = 10;

struct GraphicsObjectVK
{
	const MeshVK* pMesh = nullptr;
	const Material* pMaterial = nullptr;
	uint32_t MaterialParametersIndex = 0;
};

class SceneVK : public IScene
{
	struct SceneParameters
	{
		float RoughnessScale = 1.0f;
		float MetallicScale = 1.0f;
		float AOScale = 1.0f;
	};

	struct GraphicsObjectTransforms
	{
		glm::mat4 Transform;
		glm::mat4 PrevTransform;
	};

	struct GeometryInstance
	{
		glm::mat3x4 Transform;
		uint32_t InstanceId : 24;
		uint32_t Mask : 8;
		uint32_t InstanceOffset : 24;
		uint32_t Flags : 8;
		uint64_t AccelerationStructureHandle;
	};
	
	struct BottomLevelAccelerationStructure
	{
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		VkAccelerationStructureNV AccelerationStructure = VK_NULL_HANDLE;
		uint64_t Handle = 0;
		VkGeometryNV Geometry = {};
		uint32_t Index = 0;
		uint32_t MaterialIndex = 0;
	};

	struct TopLevelAccelerationStructure
	{
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		VkAccelerationStructureNV AccelerationStructure = VK_NULL_HANDLE;
		uint64_t Handle = 0;
	};

	struct MaterialParameters
	{
		glm::vec4 Albedo;
		float Metallic;
		float Roughness;
		float AO;
		float Padding;
	};
	
public:
	DECL_NO_COPY(SceneVK);

	SceneVK(IGraphicsContext* pContext);
	~SceneVK();

	virtual bool initFromFile(const std::string& dir, const std::string& fileName) override;

	virtual bool finalize() override;
	virtual void updateMeshesAndGraphicsObjects() override;
	virtual void updateMaterials() override;

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

	const std::vector<const ImageViewVK*>& getAlbedoMaps()			{ return m_AlbedoMaps; }
	const std::vector<const ImageViewVK*>& getNormalMaps()			{ return m_NormalMaps; }
	const std::vector<const ImageViewVK*>& getAOMaps()				{ return m_AOMaps; }
	const std::vector<const ImageViewVK*>& getMetallicMaps()		{ return m_MetallicMaps; }
	const std::vector<const ImageViewVK*>& getRoughnessMaps()		{ return m_RoughnessMaps; }
	const std::vector<const SamplerVK*>& getSamplers()				{ return m_Samplers; }
	const BufferVK* getMaterialParametersBuffer()					{ return m_pMaterialParametersBuffer; }
	const BufferVK* getTransformsBuffer()							{ return m_pTransformsBuffer; }

	const TopLevelAccelerationStructure& getTLAS() { return m_TopLevelAccelerationStructure; }

	ProfilerVK* getProfiler() { return m_pProfiler; }
	
	void generateLightProbeGeometry(float probeStepX, float probeStepY, float probeStepZ, uint32_t samplesPerProbe, uint32_t numProbesPerDimension);

	virtual void renderUI() override;

private:
	bool createDefaultTexturesAndSamplers();

	void initBuffers();
	void initAccelerationStructureBuffers();

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
	void updateTransformBuffer();
	bool createCombinedGraphicsObjectData();
	VkDeviceSize findMaxMemReqBLAS();
	VkDeviceSize findMaxMemReqTLAS();

private:
	GraphicsContextVK* m_pContext;
	DeviceVK* m_pDevice;
	ProfilerVK* m_pProfiler;
	Timestamp m_TimestampBuildAccelStruct; //Todo: create more of these

	std::vector<MeshVK*> m_SceneMeshes;
	std::vector<Material*> m_SceneMaterials;

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

	std::vector<const Material*> m_Materials;
	std::map<const Material*, uint32_t> m_MaterialIndices; //This is only used when Ray Tracing is Disabled

	std::vector<const ImageViewVK*> m_AlbedoMaps;
	std::vector<const ImageViewVK*> m_NormalMaps;
	std::vector<const ImageViewVK*> m_AOMaps;
	std::vector<const ImageViewVK*> m_MetallicMaps;
	std::vector<const ImageViewVK*> m_RoughnessMaps;
	std::vector<const SamplerVK*> m_Samplers;
	std::vector<MaterialParameters> m_MaterialParameters;
	BufferVK* m_pMaterialParametersBuffer;

	std::vector<GraphicsObjectTransforms> m_SceneTransforms;
	BufferVK* m_pTransformsBuffer;

	TopLevelAccelerationStructure m_OldTopLevelAccelerationStructure;
	TopLevelAccelerationStructure m_TopLevelAccelerationStructure;
	std::map<const MeshVK*, std::map<const Material*, BottomLevelAccelerationStructure>> m_NewBottomLevelAccelerationStructures;
	std::map<const MeshVK*, std::map<const Material*, BottomLevelAccelerationStructure>> m_FinalizedBottomLevelAccelerationStructures;
	uint32_t m_NumBottomLevelAccelerationStructures;

	BufferVK* m_pScratchBuffer;
	BufferVK* m_pInstanceBuffer;
	BufferVK* m_pGarbageScratchBuffer;
	BufferVK* m_pGarbageInstanceBuffer;
	BufferVK* m_pGarbageTransformsBuffer;

	bool m_BottomLevelIsDirty;
	bool m_TopLevelIsDirty;

	Texture2DVK* m_pDefaultTexture;
	Texture2DVK* m_pDefaultNormal;
	SamplerVK* m_pDefaultSampler;

	bool m_RayTracingEnabled;

	//Temp / Debug
	CommandPoolVK* m_pTempCommandPool;
	CommandBufferVK* m_pTempCommandBuffer;

	MeshVK* m_pLightProbeMesh;
	const Material* m_pVeryTempMaterial;

	SceneParameters m_SceneParameters;
};
