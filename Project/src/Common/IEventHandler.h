#pragma once
#include "Core.h"

class IEventHandler
{
public:
	DECL_INTERFACE(IEventHandler);

	virtual void onWindowResize(uint32_t width, uint32_t height) = 0;
	virtual void onWindowClose() = 0;
};