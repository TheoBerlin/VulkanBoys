#include "Material.h"

#include "Common/IGraphicsContext.h"
#include "Common/ISampler.h"

uint32_t Material::s_ID = 0;

Material::Material()
	: m_pAlbedoMap(nullptr),
	m_pNormalMap(nullptr),
	m_pSampler(nullptr),
	m_ID(s_ID++)
{
}

Material::~Material()
{
	release();
}

bool Material::createSampler(IGraphicsContext* pContext, const SamplerParams& params)
{
	ASSERT(m_pSampler == nullptr);

	m_pSampler = pContext->createSampler();
	return m_pSampler->init(params);
}

void Material::setAlbedoMap(ITexture2D* pAlbedo)
{
	m_pAlbedoMap = pAlbedo;
}

void Material::setNormalMap(ITexture2D* pNormal)
{
	m_pNormalMap = pNormal;
}

void Material::release()
{
	SAFEDELETE(m_pSampler);
}
