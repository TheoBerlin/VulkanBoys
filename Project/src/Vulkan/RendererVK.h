#pragma once
#include "Common/IRenderer.h"

#include "VulkanCommon.h"

#include "Common/IMesh.h"
#include "Core/Material.h"

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
class DescriptorSetVK;
class DescriptorPoolVK;
class PipelineLayoutVK;
class SkyboxRendererVK;
class GraphicsContextVK;
class DescriptorSetLayoutVK;

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
#define LIGHT_BUFFER_BINDING 4

//Stealing name from Unity
struct MeshFilter
{
	const IMesh* pMesh			= nullptr;
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

			return ((hash<uint32_t>()(filter.pMesh->getMeshID()) ^
				(hash<uint32_t>()(filter.pMaterial->getMaterialID()) << 1)) >> 1);
		}
	};
}

//Meshfilter is key, returns a meshpipeline -> gets descriptorset with correct vertexbuffer, textures, etc.
struct MeshPipeline
{
	DescriptorSetVK* pDescriptorSets;
};

class RendererVK : public IRenderer
{
public:
	RendererVK(GraphicsContextVK* pContext);
	~RendererVK();

	virtual bool init() override;

	virtual ITextureCube* generateTextureCubeFromPanorama(ITexture2D* pPanorama, uint32_t width, uint32_t miplevels, ETextureFormat format) override;

	virtual void onWindowResize(uint32_t width, uint32_t height) override;

	virtual void beginFrame(const Camera& camera, const LightSetup& lightSetup) override;
	virtual void endFrame() override;

	virtual void setClearColor(float r, float g, float b) override;
	virtual void setClearColor(const glm::vec3& color) override;
	virtual void setSkybox(ITextureCube* pSkybox) override;
	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;

	virtual void swapBuffers() override;

	virtual void submitMesh(IMesh* pMesh, const Material& material, const glm::mat4& transform) override;

	virtual void drawImgui(IImgui* pImgui) override;

private:
	void createFramebuffers();
	void releaseFramebuffers();

	bool createGBuffer();
	bool createSemaphores();
	bool createCommandPoolAndBuffers();
	bool createRenderPass();
	bool createPipelines();
	bool createPipelineLayouts();
	bool createBuffersAndTextures();
	bool createSamplers();

	DescriptorSetVK* getDescriptorSetFromMeshAndMaterial(const IMesh* pMesh, const Material* pMaterial);

private:
	std::unordered_map<MeshFilter, MeshPipeline> m_MeshTable;

	GraphicsContextVK* m_pContext;
	SkyboxRendererVK* m_pSkyboxRenderer;
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	GBufferVK* m_pGBuffer;
	RenderPassVK* m_pRenderPass;
	RenderPassVK* m_pBackBufferRenderPass;
	FrameBufferVK* m_ppBackbuffers[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];

	DescriptorPoolVK* m_pDescriptorPool;
	
	SamplerVK* m_pSkyboxSampler;
	SamplerVK* m_pGBufferSampler;
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
	
	//TEMPORARY MOVE TO MATERIAL or SOMETHING
	PipelineVK* m_pGeometryPipeline;
	PipelineLayoutVK* m_pGeometryPipelineLayout;
	DescriptorSetLayoutVK* m_pGeometryDescriptorSetLayout;

	TextureCubeVK* m_pSkybox;
	VkClearValue m_ClearColor;
	VkClearValue m_ClearDepth;
	VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

	uint64_t m_CurrentFrame;
};

