#pragma once

#include "../Renderer.h"

#include <SDL.h>
#include <GL/glew.h>
//#include <SDL_opengl.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2main.lib")



class OpenGLRenderer : public Renderer
{
public:
	OpenGLRenderer();
	~OpenGLRenderer();

	Material* makeMaterial(const std::string& name);
	Mesh* makeMesh();
	//VertexBuffer* makeVertexBuffer();
	VertexBuffer* makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage);
	ConstantBuffer* makeConstantBuffer(std::string NAME, unsigned int location);
//	ResourceBinding* makeResourceBinding();
	RenderState* makeRenderState();
	Technique* makeTechnique(Material* m, RenderState* r);
	Texture2D* makeTexture2D();
	Sampler2D* makeSampler2D();
	std::string getShaderPath();
	std::string getShaderExtension();

	int initialize(unsigned int width = 640, unsigned int height = 480);
	void setWinTitle(const char* title);
	int shutdown();

	void setClearColor(float, float, float, float);
	void clearBuffer(unsigned int);
//	void setRenderTarget(RenderTarget* rt); // complete parameters
	void setRenderState(RenderState* ps);
	void submit(Mesh* mesh);
	void frame();
	void present();

private:
	SDL_Window* window;
	SDL_GLContext context;

	std::vector<Mesh*> drawList;
	std::unordered_map<Technique*, std::vector<Mesh*>> drawList2;
	
	bool globalWireframeMode = false;

	//int initializeOpenGL(int major, int minor, unsigned int width, unsigned int height);
	float clearColor[4] = { 0,0,0,0 };
	std::unordered_map<int, int> BUFFER_MAP = { 
		{0, 0},
		{CLEAR_BUFFER_FLAGS::COLOR, GL_COLOR_BUFFER_BIT },
		{CLEAR_BUFFER_FLAGS::DEPTH, GL_DEPTH_BUFFER_BIT }, 
		{CLEAR_BUFFER_FLAGS::STENCIL, GL_STENCIL_BUFFER_BIT }
	};
};

