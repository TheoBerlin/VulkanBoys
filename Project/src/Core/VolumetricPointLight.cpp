#include "VolumetricPointLight.h"

#include "Common/IDescriptorSet.h"
#include "Common/IImage.h"
#include "Common/IImageView.h"

VolumetricPointLight::VolumetricPointLight(const VolumetricLightSettings& volumetricLightSettings, const glm::vec3& position, const glm::vec4& color, float radius)
	:PointLight(position, color),
	m_Radius(radius),
    m_ScatterAmount(volumetricLightSettings.m_ScatterAmount),
    m_ParticleG(volumetricLightSettings.m_ParticleG),
	m_pVolumetricLightImage(nullptr),
	m_pVolumetricLightImageView(nullptr),
	m_pDescriptorSet(nullptr)
{}

VolumetricPointLight::~VolumetricPointLight()
{
	SAFEDELETE(m_pVolumetricLightImage);
	SAFEDELETE(m_pVolumetricLightImageView);
	SAFEDELETE(m_pDescriptorSet);
}
