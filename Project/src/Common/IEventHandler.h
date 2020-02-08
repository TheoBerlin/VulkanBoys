#pragma once
#include "Core.h"

class IWindow;

class IEventHandler
{
public:
	DECL_INTERFACE(IEventHandler);

	virtual void onWindowClose() = 0;
	virtual void onWindowResize(uint32_t width, uint32_t height) = 0;
	virtual void onWindowFocusChanged(IWindow* pWindow, bool hasFocus) = 0;
};