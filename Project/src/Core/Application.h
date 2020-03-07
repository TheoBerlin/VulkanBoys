#pragma once
#include "Camera.h"
#include "Material.h"
#include "LightSetup.h"

#include "Common/CommonEventHandler.h"
#include "Common/ParticleEmitterHandler.h"

class IMesh;
class IImgui;
class IWindow;
class IRenderer;
class ITexture2D;
class ITextureCube;
class IInputHandler;
class IGraphicsContext;
class RenderingHandler;

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
	void update(double dt);
	void renderUI(double dt);
	void render(double dt);

private:
	Camera m_Camera;
	LightSetup m_LightSetup;
	IWindow* m_pWindow;
	IGraphicsContext* m_pContext;
	RenderingHandler* m_pRenderingHandler;
	IRenderer* m_pMeshRenderer;
	IRenderer* m_pParticleRenderer;
	IRenderer* m_pRayTracingRenderer;
	IImgui* m_pImgui;
	IInputHandler* m_pInputHandler;

	//TODO: Resoures should they be here?
	IMesh* m_pMesh;
	IMesh* m_pSphere;
	ITexture2D* m_pAlbedo;
	ITexture2D* m_pNormal;
	ITexture2D* m_pMetallic;
	ITexture2D* m_pRoughness;
	ITextureCube* m_pSkybox;
	Material m_GunMaterial;
	Material m_RedMaterial;

	ParticleEmitterHandler* m_pParticleEmitterHandler;
	ITexture2D* m_pParticleTexture;

	// Resources for ImGui Particle window
	size_t m_CurrentEmitterIdx;
	bool m_CreatingEmitter;
	ParticleEmitterInfo m_NewEmitterInfo;

	bool m_IsRunning;
	bool m_UpdateCamera;
	bool m_EnableRayTracing;

	static Application* s_pInstance;
};