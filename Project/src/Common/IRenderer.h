#pragma once
#include "Core/Core.h"

#include <glm/glm.hpp>

class IImgui;
class IMesh;
class Camera;

class IRenderer
{
public:
	DECL_INTERFACE(IRenderer);

	virtual bool init() = 0;

	virtual void beginFrame(const Camera& camera) = 0;
	virtual void endFrame() = 0;
};
