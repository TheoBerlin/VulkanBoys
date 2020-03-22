#pragma once

#include "Core.h"
#include "Core/PointLight.h"

class IBuffer;
class IDescriptorSet;
class IImage;
class IImageView;

struct VolumetricLightSettings {
	float m_ScatterAmount, m_ParticleG;
};

struct VolumetricPointLightBuffer {
	glm::mat4 worldMatrix;
	glm::vec4 position, color;
	float radius, scatterAmount, particleG;
};

class VolumetricPointLight : public PointLight
{
public:
	VolumetricPointLight(const VolumetricLightSettings& volumetricLightSettings, const glm::vec3& position = glm::vec3(0.0f), const glm::vec4& color = glm::vec4(1.0f), float radius = 10.0f);
	~VolumetricPointLight();

	float getRadius() const { return m_Radius; }
	void setRadius(float radius);

	float getScatterAmount() const { return m_ScatterAmount; }
	float getParticleG() const { return m_ParticleG; }
	void setScatterAmount(float scatterAmount);
	void setParticleG(float particleG);

	void createVolumetricPointLightBuffer(VolumetricPointLightBuffer& buffer);
	IBuffer* getVolumetricLightBuffer() { return m_pVolumetricLightBuffer; }
	void setVolumetricLightBuffer(IBuffer* pBuffer) { m_pVolumetricLightBuffer = pBuffer; }

	IImage* getVolumetricLightImage() { return m_pVolumetricLightImage; }
	void setVolumetricLightImage(IImage* pImage) { m_pVolumetricLightImage = pImage; }

	IImageView* getVolumetricLightImageView() { return m_pVolumetricLightImageView; }
	void setVolumetricLightImageView(IImageView* pImageView) { m_pVolumetricLightImageView = pImageView; }

	IDescriptorSet* getVolumetricLightDescriptorSet() { return m_pDescriptorSet; }
	void setVolumetricLightDescriptorSet(IDescriptorSet* pDescriptorSet) { m_pDescriptorSet = pDescriptorSet; }

	// Whether or not the light's settings have been changed during the current frame
    bool m_LightUpdated;

private:
	float m_Radius;

	// Volumetric lighting resources
	float m_ScatterAmount, m_ParticleG;

	IBuffer* m_pVolumetricLightBuffer;

	IImage* m_pVolumetricLightImage;
	IImageView* m_pVolumetricLightImageView;

	IDescriptorSet* m_pDescriptorSet;
};
