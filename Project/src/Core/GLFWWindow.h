#pragma once
#include "Common/IWindow.h"

#include <GLFW/glfw3.h>
#include <vector>

class GLFWWindow : public IWindow
{
public:
	GLFWWindow(const std::string& title, uint32_t width, uint32_t height);
	~GLFWWindow();

	virtual void addEventHandler(IEventHandler* pEventHandler) override;
	virtual void removeEventHandler(IEventHandler* pEventHandler) override;

	virtual void peekEvents() override;
	virtual void show() override;

	virtual void setFullscreenState(bool enable) override;
	virtual bool getFullscreenState() const override;
	virtual void toggleFullscreenState() override;
	
	virtual bool hasFocus() const override;

	virtual uint32_t getWidth() override;
	virtual uint32_t getHeight() override;
	virtual uint32_t getClientWidth() override;
	virtual uint32_t getClientHeight() override;
	virtual float getScaleX() override;
	virtual float getScaleY() override;
	virtual void* getNativeHandle() override;

private:
	GLFWwindow* m_pWindow;
	std::vector<IEventHandler*> m_ppEventHandlers;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_ClientWidth;
	uint32_t m_ClientHeight;
	uint32_t m_OldClientWidth;
	uint32_t m_OldClientHeight;
	uint32_t m_PosX;
	uint32_t m_PosY;
	bool m_IsFullscreen;
	bool m_HasFocus;

	static uint32_t s_WindowCount;
	static bool s_HasGLFW;
};