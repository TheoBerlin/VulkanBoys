#pragma once
#include "Common/IEventHandler.h"

class CommonEventHandler : public IEventHandler
{
public:
	CommonEventHandler() = default;
	~CommonEventHandler() = default;

	DECL_NO_COPY(CommonEventHandler);

	virtual void onWindowClose() override {}
	virtual void onWindowResize(uint32_t, uint32_t) override {}
	virtual void onWindowFocusChanged(IWindow*, bool) override {}

	virtual void onMouseMove(uint32_t, uint32_t) override {}
	virtual void onMousePressed(int32_t) override {}
	virtual void onMouseScroll(double, double) override {}
	virtual void onMouseReleased(int32_t) override {}

	virtual void onKeyTyped(uint32_t) override {}
	virtual void onKeyPressed(EKey) override {}
	virtual void onKeyReleased(EKey) override {}
};