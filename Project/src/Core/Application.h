#pragma once
#include "Camera.h"
#include "Material.h"

#include "Common/CommonEventHandler.h"
#include "Common/ParticleEmitterHandler.h"

#include <spline_library/splines/uniform_cr_spline.h>

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
	struct ApplicationParameters
	{
		bool IsDirty = false;

		int RayTracingResolutionDenominator = 1;
	};

	struct TestParameters
	{
		bool Running = false;
		int CurrentRound = 0;
		float FrameTimeSum = 0.0f;
		float FrameCount = 0.0f;
		float AverageFrametime = 0.0f;
		float BestFrametime = 0.0f;
		float WorstFrametime = 0.0f;

		char TestName[256] = "";
		int NumRounds = 1;
	};

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

	FORCEINLINE IWindow*	getWindow() const { return m_pWindow; }
	FORCEINLINE IScene*		getScene() const  { return m_pScene; }
	
	static Application* get();

private:
	void update(double dt);
	void renderUI(double dt);
	void render(double dt);

	void testFinished();
	void sanitizeString(char string[], uint32_t numCharacters);

private:
	Camera m_Camera;
	IWindow* m_pWindow;
	IGraphicsContext* m_pContext;
	RenderingHandler* m_pRenderingHandler;
	IRenderer* m_pMeshRenderer;
	IRenderer* m_pParticleRenderer;
	IRenderer* m_pRayTracingRenderer;
	IImgui* m_pImgui;
	IInputHandler* m_pInputHandler;

	IScene* m_pScene;

	//TODO: Resoures should they be here?
	static constexpr uint32_t SPHERE_COUNT_DIMENSION = 8;

	IMesh* m_pGunMesh;
	ITexture2D* m_pGunAlbedo;
	ITexture2D* m_pGunNormal;
	ITexture2D* m_pGunMetallic;
	ITexture2D* m_pGunRoughness;
	Material m_GunMaterial;

	ITextureCube* m_pSkybox;

	ParticleEmitterHandler* m_pParticleEmitterHandler;
	ITexture2D* m_pParticleTexture;

	// Resources for ImGui Particle window
	size_t m_CurrentEmitterIdx;
	bool m_CreatingEmitter;
	ParticleEmitterInfo m_NewEmitterInfo;

	bool m_IsRunning;
	bool m_UpdateCamera;

	LoopingUniformCRSpline<glm::vec3, float>* m_pCameraPositionSpline;
	LoopingUniformCRSpline<glm::vec3, float>* m_pCameraDirectionSpline;
	float m_CameraSplineTimer;
	bool m_CameraSplineEnabled;

	ApplicationParameters m_ApplicationParameters;
	TestParameters m_TestParameters;
	bool m_KeyInputEnabled;

	uint32_t m_GraphicsIndex0;
	uint32_t m_GraphicsIndex1;
	uint32_t m_GraphicsIndex2;

	static Application* s_pInstance;
};