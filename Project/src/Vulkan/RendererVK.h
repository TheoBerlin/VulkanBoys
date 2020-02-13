#pragma once
#include "Common/IRenderer.h"

#include "VulkanCommon.h"

#include "Common/IMesh.h"
#include "Core/Material.h"

#include <unordered_map>

class BufferVK;
class PipelineVK;
class Texture2DVK;
class RenderPassVK;
class FrameBufferVK;
class CommandPoolVK;
class CommandBufferVK;
class DescriptorSetVK;
class DescriptorPoolVK;
class PipelineLayoutVK;
class GraphicsContextVK;
class DescriptorSetLayoutVK;

#define CAMERA_BUFFER_BINDING 0
#define VERTEX_BUFFER_BINDING 1
#define LIGHT_BUFFER_BINDING 2
#define ALBEDO_MAP_BINDING 3
#define NORMAL_MAP_BINDING 4

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

	virtual void onWindowResize(uint32_t width, uint32_t height) override;

	virtual void beginFrame(const Camera& camera, const LightSetup& lightSetup) override;
	virtual void endFrame() override;

	virtual void setClearColor(float r, float g, float b) override;
	virtual void setClearColor(const glm::vec3& color) override;
	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;

	virtual void swapBuffers() override;

	virtual void submitMesh(IMesh* pMesh, const Material& material, const glm::mat4& transform) override;

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
	bool createBuffersAndTextures();

	DescriptorSetVK* getDescriptorSetFromMeshAndMaterial(const IMesh* pMesh, const Material* pMaterial);

private:
	std::unordered_map<MeshFilter, MeshPipeline> m_MeshTable;

	GraphicsContextVK* m_pContext;
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	RenderPassVK* m_pRenderPass;
	FrameBufferVK* m_ppBackbuffers[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];

	DescriptorPoolVK* m_pDescriptorPool;
	
	Texture2DVK* m_pDefaultTexture;
	BufferVK* m_pCameraBuffer;
	BufferVK* m_pLightBuffer;
	
	//TEMPORARY MOVE TO MATERIAL or SOMETHING
	PipelineVK* m_pPipeline;
	PipelineLayoutVK* m_pPipelineLayout;
	DescriptorSetLayoutVK* m_pDescriptorSetLayout;

	VkClearValue m_ClearColor;
	VkClearValue m_ClearDepth;
	VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

	uint64_t m_CurrentFrame;
	uint32_t m_BackBufferIndex;
};

