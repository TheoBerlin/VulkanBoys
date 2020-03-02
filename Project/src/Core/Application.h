#pragma once
#include "Common/IEventHandler.h"
#include "Common/IParticleEmitterHandler.h"

#include "Camera.h"
#include "Common/CommonEventHandler.h"

class IGraphicsContext;
class IImgui;
class IInputHandler;
class IMesh;
class IRenderer;
class IRenderingHandler;
class ITexture2D;
class IWindow;

class Application : public CommonEventHandler
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
	
	virtual void onKeyPressed(EKey key) override;
	virtual void onMouseMove(uint32_t x, uint32_t y) override;

	FORCEINLINE IWindow* getWindow() const { return m_pWindow; }

	static Application* get();

private:
	//Deltatime should be in seconds
	void update(double dt);
	void renderUI(double dt);
	void render(double dt);

private:
	Camera m_Camera;
	IWindow* m_pWindow;
	IGraphicsContext* m_pContext;
	IRenderingHandler* m_pRenderingHandler;
	IRenderer* m_pMeshRenderer, *m_pParticleRenderer;
	IRenderer* m_pRayTracingRenderer;
	IImgui* m_pImgui;
	IInputHandler* m_pInputHandler;

	//TODO: Resoures should they be here?
	IMesh* m_pMesh;
	ITexture2D* m_pAlbedo;

	IParticleEmitterHandler* m_pParticleEmitterHandler;
	ITexture2D* m_pParticleTexture;

	// Resources for ImGui Particle window
	size_t m_CurrentEmitterIdx;

	bool m_IsRunning;
	bool m_UpdateCamera;
	bool m_EnableRayTracing;

	static Application* s_pInstance;
};