#include "Material.h"

Material::Material()
	: m_pAlbedo(nullptr)
{
}

void Material::setAlbedoMap(ITexture2D* pAlbedo)
{
	m_pAlbedo = pAlbedo;
}

