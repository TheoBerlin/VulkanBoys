#pragma once
#include "Common/IInputHandler.h"

class InputHandler : public IInputHandler
{
public:
	InputHandler();
	~InputHandler() = default;

	virtual bool isKeyReleased(EKey keycode) override;
	virtual bool isKeyPressed(EKey keycode) override;
	
	virtual void onKeyPressed(EKey keycode) override;
	virtual void onKeyReleased(EKey keycode) override;

private:
	bool m_KeyStates[EKey::KEY_COUNT];
};

