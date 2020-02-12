#include "Material.h"

#include "Common/IGraphicsContext.h"
#include "Common/ISampler.h"

Material::Material()
	: m_pAlbedo(nullptr),
	m_pSampler(nullptr)
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
	m_pAlbedo = pAlbedo;
}

void Material::release()
{
	SAFEDELETE(m_pSampler);
}

