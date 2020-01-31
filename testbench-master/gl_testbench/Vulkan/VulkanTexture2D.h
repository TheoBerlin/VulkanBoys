#include "../Texture2D.h"

#include "vulkan/vulkan.h"

class VulkanRenderer;

class VulkanTexture2D : public Texture2D
{
public:
    VulkanTexture2D(VulkanRenderer* pRenderer);
    ~VulkanTexture2D();

    int loadFromFile(std::string filename);
    // Does nothing, binding is performed through description sets
    void bind(unsigned int slot);
private:
    VulkanRenderer* m_pRenderer;

    VkImage m_TextureImage;
    VkDeviceMemory m_TextureImageMemory;
};
