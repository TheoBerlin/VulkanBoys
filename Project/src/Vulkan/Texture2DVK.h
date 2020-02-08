#include "Common/ITexture2D.h"
#include "VulkanCommon.h"

class IGraphicsContext;

class ImageVK;
class ImageViewVK;
class DeviceVK;
class GraphicsContextVK;
class CommandPoolVK;
class CommandBufferVK;

class Texture2DVK : public ITexture2D
{
public:
	DECL_NO_COPY(Texture2DVK);
	
	Texture2DVK(IGraphicsContext* pContext);
	~Texture2DVK();

	virtual bool loadFromFile(const std::string& filename) override;
	virtual bool loadFromMemory(const void* pData, uint32_t width, uint32_t height) override;

	ImageVK* getImage() const { return m_pTextureImage; }
	ImageViewVK* getImageView() const { return m_pTextureImageView; }
	
private:
	GraphicsContextVK* m_pContext;

	ImageVK* m_pTextureImage;
	ImageViewVK* m_pTextureImageView;

	CommandPoolVK* m_pTempCommandPool;
	CommandBufferVK* m_pTempCommandBuffer;
};
