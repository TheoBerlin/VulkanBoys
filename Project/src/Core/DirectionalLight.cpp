#include "DirectionalLight.h"

#include "Core/Core.h"
#include "Common/IBuffer.h"
#include "Common/IFrameBuffer.h"
#include "Common/IImage.h"
#include "Common/IImageView.h"

#define DISTANCE_TO_ORIGIN 15.0f
#define DEPTH_MAX 50.0f

DirectionalLight::DirectionalLight(const VolumetricLightSettings& volumetricLightSettings, const glm::vec3& direction, const glm::vec4& color)
    :m_Direction(direction),
    m_Color(color),
    m_ScatterAmount(volumetricLightSettings.m_ScatterAmount),
    m_ParticleG(volumetricLightSettings.m_ParticleG),
    m_pFrameBuffer(nullptr),
    m_pDepthImage(nullptr),
    m_pDepthImageView(nullptr),
    m_pTransformBuffer(nullptr),
    m_IsUpdated(true),
    m_pDescriptorSet(nullptr)
{
    m_Position = -direction * DISTANCE_TO_ORIGIN;
}

DirectionalLight::~DirectionalLight()
{
    SAFEDELETE(m_pFrameBuffer);
    SAFEDELETE(m_pDepthImage);
    SAFEDELETE(m_pDepthImageView);
    SAFEDELETE(m_pTransformBuffer);
}

void DirectionalLight::createLightTransformBuffer(LightTransformBuffer& buffer, glm::vec2 viewportDimensions)
{
    glm::vec3 right = glm::normalize(glm::cross(m_Direction, {0.0f, -1.0f, 0.0f}));
    glm::vec3 up    = glm::normalize(glm::cross(right, m_Direction));

    const float height = 30.0f;

    float aspectRatio = viewportDimensions.x / viewportDimensions.y;
    float width = height * aspectRatio;

    glm::mat4 view       = glm::lookAt(m_Position, m_Position + m_Direction, up);
    glm::mat4 projection = glm::ortho(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, -DEPTH_MAX, DEPTH_MAX);
    buffer.viewProj      = projection * view;
    buffer.invViewProj   = glm::inverse(buffer.viewProj);
}
