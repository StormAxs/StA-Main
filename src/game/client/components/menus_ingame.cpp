/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/demo.h>
#include <engine/favorites.h>
#include <engine/friends.h>
#include <engine/ghost.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/shared/localization.h>
#include <engine/textrender.h>

#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/components/countryflags.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>


#include "menus.h"
#include "motd.h"
#include "voting.h"

#include "ghost.h"
#include <engine/keys.h>
#include <engine/storage.h>

#include "parser.h"
#include <chrono>
#include <thread>

using namespace FontIcons;
using namespace std::chrono_literals;

void CMenus::RenderGame(CUIRect MainView)
{
	CUIRect Button, ButtonBar, ButtonBar2;
	MainView.HSplitTop(45.0f, &ButtonBar, &MainView);
	ButtonBar.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);

	// button bar
	ButtonBar.HSplitTop(10.0f, 0, &ButtonBar);
	ButtonBar.HSplitTop(25.0f, &ButtonBar, 0);
	ButtonBar.VMargin(10.0f, &ButtonBar);

	ButtonBar.HSplitTop(30.0f, 0, &ButtonBar2);
	ButtonBar2.HSplitTop(25.0f, &ButtonBar2, 0);

	ButtonBar.VSplitRight(120.0f, &ButtonBar, &Button);

	static CButtonContainer s_DisconnectButton;
	if(DoButton_Menu(&s_DisconnectButton, Localize("Disconnect"), 0, &Button))
	{
		if(Client()->GetCurrentRaceTime() / 60 >= g_Config.m_ClConfirmDisconnectTime && g_Config.m_ClConfirmDisconnectTime >= 0)
		{
			PopupConfirm(Localize("Disconnect"), Localize("Are you sure that you want to disconnect?"), Localize("Yes"), Localize("No"), &CMenus::PopupConfirmDisconnect);
		}
		else
		{
			Client()->Disconnect();
			RefreshBrowserTab(g_Config.m_UiPage);
		}
	}

	ButtonBar.VSplitRight(5.0f, &ButtonBar, 0);
	ButtonBar.VSplitRight(170.0f, &ButtonBar, &Button);

	bool DummyConnecting = Client()->DummyConnecting();
	static CButtonContainer s_DummyButton;
	if(!Client()->DummyAllowed())
	{
		DoButton_Menu(&s_DummyButton, Localize("Connect Dummy"), 1, &Button);
	}
	else if(DummyConnecting)
	{
		DoButton_Menu(&s_DummyButton, Localize("Connecting dummy"), 1, &Button);
	}
	else if(DoButton_Menu(&s_DummyButton, Client()->DummyConnected() ? Localize("Disconnect Dummy") : Localize("Connect Dummy"), 0, &Button))
	{
		if(!Client()->DummyConnected())
		{
			Client()->DummyConnect();
		}
		else
		{
			if(Client()->GetCurrentRaceTime() / 60 >= g_Config.m_ClConfirmDisconnectTime && g_Config.m_ClConfirmDisconnectTime >= 0)
			{
				PopupConfirm(Localize("Disconnect Dummy"), Localize("Are you sure that you want to disconnect your dummy?"), Localize("Yes"), Localize("No"), &CMenus::PopupConfirmDisconnectDummy);
			}
			else
			{
				Client()->DummyDisconnect(0);
				SetActive(false);
			}
		}
	}

	ButtonBar.VSplitRight(5.0f, &ButtonBar, 0);
	ButtonBar.VSplitRight(140.0f, &ButtonBar, &Button);

	static CButtonContainer s_DemoButton;
	bool Recording = DemoRecorder(RECORDER_MANUAL)->IsRecording();
	if(DoButton_Menu(&s_DemoButton, Recording ? Localize("Stop record") : Localize("Record demo"), 0, &Button))
	{
		if(!Recording)
			Client()->DemoRecorder_Start(Client()->GetCurrentMap(), true, RECORDER_MANUAL);
		else
			Client()->DemoRecorder_Stop(RECORDER_MANUAL);
	}

	static CButtonContainer s_SpectateButton;
	static CButtonContainer s_JoinRedButton;
	static CButtonContainer s_JoinBlueButton;

	bool Paused = false;
	bool Spec = false;
	if(m_pClient->m_Snap.m_LocalClientID >= 0)
	{
		Paused = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_Paused;
		Spec = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_Spec;
	}

	if(m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pGameInfoObj && !Paused && !Spec)
	{
		if(m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS)
		{
			ButtonBar.VSplitLeft(5.0f, 0, &ButtonBar);
			ButtonBar.VSplitLeft(120.0f, &Button, &ButtonBar);
			if(!DummyConnecting && DoButton_Menu(&s_SpectateButton, Localize("Spectate"), 0, &Button))
			{
				if(g_Config.m_ClDummy == 0 || Client()->DummyConnected())
				{
					m_pClient->SendSwitchTeam(TEAM_SPECTATORS);
					SetActive(false);
				}
			}
		}

		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags & GAMEFLAG_TEAMS)
		{
			if(m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_RED)
			{
				ButtonBar.VSplitLeft(5.0f, 0, &ButtonBar);
				ButtonBar.VSplitLeft(120.0f, &Button, &ButtonBar);
				if(!DummyConnecting && DoButton_Menu(&s_JoinRedButton, Localize("Join red"), 0, &Button))
				{
					m_pClient->SendSwitchTeam(TEAM_RED);
					SetActive(false);
				}
			}

			if(m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_BLUE)
			{
				ButtonBar.VSplitLeft(5.0f, 0, &ButtonBar);
				ButtonBar.VSplitLeft(120.0f, &Button, &ButtonBar);
				if(!DummyConnecting && DoButton_Menu(&s_JoinBlueButton, Localize("Join blue"), 0, &Button))
				{
					m_pClient->SendSwitchTeam(TEAM_BLUE);
					SetActive(false);
				}
			}
		}
		else
		{
			if(m_pClient->m_Snap.m_pLocalInfo->m_Team != 0)
			{
				ButtonBar.VSplitLeft(5.0f, 0, &ButtonBar);
				ButtonBar.VSplitLeft(120.0f, &Button, &ButtonBar);
				if(!DummyConnecting && DoButton_Menu(&s_SpectateButton, Localize("Join game"), 0, &Button))
				{
					m_pClient->SendSwitchTeam(0);
					SetActive(false);
				}
			}
		}

		if(m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS)
		{
			ButtonBar.VSplitLeft(5.0f, 0, &ButtonBar);
			ButtonBar.VSplitLeft(65.0f, &Button, &ButtonBar);

			static CButtonContainer s_KillButton;
			if(DoButton_Menu(&s_KillButton, Localize("Kill"), 0, &Button))
			{
				m_pClient->SendKill(-1);
				SetActive(false);
			}
		}
	}

	if(m_pClient->m_ReceivedDDNetPlayer && m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pGameInfoObj)
	{
		if(m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS || Paused || Spec)
		{
			ButtonBar.VSplitLeft(5.0f, 0, &ButtonBar);
			ButtonBar.VSplitLeft((!Paused && !Spec) ? 65.0f : 120.0f, &Button, &ButtonBar);

			static CButtonContainer s_PauseButton;
			if(DoButton_Menu(&s_PauseButton, (!Paused && !Spec) ? Localize("Pause") : Localize("Join game"), 0, &Button))
			{
				m_pClient->Console()->ExecuteLine("say /pause");
				SetActive(false);
			}
		}
	}
}

void CMenus::PopupConfirmDisconnect()
{
	Client()->Disconnect();
}

void CMenus::PopupConfirmDisconnectDummy()
{
	Client()->DummyDisconnect(0);
	SetActive(false);
}

