#pragma once
#include "IEventHandler.h"

//Make API independent
class CommandBufferVK;

class IImgui : public IEventHandler
{
public:
	DECL_INTERFACE(IImgui);

	virtual bool init() = 0;

	virtual void begin(double deltatime) = 0;
	virtual void end() = 0;
	
	virtual void render(CommandBufferVK* pCommandBuffer) = 0;
};