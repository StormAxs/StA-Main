#include "bindwheel.h"

#include <engine/shared/config.h>
#include <engine/shared/datafile.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/ui.h>

// Concept by sjrc6
// https://github.com/sjrc6/TaterClient-ddnet/blob/master/src/game/client/components/bindwheel.cpp

void CBindWheel::OnInit()
{
	CDataFileReader DF;
	if(!DF.Open(Storage(), "bind_wheel.binds", IStorage::TYPE_ALL))
		return;

	int Start, Num;
	DF.GetType(0, &Start, &Num);

	for(int i = 0; i < Num; i++)
	{
		const SBind *pBind = (SBind *)DF.GetItem(Start + i);
		m_vBinds.push_back(*pBind);
	}

	DF.Close();
}

void CBindWheel::OnShutdown()
{
	CDataFileWriter DF;
	DF.Open(Storage(), "bind_wheel.binds", IStorage::TYPE_ALL);

	int ID = 0;
	for(const SBind &Bind : m_vBinds)
	{
		DF.AddItem(0, ID, sizeof(SBind), &Bind);
		ID++;
	}

	DF.Finish();
}

void CBindWheel::UseBind()
{
	Console()->ExecuteLine(m_vBinds[m_Choose].m_aBind);
}

void CBindWheel::ConBindWheel(IConsole::IResult *pResult, void *pUserData)
{
	CBindWheel *pSelf = (CBindWheel *)pUserData;

	if(pSelf->Client()->State() != IClient::STATE_DEMOPLAYBACK)
		pSelf->m_Active = pResult->GetInteger(0) != 0;
}

void CBindWheel::ConNewBindWheel(IConsole::IResult *pResult, void *pUserData)
{
	CBindWheel *pSelf = (CBindWheel *)pUserData;

	const char *pName = pResult->GetString(0);
	const char *pBind = pResult->GetString(1);

	for(const SBind &Bind : pSelf->m_vBinds)
	{
		if(!str_comp(Bind.m_aName, pName) || !str_comp(Bind.m_aBind, pBind))
		{
			dbg_msg("bind_wheel", "this bind is already in wheel");
			return;
		}
	}

	SBind NewBind;
	str_copy(NewBind.m_aName, pName);
	str_copy(NewBind.m_aBind, pBind);

	pSelf->m_vBinds.push_back(NewBind);

	dbg_msg("bind_wheel", "bind '%s' with name '%s' added to wheel", pBind, pName);
}

void CBindWheel::ConRemBindWheel(IConsole::IResult *pResult, void *pUserData)
{
	CBindWheel *pSelf = (CBindWheel *)pUserData;

	const char *pArg = pResult->GetString(0);

	for(int i = 0; i < pSelf->m_vBinds.size(); i++)
	{
		const SBind &Bind = pSelf->m_vBinds[i];

		if(!str_comp(Bind.m_aName, pArg) || !str_comp(Bind.m_aBind, pArg))
		{
			dbg_msg("bind_wheel", "bind '%s' removed from wheel", Bind.m_aName);
			pSelf->m_vBinds.erase(pSelf->m_vBinds.begin() + i);

			return;
		}
	}
}

void CBindWheel::OnConsoleInit()
{
	Console()->Register("+bind_wheel", "", CFGFLAG_CLIENT, ConBindWheel, this, "Open bind wheel");
	Console()->Register("bind_wheel", "s[name] s[bind]", CFGFLAG_CLIENT, ConNewBindWheel, this, "Add new bind to bind wheel");
	Console()->Register("bind_wheel_rem", "s[name_or_bind]", CFGFLAG_CLIENT, ConRemBindWheel, this, "Remove bind from bind wheel");
}

bool CBindWheel::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(!m_Active)
		return false;

	UI()->ConvertMouseMove(&x, &y, CursorType);
	m_MousePos += vec2(x, y);

	return true;
}

void CBindWheel::OnRender()
{
	if(!m_Active)
	{
		g_Config.m_ClOutline = 0;

		if(m_WasActive && !m_vBinds.empty() && m_Choose != -1)
			UseBind();
		m_WasActive = false;

		return;
	}
	m_WasActive = true;

	CUIRect Screen = *UI()->Screen();

	// Clear & map screen
	UI()->MapScreen();
	Graphics()->BlendNormal();
	Graphics()->TextureClear();

	// Render chircle
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0, 0, 0, 0.3f);
	Graphics()->DrawCircle(Screen.w / 2, Screen.h / 2, 190.0f, 64);
	Graphics()->QuadsEnd();

	if(length(m_MousePos) > 170.f)
		m_MousePos = normalize(m_MousePos) * 170.0f;

	// Get chose bind
	float SelectedAngle = angle(m_MousePos) + 2 * pi / 24;
	if(SelectedAngle < 0)
		SelectedAngle += 2 * pi;

	m_Choose = -1;
	if(length(m_MousePos) > 110.f)
		m_Choose = (int)(SelectedAngle / (2 * pi) * m_vBinds.size());

	// Render binds
	if(!m_vBinds.empty())
	{
		const float Theta = pi * 2 / m_vBinds.size();

		for(int i = 0; i < m_vBinds.size(); i++)
		{
			const SBind &Bind = m_vBinds[i];

			const float a = Theta * i;
			vec2 Pos = {cosf(a), sinf(a)};
			Pos *= 140.f;

			const float FontSize = (i == m_Choose) ? 20.f : 12.f;
			float W = TextRender()->TextWidth(FontSize, Bind.m_aName);
			TextRender()->Text(Screen.w / 2.f + Pos.x - W / 2.f, Screen.h / 2.f + Pos.y - FontSize / 2.f, FontSize, Bind.m_aName);
		}
	}
	else
	{
		const char *pEmpty = "Empty";
		float W = TextRender()->TextWidth(8.f, pEmpty);
		TextRender()->Text(Screen.w / 2.f - W / 2.f, Screen.h / 2.f - 12.f, 8.f, pEmpty);
	}

	RenderTools()->RenderCursor(m_MousePos + vec2(Screen.w, Screen.h) / 2.f, 24.0f);
}