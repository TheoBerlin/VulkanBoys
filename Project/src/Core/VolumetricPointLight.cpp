#include "VolumetricPointLight.h"

#include "Common/IBuffer.h"
#include "Common/IDescriptorSet.h"
#include "Common/IImage.h"
#include "Common/IImageView.h"

#include "glm/glm.hpp"

VolumetricPointLight::VolumetricPointLight(const VolumetricLightSettings& volumetricLightSettings, const glm::vec3& position, const glm::vec4& color, float radius)
	:PointLight(position, color),
	m_Radius(radius),
    m_ScatterAmount(volumetricLightSettings.m_ScatterAmount),
    m_ParticleG(volumetricLightSettings.m_ParticleG),
	m_pVolumetricLightBuffer(nullptr),
	m_pVolumetricLightImage(nullptr),
	m_pVolumetricLightImageView(nullptr),
	m_pDescriptorSet(nullptr),
	m_LightUpdated(false)
{}

VolumetricPointLight::~VolumetricPointLight()
{
	SAFEDELETE(m_pVolumetricLightBuffer);
	SAFEDELETE(m_pVolumetricLightImage);
	SAFEDELETE(m_pVolumetricLightImageView);
	SAFEDELETE(m_pDescriptorSet);
}

void VolumetricPointLight::createVolumetricPointLightBuffer(VolumetricPointLightBuffer& buffer)
{
	glm::vec3 position = {m_Position.x, m_Position.y, m_Position.z};
	buffer.worldMatrix = glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(m_Radius));
	buffer.position = m_Position;
	buffer.color = m_Color;
	buffer.radius = m_Radius;
	buffer.scatterAmount = m_ScatterAmount;
	buffer.particleG = m_ParticleG;
}

void VolumetricPointLight::setRadius(float radius)
{
	m_Radius = radius;
	m_LightUpdated = true;
}

void VolumetricPointLight::setScatterAmount(float scatterAmount)
{
	m_ScatterAmount = scatterAmount;
	m_LightUpdated = true;
}

void VolumetricPointLight::setParticleG(float particleG)
{
	m_ParticleG = particleG;
	m_LightUpdated = true;
}
