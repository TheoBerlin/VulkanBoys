#include "LightSetup.h"

LightSetup::LightSetup()
	: m_PointLights(),
	m_DirectionalLight()
{
}

void LightSetup::release()
{
	m_PointLights.clear();
	m_VolumetricPointLights.clear();
}

void LightSetup::addPointLight(const PointLight& pointlight)
{
	m_PointLights.emplace_back(pointlight);
}

void LightSetup::addVolumetricPointLight(const VolumetricPointLight& pointlight)
{
	m_VolumetricPointLights.emplace_back(pointlight);
}

void LightSetup::setDirectionalLight(const DirectionalLight& directionalLight)
{
	m_DirectionalLight = directionalLight;
}
