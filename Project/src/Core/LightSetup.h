#pragma once

#include "DirectionalLight.h"
#include "PointLight.h"
#include "VolumetricPointLight.h"

#include <optional>
#include <vector>

class LightSetup
{
public:
	LightSetup();
	~LightSetup() = default;

	void release();

	void addPointLight(const PointLight& pointlight);
	void addVolumetricPointLight(const VolumetricPointLight& pointlight);
	void setDirectionalLight(const DirectionalLight& directionalLight);

	uint32_t getPointLightCount() const { return uint32_t(m_PointLights.size()); }
	const PointLight* getPointLights() const	{ return m_PointLights.data(); }

	std::vector<VolumetricPointLight>& getVolumetricPointLights()	{ return m_VolumetricPointLights; }

	bool hasDirectionalLight() const { return m_DirectionalLight.has_value(); }
	DirectionalLight* getDirectionalLight() { return &m_DirectionalLight.value(); }

private:
	std::vector<PointLight> m_PointLights;
	std::vector<VolumetricPointLight> m_VolumetricPointLights;
	std::optional<DirectionalLight> m_DirectionalLight;
};
