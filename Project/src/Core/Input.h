#pragma once
#include "Core/Core.h"

#include "Common/Keycodes.h"

class IInputHandler;

class Input
{
public:
	DECL_STATIC_CLASS(Input);

	static void setInputHandler(IInputHandler* pInputHandler);

	static bool isKeyPressed(EKey keycode);
	static bool isKeyReleased(EKey keycode);

private:
	static IInputHandler* s_pInputHandler;
};

