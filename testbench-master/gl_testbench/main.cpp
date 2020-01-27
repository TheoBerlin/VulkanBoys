#include <string>
#include <SDL_keyboard.h>
#include <SDL_events.h>
#include <SDL_timer.h>
#include <type_traits> 
#include <assert.h>

#include "Renderer.h"
#include "Mesh.h"
#include "Texture2D.h"
#include <math.h>

using namespace std;
Renderer* renderer;

// flat scene at the application level...we don't care about this here.
// do what ever you want in your renderer backend.
// all these objects are loosely coupled, creation and destruction is responsibility
// of the testbench, not of the container objects
vector<Mesh*> scene;
vector<Material*> materials;
vector<Technique*> techniques;
vector<Texture2D*> textures;
vector<Sampler2D*> samplers;

VertexBuffer* pos;
VertexBuffer* nor;
VertexBuffer* uvs;

// forward decls
void updateScene();
void renderScene();

char gTitleBuff[256];
double gLastDelta = 0.0;

void updateDelta()
{
	#define WINDOW_SIZE 10
	static Uint64 start = 0;
	static Uint64 last = 0;
	static double avg[WINDOW_SIZE] = { 0.0 };
	static double lastSum = 10.0;
	static int loop = 0;

	last = start;
	start = SDL_GetPerformanceCounter();
	double deltaTime = (double)((start - last) * 1000.0 / SDL_GetPerformanceFrequency());
	// moving average window of WINDOWS_SIZE
	lastSum -= avg[loop];
	lastSum += deltaTime;
	avg[loop] = deltaTime;
	loop = (loop + 1) % WINDOW_SIZE;
	gLastDelta = (lastSum / WINDOW_SIZE);
};

// TOTAL_TRIS pretty much decides how many drawcalls in a brute force approach.
constexpr int TOTAL_TRIS = 100.0f;
// this has to do with how the triangles are spread in the screen, not important.
constexpr int TOTAL_PLACES = 2 * TOTAL_TRIS;
float xt[TOTAL_PLACES], yt[TOTAL_PLACES];

// lissajous points
typedef union { 
	struct { float x, y, z, w; };
	struct { float r, g, b, a; };
} float4;

typedef union { 
	struct { float x, y; };
	struct { float u, v; };
} float2;


void run() {

	SDL_Event windowEvent;
	while (true)
	{
		if (SDL_PollEvent(&windowEvent))
		{
			if (windowEvent.type == SDL_QUIT) break;
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE) break;
		}
		updateScene();
		renderScene();
	}
}

/*
 update positions of triangles in the screen changing a translation only
*/
void updateScene()
{
	/*
	    For each mesh in scene list, update their position 
	*/
	{
		static long long shift = 0;
		const int size = scene.size();
		for (int i = 0; i < size; i++)
		{
			const float4 trans { 
				xt[(int)(float)(i + shift) % (TOTAL_PLACES)], 
				yt[(int)(float)(i + shift) % (TOTAL_PLACES)], 
				i * (-1.0 / TOTAL_PLACES),
				0.0
			};
			scene[i]->txBuffer->setData(&trans, sizeof(trans), scene[i]->technique->getMaterial(), TRANSLATION);
		}
		// just to make them move...
		shift+=max(TOTAL_TRIS / 1000.0,TOTAL_TRIS / 100.0);
	}
	return;
};


void renderScene()
{
	renderer->clearBuffer(CLEAR_BUFFER_FLAGS::COLOR | CLEAR_BUFFER_FLAGS::DEPTH);
	for (auto m : scene)
	{
		renderer->submit(m);
	}
	renderer->frame();
	renderer->present();
	updateDelta();
	sprintf(gTitleBuff, "OpenGL - %3.0lf", gLastDelta);
	renderer->setWinTitle(gTitleBuff);
}

