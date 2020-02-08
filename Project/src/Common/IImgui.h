#pragma once
#include "IEventHandler.h"

//Make API independent
class CommandBufferVK;

class IImgui : public IEventHandler
{
public:
	DECL_INTERFACE(IImgui);

	virtual bool init(uint32_t width, uint32_t height) = 0;
	virtual void render(CommandBufferVK* pCommandBuffer) = 0;
};