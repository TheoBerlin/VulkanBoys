#include "Application.h"
#include "IWindow.h"

Application g_Application;

Application::Application()
	: m_pWindow(nullptr),
	m_IsRunning(false)
{
}

void Application::init()
{
	m_pWindow = IWindow::create("Hello Vulkan", 800, 600);
	if (m_pWindow)
	{
		m_pWindow->setEventHandler(this);
	}
}

void Application::run()
{
	m_IsRunning = true;
	
	while (m_IsRunning)
	{
		m_pWindow->peekEvents();

		update();
		render();
	}
}

void Application::release()
{
	SafeDelete(m_pWindow);
}

void Application::OnWindowResize(uint32_t width, uint32_t height)
{
	std::cout << "Resize w=" << width << " h=" << height << std::endl;
}

void Application::OnWindowClose()
{
	std::cout << "Window Closed" << std::endl;
	m_IsRunning = false;
}

Application& Application::getInstance()
{
	return g_Application;
}

void Application::update()
{
}

void Application::render()
{
}

