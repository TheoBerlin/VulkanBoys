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
	~Application();
	
	DECL_NO_COPY(Application);

	void init();
	void run();
	void release();

	IWindow* getWindow() const { return m_pWindow; }

	virtual void onWindowClose() override;
	virtual void onWindowResize(uint32_t width, uint32_t height) override;
	virtual void onWindowFocusChanged(IWindow* pWindow, bool hasFocus) override;
	
	virtual void onMouseMove(uint32_t x, uint32_t y) override;
	virtual void onMousePressed(int32_t button) override;
	virtual void onMouseScroll(double x, double y) override;
	virtual void onMouseReleased(int32_t button) override;

	virtual void onKeyTyped(uint32_t character) override;
	virtual void onKeyPressed(EKey key) override;
	virtual void onKeyReleased(EKey key) override;

	static Application* get();

private:
	//Deltatime should be in milliseconds
	void update(double dt);
	void renderUI(double dt);
	void render(double dt);

private:
	IWindow* m_pWindow;
	IGraphicsContext* m_pContext;
	IRenderer* m_pRenderer;
	IImgui* m_pImgui;
	bool m_IsRunning;

	static Application* s_pInstance;
};