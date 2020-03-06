#pragma once
#include "Common/ITextureCube.h"

class ImageVK;
class DeviceVK;
class ImageViewVK;

class TextureCubeVK : public ITextureCube
{
public:
	TextureCubeVK(DeviceVK* pDevice);
	~TextureCubeVK();

	virtual bool init(uint32_t width, uint32_t miplevels, ETextureFormat format) override;

	ImageVK* getImage() const { return m_pImage; }
	ImageViewVK* getImageView() const { return m_pImageView; }
	uint32_t getWidth() const { return m_Width; }
	uint32_t getMiplevels() const { return m_Miplevels; }
	ETextureFormat getFormat() const { return m_Format; }

private:
	DeviceVK* m_pDevice;
	ImageVK* m_pImage;
	ImageViewVK* m_pImageView;
	uint32_t m_Width;
	uint32_t m_Miplevels;
	ETextureFormat m_Format;
};