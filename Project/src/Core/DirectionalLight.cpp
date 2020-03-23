#include "DirectionalLight.h"

#define DISTANCE_TO_ORIGIN 150.0f

DirectionalLight::DirectionalLight(const VolumetricLightSettings& volumetricLightSettings, const glm::vec3 direction, const glm::vec4 color)
    :m_Direction(direction),
    m_Color(color),
    m_ScatterAmount(volumetricLightSettings.m_ScatterAmount),
    m_ParticleG(volumetricLightSettings.m_ParticleG)
{
    m_Position = -direction * DISTANCE_TO_ORIGIN;
}
