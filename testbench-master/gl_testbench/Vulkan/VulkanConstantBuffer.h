#include "../ConstantBuffer.h"
#include "Common.h"

#include "Vulkan/vulkan.h"

class VulkanRenderer;
class VulkanDevice;

struct PerFrameResources {
    VkBuffer m_BufferHandle;
    VkDeviceMemory m_BufferMemory;
};

class VulkanConstantBuffer : public ConstantBuffer
{
public:
    VulkanConstantBuffer(std::string NAME, unsigned int location, unsigned int currentFrame);
    ~VulkanConstantBuffer();

    // It is unknown when the buffer is initialized, therefore some resources need to be stored before that point
    void provideResources(VulkanRenderer* pRenderer, VulkanDevice* pVulkanDevice);

    // Initialize the current frame's constant buffer instance with necessary vulkan resources
    void initialize(VkDeviceSize size);

    void setData(const void* data, size_t size, Material* m, unsigned int location);
    void bind(Material* material);

	VkBuffer getBuffer() { return m_PerFrameResources[m_CurrentFrame].m_BufferHandle; }
    uint32_t getLocation() { return m_Location; }

    void setCurrentFrame(unsigned int frame) { m_CurrentFrame = frame; }

private:
    std::string m_Name;
    uint32_t m_Location;

    PerFrameResources m_PerFrameResources[MAX_FRAMES_IN_FLIGHT];

    VulkanRenderer* m_pRenderer;
	VulkanDevice* m_pDevice;

    unsigned int m_CurrentFrame;
};
