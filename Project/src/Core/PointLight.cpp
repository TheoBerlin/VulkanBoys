#include "PointLight.h"

#include "Common/IDescriptorSet.h"
#include "Common/IImage.h"
#include "Common/IImageView.h"

PointLight::PointLight(const glm::vec3& position, const glm::vec4& color, bool castsVolumetricLight)
	: m_Position(position, 0.0f),
	m_Color(color),
	m_CastsVolumetricLight(castsVolumetricLight),
	m_pVolumetricLight(nullptr),
	m_pVolumetricLightView(nullptr),
	m_pDescriptorSet(nullptr)
{
}

PointLight::~PointLight()
{
	SAFEDELETE(m_pVolumetricLight);
	SAFEDELETE(m_pVolumetricLightView);
	SAFEDELETE(m_pDescriptorSet);
}

void PointLight::setColor(const glm::vec4& color)
{
	m_Color = color;
}

void PointLight::setPosition(const glm::vec3& position)
{
	m_Position = glm::vec4(position, 0.0f);
}
