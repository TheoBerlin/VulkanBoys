#pragma once

#include "Core/VolumetricLight.h"

#include <glm/glm.hpp>

class DirectionalLight
{
public:
    DirectionalLight(const VolumetricLightSettings& volumetricLightSettings, const glm::vec3& position, const glm::vec3 direction, const glm::vec4 color);
    ~DirectionalLight() = default;

private:
    glm::vec3 m_Position, m_Direction;
    glm::vec4 m_Color;

    // Volumetric light settings
    float m_ScatterAmount, m_ParticleG;
};
