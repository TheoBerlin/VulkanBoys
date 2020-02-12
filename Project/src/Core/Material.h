#pragma once
#include "Core.h"

class ITexture2D;

class Material
{
public:
	Material();
	~Material() = default;

	DECL_NO_COPY(Material);

	void setAlbedoMap(ITexture2D* pAlbedo);

	ITexture2D* getAlbedoMap() const { return m_pAlbedo; }

private:
	//Resources should not be owned by the material?
	ITexture2D* m_pAlbedo;
};