#pragma once
#include "CommonEventHandler.h"

//Make API independent
class CommandBufferVK;

class IImgui : public CommonEventHandler
{
public:
	DECL_INTERFACE(IImgui);

	virtual bool init() = 0;

	virtual void begin(double deltatime) = 0;
	virtual void end() = 0;
	
	virtual void render(CommandBufferVK* pCommandBuffer, uint32_t currentFrame) = 0;
};