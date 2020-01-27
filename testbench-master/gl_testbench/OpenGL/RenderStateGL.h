#pragma once
#include <vector>
#include "../RenderState.h"

class RenderStateGL : public RenderState
{
public:
	RenderStateGL();
	~RenderStateGL();
	void setWireFrame(bool);
	void set();

	void setGlobalWireFrame(bool* global);
private:
	bool _wireframe;
	bool* globalWireFrame;
};

