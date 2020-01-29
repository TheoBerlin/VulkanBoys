#include "../ConstantBuffer.h"

#include "Vulkan/vulkan.h"

class VulkanRenderer;

class VulkanConstantBuffer
{
public:
    VulkanConstantBuffer(std::string NAME, unsigned int location);
    ~VulkanConstantBuffer();

    // It is unknown when the buffer is initialized, therefore some resources need to be stored before that point
    void provideResources(VulkanRenderer* renderer);

    // Initialize the constant buffer instance with necessary vulkan resources
    void initialize(VkDeviceSize size);

    void setData(const void* data, size_t size, Material* m, unsigned int location);
    void bind(Material* material);

private:
    std::string m_Name;

    // Used for an ugly hack. OpenGL doesn't need to know the size of constant buffers at creation, but Vulkan does.
    // The size of the buffer is first found when setData is called, and if this handle is VK_NULL_HANDLE,
    // the buffer is initialized.
    VkBuffer m_BufferHandle;

    VulkanRenderer* m_pRenderer;

    VkDevice m_Device;
    VkDeviceMemory m_BufferMemory;
};
