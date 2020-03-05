#pragma once

#include "Common/IRenderer.h"
#include "Core/Material.h"

#include "MeshVK.h"
#include "ProfilerVK.h"

#include <unordered_map>

class BufferVK;
class GBufferVK;
class SamplerVK;
class PipelineVK;
class Texture2DVK;
class RenderPassVK;
class FrameBufferVK;
class CommandPoolVK;
class TextureCubeVK;
class CommandBufferVK;
class DescriptorPoolVK;
class PipelineLayoutVK;
class SkyboxRendererVK;
class GraphicsContextVK;
class DescriptorSetLayoutVK;
class DescriptorSetVK;
class FrameBufferVK;
class GraphicsContextVK;
class PipelineLayoutVK;
class PipelineVK;
class RenderingHandlerVK;
class RenderPassVK;

//Geometry pass
#define CAMERA_BUFFER_BINDING	0
#define VERTEX_BUFFER_BINDING	1
#define ALBEDO_MAP_BINDING		2
#define NORMAL_MAP_BINDING		3
#define AO_MAP_BINDING			4
#define METALLIC_MAP_BINDING	5
#define ROUGHNESS_MAP_BINDING	6

//Light pass
#define GBUFFER_ALBEDO_BINDING		1
#define GBUFFER_NORMAL_BINDING		2
#define GBUFFER_POSITION_BINDING	3
#define IRRADIANCE_BINDING			4
#define ENVIRONMENT_BINDING			5
#define BRDF_LUT_BINDING			6
#define LIGHT_BUFFER_BINDING		7

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
		size_t operator()(const MeshFilter& filter) const
		{
			ASSERT(filter.pMesh);
			ASSERT(filter.pMaterial);

			return ((hash<uint32_t>()(filter.pMesh->getMeshID()) ^ (hash<uint32_t>()(filter.pMaterial->getMaterialID()) << 1)) >> 1);
		}
	};
}

//Meshfilter is key, returns a meshpipeline -> gets descriptorset with correct vertexbuffer, textures, etc.
struct MeshPipeline
{
	DescriptorSetVK* pDescriptorSets;
};

class MeshRendererVK : public IRenderer
{
public:
	MeshRendererVK(GraphicsContextVK* pContext, RenderingHandlerVK* pRenderingHandler);
	~MeshRendererVK();

	virtual bool init() override;

	virtual void beginFrame(const Camera& camera, const LightSetup& lightSetup) override;
	virtual void endFrame() override;

	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;
	
	void setupFrame(CommandBufferVK* pPrimaryBuffer, const Camera& camera, const LightSetup& lightsetup);

	void setClearColor(float r, float g, float b);
	void setClearColor(const glm::vec3& color);
	void setSkybox(TextureCubeVK* pSkybox, TextureCubeVK* pIrradiance, TextureCubeVK* pEnvironmentMap);

	void submitMesh(const MeshVK* pMesh, const Material* pMaterial, const glm::vec3& materialProperties, const glm::mat4& transform);
	
	void buildLightPass(RenderPassVK* pRenderPass, FrameBufferVK* pFramebuffer);

	void onWindowResize(uint32_t width, uint32_t height);
		
	FORCEINLINE ProfilerVK*			getProfiler() const					{ return m_pProfiler; }
	FORCEINLINE CommandBufferVK*	getGeometryCommandBuffer() const	{ return m_ppGeometryPassBuffers[m_CurrentFrame]; }
	FORCEINLINE CommandBufferVK*	getLightCommandBuffer() const		{ return m_ppLightPassBuffers[m_CurrentFrame]; }

private:
	bool generateBRDFLookUp();
	bool createCommandPoolAndBuffers();
	bool createRenderPass();
	bool createPipelines();
	bool createPipelineLayouts();
	bool createBuffersAndTextures();
	bool createSamplers();
	void createProfiler();

	void updateBuffers(CommandBufferVK* pPrimaryBuffer, const Camera& camera, const LightSetup& lightsetup);

	DescriptorSetVK* getDescriptorSetFromMeshAndMaterial(const MeshVK* pMesh, const Material* pMaterial);

private:
	std::unordered_map<MeshFilter, MeshPipeline> m_MeshTable;

	GraphicsContextVK* m_pContext;
	RenderingHandlerVK* m_pRenderingHandler;
	ProfilerVK* m_pProfiler;

	CommandPoolVK* m_ppGeometryPassPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppGeometryPassBuffers[MAX_FRAMES_IN_FLIGHT];

	CommandPoolVK* m_ppLightPassPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppLightPassBuffers[MAX_FRAMES_IN_FLIGHT];

	RenderPassVK* m_pBackBufferRenderPass;

	DescriptorPoolVK* m_pDescriptorPool;
	
	SamplerVK* m_pSkyboxSampler;
	SamplerVK* m_pGBufferSampler;
	SamplerVK* m_pBRDFSampler;
	Texture2DVK* m_pDefaultTexture;
	Texture2DVK* m_pDefaultNormal;
	BufferVK* m_pCameraBuffer;
	BufferVK* m_pLightBuffer;

	PipelineVK* m_pLightPipeline;
	PipelineLayoutVK* m_pLightPipelineLayout;
	DescriptorSetVK* m_pLightDescriptorSet;
	DescriptorSetLayoutVK* m_pLightDescriptorSetLayout;
	
	PipelineVK* m_pSkyboxPipeline;
	PipelineLayoutVK* m_pSkyboxPipelineLayout;
	DescriptorSetVK* m_pSkyboxDescriptorSet;
	DescriptorSetLayoutVK* m_pSkyboxDescriptorSetLayout;

	PipelineVK* m_pGeometryPipeline;
	PipelineLayoutVK* m_pGeometryPipelineLayout;
	DescriptorSetLayoutVK* m_pGeometryDescriptorSetLayout;

	Texture2DVK* m_pIntegrationLUT;
	TextureCubeVK* m_pSkybox;
	TextureCubeVK* m_pIrradianceMap;
	TextureCubeVK* m_pEnvironmentMap;
	VkClearValue m_ClearColor;
	VkClearValue m_ClearDepth;
	VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

	Timestamp m_TimestampDrawIndexed;
	uint64_t m_CurrentFrame;
};
