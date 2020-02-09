#pragma once
#include "CommonEventHandler.h"

class IInputHandler : public CommonEventHandler
{
public:
	DECL_INTERFACE(IInputHandler);

	virtual bool isKeyReleased(EKey keycode) = 0;
	virtual bool isKeyPressed(EKey keycode) = 0;

	static IInputHandler* create();
};