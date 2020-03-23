#pragma once
#include "PointLight.h"
#include "VolumetricPointLight.h"

#include <vector>

class LightSetup
{
public:
	LightSetup();
	~LightSetup() = default;

	void release();

	void addPointLight(const PointLight& pointlight);
	void addVolumetricPointLight(const VolumetricPointLight& pointlight);

	uint32_t getPointLightCount() const { return uint32_t(m_PointLights.size()); }
	const PointLight* getPointLights() const	{ return m_PointLights.data(); }

	std::vector<VolumetricPointLight>& getVolumetricPointLights()	{ return m_VolumetricPointLights; }

private:
	std::vector<PointLight> m_PointLights;
	std::vector<VolumetricPointLight> m_VolumetricPointLights;
};
