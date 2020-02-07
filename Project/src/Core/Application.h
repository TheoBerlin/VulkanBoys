#pragma once
#include "Common/IEventHandler.h"

class IWindow;
class IContext;
class IRenderer;

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

	virtual void OnWindowResize(uint32_t width, uint32_t height) override;
	virtual void OnWindowClose() override;

	static Application& getInstance();
private:
	void update();
	void render();

private:
	IWindow* m_pWindow;
	IContext* m_pIContext;
	IRenderer* m_pRenderer;
	bool m_IsRunning;
};

extern Application g_Application;