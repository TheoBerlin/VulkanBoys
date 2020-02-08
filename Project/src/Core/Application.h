#pragma once
#include "Common/IEventHandler.h"

class IImgui;
class IWindow;
class IRenderer;
class IGraphicsContext;

class Application : public IEventHandler
{
public:
	Application();
	~Application() = default;
	
	DECL_NO_COPY(Application);

	void init();
	void run();
	void release();

	IWindow* getWindow() const { return m_pWindow; }

	virtual void onWindowClose() override;
	virtual void onWindowResize(uint32_t width, uint32_t height) override;
	virtual void onWindowFocusChanged(IWindow* pWindow, bool hasFocus) override;

	static Application& getInstance();

private:
	void update();
	void renderUI();
	void render();

private:
	IWindow* m_pWindow;
	IGraphicsContext* m_pContext;
	IRenderer* m_pRenderer;
	IImgui* m_pImgui;
	bool m_IsRunning;
};

extern Application g_Application;