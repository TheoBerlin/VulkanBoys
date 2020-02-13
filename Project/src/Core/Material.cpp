#include "Material.h"

#include "Common/IGraphicsContext.h"
#include "Common/ISampler.h"

uint32_t Material::s_ID = 0;

Material::Material()
	: m_pAlbedoMap(nullptr),
	m_pNormalMap(nullptr),
	m_pSampler(nullptr),
	m_Albedo(1.0f),
	m_ID(s_ID++)
{
}

Material::~Material()
{
	release();
}

bool Material::createSampler(IGraphicsContext* pContext, const SamplerParams& params)
{
	if (m_pSampler)
	{
		//Destroy the old sampler if we want to create a new
		SAFEDELETE(m_pSampler);
	}

	m_pSampler = pContext->createSampler();
	return m_pSampler->init(params);
}

bool Material::hasAlbedoMap() const
{
	return (m_pAlbedoMap != nullptr);
}

bool Material::hasNormalMap() const
{
	return (m_pNormalMap != nullptr);
}

void Material::setAlbedo(const glm::vec4& albedo)
{
	m_Albedo = albedo;
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
