#pragma once

#include "DirectionalLight.h"
#include "PointLight.h"

#include <optional>
#include <vector>

class LightSetup
{
public:
	LightSetup();
	~LightSetup() = default;

	void addPointLight(const PointLight& pointlight);
	void setDirectionalLight(const DirectionalLight& directionalLight);

	uint32_t getPointLightCount() const { return uint32_t(m_PointLights.size()); }
	const PointLight* getPointLights() const { return m_PointLights.data(); }

	bool hasDirectionalLight() const { return m_DirectionalLight.has_value(); }
	DirectionalLight* getDirectionalLight() { return &m_DirectionalLight.value(); }

private:
	std::vector<PointLight> m_PointLights;
	std::optional<DirectionalLight> m_DirectionalLight;
};
