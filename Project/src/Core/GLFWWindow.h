#pragma once
#include "Common/IWindow.h"

#include <GLFW/glfw3.h>

class GLFWWindow : public IWindow
{
public:
	GLFWWindow(const std::string& title, uint32_t width, uint32_t height);
	~GLFWWindow();

	virtual void setEventHandler(IEventHandler* pEventHandler) override;

	virtual void peekEvents() override;
	virtual void show() override;

	virtual void setFullscreen(bool enable) override;

	virtual uint32_t getWidth() override;
	virtual uint32_t getHeight() override;
	virtual void* getNativeHandle() override;

private:
	GLFWwindow* m_pWindow;
	IEventHandler* m_pEventHandler;
	uint32_t m_Width;
	uint32_t m_Height;
	bool m_IsFullscreen;

	static bool s_HasGLFW;
};