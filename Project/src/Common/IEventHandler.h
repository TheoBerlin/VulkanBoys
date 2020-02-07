#pragma once
#include "Core.h"

class IEventHandler
{
public:
	DECL_INTERFACE(IEventHandler);

	virtual void OnWindowResize(uint32_t width, uint32_t height) = 0;
	virtual void OnWindowClose() = 0;
};