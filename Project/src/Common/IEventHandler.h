#pragma once
#include "Core/Core.h"

class IWindow;

class IEventHandler
{
public:
	DECL_INTERFACE(IEventHandler);

	virtual void onWindowClose() = 0;
	virtual void onWindowResize(uint32_t width, uint32_t height) = 0;
	virtual void onWindowFocusChanged(IWindow* pWindow, bool hasFocus) = 0;

	virtual void onMouseMove(uint32_t x, uint32_t y) = 0;
	virtual void onMousePressed(int32_t button) = 0;
	virtual void onMouseScroll(double x, double y) = 0;
	virtual void onMouseReleased(int32_t button) = 0;

	virtual void onKeyTyped(uint32_t character) = 0;
	virtual void onKeyPressed(int32_t key) = 0;
	virtual void onKeyReleased(int32_t key) = 0;
};