void CMenus::RenderPlayers(CUIRect MainView)
{
	CUIRect Button, Button2, ButtonBar, Options, Player;
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);

	// player options
	MainView.Margin(10.0f, &Options);
	Options.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, 10.0f);
	Options.Margin(10.0f, &Options);
	Options.HSplitTop(50.0f, &Button, &Options);
	UI()->DoLabel(&Button, Localize("Player options"), 34.0f, TEXTALIGN_ML);

	// headline
	Options.HSplitTop(34.0f, &ButtonBar, &Options);
	ButtonBar.VSplitRight(231.0f, &Player, &ButtonBar);
	UI()->DoLabel(&Player, Localize("Player"), 24.0f, TEXTALIGN_ML);

	ButtonBar.HMargin(1.0f, &ButtonBar);
	float Width = ButtonBar.h * 2.0f;
	ButtonBar.VSplitLeft(Width, &Button, &ButtonBar);
	RenderTools()->RenderIcon(IMAGE_GUIICONS, SPRITE_GUIICON_MUTE, &Button);

	ButtonBar.VSplitLeft(20.0f, nullptr, &ButtonBar);
	ButtonBar.VSplitLeft(Width, &Button, &ButtonBar);
	RenderTools()->RenderIcon(IMAGE_GUIICONS, SPRITE_GUIICON_EMOTICON_MUTE, &Button);

	ButtonBar.VSplitLeft(20.0f, nullptr, &ButtonBar);
	ButtonBar.VSplitLeft(Width, &Button, &ButtonBar);
	RenderTools()->RenderIcon(IMAGE_GUIICONS, SPRITE_GUIICON_FRIEND, &Button);

	Options.HSplitTop(34.0f, &ButtonBar, &Options);
	ButtonBar.VSplitRight(310.0f, &Player, &ButtonBar);
	UI()->DoLabel(&Player, Localize("Blacklist"), 10.0f, TEXTALIGN_MR);

	int TotalPlayers = 0;
	for(const auto &pInfoByName : m_pClient->m_Snap.m_apInfoByName)
	{
		if(!pInfoByName)
			continue;

		int Index = pInfoByName->m_ClientID;

		if(Index == m_pClient->m_Snap.m_LocalClientID)
			continue;

		TotalPlayers++;
	}

	static CListBox s_ListBox;
	s_ListBox.DoStart(24.0f, TotalPlayers, 1, 3, -1, &Options);

	// options
	static char s_aPlayerIDs[MAX_CLIENTS][4] = {{0}};

	for(int i = 0, Count = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_pClient->m_Snap.m_apInfoByName[i])
			continue;

		int Index = m_pClient->m_Snap.m_apInfoByName[i]->m_ClientID;
		if(Index == m_pClient->m_Snap.m_LocalClientID)
			continue;

		CGameClient::CClientData &CurrentClient = m_pClient->m_aClients[Index];
		const CListboxItem Item = s_ListBox.DoNextItem(&CurrentClient);

		Count++;

		if(!Item.m_Visible)
			continue;

		CUIRect Row = Item.m_Rect;
		if(Count % 2 == 1)
			Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, 5.0f);
		Row.VSplitRight(s_ListBox.ScrollbarWidthMax() - s_ListBox.ScrollbarWidth(), &Row, nullptr);
		Row.VSplitRight(300.0f, &Player, &Row);

		// player info
		Player.VSplitLeft(28.0f, &Button, &Player);

		CTeeRenderInfo TeeInfo = CurrentClient.m_RenderInfo;

		TeeInfo.m_Size = Button.h;

		const CAnimState *pIdleState = CAnimState::GetIdle();
		vec2 OffsetToMid;
		RenderTools()->GetRenderTeeOffsetToRenderedTee(pIdleState, &TeeInfo, OffsetToMid);
		vec2 TeeRenderPos(Button.x + Button.h / 2, Button.y + Button.h / 2 + OffsetToMid.y);

		RenderTools()->RenderTee(pIdleState, &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), TeeRenderPos);

		Player.HSplitTop(1.5f, nullptr, &Player);
		Player.VSplitMid(&Player, &Button);
		Row.VSplitRight(210.0f, &Button2, &Row);

		UI()->DoLabel(&Player, CurrentClient.m_aName, 14.0f, TEXTALIGN_ML);
		UI()->DoLabel(&Button, CurrentClient.m_aClan, 14.0f, TEXTALIGN_ML);

		m_pClient->m_CountryFlags.Render(CurrentClient.m_Country, ColorRGBA(1.0f, 1.0f, 1.0f, 0.5f),
			Button2.x, Button2.y + Button2.h / 2.0f - 0.75f * Button2.h / 2.0f, 1.5f * Button2.h, 0.75f * Button2.h);

		//blacklisting hahxd
		Row.VSplitLeft(5.0f, &Row, nullptr);
		//  Row.VSplitLeft(Width, &Button, &Row);
		Button.VSplitRight((Width - Button.h) / 4.0f, nullptr, &Button);
		Button.VSplitRight(Button.h, nullptr, &Button);

		if(DoButton_CheckBox(&s_aPlayerIDs[Index][3], Localize(""), CurrentClient.m_Foe, &Button))
		{
			CurrentClient.m_Foe ^= 1;
			if(CurrentClient.m_Foe)
				Client()->Foes()->AddFriend(CurrentClient.m_aName, CurrentClient.m_aClan);
			else
				Client()->Foes()->RemoveFriend(CurrentClient.m_aName, CurrentClient.m_aClan);
		}

		// ignore chat button
		Row.HMargin(2.0f, &Row);
		Row.VSplitLeft(Width, &Button, &Row);
		Button.VSplitLeft((Width - Button.h) / 4.0f, nullptr, &Button);
		Button.VSplitLeft(Button.h, &Button, nullptr);
		if(g_Config.m_ClShowChatFriends && !CurrentClient.m_Friend)
			DoButton_Toggle(&s_aPlayerIDs[Index][0], 1, &Button, false);
		else if(DoButton_Toggle(&s_aPlayerIDs[Index][0], CurrentClient.m_ChatIgnore, &Button, true))
			CurrentClient.m_ChatIgnore ^= 1;

		// ignore emoticon button
		Row.VSplitLeft(30.0f, nullptr, &Row);
		Row.VSplitLeft(Width, &Button, &Row);
		Button.VSplitLeft((Width - Button.h) / 4.0f, nullptr, &Button);
		Button.VSplitLeft(Button.h, &Button, nullptr);
		if(g_Config.m_ClShowChatFriends && !CurrentClient.m_Friend)
			DoButton_Toggle(&s_aPlayerIDs[Index][1], 1, &Button, false);
		else if(DoButton_Toggle(&s_aPlayerIDs[Index][1], CurrentClient.m_EmoticonIgnore, &Button, true))
			CurrentClient.m_EmoticonIgnore ^= 1;

		// friend button
		Row.VSplitLeft(10.0f, nullptr, &Row);
		Row.VSplitLeft(Width, &Button, &Row);
		Button.VSplitLeft((Width - Button.h) / 4.0f, nullptr, &Button);
		Button.VSplitLeft(Button.h, &Button, nullptr);
		if(DoButton_Toggle(&s_aPlayerIDs[Index][2], CurrentClient.m_Friend, &Button, true))
		{
			if(CurrentClient.m_Friend)
				m_pClient->Friends()->RemoveFriend(CurrentClient.m_aName, CurrentClient.m_aClan);
			else
				m_pClient->Friends()->AddFriend(CurrentClient.m_aName, CurrentClient.m_aClan);

			m_pClient->Client()->ServerBrowserUpdate();
		}
	}

	s_ListBox.DoEnd();
}

void CMenus::RenderServerInfo(CUIRect MainView)
{
	if(!m_pClient->m_Snap.m_pLocalInfo)
		return;

	// fetch server info
	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);

	// render background
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);

	CUIRect View, ServerInfo, GameInfo, Motd;

	float x = 0.0f;
	float y = 0.0f;

	char aBuf[1024];

	// set view to use for all sub-modules
	MainView.Margin(10.0f, &View);

	// serverinfo
	View.HSplitTop(View.h / 2 - 5.0f, &ServerInfo, &Motd);
	ServerInfo.VSplitLeft(View.w / 2 - 5.0f, &ServerInfo, &GameInfo);
	ServerInfo.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 10.0f);

	ServerInfo.Margin(5.0f, &ServerInfo);

	x = 5.0f;
	y = 0.0f;

	TextRender()->Text(ServerInfo.x + x, ServerInfo.y + y, 32, Localize("Server info"), -1.0f);
	y += 32.0f + 5.0f;

	mem_zero(aBuf, sizeof(aBuf));
	str_format(
		aBuf,
		sizeof(aBuf),
		"%s\n\n"
		"%s: %s\n"
		"%s: %d\n"
		"%s: %s\n"
		"%s: %s\n",
		CurrentServerInfo.m_aName,
		Localize("Address"), CurrentServerInfo.m_aAddress,
		Localize("Ping"), m_pClient->m_Snap.m_pLocalInfo->m_Latency,
		Localize("Version"), CurrentServerInfo.m_aVersion,
		Localize("Password"), CurrentServerInfo.m_Flags & 1 ? Localize("Yes") : Localize("No"));

	TextRender()->Text(ServerInfo.x + x, ServerInfo.y + y, 20, aBuf, ServerInfo.w - 10.0f);

	// copy info button
	{
		CUIRect Button;
		ServerInfo.HSplitBottom(20.0f, &ServerInfo, &Button);
		Button.VSplitRight(200.0f, &ServerInfo, &Button);
		static CButtonContainer s_CopyButton;
		if(DoButton_Menu(&s_CopyButton, Localize("Copy info"), 0, &Button))
		{
			char aInfo[256];
			CurrentServerInfo.InfoToString(aInfo, sizeof(aInfo));
			Input()->SetClipboardText(aInfo);
		}
	}

	// favorite checkbox
	{
		CUIRect Button;
		NETADDR ServerAddr = Client()->ServerAddress();
		TRISTATE IsFavorite = Favorites()->IsFavorite(&ServerAddr, 1);
		ServerInfo.HSplitBottom(20.0f, &ServerInfo, &Button);
		static int s_AddFavButton = 0;
		if(DoButton_CheckBox(&s_AddFavButton, Localize("Favorite"), IsFavorite != TRISTATE::NONE, &Button))
		{
			if(IsFavorite != TRISTATE::NONE)
				Favorites()->Remove(&ServerAddr, 1);
			else
				Favorites()->Add(&ServerAddr, 1);
		}
	}

	// gameinfo
	GameInfo.VSplitLeft(10.0f, 0x0, &GameInfo);
	GameInfo.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 10.0f);

	GameInfo.Margin(5.0f, &GameInfo);

	x = 5.0f;
	y = 0.0f;

	TextRender()->Text(GameInfo.x + x, GameInfo.y + y, 32, Localize("Game info"), -1.0f);
	y += 32.0f + 5.0f;

	if(m_pClient->m_Snap.m_pGameInfoObj)
	{
		mem_zero(aBuf, sizeof(aBuf));
		str_format(
			aBuf,
			sizeof(aBuf),
			"\n\n"
			"%s: %s\n"
			"%s: %s\n"
			"%s: %d\n"
			"%s: %d\n"
			"\n"
			"%s: %d/%d\n",
			Localize("Game type"), CurrentServerInfo.m_aGameType,
			Localize("Map"), CurrentServerInfo.m_aMap,
			Localize("Score limit"), m_pClient->m_Snap.m_pGameInfoObj->m_ScoreLimit,
			Localize("Time limit"), m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit,
			Localize("Players"), m_pClient->m_Snap.m_NumPlayers, CurrentServerInfo.m_MaxClients);
		TextRender()->Text(GameInfo.x + x, GameInfo.y + y, 20, aBuf, GameInfo.w - 10.0f);
	}

	RenderServerInfoMotd(Motd);
}

