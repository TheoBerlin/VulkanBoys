#pragma once

#include <vulkan/vulkan.h>
#include <SDL.h>

#include "../Renderer.h"

#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "Common.h"

class VulkanVertexBuffer;
class VulkanConstantBuffer;
class VulkanTexture2D;
class VulkanSampler2D;

class VulkanRenderer : public Renderer
{
	union DescriptorSetLayouts
	{
		VkDescriptorSetLayout layouts[2];

		struct
		{
			VkDescriptorSetLayout uniformAndStorageDescriptorSetLayout;
			VkDescriptorSetLayout textureDescriptorSetLayout;
		};
	};

	struct DescriptorData
	{
		VkPipelineLayout pipelineLayout;
		DescriptorSetLayouts descriptorSetLayouts;
		VkDescriptorSet* descriptorSets;
	};
	
public:
	VulkanRenderer();
	~VulkanRenderer();

	Material* makeMaterial(const std::string& name);
	Mesh* makeMesh();
	VertexBuffer* makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage);
	Texture2D* makeTexture2D();
	Sampler2D* makeSampler2D();
	RenderState* makeRenderState();
	std::string getShaderPath();
	std::string getShaderExtension();
	ConstantBuffer* makeConstantBuffer(std::string NAME, unsigned int location);
	Technique* makeTechnique(Material* pMaterial, RenderState* pRenderState);

	void createBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize size, VkDeviceSize memoryOffset, VkMemoryPropertyFlags properties, VkBufferUsageFlags usage);
	void setVertexBuffer(VulkanVertexBuffer* pBuffer, uint32_t slot);
	void setConstantBuffer(VulkanConstantBuffer* pBuffer, uint32_t slot);
	void setTexture2D(VulkanTexture2D* pTexture2D, VulkanSampler2D* pSampler2D);
	void commitState();

	
	//Renderer() { /*InitializeCriticalSection(&protectHere);*/ };
	int initialize(unsigned int width = 800, unsigned int height = 600);
	void setWinTitle(const char* title);
	void present();
	int shutdown();

	void setClearColor(float, float, float, float);
	void clearBuffer(unsigned int);
	// can be partially overriden by a specific Technique.
	void setRenderState(RenderState* ps);
	// submit work (to render) to the renderer.
	void submit(Mesh* mesh);
	void frame();

private:
	void initializeRenderPass();
	void createSemaphores();

	void createDescriptorSetLayouts(DescriptorSetLayouts& descriptorSetLayouts);
	void createPipelineLayout(VkPipelineLayout& pipelineLayout, DescriptorSetLayouts& descriptorSetLayouts);
	void createDescriptorSets(VkDescriptorSet descriptorSets[], DescriptorSetLayouts& descriptorSetLayouts);
	void updateStorageDescriptorSets();
	void updateUniformDescriptorSets();
	void updateSamplerDescriptorSets();

private:
	SDL_Window* m_pWindow;
	
	VulkanDevice m_VulkanDevice;
	VulkanCommandBuffer m_VulkanCommandBuffers[MAX_FRAMES_IN_FLIGHT];
	VulkanSwapChain m_Swapchain;

	VkRenderPass m_RenderPass;

	uint32_t m_FrameIndex;
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;

	DescriptorData* m_pDescriptorData;

	VulkanVertexBuffer* m_pVertexBuffers[3];
	VulkanConstantBuffer* m_pConstantBuffer[7];
	VulkanTexture2D* m_pTexture2D;
	VulkanSampler2D* m_pSampler2D;
};
