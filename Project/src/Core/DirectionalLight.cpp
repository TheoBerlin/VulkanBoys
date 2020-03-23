#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(const VolumetricLightSettings& volumetricLightSettings, const glm::vec3& position, const glm::vec3 direction, const glm::vec4 color)
    :m_Position(position),
    m_Direction(direction),
    m_Color(color),
    m_ScatterAmount(volumetricLightSettings.m_ScatterAmount),
    m_ParticleG(volumetricLightSettings.m_ParticleG)
{}
