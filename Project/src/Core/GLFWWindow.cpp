#include "GLFWWindow.h"

#include "Common/IEventHandler.h"

#include <iostream>

#define GET_WINDOW(pWindow)			((GLFWWindow*)glfwGetWindowUserPointer(pWindow))
#define GET_EVENTHANDLERS(pWindow)	GET_WINDOW(pWindow)->m_ppEventHandlers

bool GLFWWindow::s_HasGLFW = false;

GLFWWindow::GLFWWindow(const std::string& title, uint32_t width, uint32_t height)
	: m_pWindow(nullptr),
	m_ppEventHandlers(),
	m_Width(0),
	m_Height(0),
	m_IsFullscreen(false)
{
	if (!s_HasGLFW)
	{
		if (glfwInit() == GLFW_TRUE)
		{
			s_HasGLFW = true;
		}
	}

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


			int32_t width	= 0;
			int32_t height	= 0;
			glfwGetFramebufferSize(m_pWindow, &width, &height);

			m_Width		= width;
			m_Height	= height;
		}
		else
		{
			LOG("Failed to create window");
		}
	}
}

GLFWWindow::~GLFWWindow()
{
	glfwDestroyWindow(m_pWindow);
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
		if (enable)
		{
			GLFWmonitor* pPrimary = glfwGetPrimaryMonitor();
			const GLFWvidmode* pVidMode = glfwGetVideoMode(pPrimary);
			glfwSetWindowMonitor(m_pWindow, pPrimary, 0, 0, pVidMode->width, pVidMode->height, pVidMode->refreshRate);
		}
		else
		{
			glfwSetWindowMonitor(m_pWindow, nullptr, 0, 0, 0, 0, 0);
		}
	}
}

bool GLFWWindow::getFullscreenState() const
{
	return m_IsFullscreen;
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

void* GLFWWindow::getNativeHandle()
{
	return reinterpret_cast<void*>(m_pWindow);
}

