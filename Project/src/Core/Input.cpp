#include "Input.h"

#include "Common/IInputHandler.h"

#include "Core/GLFWWindow.h"

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

void Input::setMousePosition(IWindow* pWindow, const glm::vec2& position)
{
	ASSERT(pWindow != nullptr);
	
	GLFWwindow* pNativeWindow = (GLFWwindow*)pWindow->getNativeHandle();
	glfwSetCursorPos(pNativeWindow, position.x, position.y);
}

void Input::captureMouse(IWindow* pWindow)
{
    GLFWwindow* pNativeWindow = (GLFWwindow*)pWindow->getNativeHandle();
    
    glfwSetInputMode(pNativeWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(pNativeWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
}

void Input::releaseMouse(IWindow* pWindow)
{
    GLFWwindow* pNativeWindow = (GLFWwindow*)pWindow->getNativeHandle();
    glfwSetInputMode(pNativeWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}
