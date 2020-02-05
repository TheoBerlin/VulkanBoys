#include "../Texture2D.h"

#include "vulkan/vulkan.h"

class VulkanRenderer;

class VulkanTexture2D : public Texture2D
{
public:
    VulkanTexture2D(VulkanRenderer* pRenderer);
    ~VulkanTexture2D();

    int loadFromFile(std::string filename);
    int loadFromMemory(const void* pData, uint32_t width, uint32_t height);

    void bind(unsigned int slot);

    VkImage getImage() const { return m_TextureImage; }
    VkImageView getImageView() const { return m_TextureImageView; }
private:
    VulkanRenderer* m_pRenderer;

    VkImage m_TextureImage;
    VkImageView m_TextureImageView;
    VkDeviceMemory m_TextureImageMemory;
};
