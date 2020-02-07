#include "VulkanCommon.h"
#include "ImageViewVK.h"
#include "CommandPoolVK.h"

class IContext;

class ImageVK;
class ContextVK;
class DeviceVK;
class VulkanRenderer;

class Texture2DVK
{
public:
	DECL_NO_COPY(Texture2DVK);
	
	Texture2DVK(IContext* pContext);
	~Texture2DVK();

	bool loadFromFile(std::string filename);
	bool loadFromMemory(const void* pData, uint32_t width, uint32_t height);

	ImageVK* getImage() const { return m_pTextureImage; }
	ImageViewVK* getImageView() const { return m_pTextureImageView; }
	
private:
	ContextVK* m_pContext;

	ImageVK* m_pTextureImage;
	ImageViewVK* m_pTextureImageView;

	CommandPoolVK* m_pTempCommandPool;
	CommandBufferVK* m_pTempCommandBuffer;
};
