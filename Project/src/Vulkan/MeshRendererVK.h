#pragma once

#include "Common/IRenderer.h"
#include "ProfilerVK.h"
#include "VulkanCommon.h"

class BufferVK;
class CommandPoolVK;
class CommandBufferVK;
class DescriptorPoolVK;
class DescriptorSetLayoutVK;
class DescriptorSetVK;
class FrameBufferVK;
class GraphicsContextVK;
class PipelineLayoutVK;
class PipelineVK;
class RenderingHandlerVK;
class RenderPassVK;

//Temp
class RayTracingSceneVK;
class RayTracingPipelineVK;
class ShaderBindingTableVK;
class ImageVK;
class ImageViewVK;
class ShaderVK;
class SamplerVK;
class Texture2DVK;
struct TempMaterial;

class MeshRendererVK : public IRenderer
{
public:
	MeshRendererVK(GraphicsContextVK* pContext, RenderingHandlerVK* pRenderingHandler);
	~MeshRendererVK();

	virtual bool init() override;

	virtual void beginFrame(const Camera& camera) override;
	virtual void endFrame() override;

	void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY);
	void submitMesh(IMesh* pMesh, const glm::vec4& color, const glm::mat4& transform);

	ProfilerVK* getProfiler() { return m_pProfiler; }

private:
	bool createSemaphores();
	bool createCommandPoolAndBuffers();
	bool createPipelines();
	bool createPipelineLayouts();
	bool createRayTracingPipelineLayouts();
	void createProfiler();

private:
	GraphicsContextVK* m_pContext;
	RenderingHandlerVK* m_pRenderingHandler;
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	CommandPoolVK* m_ppComputeCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppComputeCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	RenderPassVK* m_pRenderPass;
	FrameBufferVK* m_ppBackbuffers[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];

	ProfilerVK* m_pProfiler;
	Timestamp m_TimestampDrawIndexed;

	PipelineVK* m_pPipeline;
	PipelineLayoutVK* m_pPipelineLayout;
	DescriptorSetVK* m_pDescriptorSet;
	DescriptorPoolVK* m_pDescriptorPool;
	DescriptorSetLayoutVK* m_pDescriptorSetLayout;

	VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

	uint64_t m_CurrentFrame;
	uint32_t m_BackBufferIndex;
};
