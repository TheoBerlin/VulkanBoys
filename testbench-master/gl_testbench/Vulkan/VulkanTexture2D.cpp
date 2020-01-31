#include "VulkanTexture2D.h"

#include "VulkanRenderer.h"

VulkanTexture2D::VulkanTexture2D(VulkanRenderer* pRenderer)
    :m_pRenderer(pRenderer)
{}

VulkanTexture2D::~VulkanTexture2D()
{}

int VulkanTexture2D::loadFromFile(std::string filename)
{
    return m_pRenderer->createTexture(m_TextureImage, m_TextureImageMemory, filename);
}

void VulkanTexture2D::bind(unsigned int slot)
{
    m_pRenderer->setTexture2D(this, reinterpret_cast<VulkanSampler2D*>(this->sampler));
}
