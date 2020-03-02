#pragma once
#include "Common/IInputHandler.h"

#include <vector>

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
	//May be implemented as a bitset (Implementation defined)
	std::vector<bool> m_KeyStates;
};

