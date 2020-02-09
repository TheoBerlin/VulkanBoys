#pragma once
#include "Common/IEventHandler.h"

class CommonEventHandler : public IEventHandler
{
public:
	CommonEventHandler() = default;
	~CommonEventHandler() = default;

	DECL_NO_COPY(CommonEventHandler);

	virtual void onWindowClose() {}
	virtual void onWindowResize(uint32_t, uint32_t) {}
	virtual void onWindowFocusChanged(IWindow*, bool) {}

	virtual void onMouseMove(uint32_t, uint32_t) {}
	virtual void onMousePressed(int32_t) {}
	virtual void onMouseScroll(double, double) {}
	virtual void onMouseReleased(int32_t) {}

	virtual void onKeyTyped(uint32_t) {}
	virtual void onKeyPressed(EKey) {}
	virtual void onKeyReleased(EKey) {}
};