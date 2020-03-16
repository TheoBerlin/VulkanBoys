#pragma once

#include "Core.h"
#include "Core/PointLight.h"

class IDescriptorSet;
class IImage;
class IImageView;

struct VolumetricLightSettings {
	float m_ScatterAmount, m_ParticleG;
};

class VolumetricPointLight : public PointLight
{
public:
	VolumetricPointLight(const VolumetricLightSettings& volumetricLightSettings, const glm::vec3& position = glm::vec3(0.0f), const glm::vec4& color = glm::vec4(1.0f), float radius = 10.0f);
	~VolumetricPointLight();

	void setRadius(float radius) { m_Radius = radius; };
	float getRadius() const { return m_Radius; }

	float getScatterAmount() const { return m_ScatterAmount; }
	float getParticleG() const { return m_ParticleG; }

	IImage* getVolumetricLightImage() { return m_pVolumetricLightImage; }
	void setVolumetricLightImage(IImage* pImage) { m_pVolumetricLightImage = pImage; }

	IImageView* getVolumetricLightImageView() { return m_pVolumetricLightImageView; }
	void setVolumetricLightImageView(IImageView* pImageView) { m_pVolumetricLightImageView = pImageView; }

	IDescriptorSet* getVolumetricLightDescriptorSet() { return m_pDescriptorSet; }
	void setVolumetricLightDescriptorSet(IDescriptorSet* pDescriptorSet) { m_pDescriptorSet = pDescriptorSet; }

private:
	glm::vec4 m_Color;
	glm::vec4 m_Position;
	float m_Radius;

	// Volumetric lighting resources
	float m_ScatterAmount, m_ParticleG;

	IImage* m_pVolumetricLightImage;
	IImageView* m_pVolumetricLightImageView;

	IDescriptorSet* m_pDescriptorSet;
};
