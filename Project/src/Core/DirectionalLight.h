#pragma once

#include "Core/VolumetricLight.h"

#include <glm/glm.hpp>

class IFrameBuffer;
class IImage;
class IImageView;

class DirectionalLight
{
public:
    DirectionalLight(const VolumetricLightSettings& volumetricLightSettings, const glm::vec3 direction, const glm::vec4 color);
    ~DirectionalLight();

    void setPosition(const glm::vec3& position) { m_Position = position; }
    void setDirection(const glm::vec3& direction) { m_Direction = direction; }
    void setColor(const glm::vec4& color) { m_Color = color; }
    void setScatterAmount(float scatterAmount) { m_ScatterAmount = scatterAmount; }
    void setParticleG(float particleG) { m_ParticleG = particleG; }

    glm::vec3 getPosition() const { return m_Position; }
    glm::vec3 getDirection() const { return m_Direction; }
    glm::vec4 getColor() const { return m_Color; }
    float getScatterAmount() const { return m_ScatterAmount; }
    float getParticleG() const { return m_ParticleG; }

    void setFrameBuffer(IFrameBuffer* pFrameBuffer) { m_pFrameBuffer = pFrameBuffer; }
    void setDepthImage(IImage* pDepthImage) { m_pDepthImage = pDepthImage; }
    void setDepthImageView(IImageView* pDepthImageView) { m_pDepthImageView = pDepthImageView; }

    IFrameBuffer* getFrameBuffer() { return m_pFrameBuffer; }
    IImage* getDepthImage() { return m_pDepthImage; }
    IImageView* getDepthImageView() { return m_pDepthImageView; }

private:
    glm::vec3 m_Position, m_Direction;
    glm::vec4 m_Color;

    // Volumetric light settings
    float m_ScatterAmount, m_ParticleG;

    // Shadow map resources
    IFrameBuffer* m_pFrameBuffer;
    IImage* m_pDepthImage;
    IImageView* m_pDepthImageView;
};
