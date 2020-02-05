#include "VulkanTexture2D.h"
#include "VulkanRenderer.h"

#include "stb_image.h"

VulkanTexture2D::VulkanTexture2D(VulkanRenderer* pRenderer)
    :m_pRenderer(pRenderer),
    m_TextureImage(VK_NULL_HANDLE),
    m_TextureImageView(VK_NULL_HANDLE),
    m_TextureImageMemory(VK_NULL_HANDLE)
{}

VulkanTexture2D::~VulkanTexture2D()
{
    VkDevice device = m_pRenderer->getDevice()->getDevice();

    if (m_TextureImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_TextureImage, nullptr);
    }

    if (m_TextureImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_TextureImageView, nullptr);
    }

    if (m_TextureImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_TextureImageMemory, nullptr);
    }
}

int VulkanTexture2D::loadFromFile(std::string filename)
{
    int texWidth, texHeight, bpp;
    unsigned char* pPixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &bpp, STBI_rgb_alpha);
    if (pPixels == nullptr) {
        std::cerr << "Error loading texture file: " << filename.c_str() << std::endl;
        return -1;
    }

    int result = loadFromMemory(pPixels, texWidth, texHeight);
    stbi_image_free(pPixels);
    return result;
}

int VulkanTexture2D::loadFromMemory(const void* pData, uint32_t width, uint32_t height)
{
    return m_pRenderer->createTexture(m_TextureImage, m_TextureImageView, m_TextureImageMemory, pData, width, height);
}

void VulkanTexture2D::bind(unsigned int slot)
{
    m_pRenderer->setTexture2D(this, reinterpret_cast<VulkanSampler2D*>(this->sampler));
}