int initialiseTestbench()
{
	std::string definePos = "#define POSITION " + std::to_string(POSITION) + "\n";
	std::string defineNor = "#define NORMAL " + std::to_string(NORMAL) + "\n";
	std::string defineUV = "#define TEXTCOORD " + std::to_string(TEXTCOORD) + "\n";

	std::string defineTX = "#define TRANSLATION " + std::to_string(TRANSLATION) + "\n";
	std::string defineTXName = "#define TRANSLATION_NAME " + std::string(TRANSLATION_NAME) + "\n";
	
	std::string defineDiffCol = "#define DIFFUSE_TINT " + std::to_string(DIFFUSE_TINT) + "\n";
	std::string defineDiffColName = "#define DIFFUSE_TINT_NAME " + std::string(DIFFUSE_TINT_NAME) + "\n";

	std::string defineDiffuse = "#define DIFFUSE_SLOT " + std::to_string(DIFFUSE_SLOT) + "\n";

	std::vector<std::vector<std::string>> materialDefs = {
		// vertex shader, fragment shader, defines
		// shader filename extension must be asked to the renderer
		// these strings should be constructed from the IA.h file!!!

		{ "VertexShader", "FragmentShader", definePos + defineNor + defineUV + defineTX + 
		   defineTXName + defineDiffCol + defineDiffColName }, 

		{ "VertexShader", "FragmentShader", definePos + defineNor + defineUV + defineTX + 
		   defineTXName + defineDiffCol + defineDiffColName }, 

		{ "VertexShader", "FragmentShader", definePos + defineNor + defineUV + defineTX + 
		   defineTXName + defineDiffCol + defineDiffColName + defineDiffuse	},

		{ "VertexShader", "FragmentShader", definePos + defineNor + defineUV + defineTX + 
		   defineTXName + defineDiffCol + defineDiffColName }, 
	};

	float degToRad = M_PI / 180.0;
	float scale = (float)TOTAL_PLACES / 359.9;
	for (int a = 0; a < TOTAL_PLACES; a++)
	{
		xt[a] = 0.8f * cosf(degToRad * ((float)a/scale) * 3.0);
		yt[a] = 0.8f * sinf(degToRad * ((float)a/scale) * 2.0);
	};

	// triangle geometry:
	float4 triPos[3] = { { 0.0f,  0.05, 0.0f, 1.0f },{ 0.05, -0.05, 0.0f, 1.0f },{ -0.05, -0.05, 0.0f, 1.0f } };
	float4 triNor[3] = { { 0.0f,  0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 0.0f } };
	float2 triUV[3] =  { { 0.5f,  -0.99f },{ 1.49f, 1.1f },{ -0.51, 1.1f } };

	// load Materials.
	std::string shaderPath = renderer->getShaderPath();
	std::string shaderExtension = renderer->getShaderExtension();
	float diffuse[4][4] = {
		0.0,0.0,1.0,1.0,
		0.0,1.0,0.0,1.0,
		1.0,1.0,1.0,1.0,
		1.0,0.0,0.0,1.0
	};

	for (int i = 0; i < materialDefs.size(); i++)
	{
		// set material name from text file?
		Material* m = renderer->makeMaterial("material_" + std::to_string(i));
		m->setShader(shaderPath + materialDefs[i][0] + shaderExtension, Material::ShaderType::VS);
		m->setShader(shaderPath + materialDefs[i][1] + shaderExtension, Material::ShaderType::PS);

		m->addDefine(materialDefs[i][2], Material::ShaderType::VS);
		m->addDefine(materialDefs[i][2], Material::ShaderType::PS);

		std::string err;
		m->compileMaterial(err);

		// add a constant buffer to the material, to tint every triangle using this material
		m->addConstantBuffer(DIFFUSE_TINT_NAME, DIFFUSE_TINT);
		// no need to update anymore
		// when material is bound, this buffer should be also bound for access.

		m->updateConstantBuffer(diffuse[i], 4 * sizeof(float), DIFFUSE_TINT);
		
		materials.push_back(m);
	}

	// one technique with wireframe
	RenderState* renderState1 = renderer->makeRenderState();
	renderState1->setWireFrame(true);

	// basic technique
	techniques.push_back(renderer->makeTechnique(materials[0], renderState1));
	techniques.push_back(renderer->makeTechnique(materials[1], renderer->makeRenderState()));
	techniques.push_back(renderer->makeTechnique(materials[2], renderer->makeRenderState()));
	techniques.push_back(renderer->makeTechnique(materials[3], renderer->makeRenderState()));

	// create texture
	Texture2D* fatboy = renderer->makeTexture2D();
	fatboy->loadFromFile("../assets/textures/fatboy.png");
	Sampler2D* sampler = renderer->makeSampler2D();
	sampler->setWrap(WRAPPING::REPEAT, WRAPPING::REPEAT);
	fatboy->sampler = sampler;

	textures.push_back(fatboy);
	samplers.push_back(sampler);

	// pre-allocate one single vertex buffer for ALL triangles
	pos = renderer->makeVertexBuffer(TOTAL_TRIS * sizeof(triPos), VertexBuffer::DATA_USAGE::STATIC);
	nor = renderer->makeVertexBuffer(TOTAL_TRIS * sizeof(triNor), VertexBuffer::DATA_USAGE::STATIC);
	uvs = renderer->makeVertexBuffer(TOTAL_TRIS * sizeof(triUV), VertexBuffer::DATA_USAGE::STATIC);

	// Create a mesh array with 3 basic vertex buffers.
	for (int i = 0; i < TOTAL_TRIS; i++) {

		Mesh* m = renderer->makeMesh();

		constexpr auto numberOfPosElements = std::extent<decltype(triPos)>::value;
		size_t offset = i * sizeof(triPos);
		pos->setData(triPos, sizeof(triPos), offset);
		m->addIAVertexBufferBinding(pos, offset, numberOfPosElements, sizeof(float4), POSITION);

		constexpr auto numberOfNorElements = std::extent<decltype(triNor)>::value;
		offset = i * sizeof(triNor);
		nor->setData(triNor, sizeof(triNor), offset);
		m->addIAVertexBufferBinding(nor, offset, numberOfNorElements, sizeof(float4), NORMAL);

		constexpr auto numberOfUVElements = std::extent<decltype(triUV)>::value;
		offset = i * sizeof(triUV);
		uvs->setData(triUV, sizeof(triUV), offset);
		m->addIAVertexBufferBinding(uvs, offset, numberOfUVElements , sizeof(float2), TEXTCOORD);

		// we can create a constant buffer outside the material, for example as part of the Mesh.
		m->txBuffer = renderer->makeConstantBuffer(std::string(TRANSLATION_NAME), TRANSLATION);
		
		m->technique = techniques[ i % 4];
		if (i % 4 == 2)
			m->addTexture(textures[0], DIFFUSE_SLOT);

		scene.push_back(m);
	}
	return 0;
}

void shutdown() {
	// shutdown.
	// delete dynamic objects
	for (auto m : materials)
	{
		delete(m);
	}
	for (auto t : techniques)
	{
		delete(t);
	}
	for (auto m : scene)
	{
		delete(m);
	};
	assert(pos->refCount() == 0);
	delete pos;
	assert(nor->refCount() == 0);
	delete nor;
	assert(uvs->refCount() == 0);
	delete uvs;
	
	for (auto s : samplers)
	{
		delete s;
	}

	for (auto t : textures)
	{
		delete t;
	}
	renderer->shutdown();
};

int main(int argc, char *argv[])
{
	renderer = Renderer::makeRenderer(Renderer::BACKEND::GL45);
	renderer->initialize(800,600);
	renderer->setWinTitle("OpenGL");
	renderer->setClearColor(0.0, 0.1, 0.1, 1.0);
	initialiseTestbench();
	run();
	shutdown();
	return 0;
};