void CMenus::RenderServerInfoMotd(CUIRect Motd)
{
	const float MotdFontSize = 16.0f;
	Motd.HSplitTop(10.0f, nullptr, &Motd);
	Motd.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 10.0f);
	Motd.HMargin(5.0f, &Motd);
	Motd.VMargin(10.0f, &Motd);

	CUIRect MotdHeader;
	Motd.HSplitTop(2.0f * MotdFontSize, &MotdHeader, &Motd);
	Motd.HSplitTop(5.0f, nullptr, &Motd);
	TextRender()->Text(MotdHeader.x, MotdHeader.y, 2.0f * MotdFontSize, Localize("MOTD"), -1.0f);

	if(!m_pClient->m_Motd.ServerMotd()[0])
		return;

	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0.0f, 0.0f);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 5 * MotdFontSize;
	s_ScrollRegion.Begin(&Motd, &ScrollOffset, &ScrollParams);
	Motd.y += ScrollOffset.y;

	static float s_MotdHeight = 0.0f;
	static int64_t s_MotdLastUpdateTime = -1;
	if(!m_MotdTextContainerIndex.Valid() || s_MotdLastUpdateTime == -1 || s_MotdLastUpdateTime != m_pClient->m_Motd.ServerMotdUpdateTime())
	{
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, 0.0f, 0.0f, MotdFontSize, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = Motd.w;
		TextRender()->RecreateTextContainer(m_MotdTextContainerIndex, &Cursor, m_pClient->m_Motd.ServerMotd());
		s_MotdHeight = Cursor.Height();
		s_MotdLastUpdateTime = m_pClient->m_Motd.ServerMotdUpdateTime();
	}

	CUIRect MotdTextArea;
	Motd.HSplitTop(s_MotdHeight, &MotdTextArea, &Motd);
	s_ScrollRegion.AddRect(MotdTextArea);

	if(m_MotdTextContainerIndex.Valid())
		TextRender()->RenderTextContainer(m_MotdTextContainerIndex, TextRender()->DefaultTextColor(), TextRender()->DefaultTextOutlineColor(), MotdTextArea.x, MotdTextArea.y);

	s_ScrollRegion.End();
}

bool CMenus::RenderServerControlServer(CUIRect MainView)
{
	CUIRect List = MainView;
	int Total = m_pClient->m_Voting.m_NumVoteOptions;
	int NumVoteOptions = 0;
	int aIndices[MAX_VOTE_OPTIONS];
	static int s_CurVoteOption = 0;
	int TotalShown = 0;

	for(CVoteOptionClient *pOption = m_pClient->m_Voting.m_pFirst; pOption; pOption = pOption->m_pNext)
	{
		if(!m_FilterInput.IsEmpty() && !str_utf8_find_nocase(pOption->m_aDescription, m_FilterInput.GetString()))
			continue;
		TotalShown++;
	}

	static CListBox s_ListBox;
	s_ListBox.DoStart(19.0f, TotalShown, 1, 3, s_CurVoteOption, &List);

	int i = -1;
	for(CVoteOptionClient *pOption = m_pClient->m_Voting.m_pFirst; pOption; pOption = pOption->m_pNext)
	{
		i++;
		if(!m_FilterInput.IsEmpty() && !str_utf8_find_nocase(pOption->m_aDescription, m_FilterInput.GetString()))
			continue;

		if(NumVoteOptions < Total)
			aIndices[NumVoteOptions] = i;
		NumVoteOptions++;

		const CListboxItem Item = s_ListBox.DoNextItem(pOption);
		if(!Item.m_Visible)
			continue;

		CUIRect Label;
		Item.m_Rect.VMargin(2.0f, &Label);
		UI()->DoLabel(&Label, pOption->m_aDescription, 13.0f, TEXTALIGN_ML);
	}

	s_CurVoteOption = s_ListBox.DoEnd();
	if(s_CurVoteOption < Total)
		m_CallvoteSelectedOption = aIndices[s_CurVoteOption];
	return s_ListBox.WasItemActivated();
}

bool CMenus::RenderServerControlKick(CUIRect MainView, bool FilterSpectators)
{
	int NumOptions = 0;
	int Selected = 0;
	static int aPlayerIDs[MAX_CLIENTS];
	for(const auto &pInfoByName : m_pClient->m_Snap.m_apInfoByName)
	{
		if(!pInfoByName)
			continue;

		int Index = pInfoByName->m_ClientID;
		if(Index == m_pClient->m_Snap.m_LocalClientID || (FilterSpectators && pInfoByName->m_Team == TEAM_SPECTATORS))
			continue;

		if(!str_utf8_find_nocase(m_pClient->m_aClients[Index].m_aName, m_FilterInput.GetString()))
			continue;

		if(m_CallvoteSelectedPlayer == Index)
			Selected = NumOptions;
		aPlayerIDs[NumOptions++] = Index;
	}

	static CListBox s_ListBox;
	s_ListBox.DoStart(24.0f, NumOptions, 1, 3, Selected, &MainView);

	for(int i = 0; i < NumOptions; i++)
	{
		const CListboxItem Item = s_ListBox.DoNextItem(&aPlayerIDs[i]);
		if(!Item.m_Visible)
			continue;

		CUIRect TeeRect, Label;
		Item.m_Rect.VSplitLeft(Item.m_Rect.h, &TeeRect, &Label);

		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[aPlayerIDs[i]].m_RenderInfo;
		TeeInfo.m_Size = TeeRect.h;

		const CAnimState *pIdleState = CAnimState::GetIdle();
		vec2 OffsetToMid;
		RenderTools()->GetRenderTeeOffsetToRenderedTee(pIdleState, &TeeInfo, OffsetToMid);
		vec2 TeeRenderPos(TeeRect.x + TeeInfo.m_Size / 2, TeeRect.y + TeeInfo.m_Size / 2 + OffsetToMid.y);

		RenderTools()->RenderTee(pIdleState, &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), TeeRenderPos);

		UI()->DoLabel(&Label, m_pClient->m_aClients[aPlayerIDs[i]].m_aName, 16.0f, TEXTALIGN_ML);
	}

	Selected = s_ListBox.DoEnd();
	m_CallvoteSelectedPlayer = Selected != -1 ? aPlayerIDs[Selected] : -1;
	return s_ListBox.WasItemActivated();
}

