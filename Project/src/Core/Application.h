#pragma once
#include "Camera.h"
#include "Material.h"
#include "LightSetup.h"

#include "Common/IEventHandler.h"

class IMesh;
class IImgui;
class IWindow;
class IRenderer;
class ITexture2D;
class IInputHandler;
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

	IWindow* getWindow() const { return m_pWindow; }

	static Application* get();

private:
	//Deltatime should be in seconds
	void update(double dt);
	void renderUI(double dt);
	void render(double dt);

private:
	Camera m_Camera;
	LightSetup m_LightSetup;
	IWindow* m_pWindow;
	IGraphicsContext* m_pContext;
	IRenderer* m_pRenderer;
	IImgui* m_pImgui;
	IInputHandler* m_pInputHandler;

	//TODO: Resoures should they be here?
	IMesh* m_pMesh;
	IMesh* m_pSphere;
	ITexture2D* m_pAlbedo;
	ITexture2D* m_pNormal;
	Material m_GunMaterial;
	Material m_RedMaterial;

	bool m_IsRunning;
	bool m_UpdateCamera;

	static Application* s_pInstance;
};