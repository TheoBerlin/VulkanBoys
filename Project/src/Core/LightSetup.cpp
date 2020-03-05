#include "LightSetup.h"

LightSetup::LightSetup()
	: m_PointLights()
{
}

void LightSetup::addPointLight(const PointLight& pointlight)
{
	m_PointLights.emplace_back(pointlight);
}

