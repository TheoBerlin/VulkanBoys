#include "InputHandler.h"

InputHandler::InputHandler()
	: m_KeyStates()
{
	m_KeyStates.resize(EKey::KEY_COUNT, false);
}

bool InputHandler::isKeyReleased(EKey keycode)
{
	if (keycode == EKey::KEY_UNKNOWN)
	{
		return false;
	}
	else
	{
		return (m_KeyStates[keycode] == false);
	}
}

bool InputHandler::isKeyPressed(EKey keycode)
{
	if (keycode == EKey::KEY_UNKNOWN)
	{
		return false;
	}
	else
	{
		return (m_KeyStates[keycode] == true);
	}
}

void InputHandler::onKeyPressed(EKey keycode)
{
	if (keycode != EKey::KEY_UNKNOWN)
	{
		ASSERT(keycode < EKey::KEY_COUNT);
		m_KeyStates[keycode] = true;
	}
}

void InputHandler::onKeyReleased(EKey keycode)
{
	if (keycode != EKey::KEY_UNKNOWN)
	{
		ASSERT(keycode < EKey::KEY_COUNT);
		m_KeyStates[keycode] = false;
	}
}