#pragma once

#include "Core/VolumetricLight.h"

#include <glm/glm.hpp>

#define LIGHT_BUFFER_BINDING 9
#define SHADOW_MAP_BINDING 10

class IBuffer;
class IDescriptorSet;
class IFrameBuffer;
class IImage;
class IImageView;

struct DirectionalLightBuffer {
    glm::mat4 viewProj, invViewProj;
    glm::vec4 direction, color;
    float scatterAmount, particleG;
};

class DirectionalLight
{
public:
    DirectionalLight(const VolumetricLightSettings& volumetricLightSettings, const glm::vec3& direction, const glm::vec4& color);
    ~DirectionalLight();

    void createLightTransformBuffer(DirectionalLightBuffer& buffer, glm::vec2 viewportDimensions);

    void setDirection(const glm::vec3& direction);
    void setColor(const glm::vec4& color);
    void setScatterAmount(float scatterAmount);
    void setParticleG(float particleG);

    glm::vec3 getDirection() const  { return m_Direction; }
    glm::vec4 getColor() const      { return m_Color; }
    float getScatterAmount() const  { return m_ScatterAmount; }
    float getParticleG() const      { return m_ParticleG; }

    void setFrameBuffer(IFrameBuffer* pFrameBuffer)     { m_pFrameBuffer = pFrameBuffer; }
    void setDepthImage(IImage* pDepthImage)             { m_pDepthImage = pDepthImage; }
    void setDepthImageView(IImageView* pDepthImageView) { m_pDepthImageView = pDepthImageView; }

    IFrameBuffer* getFrameBuffer()  { return m_pFrameBuffer; }
    IImage* getDepthImage()         { return m_pDepthImage; }
    IImageView* getDepthImageView() { return m_pDepthImageView; }

    IBuffer* getTransformBuffer()               { return m_pTransformBuffer; }
    void setTransformBuffer(IBuffer* pBuffer)   { m_pTransformBuffer = pBuffer; }

    IDescriptorSet* getDescriptorSet()                      { return m_pDescriptorSet; }
    void setDescriptorSet(IDescriptorSet* pDescriptorSet)   { m_pDescriptorSet = pDescriptorSet; }

public:
    bool m_IsUpdated;

private:
    glm::vec3 m_Position, m_Direction;
    glm::vec4 m_Color;

    // Volumetric light settings
    float m_ScatterAmount, m_ParticleG;

    // Shadow map resources
    IFrameBuffer* m_pFrameBuffer;
    IImage* m_pDepthImage;
    IImageView* m_pDepthImageView;

    IBuffer* m_pTransformBuffer;
    IDescriptorSet* m_pDescriptorSet;
};
