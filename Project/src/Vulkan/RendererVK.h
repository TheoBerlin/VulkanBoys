#pragma once
#include "Common/IRenderer.h"
#include "VulkanCommon.h"

class BufferVK;
class PipelineVK;
class RenderPassVK;
class FrameBufferVK;
class CommandPoolVK;
class CommandBufferVK;
class DescriptorSetVK;
class DescriptorPoolVK;
class PipelineLayoutVK;
class GraphicsContextVK;
class DescriptorSetLayoutVK;

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

class RendererVK : public IRenderer
{
public:
	RendererVK(GraphicsContextVK* pContext);
	~RendererVK();

	virtual bool init() override;

	virtual void onWindowResize(uint32_t width, uint32_t height) override;

	virtual void beginFrame(const Camera& camera) override;
	virtual void endFrame() override;

	virtual void beginRayTraceFrame(const Camera& camera) override;
	virtual void endRayTraceFrame() override;
	
	virtual void setClearColor(float r, float g, float b) override;
	virtual void setClearColor(const glm::vec3& color) override;
	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;

	virtual void swapBuffers() override;

	virtual void submitMesh(IMesh* pMesh, const glm::vec4& color, const glm::mat4& transform) override;

	virtual void traceRays() override;

	virtual void drawImgui(IImgui* pImgui) override;

	//Temporary function
	virtual void drawTriangle(const glm::vec4& color, const glm::mat4& transform) override;

private:
	void createFramebuffers();
	void releaseFramebuffers();

	bool createSemaphores();
	bool createCommandPoolAndBuffers();
	bool createRenderPass();
	bool createPipelines();
	bool createPipelineLayouts();
	bool createBuffers();

	bool createRayTracingPipelineLayouts();
	
private:
	GraphicsContextVK* m_pContext;
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	CommandPoolVK* m_ppComputeCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppComputeCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	RenderPassVK* m_pRenderPass;
	FrameBufferVK* m_ppBackbuffers[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];

	//TEMPORARY MOVE TO MATERIAL or SOMETHING
	PipelineVK* m_pPipeline;
	PipelineLayoutVK* m_pPipelineLayout;
	DescriptorSetVK* m_pDescriptorSet;
	DescriptorPoolVK* m_pDescriptorPool;
	DescriptorSetLayoutVK* m_pDescriptorSetLayout;
	BufferVK* m_pCameraBuffer;

	VkClearValue m_ClearColor;
	VkClearValue m_ClearDepth;
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

