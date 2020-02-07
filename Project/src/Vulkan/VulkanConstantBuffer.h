#include "../ConstantBuffer.h"
#include "VulkanCommon.h"

#include "Vulkan/vulkan.h"

class VulkanCommandBuffer;
class VulkanRenderer;
class VulkanDevice;

struct PerFrameStagingBuffers
{
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
	void copyToBuffer(VulkanCommandBuffer* pCommandBuffer);
    void bind(Material* material);

	VkBuffer getBuffer() { return m_BufferHandle; }
    uint32_t getLocation() { return m_Location; }

    void setCurrentFrame(unsigned int frame) { m_CurrentFrame = frame; }

private:
    std::string m_Name;
    uint32_t m_Location;
	uint32_t m_Size;
	bool m_IsDirty;

    PerFrameStagingBuffers m_PerFrameStagingBuffers[MAX_FRAMES_IN_FLIGHT];
	VkBuffer m_BufferHandle;
	VkDeviceMemory m_BufferMemory;

    VulkanRenderer* m_pRenderer;
	VulkanDevice* m_pDevice;

    unsigned int m_CurrentFrame;
};
