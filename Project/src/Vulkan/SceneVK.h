#pragma once
#include "Common/IScene.h"

#include "Core/Material.h"
#include "Vulkan/MeshVK.h"
#include "Vulkan/ProfilerVK.h"
#include "Vulkan/Texture2DVK.h"
#include "Vulkan/VulkanCommon.h"

#include <vector>
#include <map>

class BufferVK;
class DescriptorPoolVK;
class DescriptorSetLayoutVK;
class DescriptorSetVK;
class DeviceVK;
class GraphicsContextVK;
class IGraphicsContext;
class ITexture2D;
class PipelineLayoutVK;
class RenderingHandlerVK;
class SamplerVK;
class Texture2DVK;
class CommandPoolVK;
class CommandBufferVK;

//Geometry pass
#define CAMERA_BUFFER_BINDING		0
#define VERTEX_BUFFER_BINDING		1
#define ALBEDO_MAP_BINDING			2
#define NORMAL_MAP_BINDING			3
#define AO_MAP_BINDING				4
#define METALLIC_MAP_BINDING		5
#define ROUGHNESS_MAP_BINDING		6
#define MATERIAL_PARAMETERS_BINDING	7
#define INSTANCE_TRANSFORMS_BINDING	8

constexpr uint32_t NUM_INITIAL_GRAPHICS_OBJECTS = 10;

struct GraphicsObjectVK
{
	const MeshVK* pMesh = nullptr;
	const Material* pMaterial = nullptr;
	uint32_t MaterialParametersIndex = 0;
};

//Meshfilter is key, returns a meshpipeline -> gets descriptorset with correct vertexbuffer, textures, etc.
struct MeshPipeline
{
	DescriptorSetVK* pDescriptorSets;
};

struct MeshFilter
{
	const MeshVK*	pMesh		= nullptr;
	const Material* pMaterial	= nullptr;

	FORCEINLINE bool operator==(const MeshFilter& other) const
	{
		ASSERT(pMesh		&& other.pMesh);
		ASSERT(pMaterial	&& other.pMaterial);

		return (pMesh->getMeshID() == other.pMesh->getMeshID()) && (pMaterial->getMaterialID() == other.pMaterial->getMaterialID());
	}
};

namespace std
{
	template<> struct hash<MeshFilter>
	{
		FORCEINLINE size_t operator()(const MeshFilter& filter) const
		{
			ASSERT(filter.pMesh);
			ASSERT(filter.pMaterial);

			return ((hash<uint32_t>()(filter.pMesh->getMeshID()) ^ (hash<uint32_t>()(filter.pMaterial->getMaterialID()) << 1)) >> 1);
		}
	};
}

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

	SceneVK(IGraphicsContext* pContext, const RenderingHandlerVK* pRenderingHandler);
	~SceneVK();

	virtual bool loadFromFile(const std::string& dir, const std::string& fileName) override;

	virtual bool init() override;
	virtual bool finalize() override;
	virtual void updateMeshesAndGraphicsObjects() override;
	virtual void updateMaterials() override;

	virtual void updateCamera(const Camera& camera) override;

	virtual uint32_t submitGraphicsObject(const IMesh* pMesh, const Material* pMaterial, const glm::mat4& transform = glm::mat4(1.0f), uint8_t customMask = 0x80) override;
	virtual void updateGraphicsObjectTransform(uint32_t index, const glm::mat4& transform) override;

	void copySceneData(CommandBufferVK* pTransferBuffer);

	// Used for geometry rendering
	void updateSceneData();
	DescriptorSetVK* getDescriptorSetFromMeshAndMaterial(const MeshVK* pMesh, const Material* pMaterial);

	PipelineLayoutVK* getGeometryPipelineLayout() { return m_pGeometryPipelineLayout; }

	const Camera& getCamera() { return m_Camera; }

	virtual void renderUI() override;
	virtual void updateDebugParameters() override;

	virtual LightSetup& getLightSetup() override { return m_LightSetup; }

	const Camera&							getCamera() const			{ return m_Camera; }
	const std::vector<GraphicsObjectVK>&	getGraphicsObjects() const	{ return m_GraphicsObjects; }

	FORCEINLINE BufferVK*	getCombinedVertexBuffer()		{ return m_pCombinedVertexBuffer; }
	FORCEINLINE BufferVK*	getCombinedIndexBuffer()		{ return m_pCombinedIndexBuffer; }
	FORCEINLINE BufferVK*	getMeshIndexBuffer()			{ return m_pMeshIndexBuffer; }
	FORCEINLINE ProfilerVK* getProfiler()					{ return m_pProfiler; }

	FORCEINLINE const std::vector<const ImageViewVK*>&	getAlbedoMaps() const				{ return m_AlbedoMaps; }
	FORCEINLINE const std::vector<const ImageViewVK*>&	getNormalMaps() const				{ return m_NormalMaps; }
	FORCEINLINE const std::vector<const ImageViewVK*>&	getAOMaps() const					{ return m_AOMaps; }
	FORCEINLINE const std::vector<const ImageViewVK*>&	getMetallicMaps() const				{ return m_MetallicMaps; }
	FORCEINLINE const std::vector<const ImageViewVK*>&	getRoughnessMaps() const			{ return m_RoughnessMaps; }
	FORCEINLINE const std::vector<const SamplerVK*>&	getSamplers() const					{ return m_Samplers; }
	FORCEINLINE const BufferVK*							getMaterialParametersBuffer() const	{ return m_pMaterialParametersBuffer; }
	FORCEINLINE const BufferVK*							getTransformsBuffer() const			{ return m_pTransformsBuffer; }
	FORCEINLINE const TopLevelAccelerationStructure&	getTLAS() const						{ return m_TopLevelAccelerationStructure; }

private:
	bool createDefaultTexturesAndSamplers();

	void initBuffers();

	bool createGeometryPipelineLayout();

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

	uint32_t registerMaterial(const Material* pMaterial);

private:
	GraphicsContextVK* m_pContext;
	DeviceVK* m_pDevice;
	ProfilerVK* m_pProfiler;
	Timestamp m_TimestampBuildAccelStruct; //Todo: create more of these

	std::vector<MeshVK*> m_SceneMeshes;
	std::unordered_map<std::string, ITexture2D*> m_SceneTextures;
	std::vector<Material*> m_SceneMaterials;

	Camera m_Camera;
	LightSetup m_LightSetup;

	std::vector<GraphicsObjectVK> m_GraphicsObjects;

	std::vector<GeometryInstance> m_GeometryInstances;

	// Geometry pass resources
	std::unordered_map<MeshFilter, MeshPipeline> m_MeshTable;
	BufferVK* m_pCameraBuffer;
	DescriptorPoolVK* m_pDescriptorPool;
	PipelineLayoutVK* m_pGeometryPipelineLayout;
	DescriptorSetLayoutVK* m_pGeometryDescriptorSetLayout;

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
	bool m_TransformDataIsDirty;
	bool m_MaterialDataIsDirty;
	bool m_MeshDataIsDirty;

	Texture2DVK* m_pDefaultTexture;
	Texture2DVK* m_pDefaultNormal;
	SamplerVK* m_pDefaultSampler;

	bool m_RayTracingEnabled;

	//Temp / Debug
	CommandPoolVK* m_pTempCommandPool;
	CommandBufferVK* m_pTempCommandBuffer;

	SceneParameters m_SceneParameters;

	bool m_DebugParametersDirty;
};
