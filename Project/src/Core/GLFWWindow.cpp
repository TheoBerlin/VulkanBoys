#include "GLFWWindow.h"
#include "IEventHandler.h"

#include <iostream>

#define GET_EVENTHANDLER(pWindow) ((GLFWWindow*)glfwGetWindowUserPointer(pWindow))->m_pEventHandler


bool GLFWWindow::s_HasGLFW = false;

GLFWWindow::GLFWWindow(const std::string& title, uint32_t width, uint32_t height)
	: m_pWindow(nullptr),
	m_pEventHandler(nullptr)
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
					IEventHandler* pEventHandler = GET_EVENTHANDLER(pWindow);
					pEventHandler->OnWindowClose();
				});

			glfwSetWindowSizeCallback(m_pWindow, [](GLFWwindow* pWindow, int width, int height)
				{
					IEventHandler* pEventHandler = GET_EVENTHANDLER(pWindow);
					pEventHandler->OnWindowResize(uint32_t(width), uint32_t(height));
				});
		}
		else
		{
			std::cerr << "Failed to create window" << std::endl;
		}
	}
}

GLFWWindow::~GLFWWindow()
{
	glfwDestroyWindow(m_pWindow);
}

void GLFWWindow::setEventHandler(IEventHandler* pEventHandler)
{
	m_pEventHandler = pEventHandler;
}

void GLFWWindow::peekEvents()
{
	glfwPollEvents();
}

void GLFWWindow::show()
{
	glfwShowWindow(m_pWindow);
}

uint32_t GLFWWindow::getWidth()
{
	int width = 0;
	int height = 0;
	glfwGetWindowSize(m_pWindow, &width, &height);
	return uint32_t(width);
}

uint32_t GLFWWindow::getHeight()
{
	int width = 0;
	int height = 0;
	glfwGetWindowSize(m_pWindow, &width, &height);
	return uint32_t(height);
}

void* GLFWWindow::getNativeHandle()
{
	return reinterpret_cast<void*>(m_pWindow);
}
