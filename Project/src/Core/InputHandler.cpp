#include "InputHandler.h"

InputHandler::InputHandler()
	: m_KeyStates()
{
	memset(m_KeyStates, 0, sizeof(m_KeyStates));
}

bool InputHandler::isKeyReleased(EKey keycode)
{
	return (m_KeyStates[keycode] == false);
}

bool InputHandler::isKeyPressed(EKey keycode)
{
	return (m_KeyStates[keycode] == true);
}

void InputHandler::onKeyPressed(EKey keycode)
{
	m_KeyStates[keycode] = true;
}

void InputHandler::onKeyReleased(EKey keycode)
{
	m_KeyStates[keycode] = false;
}
