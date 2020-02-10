#pragma once
#include "Core/Core.h"

#include "Common/Keycodes.h"

class IWindow;
class IInputHandler;

class Input
{
public:
	DECL_STATIC_CLASS(Input);

	static void setInputHandler(IInputHandler* pInputHandler);

	static bool isKeyPressed(EKey keycode);
	static bool isKeyReleased(EKey keycode);

	static void setMousePosition(IWindow* pWindow, const glm::vec2& position);

private:
	static IInputHandler* s_pInputHandler;
};

