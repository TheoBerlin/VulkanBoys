#include "IInputHandler.h"

#include "Core/InputHandler.h"

IInputHandler* IInputHandler::create()
{
	return new InputHandler();
}
