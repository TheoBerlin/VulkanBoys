#include "GLFWWindow.h"

#include "Common/IEventHandler.h"

#include <iostream>

#define GET_WINDOW(pWindow)			((GLFWWindow*)glfwGetWindowUserPointer(pWindow))
#define GET_EVENTHANDLERS(pWindow)	GET_WINDOW(pWindow)->m_ppEventHandlers

uint32_t GLFWWindow::s_WindowCount = 0;
bool GLFWWindow::s_HasGLFW = false;

static void glfwErrorCallback(int32_t error, const char* pDescription)
{
	LOG("--- GLFW Error(=%d): %s", error, pDescription);
}

GLFWWindow::GLFWWindow(const std::string& title, uint32_t width, uint32_t height)
	: m_pWindow(nullptr),
	m_ppEventHandlers(),
	m_Width(0),
	m_Height(0),
	m_ClientWidth(0),
	m_ClientHeight(0),
	m_OldClientWidth(0),
	m_OldClientHeight(0),
	m_IsFullscreen(false)
{
	if (s_WindowCount < 1)
	{
		if (glfwInit() == GLFW_TRUE)
		{
			s_HasGLFW = true;
			glfwSetErrorCallback(glfwErrorCallback);
		}
	}

	//Increase windowcount
	s_WindowCount++;

	if (s_HasGLFW)
	{
		glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_pWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
		if (m_pWindow)
		{
			//Set this pointer so we can retrive it from the callbacks
			glfwSetWindowUserPointer(m_pWindow, this);

			//Setup callbacks
			glfwSetWindowCloseCallback(m_pWindow, [](GLFWwindow* pWindow)
				{
					for (IEventHandler* pEventHandler : GET_EVENTHANDLERS(pWindow))
					{
						pEventHandler->onWindowClose();
					}
				});

			glfwSetWindowSizeCallback(m_pWindow, [](GLFWwindow* pWindow, int32_t width, int32_t height)
				{
					GET_WINDOW(pWindow)->m_ClientWidth	= width;
					GET_WINDOW(pWindow)->m_ClientHeight	= height;
				});

            glfwSetFramebufferSizeCallback(m_pWindow, [](GLFWwindow* pWindow, int32_t width, int32_t height)
				{
					GET_WINDOW(pWindow)->m_Width	= width;
					GET_WINDOW(pWindow)->m_Height	= height;

					for (IEventHandler* pEventHandler : GET_EVENTHANDLERS(pWindow))
					{
						pEventHandler->onWindowResize(uint32_t(width), uint32_t(height));
					}					
				});

			glfwSetWindowFocusCallback(m_pWindow, [](GLFWwindow* pWindow, int32_t hasFocus) 
				{
					GLFWWindow* pGLFWWindow = GET_WINDOW(pWindow);
					pGLFWWindow->m_HasFocus = (hasFocus == GLFW_TRUE);

					for (IEventHandler* pEventHandler : GET_EVENTHANDLERS(pWindow))
					{
						pEventHandler->onWindowFocusChanged(pGLFWWindow, pGLFWWindow->m_HasFocus);
					}					
				});

			glfwSetKeyCallback(m_pWindow, [](GLFWwindow* pWindow, int32_t key, int32_t scancode, int32_t action, int32_t mods)
			{
				for (IEventHandler* pEventHandler : GET_EVENTHANDLERS(pWindow))
				{
					//TODO: GLFW and EKey have the same integer value so this is valid however, a convertion function maybe should be 
					//considered in the future
					if (action == GLFW_PRESS || action == GLFW_REPEAT)
					{
						pEventHandler->onKeyPressed(EKey(key));
					}
					else if (action == GLFW_RELEASE)
					{
						pEventHandler->onKeyReleased(EKey(key));
					}
				}
			});

			glfwSetCharCallback(m_pWindow, [](GLFWwindow* pWindow, uint32_t codepoint)
				{
					for (IEventHandler* pEventHandler : GET_EVENTHANDLERS(pWindow))
					{
						pEventHandler->onKeyTyped(codepoint);
					}
				});

			glfwSetCursorPosCallback(m_pWindow, [](GLFWwindow* pWindow, double x, double y)
				{
					for (IEventHandler* pEventHandler : GET_EVENTHANDLERS(pWindow))
					{
						pEventHandler->onMouseMove(uint32_t(x), uint32_t(y));
					}
				});

			glfwSetMouseButtonCallback(m_pWindow, [](GLFWwindow* pWindow, int32_t button, int32_t action, int32_t mods)
				{
					for (IEventHandler* pEventHandler : GET_EVENTHANDLERS(pWindow))
					{
						if (action == GLFW_PRESS || action == GLFW_REPEAT)
						{
							pEventHandler->onMousePressed(button);
						}
						else if (action == GLFW_RELEASE)
						{
							pEventHandler->onMouseReleased(button);
						}
					}
				});

			glfwSetScrollCallback(m_pWindow, [](GLFWwindow* pWindow, double x, double y)
				{
					for (IEventHandler* pEventHandler : GET_EVENTHANDLERS(pWindow))
					{
						pEventHandler->onMouseScroll(x, y);
					}
				});

			glfwSetWindowPosCallback(m_pWindow, [](GLFWwindow* pWindow, int32_t x, int32_t y) 
				{
					if (!GET_WINDOW(pWindow)->m_IsFullscreen)
					{
						GET_WINDOW(pWindow)->m_PosX = x;
						GET_WINDOW(pWindow)->m_PosY = y;
					}
				});

			//Init members
			int32_t width	= 0;
			int32_t height	= 0;
			glfwGetFramebufferSize(m_pWindow, &width, &height);

			m_Width		= width;
			m_Height	= height;

			width	= 0;
			height	= 0;
			glfwGetWindowSize(m_pWindow, &width, &height);

			m_ClientWidth	= width;
			m_ClientHeight	= height;

			int32_t x = 0;
			int32_t y = 0;
			glfwGetWindowPos(m_pWindow, &x, &y);

			m_PosX = x;
			m_PosY = y;
		}
		else
		{
			LOG("Failed to create window");
		}
	}
}

