#pragma once
#include "Core.h"

class IDescriptorSet;
class IImage;
class IImageView;

class PointLight
{
public:
	PointLight(const glm::vec3& position = glm::vec3(0.0f), const glm::vec4& color = glm::vec4(1.0f), bool castsVolumetricLight = false);
	~PointLight();

	void setColor(const glm::vec4& color);
	void setPosition(const glm::vec3& position);

	const glm::vec4& getColor() const { return m_Color; }
	//Return a new vec3
	glm::vec3 getPosition() const { return m_Position; }

	IImage* getVolumetricLightImage() { return m_pVolumetricLight; }
	void setVolumetricLightImage(IImage* pImage) { m_pVolumetricLight = pImage; }

	IImageView* getVolumetricLightImageView() { return m_pVolumetricLightView; }
	void setVolumetricLightImageView(IImageView* pImageView) { m_pVolumetricLightView = pImageView; }

	IDescriptorSet* getVolumetricLightDescriptorSet() { return m_pDescriptorSet; }
	void setVolumetricLightDescriptorSet(IDescriptorSet* pDescriptorSet) { m_pDescriptorSet = pDescriptorSet; }

	bool castsVolumetricLight() const { return m_CastsVolumetricLight; }
	void toggleVolumetricLight() { m_CastsVolumetricLight = !m_CastsVolumetricLight; }

private:
	glm::vec4 m_Color;
	glm::vec4 m_Position;

	// Volumetric lighting resources
	bool m_CastsVolumetricLight;
	IImage* m_pVolumetricLight;
	IImageView* m_pVolumetricLightView;

	IDescriptorSet* m_pDescriptorSet;
};