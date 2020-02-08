#pragma once
#include "Core/Core.h"

#include <string>

class IEventHandler;

class IWindow
{
public:
	DECL_INTERFACE(IWindow);

	virtual void setEventHandler(IEventHandler* pEventHandler) = 0;

	virtual void peekEvents() = 0;
	virtual void show() = 0;

	virtual void setFullscreenState(bool enable) = 0;
	virtual bool getFullscreenState() const = 0;

	virtual bool hasFocus() const = 0;

	virtual uint32_t getWidth() = 0;
	virtual uint32_t getHeight() = 0;
	virtual void* getNativeHandle() = 0;

	static IWindow* create(const std::string& title, uint32_t width, uint32_t height);
};