#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
{
}

VulkanRenderer::~VulkanRenderer()
{
}

Material* VulkanRenderer::makeMaterial(const std::string& name)
{
	return nullptr;
}

Mesh* VulkanRenderer::makeMesh()
{
	return nullptr;
}

VertexBuffer* VulkanRenderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage)
{
	return nullptr;
}

Texture2D* VulkanRenderer::makeTexture2D()
{
	return nullptr;
}

Sampler2D* VulkanRenderer::makeSampler2D()
{
	return nullptr;
}

RenderState* VulkanRenderer::makeRenderState()
{
	return nullptr;
}

std::string VulkanRenderer::getShaderPath()
{
	return "Unknown";
}

std::string VulkanRenderer::getShaderExtension()
{
	return "Unknown";
}

ConstantBuffer* VulkanRenderer::makeConstantBuffer(std::string NAME, unsigned location)
{
	return nullptr;
}

Technique* VulkanRenderer::makeTechnique(Material*, RenderState*)
{
	return nullptr;
}

int VulkanRenderer::initialize(unsigned width, unsigned height)
{
	m_VulkanDevice.initialize("Prankster");

	for (auto& commandBuffer : m_VulkanCommandBuffers) {
		commandBuffer.initialize(&m_VulkanDevice);
	}

	return 0;
}

void VulkanRenderer::setWinTitle(const char* title)
{
}

void VulkanRenderer::present()
{
}

int VulkanRenderer::shutdown()
{
	m_VulkanDevice.release();
	return 0;
}

void VulkanRenderer::setClearColor(float, float, float, float)
{
}

void VulkanRenderer::clearBuffer(unsigned)
{
}

void VulkanRenderer::setRenderState(RenderState* ps)
{
}

void VulkanRenderer::submit(Mesh* mesh)
{
}

void VulkanRenderer::frame()
{
}
