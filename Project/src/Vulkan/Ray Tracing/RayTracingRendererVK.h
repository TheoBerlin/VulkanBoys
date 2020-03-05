#pragma once

#include "Common/IRenderer.h"
#include "Vulkan/ProfilerVK.h"
#include "Vulkan/VulkanCommon.h"

class GraphicsContextVK;
class RenderingHandlerVK;
class SceneVK;
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
class Material;
class MeshVK;

class RayTracingRendererVK : public IRenderer
{
public:
	RayTracingRendererVK(GraphicsContextVK* pContext, RenderingHandlerVK* pRenderingHandler);
	~RayTracingRendererVK();

	virtual bool init() override;

	virtual void beginFrame(IScene* pScene) override;
	virtual void endFrame(IScene* pScene) override;

	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;

	void onWindowResize(uint32_t width, uint32_t height);

	CommandBufferVK* getComputeCommandBufferTemp() const;
	CommandBufferVK* getGraphicsCommandBufferTemp() const;
	ProfilerVK* getProfiler() { return m_pProfiler; }

private:
	bool createCommandPoolAndBuffers();
	bool createPipelineLayouts();
	void createProfiler();

private:
	GraphicsContextVK* m_pContext;
	RenderingHandlerVK* m_pRenderingHandler;
	ProfilerVK* m_pProfiler;
	Timestamp m_TimestampTraceRays;

	CommandPoolVK* m_ppGraphicsCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppGraphicsCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	CommandPoolVK* m_ppComputeCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppComputeCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	RayTracingPipelineVK* m_pRayTracingPipeline;

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

	Texture2DVK* m_pGunAlbedo;
	Texture2DVK* m_pGunNormalMap;
	Texture2DVK* m_pGunRoughnessMap;
	Texture2DVK* m_pCubeAlbedo;
	Texture2DVK* m_pCubeNormalMap;
	Texture2DVK* m_pCubeRoughnessMap;
	Texture2DVK* m_pSphereAlbedo;
	Texture2DVK* m_pSphereNormalMap;
	Texture2DVK* m_pSphereRoughnessMap;
	Texture2DVK* m_pPlaneAlbedo;
	Texture2DVK* m_pPlaneNormalMap;
	Texture2DVK* m_pPlaneRoughnessMap;

	Material* m_pCubeMaterial;
	Material* m_pGunMaterial;
	Material* m_pSphereMaterial;
	Material* m_pPlaneMaterial;

	bool m_TempSubmitLimit;

	float m_TempTimer;
};