void CMenus::RenderServerControl(CUIRect MainView)
{
	static int s_ControlPage = 0;
	// render background
	CUIRect Bottom, RconExtension, TabBar, Button;
	MainView.HSplitTop(20.0f, &Bottom, &MainView);
	Bottom.Draw(ms_ColorTabbarActive, 0, 10.0f);
	MainView.HSplitTop(20.0f, &TabBar, &MainView);
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);
	MainView.Margin(10.0f, &MainView);

	if(Client()->RconAuthed())
		MainView.HSplitBottom(90.0f, &MainView, &RconExtension);

	// tab bar
	TabBar.VSplitLeft(TabBar.w / 3, &Button, &TabBar);
	static CButtonContainer s_Button0;
	if(DoButton_MenuTab(&s_Button0, Localize("Change settings"), s_ControlPage == 0, &Button, 0))
		s_ControlPage = 0;

	TabBar.VSplitMid(&Button, &TabBar);
	static CButtonContainer s_Button1;
	if(DoButton_MenuTab(&s_Button1, Localize("Kick player"), s_ControlPage == 1, &Button, 0))
		s_ControlPage = 1;

	static CButtonContainer s_Button2;
	if(DoButton_MenuTab(&s_Button2, Localize("Move player to spectators"), s_ControlPage == 2, &TabBar, 0))
		s_ControlPage = 2;

	// render page
	MainView.HSplitBottom(ms_ButtonHeight + 5 * 2, &MainView, &Bottom);
	Bottom.HMargin(5.0f, &Bottom);

	bool Call = false;
	if(s_ControlPage == 0)
		Call = RenderServerControlServer(MainView);
	else if(s_ControlPage == 1)
		Call = RenderServerControlKick(MainView, false);
	else if(s_ControlPage == 2)
		Call = RenderServerControlKick(MainView, true);

	// vote menu
	CUIRect QuickSearch;

	// render quick search
	Bottom.VSplitLeft(5.0f, 0, &Bottom);
	Bottom.VSplitLeft(250.0f, &QuickSearch, &Bottom);
	QuickSearch.HSplitTop(5.0f, 0, &QuickSearch);
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);

	UI()->DoLabel(&QuickSearch, FONT_ICON_MAGNIFYING_GLASS, 14.0f, TEXTALIGN_ML);
	float wSearch = TextRender()->TextWidth(14.0f, FONT_ICON_MAGNIFYING_GLASS, -1, -1.0f);
	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	QuickSearch.VSplitLeft(wSearch, 0, &QuickSearch);
	QuickSearch.VSplitLeft(5.0f, 0, &QuickSearch);

	if(m_ControlPageOpening || (Input()->KeyPress(KEY_F) && Input()->ModifierIsPressed()))
	{
		UI()->SetActiveItem(&m_FilterInput);
		m_ControlPageOpening = false;
		m_FilterInput.SelectAll();
	}
	m_FilterInput.SetEmptyText(Localize("Search"));
	UI()->DoClearableEditBox(&m_FilterInput, &QuickSearch, 14.0f);

	// call vote
	Bottom.VSplitRight(10.0f, &Bottom, 0);
	Bottom.VSplitRight(120.0f, &Bottom, &Button);
	Button.HSplitTop(5.0f, 0, &Button);

	static CButtonContainer s_CallVoteButton;
	if(DoButton_Menu(&s_CallVoteButton, Localize("Call vote"), 0, &Button) || Call)
	{
		if(s_ControlPage == 0)
		{
			m_pClient->m_Voting.CallvoteOption(m_CallvoteSelectedOption, m_CallvoteReasonInput.GetString());
			if(g_Config.m_UiCloseWindowAfterChangingSetting)
				SetActive(false);
		}
		else if(s_ControlPage == 1)
		{
			if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
				m_pClient->m_Snap.m_apPlayerInfos[m_CallvoteSelectedPlayer])
			{
				m_pClient->m_Voting.CallvoteKick(m_CallvoteSelectedPlayer, m_CallvoteReasonInput.GetString());
				SetActive(false);
			}
		}
		else if(s_ControlPage == 2)
		{
			if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
				m_pClient->m_Snap.m_apPlayerInfos[m_CallvoteSelectedPlayer])
			{
				m_pClient->m_Voting.CallvoteSpectate(m_CallvoteSelectedPlayer, m_CallvoteReasonInput.GetString());
				SetActive(false);
			}
		}
		m_CallvoteReasonInput.Clear();
	}

	// render kick reason
	CUIRect Reason;
	Bottom.VSplitRight(20.0f, &Bottom, 0);
	Bottom.VSplitRight(200.0f, &Bottom, &Reason);
	Reason.HSplitTop(5.0f, 0, &Reason);
	const char *pLabel = Localize("Reason:");
	UI()->DoLabel(&Reason, pLabel, 14.0f, TEXTALIGN_ML);
	float w = TextRender()->TextWidth(14.0f, pLabel, -1, -1.0f);
	Reason.VSplitLeft(w + 10.0f, 0, &Reason);
	if(Input()->KeyPress(KEY_R) && Input()->ModifierIsPressed())
	{
		UI()->SetActiveItem(&m_CallvoteReasonInput);
		m_CallvoteReasonInput.SelectAll();
	}
	UI()->DoEditBox(&m_CallvoteReasonInput, &Reason, 14.0f);

	// extended features (only available when authed in rcon)
	if(Client()->RconAuthed())
	{
		// background
		RconExtension.HSplitTop(10.0f, 0, &RconExtension);
		RconExtension.HSplitTop(20.0f, &Bottom, &RconExtension);
		RconExtension.HSplitTop(5.0f, 0, &RconExtension);

		// force vote
		Bottom.VSplitLeft(5.0f, 0, &Bottom);
		Bottom.VSplitLeft(120.0f, &Button, &Bottom);

		static CButtonContainer s_ForceVoteButton;
		if(DoButton_Menu(&s_ForceVoteButton, Localize("Force vote"), 0, &Button))
		{
			if(s_ControlPage == 0)
				m_pClient->m_Voting.CallvoteOption(m_CallvoteSelectedOption, m_CallvoteReasonInput.GetString(), true);
			else if(s_ControlPage == 1)
			{
				if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
					m_pClient->m_Snap.m_apPlayerInfos[m_CallvoteSelectedPlayer])
				{
					m_pClient->m_Voting.CallvoteKick(m_CallvoteSelectedPlayer, m_CallvoteReasonInput.GetString(), true);
					SetActive(false);
				}
			}
			else if(s_ControlPage == 2)
			{
				if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
					m_pClient->m_Snap.m_apPlayerInfos[m_CallvoteSelectedPlayer])
				{
					m_pClient->m_Voting.CallvoteSpectate(m_CallvoteSelectedPlayer, m_CallvoteReasonInput.GetString(), true);
					SetActive(false);
				}
			}
			m_CallvoteReasonInput.Clear();
		}

		if(s_ControlPage == 0)
		{
			// remove vote
			Bottom.VSplitRight(10.0f, &Bottom, 0);
			Bottom.VSplitRight(120.0f, 0, &Button);
			static CButtonContainer s_RemoveVoteButton;
			if(DoButton_Menu(&s_RemoveVoteButton, Localize("Remove"), 0, &Button))
				m_pClient->m_Voting.RemovevoteOption(m_CallvoteSelectedOption);

			// add vote
			RconExtension.HSplitTop(20.0f, &Bottom, &RconExtension);
			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(250.0f, &Button, &Bottom);
			UI()->DoLabel(&Button, Localize("Vote description:"), 14.0f, TEXTALIGN_ML);

			Bottom.VSplitLeft(20.0f, 0, &Button);
			UI()->DoLabel(&Button, Localize("Vote command:"), 14.0f, TEXTALIGN_ML);

			static CLineInputBuffered<VOTE_DESC_LENGTH> s_VoteDescriptionInput;
			static CLineInputBuffered<VOTE_CMD_LENGTH> s_VoteCommandInput;
			RconExtension.HSplitTop(20.0f, &Bottom, &RconExtension);
			Bottom.VSplitRight(10.0f, &Bottom, 0);
			Bottom.VSplitRight(120.0f, &Bottom, &Button);
			static CButtonContainer s_AddVoteButton;
			if(DoButton_Menu(&s_AddVoteButton, Localize("Add"), 0, &Button))
				if(!s_VoteDescriptionInput.IsEmpty() && !s_VoteCommandInput.IsEmpty())
					m_pClient->m_Voting.AddvoteOption(s_VoteDescriptionInput.GetString(), s_VoteCommandInput.GetString());

			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(250.0f, &Button, &Bottom);
			UI()->DoEditBox(&s_VoteDescriptionInput, &Button, 14.0f);

			Bottom.VMargin(20.0f, &Button);
			UI()->DoEditBox(&s_VoteCommandInput, &Button, 14.0f);
		}
	}
}

