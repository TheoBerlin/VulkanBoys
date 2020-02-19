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

private:
	DeviceVK* m_pDevice;
	ImageVK* m_pImage;
	ImageViewVK* m_pImageView;
};