#include "Input.h"

#include "Common/IInputHandler.h"

IInputHandler* Input::s_pInputHandler = nullptr;

void Input::setInputHandler(IInputHandler* pInputHandler)
{
	s_pInputHandler = pInputHandler;
}

bool Input::isKeyPressed(EKey keycode)
{
	ASSERT(s_pInputHandler != nullptr);
	return s_pInputHandler->isKeyPressed(keycode);
}

bool Input::isKeyReleased(EKey keycode)
{
	ASSERT(s_pInputHandler != nullptr);
	return s_pInputHandler->isKeyReleased(keycode);
}
