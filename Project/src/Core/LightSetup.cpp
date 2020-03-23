#include "LightSetup.h"

LightSetup::LightSetup()
	: m_PointLights(),
	m_DirectionalLight()
{
}

void LightSetup::addPointLight(const PointLight& pointlight)
{
	m_PointLights.emplace_back(pointlight);
}

void LightSetup::setDirectionalLight(const DirectionalLight& directionalLight)
{
	m_DirectionalLight = directionalLight;
}