void CMenus::RenderStats(CUIRect MainView)
{
	CUIRect Bottom, TabBar, Button;
	MainView.HSplitTop(20.0f, &Bottom, &MainView);
	Bottom.Draw(ms_ColorTabbarActive, 10, 0.0f);
	MainView.HSplitTop(20.0f, &TabBar, &MainView);
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);
	MainView.Margin(10.0f, &MainView);

	static int s_StatsPage = 0;

	TabBar.VSplitLeft(TabBar.w / 3, &Button, &TabBar);
	static CButtonContainer s_Button0;
	if(DoButton_MenuTab(&s_Button0, Localize("Account"), s_StatsPage == 0, &Button, 0))
		s_StatsPage = 0;

	TabBar.VSplitMid(&Button, &TabBar);
	static CButtonContainer s_Button1;
	if(DoButton_MenuTab(&s_Button1, Localize("Map Tracker"), s_StatsPage == 1, &Button, 0))
		s_StatsPage = 1;

	static CButtonContainer s_Button2;
	if(DoButton_MenuTab(&s_Button2, Localize("DDStats"), s_StatsPage == 2, &TabBar, 0))
		s_StatsPage = 2;

	if(s_StatsPage == 0)
	{
		float FontSize_BodyLeft = 25.0f;
		float FontSize_HeadLeft = 40.0f;

		CUIRect WB, StALogo, RT1, RT2, CP, LF, Tpoints, Tpoints2,Tpoints3, MP1, MP2, LP, PointsS, button, text, GR, GRP, PP;
		CUIRect RefreshButton, DDStatsButton, DDraceButton, DiscordButton, ABOUTButton;

		WB.x = 0;
		WB.y = 0;
		WB.w = MainView.w * 0.75f;
		WB.h = MainView.h * 0.15f;

		StALogo.x = 0;
		StALogo.y = WB.h;
		StALogo.w = 50.0f;
		StALogo.h = 50.0f;

		CP.x = WB.w;
		CP.y = StALogo.y;
		CP.w = MainView.w - WB.w;
		CP.h = StALogo.h;

		LP.x = CP.x;
		LP.y = CP.y + CP.h;
		LP.w = CP.w;
		LP.h = MainView.h - LP.y;

		CUIRect PointRect = MainView;

		MainView.HSplitTop(MainView.h / 3, &WB, &MainView);
		WB.VSplitRight(WB.w / 2 - 50, &WB, &StALogo);
		WB.HSplitTop(WB.h / 2 + 50, &WB, &CP);
		WB.Margin(1.0f, &WB);
		CP.VSplitLeft(120.0f, &RT1, &CP);
		CP.VSplitLeft(200.0f, &CP, &GRP);
		MainView.HSplitTop(MainView.h / 2, &LF, &PointsS);
		LF.VSplitRight(LF.w / 3 - 65, &LF, &LP);
		LF.HSplitTop(LF.h / 2 - 50.0f, &LF, &Tpoints);
		LF.VSplitLeft(120.0f, &RT2, &LF);
		LF.VSplitLeft(250.0f, &LF, &GR);
		Tpoints.VSplitLeft(240.0f, &MP1, &Tpoints);
		Tpoints.VSplitLeft(Tpoints.w / 2 + 10, &Tpoints, &PP);
		MP1.HSplitTop(MP1.h, nullptr, &MP2);
		PointsS.VSplitLeft(240.0f, &MP2, &PointsS);
		Tpoints.HSplitTop(Tpoints.h / 3, &Tpoints, &Tpoints2);
		Tpoints2.HSplitTop(Tpoints2.h / 2, &Tpoints2, &Tpoints3);
		PointsS.VSplitRight(70.0f, &PointsS, &RefreshButton);
		RefreshButton.HSplitTop(33.0f, &RefreshButton, &DDStatsButton);
		DDStatsButton.HSplitTop(33.0f, &DDStatsButton, &DDraceButton);
		DDraceButton.HSplitTop(33.0f, &DDraceButton, &DiscordButton);
		DiscordButton.HSplitTop(33.0f, &DiscordButton, &ABOUTButton);
		ABOUTButton.HSplitTop(33.0f, &ABOUTButton, nullptr);


		// rendering margin
		PointsS.HMargin(1.0f, &PointsS);
		Tpoints.Margin(1.0f, &Tpoints);
		LP.Margin(1.0f, &LP);
		CP.Margin(1.0f, &CP);
		StALogo.Margin(1.5f, &StALogo);
		LF.Margin(1.0f, &LF);
		GR.Margin(1.0f, &GR);
		GRP.Margin(1.0f, &GRP);
		MP1.VMargin(1.0f, &MP1);
		MP2.VMargin(1.0f, &MP2);
		PP.Margin(1.0f, &PP);
		Tpoints2.Margin(1.0f, &Tpoints2);
		Tpoints3.Margin(1.0f, &Tpoints3);
		RefreshButton.HMargin(1.0f, &RefreshButton);
		DDStatsButton.HMargin(1.0f, &DDStatsButton);
		DDraceButton.HMargin(1.0f, &DDraceButton);
		DiscordButton.HMargin(1.0f, &DiscordButton);
		ABOUTButton.HMargin(1.0f, &ABOUTButton);




		const float LineMargin = 22.0f;
		char *pSkinName = g_Config.m_ClPlayerSkin;
		int *pUseCustomColor = &g_Config.m_ClPlayerUseCustomColor;
		unsigned *pColorBody = &g_Config.m_ClPlayerColorBody;
		unsigned *pColorFeet = &g_Config.m_ClPlayerColorFeet;
		int CurrentFlag = m_Dummy ? g_Config.m_ClDummyCountry : g_Config.m_PlayerCountry;

		CTeeRenderInfo OwnSkinInfo;
		const CSkin *pSkin = m_pClient->m_Skins.Find(pSkinName);
		OwnSkinInfo.m_OriginalRenderSkin = pSkin->m_OriginalSkin;
		OwnSkinInfo.m_ColorableRenderSkin = pSkin->m_ColorableSkin;
		OwnSkinInfo.m_SkinMetrics = pSkin->m_Metrics;
		OwnSkinInfo.m_CustomColoredSkin = *pUseCustomColor;
		if(*pUseCustomColor)
		{
			OwnSkinInfo.m_ColorBody = color_cast<ColorRGBA>(ColorHSLA(*pColorBody).UnclampLighting());
			OwnSkinInfo.m_ColorFeet = color_cast<ColorRGBA>(ColorHSLA(*pColorFeet).UnclampLighting());
		}
		else
		{
			OwnSkinInfo.m_ColorBody = ColorRGBA(1.0f, 1.0f, 1.0f);
			OwnSkinInfo.m_ColorFeet = ColorRGBA(1.0f, 1.0f, 1.0f);
		}
		OwnSkinInfo.m_Size = 70.0f;

		const CAnimState *pIdleState = CAnimState::GetIdle();
		vec2 OffsetToMid;
		RenderTools()->GetRenderTeeOffsetToRenderedTee(pIdleState, &OwnSkinInfo, OffsetToMid);
		vec2 TeeRenderPos(RT1.x + 60.0f, (RT1.y + 7.0f) + RT2.h);
		int Emote = m_Dummy ? g_Config.m_ClDummyDefaultEyes : g_Config.m_ClPlayerDefaultEyes;
		RenderTools()->RenderTee(pIdleState, &OwnSkinInfo, Emote, vec2(1, 0), TeeRenderPos);

		WB.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 2.0f);
		CP.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 2.0f);
		GR.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 2.0f);
		GRP.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 2.0f);
		LF.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 2.0f);
		LP.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 2.0f);
		Tpoints.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 2.0f);
		Tpoints2.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 2.0f);
		Tpoints3.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 2.0f);
		MP1.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_T, 2.0f);
		PointsS.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_T | IGraphics::CORNER_B | IGraphics::CORNER_L, 2.0f);
		MP2.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_B, 2.0f);
		PP.Draw(vec4(1, 1, 1, 0.25f), IGraphics::CORNER_ALL, 2.0f);
		static CStatsPlayer s_StatsPlayer;

		const auto &&SetIconMode = [&](bool Enable) {
			if(Enable)
			{
				TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
				TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
			}
			else
			{
				TextRender()->SetRenderFlags(0);
				TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
			}
		};

		const char *pName = Client()->PlayerName();
		static bool IsParsed = false; // Make it a static variable to prevent it from be resassigned to false every time.
		static bool IsParsedDDN = false;

		if(!IsParsed)
		{
			if(!s_StatsPlayer.m_pGetStatsDDStats) // Only run FetchPlayer, if s_StatsPlayer isn't initalized (The FetchPlayer() will initalize it).
				m_pClient->m_Stats.FetchPlayer(&s_StatsPlayer, pName);

			if(s_StatsPlayer.m_pGetStatsDDStats->State() == s_StatsPlayer.m_pGetStatsDDStats->STATE_PENDING) // check if download is still in progress.
				return;

			// Once the download is finished.
			if(!s_StatsPlayer.StatsParsed && s_StatsPlayer.m_pGetStatsDDStats->State() == s_StatsPlayer.m_pGetStatsDDStats->STATE_DONE)
			{
				IsParsed = true; // Set it to true, so it won't run this if branch again. (untill you set IsParsed to false again and free s_StatsPlayer)
				m_pClient->m_Stats.ParseJSON(&s_StatsPlayer); // parse the json.
				return;
			}
		}
		if(!IsParsedDDN)
		{
			if(!s_StatsPlayer.m_pGetStatsDDNet) // Only run FetchPlayer, if s_StatsPlayer isn't initalized (The FetchPlayer() will initalize it).
				m_pClient->m_Stats.FetchPlayer(&s_StatsPlayer, pName);

			if(s_StatsPlayer.m_pGetStatsDDNet->State() == s_StatsPlayer.m_pGetStatsDDNet->STATE_PENDING) // check if download is still in progress.
				return;

			// Once the download is finished.
			if(!s_StatsPlayer.StatsParsed && s_StatsPlayer.m_pGetStatsDDNet->State() == s_StatsPlayer.m_pGetStatsDDNet->STATE_DONE)
			{
				IsParsedDDN = true; // Set it to true, so it won't run this if branch again. (untill you set IsParsed to false again and free s_StatsPlayer)
				m_pClient->m_Stats.ParseJSON(&s_StatsPlayer); // parse the json.
				return;
			}
			//dont u dare to touch this bullshit
		}
		SetIconMode(true);
		static CButtonContainer s_RefreshButton;
		if (DoButton_Menu(&s_RefreshButton, FONT_ICON_ARROW_ROTATE_RIGHT, 0, &RefreshButton, 0, IGraphics::CORNER_R))
		{

			//PRAY TO GOD IT WILL WORK MELON
			IsParsed = false;
			IsParsedDDN = false;
			s_StatsPlayer.Reset();


			if(!IsParsed)
			{
				if(!s_StatsPlayer.m_pGetStatsDDStats) // Only run FetchPlayer, if s_StatsPlayer isn't initalized (The FetchPlayer() will initalize it).
					m_pClient->m_Stats.FetchPlayer(&s_StatsPlayer, pName);

				if(s_StatsPlayer.m_pGetStatsDDStats->State() == s_StatsPlayer.m_pGetStatsDDStats->STATE_PENDING) // check if download is still in progress.
					return;

				// Once the download is finished.
				if(!s_StatsPlayer.StatsParsed && s_StatsPlayer.m_pGetStatsDDStats->State() == s_StatsPlayer.m_pGetStatsDDStats->STATE_DONE)
				{
					IsParsed = true; // Set it to true, so it won't run this if branch again. (untill you set IsParsed to false again and free s_StatsPlayer)
					m_pClient->m_Stats.ParseJSON(&s_StatsPlayer); // parse the json.
					return;
				}
			}
			if(!IsParsedDDN)
			{
				if(!s_StatsPlayer.m_pGetStatsDDNet) // Only run FetchPlayer, if s_StatsPlayer isn't initalized (The FetchPlayer() will initalize it).
					m_pClient->m_Stats.FetchPlayer(&s_StatsPlayer, pName);

				if(s_StatsPlayer.m_pGetStatsDDNet->State() == s_StatsPlayer.m_pGetStatsDDNet->STATE_PENDING) // check if download is still in progress.
					return;

				// Once the download is finished.
				if(!s_StatsPlayer.StatsParsed && s_StatsPlayer.m_pGetStatsDDNet->State() == s_StatsPlayer.m_pGetStatsDDNet->STATE_DONE)
				{
					IsParsedDDN = true; // Set it to true, so it won't run this if branch again. (untill you set IsParsed to false again and free s_StatsPlayer)
					m_pClient->m_Stats.ParseJSON(&s_StatsPlayer); // parse the json.
					return;
				}
				//dont u dare to touch this bullshit

			}
		}

		SetIconMode(false);

		static CButtonContainer s_DDStatsButton;
		if (DoButton_Menu(&s_DDStatsButton, "DDStats", 0, &DDStatsButton, 0, IGraphics::CORNER_R))
		{
			const char* playerName = Client()->PlayerName();
			char url[256];
			str_format(url, sizeof(url), "https://ddstats.qwik.space/player/%s", playerName);

			if (!open_link(url))
			{
				dbg_msg("menus", "Couldn't open link: %s", url);
			}
		}

		static CButtonContainer s_DDRaceButton;
		if (DoButton_Menu(&s_DDRaceButton, "DDNet", 0, &DDraceButton, 0, IGraphics::CORNER_R))
		{
			const char* playerName = Client()->PlayerName();
			char url[256];
			str_format(url, sizeof(url), "https://ddnet.org/players/%s", playerName);

			if (!open_link(url))
			{
				dbg_msg("menus", "Couldn't open link: %s", url);
			}
		}

		static CButtonContainer s_DiscordButton;
		if (DoButton_Menu(&s_DiscordButton, "Discord", 0, &DiscordButton, 0, IGraphics::CORNER_R))
		{
			char url[256];
			str_format(url, sizeof(url), "https://discord.gg/BhWXQXcgsT");

			if (!open_link(url))
			{
				dbg_msg("menus", "Couldn't open link: %s", url);
			}
		}


		static CButtonContainer s_AboutButton;
		if (DoButton_Menu(&s_AboutButton, "About", 0, &ABOUTButton, 0, IGraphics::CORNER_R))
		{
			char url[256];
			str_format(url, sizeof(url), "https://stormaxs.github.io/StA-site/");

			if (!open_link(url))
			{
				dbg_msg("menus", "Couldn't open link: %s", url);
			}
		}

		char Welcome[128];
		// TODO: fix it somewhen
		/*
const char *names[] =
	{
		"meloƞ", "-StormAx", "我叫芙焦", "Mʎɹ シ", "Cheeru", "Mónik"
	};

  for(int i = 0; i < sizeof(names) / sizeof(names[0]); i++)
  {
	  if(strcmp(Client()->PlayerName(), names[i]) == 0)
	  {
		  break;
	  }
	  else
	  {
		  ColorRGBA col = color_cast<ColorRGBA>(ColorHSVA(round_to_int(LocalTime() * 15.f) % 255 / 255.f, 1.f, 1.f));
		  TextRender()->TextColor(col);
	  }
  }
  */

		ColorRGBA col = color_cast<ColorRGBA>(ColorHSVA(round_to_int(LocalTime() * 15.f) % 255 / 255.f, 1.f, 1.f));


		CStatsPlayer statsPlayer;
		CUIRect FetchButton;
		static CButtonContainer s_FetchButton;



		if(strcmp(Client()->PlayerName(), "-StormAx") == 0 || strcmp(Client()->PlayerName(), "meloƞ") == 0 || strcmp(Client()->PlayerName(), "我叫芙焦") == 0 || strcmp(Client()->PlayerName(), "Mʎɹ シ") == 0 || strcmp(Client()->PlayerName(), "Cheeru") == 0 || strcmp(Client()->PlayerName(), "Mónik") == 0)
		{
			ColorRGBA col = color_cast<ColorRGBA>(ColorHSVA(round_to_int(LocalTime() * 15.f) % 255 / 255.f, 1.f, 1.f));
			TextRender()->TextColor(col);
		}



		// render logo
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_ACLOGO].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1, 1, 1, 1);
		IGraphics::CQuadItem QuadItem(MainView.w / 2 + 95, 80, 390, 160);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		if (IsParsed == false | IsParsedDDN == false)
		{
			str_format(Welcome, sizeof(Welcome), " Parsing Player Info \n Please wait...", Client()->PlayerName());
			UI()->DoLabel(&PointsS, Welcome, 40.0f, TEXTALIGN_ML);
		}

		if (IsParsed == true | IsParsedDDN == true)
		{

			str_format(Welcome, sizeof(Welcome), " Welcome Back %s", Client()->PlayerName());
			UI()->DoLabel(&WB, Welcome, 40.0f, TEXTALIGN_ML);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);



			char pts[256];
			int playerPoints = s_StatsPlayer.Points;
			str_format(pts, sizeof(pts), " Current points: %d", playerPoints);
			UI()->DoLabel(&CP, pts, 35.0f, TEXTALIGN_ML);

			int RankPoints = s_StatsPlayer.RankPoints;
			if(RankPoints == 0)
			{
				str_format(pts, sizeof(pts), " No Rank Points Yet");
				UI()->DoLabel(&LF, pts, 28.0f, TEXTALIGN_MIDDLE);
			}
			else
			{
				str_format(pts, sizeof(pts), " Current Rank Points: %d", RankPoints);
				UI()->DoLabel(&LF, pts, 35.0f, TEXTALIGN_ML);
			}

			UI()->DoLabel(&MP1, "Most Played Maps", 24.0f, TEXTALIGN_TC);
			MP1.HSplitTop(30.0f, nullptr, &MP1);
			int PlayerTime;
			for(int i = 0; i < 11; ++i)
			{
				str_format(pts, sizeof(pts), " %s - %.0f hrs.", s_StatsPlayer.aMap[i], s_StatsPlayer.aTime[i]);
				UI()->DoLabel(&MP1, pts, 18.0f, TEXTALIGN_TL);
				MP1.HSplitTop(23.0f, nullptr, &MP1);
			}

			str_format(pts, sizeof(pts), " Best Region: %s", s_StatsPlayer.PlayTimeLocation);

			if (strcmp(s_StatsPlayer.PlayTimeLocation, "eu") == 0) {
				str_format(pts, sizeof(pts), " Best Region: EU");
			}
			if (strcmp(s_StatsPlayer.PlayTimeLocation, "na") == 0) {
				str_format(pts, sizeof(pts), " Best Region: NA");
			}
			if (strcmp(s_StatsPlayer.PlayTimeLocation, "as:cn") == 0) {
				str_format(pts, sizeof(pts), " Best Region: AS:CN");
			}
			if (strcmp(s_StatsPlayer.PlayTimeLocation, "as") == 0) {
				str_format(pts, sizeof(pts), " Best Region: AS");
			}
			if (strcmp(s_StatsPlayer.PlayTimeLocation, "sa") == 0) {
				str_format(pts, sizeof(pts), " Best Region: SA");
			}
			if (strcmp(s_StatsPlayer.PlayTimeLocation, "unknown") == 0) {
				str_format(pts, sizeof(pts), " Best Region: UNKNOWN");
			}
			if (strcmp(s_StatsPlayer.PlayTimeLocation, "oc") == 0) {
				str_format(pts, sizeof(pts), " Best Region: OC");
			}
			if (strcmp(s_StatsPlayer.PlayTimeLocation, "af") == 0) {
				str_format(pts, sizeof(pts), " Best Region: AF");
			}

			UI()->DoLabel(&Tpoints, pts, 24.0f, TEXTALIGN_ML);

			int sum = 0;

			for (int i = 0; i < 15; ++i) {
				sum += s_StatsPlayer.totalPlaytime[i];
			}

			str_format(pts, sizeof(pts), " Total Hours Played: %d hrs", sum);
			UI()->DoLabel(&Tpoints3, pts, 30.0f, TEXTALIGN_ML);

			//DDNET============================
			//blyat
			str_format(pts, sizeof(pts), " Points rank: #%d ", s_StatsPlayer.PointCategoryDDR);
			UI()->DoLabel(&GRP, pts, 35.0f, TEXTALIGN_ML);

			if (s_StatsPlayer.RankInWorld == 0)
			{
				str_format(pts, sizeof(pts), " Not Ranked");
				UI()->DoLabel(&GR, pts, 28.0f, TEXTALIGN_MIDDLE);

			}
			else
			{
				str_format(pts, sizeof(pts), " World Rank: #%d ", s_StatsPlayer.RankInWorld);
				UI()->DoLabel(&GR, pts, 28.0f, TEXTALIGN_MIDDLE);
			}
			//Last FINISHED???______________________KURWA
			UI()->DoLabel(&LP, "Last Finished Maps", 20.0f, TEXTALIGN_TC);
			LP.HSplitTop(21.0f, nullptr, &LP);
			for(int i = 0; i < 7; ++i)
			{
				str_format(pts, sizeof(pts), " %s - %s", s_StatsPlayer.LastFinish[i], s_StatsPlayer.aJson[i]);
				UI()->DoLabel(&LP, pts, 17.0f, TEXTALIGN_TL);
				LP.HSplitTop(18.0f, nullptr, &LP);
			}

			//TEAMMETES============================================
			UI()->DoLabel(&PP, "Best Teammates", 20.0f, TEXTALIGN_TC);
			PP.HSplitTop(27.0f, nullptr, &PP);
			for(int i = 0; i < 5; ++i)
			{
				str_format(pts, sizeof(pts), " %s with %d ranks", s_StatsPlayer.FavouritePartners[i], s_StatsPlayer.BestPartnerFinishes[i]);
				UI()->DoLabel(&PP, pts, 17.0f, TEXTALIGN_TL);
				PP.HSplitTop(18.0f, nullptr, &PP);
			}

			str_format(pts, sizeof(pts), " Point Last Month: %d", s_StatsPlayer.PLM);
			UI()->DoLabel(&Tpoints2, pts, 20.0f, TEXTALIGN_ML);

		}




	}
	else if(s_StatsPage == 1)
	{
	}

	else if(s_StatsPage == 2) //only uses DDstats!
	{
		//test
		static CStatsPlayer s_StatsPlayer;

		// render background
		CUIRect Bottom1, TabBar1, Label, Button1, SearchBox, FetchButton;
		MainView.HSplitTop(20.0f, &Bottom1, &MainView);
		Bottom1.Draw(ms_ColorTabbarActive, 0, 10.0f);
		MainView.HSplitTop(20.0f, &TabBar1, &MainView);
		MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);
		MainView.Margin(10.0f, &MainView);

		// tab bar
		TabBar1.VSplitLeft(TabBar1.w, &Button1, &TabBar1);
		static CButtonContainer s_Button2;
		if(DoButton_MenuTab(&s_Button2, Localize("Player profile"), s_StatsPage == 1, &Button1, 0))
			s_StatsPage = 1;

		// Title
		MainView.HSplitTop(20.0f, &Label, &MainView);
		Label.VSplitRight(300.0f, &Label, &SearchBox);
		char aWelcome[128];
		str_format(aWelcome, sizeof(aWelcome), "Player profile");
		UI()->DoLabel(&Label, aWelcome, 14.0f, TEXTALIGN_ML);

		SearchBox.VSplitLeft(200.0f, &SearchBox, &FetchButton);
		SearchBox.VMargin(15.0f, &SearchBox);
		UI()->DoEditBox(&m_StatsPlayerInput, &SearchBox, 14.0f);

		static CButtonContainer s_FetchButton;
		if((DoButton_Menu(&s_FetchButton, Localize("Fetch(Find)"), 0, &FetchButton) || (Input()->KeyPress(KEY_RETURN) && m_StatsPlayerInput.IsActive())) && !m_StatsPlayerInput.IsEmpty())
		{
			m_pClient->m_Stats.FetchPlayer(&s_StatsPlayer, m_StatsPlayerInput.GetString());
		}

		if(!s_StatsPlayer.m_pGetStatsDDStats)
			return;

		if(s_StatsPlayer.m_pGetStatsDDStats->State() == s_StatsPlayer.m_pGetStatsDDStats->STATE_PENDING)
			return;

		if(!s_StatsPlayer.StatsParsed && s_StatsPlayer.m_pGetStatsDDStats->State() == s_StatsPlayer.m_pGetStatsDDStats->STATE_DONE)
			return m_pClient->m_Stats.ParseJSON(&s_StatsPlayer);

		MainView.HSplitTop(20.0f, &Label, &MainView);
		if(s_StatsPlayer.m_pGetStatsDDStats->State() == -1) // error (404)
		{
			UI()->DoLabel(&Label, "No player found.", 14.0f, TEXTALIGN_ML);
			return;
		}
		char aPoints[128];
		str_format(aPoints, sizeof(aPoints), "%s has %d points", s_StatsPlayer.aPlayer, s_StatsPlayer.Points);
		UI()->DoLabel(&Label, aPoints, 14.0f, TEXTALIGN_ML);
	}
}
void CMenus::RenderInGameNetwork(CUIRect MainView)
{
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);

	CUIRect TabBar, Button;
	MainView.HSplitTop(24.0f, &TabBar, &MainView);

	int NewPage = g_Config.m_UiPage;

	TabBar.VSplitLeft(100.0f, &Button, &TabBar);
	static CButtonContainer s_InternetButton;
	if(DoButton_MenuTab(&s_InternetButton, Localize("Internet"), g_Config.m_UiPage == PAGE_INTERNET, &Button, IGraphics::CORNER_NONE))
	{
		if(g_Config.m_UiPage != PAGE_INTERNET)
		{
			if(g_Config.m_UiPage != PAGE_FAVORITES)
				Client()->RequestDDNetInfo();
			ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
		}
		NewPage = PAGE_INTERNET;
	}

	TabBar.VSplitLeft(80.0f, &Button, &TabBar);
	static CButtonContainer s_LanButton;
	if(DoButton_MenuTab(&s_LanButton, Localize("LAN"), g_Config.m_UiPage == PAGE_LAN, &Button, IGraphics::CORNER_NONE))
	{
		if(g_Config.m_UiPage != PAGE_LAN)
			ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
		NewPage = PAGE_LAN;
	}

	TabBar.VSplitLeft(110.0f, &Button, &TabBar);
	static CButtonContainer s_FavoritesButton;
	if(DoButton_MenuTab(&s_FavoritesButton, Localize("Favorites"), g_Config.m_UiPage == PAGE_FAVORITES, &Button, IGraphics::CORNER_NONE))
	{
		if(g_Config.m_UiPage != PAGE_FAVORITES)
		{
			if(g_Config.m_UiPage != PAGE_INTERNET)
				Client()->RequestDDNetInfo();
			ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
		}
		NewPage = PAGE_FAVORITES;
	}

	if(NewPage != g_Config.m_UiPage)
	{
		if(Client()->State() != IClient::STATE_OFFLINE)
			SetMenuPage(NewPage);
	}

	RenderServerbrowser(MainView);
}

