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

		// API profiler results
		float FrameTimeSumMeshRenderer = 0.0f;
		float FrameTimeSumShadowmapRenderer = 0.0f;
		float FrameTimeSumParticleRenderer = 0.0f;
		float FrameTimeSumRayTracer = 0.0f;
		float FrameTimeSumVolumetricLighting = 0.0f;

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

	FORCEINLINE IWindow* getWindow() const { return m_pWindow; }
	FORCEINLINE IScene*	 getScene() const  { return m_pScene; }

	static Application* get();

private:
	void update(double dt);
	void renderUI(double dt);
	void render(double dt);

	void testFinished();
	void sanitizeString(char string[], uint32_t numCharacters);

private:
	Camera					m_Camera;
	Material				m_GunMaterial;
	ApplicationParameters	m_ApplicationParameters;
	TestParameters			m_TestParameters;

	IWindow*			m_pWindow;
	IGraphicsContext*	m_pContext;
	RenderingHandler*	m_pRenderingHandler;
	IRenderer*			m_pMeshRenderer;
	IRenderer*			m_pShadowMapRenderer;
	IRenderer*			m_pParticleRenderer;
	IRenderer*			m_pRayTracingRenderer;
	IRenderer* 			m_pVolumetricLightRenderer;
	IImgui*				m_pImgui;
	IInputHandler*		m_pInputHandler;

	IScene* m_pScene;

	IMesh*		m_pGunMesh;
	ITexture2D* m_pGunAlbedo;
	ITexture2D* m_pGunNormal;
	ITexture2D* m_pGunMetallic;
	ITexture2D* m_pGunRoughness;
	ITexture2D* m_pParticleTexture;

	ITextureCube* m_pSkybox;

	ParticleEmitterHandler* m_pParticleEmitterHandler;

	// Resources for ImGui Particle window
	size_t m_CurrentEmitterIdx;
	ParticleEmitterInfo m_NewEmitterInfo;

	LoopingUniformCRSpline<glm::vec3, float>* m_pCameraPositionSpline;
	LoopingUniformCRSpline<glm::vec3, float>* m_pCameraDirectionSpline;
	float m_CameraSplineTimer;

	uint32_t m_GraphicsIndex0;
	uint32_t m_GraphicsIndex1;
	uint32_t m_GraphicsIndex2;

	bool m_IsRunning;
	bool m_UpdateCamera;
	bool m_KeyInputEnabled;
	bool m_CameraSplineEnabled;
	bool m_CreatingEmitter;

	static constexpr uint32_t SPHERE_COUNT_DIMENSION = 8;
	static Application* s_pInstance;
};