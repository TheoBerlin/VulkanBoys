#pragma once

#include "Common/IRenderer.h"
#include "Core/Material.h"

#include "MeshVK.h"
#include "ProfilerVK.h"

#include <unordered_map>

class BufferVK;
class CommandBufferVK;
class CommandPoolVK;
class DescriptorSetLayoutVK;
class DescriptorSetVK;
class DescriptorPoolVK;
class FrameBufferVK;
class GBufferVK;
class SamplerVK;
class PipelineVK;
class Texture2DVK;
class RenderPassVK;
class TextureCubeVK;
class PipelineLayoutVK;
class SkyboxRendererVK;
class GraphicsContextVK;
class PipelineLayoutVK;
class PipelineVK;
class RenderingHandlerVK;
class RenderPassVK;
class SceneVK;
class ImageViewVK;

//Light pass
#define GBUFFER_ALBEDO_BINDING		1
#define GBUFFER_NORMAL_BINDING		2
#define GBUFFER_VELOCITY_BINDING	3
#define GBUFFER_DEPTH_BINDING		4
#define IRRADIANCE_BINDING			5
#define ENVIRONMENT_BINDING			6
#define BRDF_LUT_BINDING			7
#define RADIANCE_BINDING			8
#define GLOSSY_BINDING				9
#define LIGHT_BUFFER_BINDING		10

class MeshRendererVK : public IRenderer
{
public:
	MeshRendererVK(GraphicsContextVK* pContext, RenderingHandlerVK* pRenderingHandler);
	~MeshRendererVK();

	virtual bool init() override;

	virtual void beginFrame(IScene* pScene) override;
	virtual void endFrame(IScene* pScene) override;

	virtual void renderUI() override;

	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;
	void setClearColor(float r, float g, float b);
	void setClearColor(const glm::vec3& color);
	void setSkybox(TextureCubeVK* pSkybox, TextureCubeVK* pIrradiance, TextureCubeVK* pEnvironmentMap);
	void setRayTracingResultImages(ImageViewVK* pRadianceImageView, ImageViewVK* pGlossyImageView);

	void submitMesh(const MeshVK* pMesh, const Material* pMaterial, uint32_t materialIndex, uint32_t transformsIndex);

	void buildLightPass(RenderPassVK* pRenderPass, FrameBufferVK* pFramebuffer);

	void onWindowResize(uint32_t width, uint32_t height);

	FORCEINLINE void setupFrame(CommandBufferVK* pPrimaryBuffer)
	{
		m_pGPassProfiler->reset(uint32_t(m_CurrentFrame), pPrimaryBuffer);
		m_pLightPassProfiler->reset(uint32_t(m_CurrentFrame), pPrimaryBuffer);
	}

	FORCEINLINE Texture2DVK*		getBRDFLookUp() const				{ return m_pIntegrationLUT; }
	FORCEINLINE ProfilerVK*			getLightProfiler() const			{ return m_pLightPassProfiler; }
	FORCEINLINE ProfilerVK*			getGeometryProfiler() const			{ return m_pGPassProfiler; }
	FORCEINLINE CommandBufferVK*	getGeometryCommandBuffer() const	{ return m_ppGeometryPassBuffers[m_CurrentFrame]; }
	FORCEINLINE CommandBufferVK*	getLightCommandBuffer() const		{ return m_ppLightPassBuffers[m_CurrentFrame]; }

private:
	bool generateBRDFLookUp();
	bool createCommandPoolAndBuffers();
	bool createPipelines();
	bool createPipelineLayouts();
	bool createTextures();
	bool createSamplers();
	void createProfiler();

	void updateGBufferDescriptors();

	VkClearValue	m_ClearColor;
	VkClearValue	m_ClearDepth;
	VkViewport		m_Viewport;
	VkRect2D		m_ScissorRect;

	GraphicsContextVK*	m_pContext;
	RenderingHandlerVK* m_pRenderingHandler;
	ProfilerVK*			m_pGPassProfiler;
	ProfilerVK*			m_pLightPassProfiler;

	// Per frame
	SceneVK* m_pScene;

	CommandPoolVK*		m_ppGeometryPassPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK*	m_ppGeometryPassBuffers[MAX_FRAMES_IN_FLIGHT];

	CommandPoolVK*		m_ppLightPassPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK*	m_ppLightPassBuffers[MAX_FRAMES_IN_FLIGHT];

	DescriptorPoolVK* m_pDescriptorPool;

	SamplerVK*		m_pSkyboxSampler;
	SamplerVK*		m_pGBufferSampler;
	SamplerVK*		m_pBRDFSampler;
	SamplerVK*		m_pRTSampler;
	Texture2DVK*	m_pDefaultTexture;
	Texture2DVK*	m_pDefaultNormal;

	const BufferVK* m_pMaterialParametersBuffer;
	const BufferVK* m_pTransformsBuffer;

	PipelineVK*				m_pLightPipeline;
	PipelineLayoutVK*		m_pLightPipelineLayout;
	DescriptorSetVK*		m_pLightDescriptorSet;
	DescriptorSetLayoutVK*	m_pLightDescriptorSetLayout;

	PipelineVK*				m_pSkyboxPipeline;
	PipelineLayoutVK*		m_pSkyboxPipelineLayout;
	DescriptorSetVK*		m_pSkyboxDescriptorSet;
	DescriptorSetLayoutVK*	m_pSkyboxDescriptorSetLayout;

	PipelineVK*				m_pGeometryPipeline;

	Texture2DVK*	m_pIntegrationLUT;
	TextureCubeVK*	m_pSkybox;
	TextureCubeVK*	m_pIrradianceMap;
	TextureCubeVK*	m_pEnvironmentMap;

	uint64_t m_CurrentFrame;
};
