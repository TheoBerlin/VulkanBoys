#include "IWindow.h"
#include "Core/GLFWWindow.h"

IWindow* IWindow::create(const std::string& title, uint32_t width, uint32_t height)
{
	return DBG_NEW GLFWWindow(title, width, height);
}