GLFWWindow::~GLFWWindow()
{
	if (s_HasGLFW)
	{
		if (m_pWindow)
		{
			glfwDestroyWindow(m_pWindow);
			m_pWindow = nullptr;
		}

		s_WindowCount--;
		if (s_WindowCount < 1)
		{
			glfwTerminate();
			s_HasGLFW = false;
		}
	}
}

void GLFWWindow::addEventHandler(IEventHandler* pEventHandler)
{
	m_ppEventHandlers.emplace_back(pEventHandler);
}

void GLFWWindow::removeEventHandler(IEventHandler* pEventHandler)
{
	for (auto it = m_ppEventHandlers.begin(); it != m_ppEventHandlers.end(); it++)
	{
		if (*it == pEventHandler)
		{
			m_ppEventHandlers.erase(it);
			break;
		}
	}
}

void GLFWWindow::peekEvents()
{
	glfwPollEvents();
}

void GLFWWindow::show()
{
	glfwShowWindow(m_pWindow);
}

void GLFWWindow::setFullscreenState(bool enable)
{
	if (m_IsFullscreen != enable)
	{
		m_IsFullscreen = enable;

		if (enable)
		{
			m_OldClientWidth	= m_ClientWidth;
			m_OldClientHeight	= m_ClientHeight;

			GLFWmonitor* pPrimary = glfwGetPrimaryMonitor();
			const GLFWvidmode* pVidMode = glfwGetVideoMode(pPrimary);
			glfwSetWindowMonitor(m_pWindow, pPrimary, 0, 0, pVidMode->width, pVidMode->height, pVidMode->refreshRate);
		}
		else
		{
			glfwSetWindowMonitor(m_pWindow, nullptr, m_PosX, m_PosY, m_OldClientWidth, m_OldClientHeight, 0);

			m_OldClientWidth	= 0;
			m_OldClientHeight	= 0;
		}
	}
}

bool GLFWWindow::getFullscreenState() const
{
	return m_IsFullscreen;
}

void GLFWWindow::toggleFullscreenState()
{
	setFullscreenState(!m_IsFullscreen);
}

bool GLFWWindow::hasFocus() const
{
	return m_HasFocus;
}

uint32_t GLFWWindow::getWidth()
{
	return m_Width;
}

uint32_t GLFWWindow::getHeight()
{
	return m_Height;
}

uint32_t GLFWWindow::getClientWidth()
{
	return m_ClientWidth;
}

uint32_t GLFWWindow::getClientHeight()
{
	return m_ClientHeight;
}

float GLFWWindow::getScaleX()
{
	return float(m_Width) / float(m_ClientWidth);
}

float GLFWWindow::getScaleY()
{
	return float(m_Height) / float(m_ClientHeight);
}

void* GLFWWindow::getNativeHandle()
{
	return reinterpret_cast<void*>(m_pWindow);
}