// ghost stuff
int CMenus::GhostlistFetchCallback(const CFsFileInfo *pInfo, int IsDir, int StorageType, void *pUser)
{
	CMenus *pSelf = (CMenus *)pUser;
	const char *pMap = pSelf->Client()->GetCurrentMap();
	if(IsDir || !str_endswith(pInfo->m_pName, ".gho") || !str_startswith(pInfo->m_pName, pMap))
		return 0;

	char aFilename[IO_MAX_PATH_LENGTH];
	str_format(aFilename, sizeof(aFilename), "%s/%s", pSelf->m_pClient->m_Ghost.GetGhostDir(), pInfo->m_pName);

	CGhostInfo Info;
	if(!pSelf->m_pClient->m_Ghost.GhostLoader()->GetGhostInfo(aFilename, &Info, pMap, pSelf->Client()->GetCurrentMapSha256(), pSelf->Client()->GetCurrentMapCrc()))
		return 0;

	CGhostItem Item;
	str_copy(Item.m_aFilename, aFilename);
	str_copy(Item.m_aPlayer, Info.m_aOwner);
	Item.m_Date = pInfo->m_TimeModified;
	Item.m_Time = Info.m_Time;
	if(Item.m_Time > 0)
		pSelf->m_vGhosts.push_back(Item);

	if(time_get_nanoseconds() - pSelf->m_GhostPopulateStartTime > 500ms)
	{
		pSelf->RenderLoading(Localize("Loading ghost files"), "", 0, false);
	}

	return 0;
}

