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

	// TODO: Move these and all other ray tracing features to a separate renderer
	void beginRayTraceFrame(const Camera& camera);
	void endRayTraceFrame();
	void traceRays();

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

	// TEMPORARY MOVE TO MATERIAL or SOMETHING
	PipelineVK* m_pPipeline;
	PipelineLayoutVK* m_pPipelineLayout;
	DescriptorSetVK* m_pDescriptorSet;
	DescriptorPoolVK* m_pDescriptorPool;
	DescriptorSetLayoutVK* m_pDescriptorSetLayout;

	VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

	uint64_t m_CurrentFrame;
	uint32_t m_BackBufferIndex;

	//Temp Ray Tracing Stuff
	RayTracingSceneVK* m_pRayTracingScene;
	RayTracingPipelineVK* m_pRayTracingPipeline;
	PipelineLayoutVK* m_pRayTracingPipelineLayout;
	ShaderBindingTableVK* m_pSBT;
	ImageVK* m_pRayTracingStorageImage;
	ImageViewVK* m_pRayTracingStorageImageView;
	
	DescriptorSetVK* m_pRayTracingDescriptorSet;
	DescriptorPoolVK* m_pRayTracingDescriptorPool;
	DescriptorSetLayoutVK* m_pRayTracingDescriptorSetLayout;

	BufferVK* m_pRayTracingUniformBuffer;

	IMesh* m_pMeshCube;
	IMesh* m_pMeshGun;
	IMesh* m_pMeshSphere;
	IMesh* m_pMeshPlane;

	glm::mat4 m_Matrix0;
	glm::mat4 m_Matrix1;
	glm::mat4 m_Matrix2;
	glm::mat4 m_Matrix3;
	glm::mat4 m_Matrix4;
	glm::mat4 m_Matrix5;

	uint32_t m_InstanceIndex0;
	uint32_t m_InstanceIndex1;
	uint32_t m_InstanceIndex2;
	uint32_t m_InstanceIndex3;
	uint32_t m_InstanceIndex4;
	uint32_t m_InstanceIndex5;

	ShaderVK* m_pRaygenShader;
	ShaderVK* m_pClosestHitShader;
	ShaderVK* m_pClosestHitShadowShader;
	ShaderVK* m_pMissShader;
	ShaderVK* m_pMissShadowShader;

	SamplerVK* m_pSampler;
	
	TempMaterial* m_pCubeMaterial;
	TempMaterial* m_pGunMaterial;
	TempMaterial* m_pSphereMaterial;
	TempMaterial* m_pPlaneMaterial;

	float m_TempTimer;
};
