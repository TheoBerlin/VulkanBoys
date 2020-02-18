#include "Common/ITexture2D.h"
#include "VulkanCommon.h"

class IGraphicsContext;

class ImageVK;
class ImageViewVK;
class DeviceVK;
class CommandPoolVK;
class CommandBufferVK;

class Texture2DVK : public ITexture2D
{
public:
	DECL_NO_COPY(Texture2DVK);
	
	Texture2DVK(DeviceVK* pDevice);
	~Texture2DVK();

	virtual bool initFromFile(const std::string& filename, bool generateMips) override;
	virtual bool initFromMemory(const void* pData, uint32_t width, uint32_t height, bool generateMips) override;

	ImageVK* getImage() const { return m_pTextureImage; }
	ImageViewVK* getImageView() const { return m_pTextureImageView; }

private:
	DeviceVK* m_pDevice;
	ImageVK* m_pTextureImage;
	ImageViewVK* m_pTextureImageView;
};