void CMenus::GhostlistPopulate()
{
	m_vGhosts.clear();
	m_GhostPopulateStartTime = time_get_nanoseconds();
	Storage()->ListDirectoryInfo(IStorage::TYPE_ALL, m_pClient->m_Ghost.GetGhostDir(), GhostlistFetchCallback, this);
	std::sort(m_vGhosts.begin(), m_vGhosts.end());

	CGhostItem *pOwnGhost = 0;
	for(auto &Ghost : m_vGhosts)
	{
		Ghost.m_Failed = false;
		if(str_comp(Ghost.m_aPlayer, Client()->PlayerName()) == 0 && (!pOwnGhost || Ghost < *pOwnGhost))
			pOwnGhost = &Ghost;
	}

	if(pOwnGhost)
	{
		pOwnGhost->m_Own = true;
		pOwnGhost->m_Slot = m_pClient->m_Ghost.Load(pOwnGhost->m_aFilename);
	}
}

CMenus::CGhostItem *CMenus::GetOwnGhost()
{
	for(auto &Ghost : m_vGhosts)
		if(Ghost.m_Own)
			return &Ghost;
	return nullptr;
}

void CMenus::UpdateOwnGhost(CGhostItem Item)
{
	int Own = -1;
	for(size_t i = 0; i < m_vGhosts.size(); i++)
		if(m_vGhosts[i].m_Own)
			Own = i;

	if(Own != -1)
	{
		if(g_Config.m_ClRaceGhostSaveBest)
		{
			if(Item.HasFile() || !m_vGhosts[Own].HasFile())
				DeleteGhostItem(Own);
		}
		if(m_vGhosts[Own].m_Time > Item.m_Time)
		{
			Item.m_Own = true;
			m_vGhosts[Own].m_Own = false;
			m_vGhosts[Own].m_Slot = -1;
		}
		else
		{
			Item.m_Own = false;
			Item.m_Slot = -1;
		}
	}
	else
	{
		Item.m_Own = true;
	}

	Item.m_Date = std::time(0);
	Item.m_Failed = false;
	m_vGhosts.insert(std::lower_bound(m_vGhosts.begin(), m_vGhosts.end(), Item), Item);
}

void CMenus::DeleteGhostItem(int Index)
{
	if(m_vGhosts[Index].HasFile())
		Storage()->RemoveFile(m_vGhosts[Index].m_aFilename, IStorage::TYPE_SAVE);
	m_vGhosts.erase(m_vGhosts.begin() + Index);
}

