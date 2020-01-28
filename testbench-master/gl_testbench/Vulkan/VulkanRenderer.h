#pragma once

#include <vulkan/vulkan.h>

#include "../Renderer.h"

#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"

const uint32_t IMAGE_COUNT = 3;

class VulkanRenderer : public Renderer
{
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
	Technique* makeTechnique(Material*, RenderState*);

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
	VulkanDevice m_VulkanDevice;
	VulkanCommandBuffer m_VulkanCommandBuffers[IMAGE_COUNT];
};
