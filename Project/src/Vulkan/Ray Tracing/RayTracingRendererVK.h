#pragma once

#include "Common/IRenderer.h"
#include "Vulkan/VulkanCommon.h"

class GraphicsContextVK;
class RenderingHandlerVK;
class RayTracingSceneVK;
class RayTracingPipelineVK;
class ShaderBindingTableVK;
class DescriptorSetLayoutVK;
class PipelineLayoutVK;
class DescriptorPoolVK;
class DescriptorSetVK;
class CommandPoolVK;
class CommandBufferVK;
class ImageVK;
class ImageViewVK;
class ShaderVK;
class SamplerVK;
class Texture2DVK;
class BufferVK;

struct TempMaterial;

class RayTracingRendererVK : public IRenderer
{
public:
	RayTracingRendererVK(GraphicsContextVK* pContext, RenderingHandlerVK* pRenderingHandler);
	~RayTracingRendererVK();

	virtual bool init() override;

	virtual void beginFrame(const Camera& camera) override;
	virtual void endFrame() override;

	virtual void submitMesh(IMesh* pMesh, TempMaterial* pMaterial, const glm::mat4& transform);

private:
	bool createCommandPoolAndBuffers();
	bool createPipelineLayouts();

private:
	GraphicsContextVK* m_pContext;
	RenderingHandlerVK* m_pRenderingHandler;

	CommandPoolVK* m_ppGraphicsCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppGraphicsCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	CommandPoolVK* m_ppComputeCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppComputeCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	RayTracingPipelineVK* m_pRayTracingPipeline;
	RayTracingSceneVK* m_pRayTracingScene;

	PipelineLayoutVK* m_pRayTracingPipelineLayout;

	ImageVK* m_pRayTracingStorageImage;
	ImageViewVK* m_pRayTracingStorageImageView;

	DescriptorSetVK* m_pRayTracingDescriptorSet;
	DescriptorPoolVK* m_pRayTracingDescriptorPool;
	DescriptorSetLayoutVK* m_pRayTracingDescriptorSetLayout;

	//Temp?
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