void CMenus::RenderGhost(CUIRect MainView)
{
	// render background
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitBottom(5.0f, &MainView, 0);
	MainView.VSplitLeft(5.0f, 0, &MainView);
	MainView.VSplitRight(5.0f, &MainView, 0);

	CUIRect Headers, Status;
	CUIRect View = MainView;

	View.HSplitTop(17.0f, &Headers, &View);
	View.HSplitBottom(28.0f, &View, &Status);

	// split of the scrollbar
	Headers.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_T, 5.0f);
	Headers.VSplitRight(20.0f, &Headers, 0);

	struct CColumn
	{
		const char *m_pCaption;
		int m_Id;
		float m_Width;
		CUIRect m_Rect;
		CUIRect m_Spacer;
	};

	enum
	{
		COL_ACTIVE = 0,
		COL_NAME,
		COL_TIME,
		COL_DATE,
	};

	static CColumn s_aCols[] = {
		{"", -1, 2.0f, {0}, {0}},
		{"", COL_ACTIVE, 30.0f, {0}, {0}},
		{Localizable("Name"), COL_NAME, 200.0f, {0}, {0}},
		{Localizable("Time"), COL_TIME, 90.0f, {0}, {0}},
		{Localizable("Date"), COL_DATE, 150.0f, {0}, {0}},
	};

	int NumCols = std::size(s_aCols);

	// do layout
	for(int i = 0; i < NumCols; i++)
	{
		Headers.VSplitLeft(s_aCols[i].m_Width, &s_aCols[i].m_Rect, &Headers);

		if(i + 1 < NumCols)
			Headers.VSplitLeft(2, &s_aCols[i].m_Spacer, &Headers);
	}

	// do headers
	for(int i = 0; i < NumCols; i++)
		DoButton_GridHeader(&s_aCols[i].m_Id, Localize(s_aCols[i].m_pCaption), 0, &s_aCols[i].m_Rect);

	View.Draw(ColorRGBA(0, 0, 0, 0.15f), 0, 0);

	const int NumGhosts = m_vGhosts.size();
	int NumFailed = 0;
	int NumActivated = 0;
	static int s_SelectedIndex = 0;
	static CListBox s_ListBox;
	s_ListBox.DoStart(17.0f, NumGhosts, 1, 3, s_SelectedIndex, &View, false);

	for(int i = 0; i < NumGhosts; i++)
	{
		const CGhostItem *pGhost = &m_vGhosts[i];
		const CListboxItem Item = s_ListBox.DoNextItem(pGhost);

		if(pGhost->m_Failed)
			NumFailed++;
		if(pGhost->Active())
			NumActivated++;

		if(!Item.m_Visible)
			continue;

		ColorRGBA rgb = ColorRGBA(1.0f, 1.0f, 1.0f);
		if(pGhost->m_Own)
			rgb = color_cast<ColorRGBA>(ColorHSLA(0.33f, 1.0f, 0.75f));

		if(pGhost->m_Failed)
			rgb = ColorRGBA(0.6f, 0.6f, 0.6f, 1.0f);

		TextRender()->TextColor(rgb.WithAlpha(pGhost->HasFile() ? 1.0f : 0.5f));

		for(int c = 0; c < NumCols; c++)
		{
			CUIRect Button;
			Button.x = s_aCols[c].m_Rect.x;
			Button.y = Item.m_Rect.y;
			Button.h = Item.m_Rect.h;
			Button.w = s_aCols[c].m_Rect.w;

			int Id = s_aCols[c].m_Id;

			if(Id == COL_ACTIVE)
			{
				if(pGhost->Active())
				{
					Graphics()->WrapClamp();
					Graphics()->TextureSet(GameClient()->m_EmoticonsSkin.m_aSpriteEmoticons[(SPRITE_OOP + 7) - SPRITE_OOP]);
					Graphics()->QuadsBegin();
					IGraphics::CQuadItem QuadItem(Button.x + Button.w / 2, Button.y + Button.h / 2, 20.0f, 20.0f);
					Graphics()->QuadsDraw(&QuadItem, 1);

					Graphics()->QuadsEnd();
					Graphics()->WrapNormal();
				}
			}
			else if(Id == COL_NAME)
			{
				UI()->DoLabel(&Button, pGhost->m_aPlayer, 12.0f, TEXTALIGN_ML);
			}
			else if(Id == COL_TIME)
			{
				char aBuf[64];
				str_time(pGhost->m_Time / 10, TIME_HOURS_CENTISECS, aBuf, sizeof(aBuf));
				UI()->DoLabel(&Button, aBuf, 12.0f, TEXTALIGN_ML);
			}
			else if(Id == COL_DATE)
			{
				char aBuf[64];
				str_timestamp_ex(pGhost->m_Date, aBuf, sizeof(aBuf), FORMAT_SPACE);
				UI()->DoLabel(&Button, aBuf, 12.0f, TEXTALIGN_ML);
			}
		}

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}

	s_SelectedIndex = s_ListBox.DoEnd();

	Status.Draw(ColorRGBA(1, 1, 1, 0.25f), IGraphics::CORNER_B, 5.0f);
	Status.Margin(5.0f, &Status);

	CUIRect Button;
	Status.VSplitLeft(25.0f, &Button, &Status);

	static CButtonContainer s_ReloadButton;
	static CButtonContainer s_DirectoryButton;
	static CButtonContainer s_ActivateAll;

	if(DoButton_FontIcon(&s_ReloadButton, FONT_ICON_ARROW_ROTATE_RIGHT, 0, &Button) || Input()->KeyPress(KEY_F5) || (Input()->KeyPress(KEY_R) && Input()->ModifierIsPressed()))
	{
		m_pClient->m_Ghost.UnloadAll();
		GhostlistPopulate();
	}

	Status.VSplitLeft(5.0f, &Button, &Status);
	Status.VSplitLeft(175.0f, &Button, &Status);
	if(DoButton_Menu(&s_DirectoryButton, Localize("Ghosts directory"), 0, &Button))
	{
		char aBuf[IO_MAX_PATH_LENGTH];
		Storage()->GetCompletePath(IStorage::TYPE_SAVE, "ghosts", aBuf, sizeof(aBuf));
		Storage()->CreateFolder("ghosts", IStorage::TYPE_SAVE);
		if(!open_file(aBuf))
		{
			dbg_msg("menus", "couldn't open file '%s'", aBuf);
		}
	}

	Status.VSplitLeft(5.0f, &Button, &Status);
	if(NumGhosts - NumFailed > 0)
	{
		Status.VSplitLeft(175.0f, &Button, &Status);
		bool ActivateAll = ((NumGhosts - NumFailed) != NumActivated) && m_pClient->m_Ghost.FreeSlots();

		const char *pActionText = ActivateAll ? Localize("Activate all") : Localize("Deactivate all");
		if(DoButton_Menu(&s_ActivateAll, pActionText, 0, &Button))
		{
			for(int i = 0; i < NumGhosts; i++)
			{
				CGhostItem *pGhost = &m_vGhosts[i];
				if(pGhost->m_Failed || (ActivateAll && pGhost->m_Slot != -1))
					continue;

				if(ActivateAll)
				{
					if(!m_pClient->m_Ghost.FreeSlots())
						break;

					pGhost->m_Slot = m_pClient->m_Ghost.Load(pGhost->m_aFilename);
					if(pGhost->m_Slot == -1)
						pGhost->m_Failed = true;
				}
				else
				{
					m_pClient->m_Ghost.UnloadAll();
					pGhost->m_Slot = -1;
				}
			}
		}
	}

	if(s_SelectedIndex == -1 || s_SelectedIndex >= (int)m_vGhosts.size())
		return;

	CGhostItem *pGhost = &m_vGhosts[s_SelectedIndex];

	CGhostItem *pOwnGhost = GetOwnGhost();
	int ReservedSlots = !pGhost->m_Own && !(pOwnGhost && pOwnGhost->Active());
	if(!pGhost->m_Failed && pGhost->HasFile() && (pGhost->Active() || m_pClient->m_Ghost.FreeSlots() > ReservedSlots))
	{
		Status.VSplitRight(120.0f, &Status, &Button);

		static CButtonContainer s_GhostButton;
		const char *pText = pGhost->Active() ? Localize("Deactivate") : Localize("Activate");
		if(DoButton_Menu(&s_GhostButton, pText, 0, &Button) || s_ListBox.WasItemActivated())
		{
			if(pGhost->Active())
			{
				m_pClient->m_Ghost.Unload(pGhost->m_Slot);
				pGhost->m_Slot = -1;
			}
			else
			{
				pGhost->m_Slot = m_pClient->m_Ghost.Load(pGhost->m_aFilename);
				if(pGhost->m_Slot == -1)
					pGhost->m_Failed = true;
			}
		}
		Status.VSplitRight(5.0f, &Status, 0);
	}

	Status.VSplitRight(120.0f, &Status, &Button);

	static CButtonContainer s_DeleteButton;
	if(DoButton_Menu(&s_DeleteButton, Localize("Delete"), 0, &Button))
	{
		if(pGhost->Active())
			m_pClient->m_Ghost.Unload(pGhost->m_Slot);
		DeleteGhostItem(s_SelectedIndex);
	}

	Status.VSplitRight(5.0f, &Status, 0);

	bool Recording = m_pClient->m_Ghost.GhostRecorder()->IsRecording();
	if(!pGhost->HasFile() && !Recording && pGhost->Active())
	{
		static CButtonContainer s_SaveButton;
		Status.VSplitRight(120.0f, &Status, &Button);
		if(DoButton_Menu(&s_SaveButton, Localize("Save"), 0, &Button))
			m_pClient->m_Ghost.SaveGhost(pGhost);
	}
}

void CMenus::RenderIngameHint()
{
	float Width = 300 * Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, Width, 300);
	TextRender()->TextColor(1, 1, 1, 1);
	TextRender()->Text(5, 280, 5, Localize("Menu opened. Press Esc key again to close menu."), -1.0f);
	UI()->MapScreen();
}