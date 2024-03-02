#ifndef GAME_CLIENT_MACHO_BIND_WHEEL_H
#define GAME_CLIENT_MACHO_BIND_WHEEL_H

#include "base/vmath.h"
#include "engine/console.h"
#include "game/client/component.h"

enum
{
	BIND_WHEEL_MAX_NAME = 64,
	BIND_WHEEL_MAX_BIND = 40
};

class CBindWheel : public CComponent
{
	vec2 m_MousePos;
	bool m_Active;
	bool m_WasActive;
	int m_Choose;

	static void ConBindWheel(IConsole::IResult *pResult, void *pUserData);
	static void ConNewBindWheel(IConsole::IResult *pResult, void *pUserData);
	static void ConRemBindWheel(IConsole::IResult *pResult, void *pUserData);

	void UseBind();

public:
	virtual int Sizeof() const override { return sizeof(*this); }

	struct SBind
	{
		char m_aName[BIND_WHEEL_MAX_NAME];
		char m_aBind[BIND_WHEEL_MAX_BIND];
	};

	std::vector<SBind> m_vBinds;

	void OnInit() override;
	void OnShutdown() override;
	void OnConsoleInit() override;
	void OnRender() override;
	bool OnCursorMove(float x, float y, IInput::ECursorType CursorType) override;
};

#endif