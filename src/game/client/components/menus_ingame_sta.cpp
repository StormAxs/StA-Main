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
#include "voting.h"

#include <engine/keys.h>
#include <engine/storage.h>

#include "parser.h"

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
		CUIRect WB, StALogo, RT1, RT2, CP, LF, Tpoints, Tpoints2, Tpoints3, MP1, MP2, LP, PointsS, GR, GRP, PP;
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

		char *pSkinName = g_Config.m_ClPlayerSkin;
		int *pUseCustomColor = &g_Config.m_ClPlayerUseCustomColor;
		unsigned *pColorBody = &g_Config.m_ClPlayerColorBody;
		unsigned *pColorFeet = &g_Config.m_ClPlayerColorFeet;

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

#include "engine/textrender.h"

		SetIconMode(true);
		static CButtonContainer s_RefreshButton;
		if(DoButton_Menu(&s_RefreshButton, "\xEF\x80\x9E", 0, &RefreshButton, 0, IGraphics::CORNER_R))
		{
			//PRAY TO GOD IT WILL WORK LOL
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
		if(DoButton_Menu(&s_DDStatsButton, "DDStats", 0, &DDStatsButton, 0, IGraphics::CORNER_R))
		{
			const char *playerName = Client()->PlayerName();
			char url[256];
			str_format(url, sizeof(url), "https://ddstats.qwik.space/player/%s", playerName);

			if(!open_link(url))
			{
				dbg_msg("menus", "Couldn't open link: %s", url);
			}
		}

		static CButtonContainer s_DDRaceButton;
		if(DoButton_Menu(&s_DDRaceButton, "DDNet", 0, &DDraceButton, 0, IGraphics::CORNER_R))
		{
			const char *playerName = Client()->PlayerName();
			char url[256];
			str_format(url, sizeof(url), "https://ddnet.org/players/%s", playerName);

			if(!open_link(url))
			{
				dbg_msg("menus", "Couldn't open link: %s", url);
			}
		}

		static CButtonContainer s_DiscordButton;
		if(DoButton_Menu(&s_DiscordButton, "Discord", 0, &DiscordButton, 0, IGraphics::CORNER_R))
		{
			char url[256];
			str_format(url, sizeof(url), "https://discord.gg/BhWXQXcgsT");

			if(!open_link(url))
			{
				dbg_msg("menus", "Couldn't open link: %s", url);
			}
		}

		static CButtonContainer s_AboutButton;
		if(DoButton_Menu(&s_AboutButton, "About", 0, &ABOUTButton, 0, IGraphics::CORNER_R))
		{
			char url[256];
			str_format(url, sizeof(url), "https://stormaxs.github.io/StA-site/");

			if(!open_link(url))
			{
				dbg_msg("menus", "Couldn't open link: %s", url);
			}
		}

		char Welcome[128];
		// TODO: fix it somewhen
		const char *names[] =
			{
				"meloƞ", "-StormAx", "我叫芙焦", "Mʎɹ シ", "Cheeru", "Mónik"};

		//same here - use range based for loop
		for(auto &name : names)
		{
			if(strcmp(Client()->PlayerName(), name) == 0)
			{
				break;
			}
			else
			{
				ColorRGBA col = color_cast<ColorRGBA>(ColorHSVA(round_to_int(LocalTime() * 15.f) % 255 / 255.f, 1.f, 1.f));
				TextRender()->TextColor(col);
			}
		}

		ColorRGBA col = color_cast<ColorRGBA>(ColorHSVA(round_to_int(LocalTime() * 15.f) % 255 / 255.f, 1.f, 1.f));

		CStatsPlayer statsPlayer;

		if(strcmp(Client()->PlayerName(), "-StormAx") == 0 || strcmp(Client()->PlayerName(), "meloƞ") == 0 || strcmp(Client()->PlayerName(), "我叫芙焦") == 0 || strcmp(Client()->PlayerName(), "Mʎɹ シ") == 0 || strcmp(Client()->PlayerName(), "Cheeru") == 0 || strcmp(Client()->PlayerName(), "Mónik") == 0)
		{
			TextRender()->TextColor(col);
		}

		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_ACLOGO].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1, 1, 1, 1);
		IGraphics::CQuadItem QuadItem(MainView.w / 2 + 95, 80, 390, 160);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		if(IsParsed == false | IsParsedDDN == false)
		{
			str_format(Welcome, sizeof(Welcome), " Parsing Player Info \n Please wait...", Client()->PlayerName());
			UI()->DoLabel(&PointsS, Welcome, 40.0f, TEXTALIGN_ML);
		}

		if(IsParsed == true | IsParsedDDN == true)
		{
			str_format(Welcome, sizeof(Welcome), " Welcome Back %s", Client()->PlayerName());
			UI()->DoLabel(&WB, Welcome, 40.0f, TEXTALIGN_ML);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

			int playerPoints = s_StatsPlayer.Points;
			{
				char aBuf[32];
				str_format(aBuf, sizeof(aBuf), " Current points: %d", playerPoints);
				UI()->DoLabel(&CP, aBuf, 35.0f, TEXTALIGN_ML);
			}
			int RankPoints = s_StatsPlayer.RankPoints;
			if(RankPoints == 0)
			{
				char aBuf[16];
				str_format(aBuf, sizeof(aBuf), " No Rank Points Yet");
				UI()->DoLabel(&LF, aBuf, 28.0f, TEXTALIGN_MIDDLE);
			}
			else
			{
				char aBuf[32];
				str_format(aBuf, sizeof(aBuf), " Current Rank Points: %d", RankPoints);
				UI()->DoLabel(&LF, aBuf, 35.0f, TEXTALIGN_ML);
			}

			UI()->DoLabel(&MP1, "Most Played Maps", 24.0f, TEXTALIGN_TC);
			MP1.HSplitTop(30.0f, nullptr, &MP1);
			for(int i = 0; i < 11; ++i)
			{
				char aBuf[32];
				str_format(aBuf, sizeof(aBuf), " %s - %.0f hrs.", s_StatsPlayer.aMap[i], s_StatsPlayer.aTime[i]);
				UI()->DoLabel(&MP1, aBuf, 18.0f, TEXTALIGN_TL);
				MP1.HSplitTop(23.0f, nullptr, &MP1);
			}

			char aBuf[32];

			str_format(aBuf, sizeof(aBuf), " Best Region: %s", s_StatsPlayer.PlayTimeLocation);

			if(strcmp(s_StatsPlayer.PlayTimeLocation, "eu") == 0)
			{
				str_format(aBuf, sizeof(aBuf), " Best Region: EU");
			}
			if(strcmp(s_StatsPlayer.PlayTimeLocation, "na") == 0)
			{
				str_format(aBuf, sizeof(aBuf), " Best Region: NA");
			}
			if(strcmp(s_StatsPlayer.PlayTimeLocation, "as:cn") == 0)
			{
				str_format(aBuf, sizeof(aBuf), " Best Region: AS:CN");
			}
			if(strcmp(s_StatsPlayer.PlayTimeLocation, "as") == 0)
			{
				str_format(aBuf, sizeof(aBuf), " Best Region: AS");
			}
			if(strcmp(s_StatsPlayer.PlayTimeLocation, "sa") == 0)
			{
				str_format(aBuf, sizeof(aBuf), " Best Region: SA");
			}
			if(strcmp(s_StatsPlayer.PlayTimeLocation, "unknown") == 0)
			{
				str_format(aBuf, sizeof(aBuf), " Best Region: UNKNOWN");
			}
			if(strcmp(s_StatsPlayer.PlayTimeLocation, "oc") == 0)
			{
				str_format(aBuf, sizeof(aBuf), " Best Region: OC");
			}
			if(strcmp(s_StatsPlayer.PlayTimeLocation, "af") == 0)
			{
				str_format(aBuf, sizeof(aBuf), " Best Region: AF");
			}

			UI()->DoLabel(&Tpoints, aBuf, 24.0f, TEXTALIGN_ML);
		}
		{
			int sum = 0;
			char THP[32];

			//use range based for loop - it's easier to read
			for(int i : s_StatsPlayer.totalPlaytime)
			{
				sum += i;
				str_format(THP, sizeof(THP), " Total Hours Played: %d hrs", sum);
			}
			UI()->DoLabel(&Tpoints3, THP, 30.0f, TEXTALIGN_ML);

			//DDNET============================
			{
				char aBuf[32];

				str_format(aBuf, sizeof(aBuf), " Points rank: #%d ", s_StatsPlayer.PointCategoryDDR);
				UI()->DoLabel(&GRP, aBuf, 35.0f, TEXTALIGN_ML);
			}
			if(s_StatsPlayer.RankInWorld == 0)
			{
				char aBuf[16];
				str_format(aBuf, sizeof(aBuf), " Not Ranked");
				UI()->DoLabel(&GR, aBuf, 28.0f, TEXTALIGN_MIDDLE);
			}
			else
			{
				char aBuf[32];
				str_format(aBuf, sizeof(aBuf), " World Rank: #%d ", s_StatsPlayer.RankInWorld);
				UI()->DoLabel(&GR, aBuf, 28.0f, TEXTALIGN_MIDDLE);
			}
			//Last FINISHED???______________________KURWA STORMAX STOP SWEARING SO MUCH
			UI()->DoLabel(&LP, "Last Finished Maps", 20.0f, TEXTALIGN_TC);
			LP.HSplitTop(21.0f, nullptr, &LP);
			for(int i = 0; i < 7; ++i)
			{
				char aBuf[32];
				str_format(aBuf, sizeof(aBuf), " %s - %s", s_StatsPlayer.LastFinish[i], s_StatsPlayer.aJson[i]);
				UI()->DoLabel(&LP, aBuf, 17.0f, TEXTALIGN_TL);
				LP.HSplitTop(18.0f, nullptr, &LP);
			}

			//TEAMMETES============================================
			UI()->DoLabel(&PP, "Best Teammates", 20.0f, TEXTALIGN_TC);
			PP.HSplitTop(27.0f, nullptr, &PP);
			for(int i = 0; i < 5; ++i)
			{
				char aBuf[32];
				str_format(aBuf, sizeof(aBuf), " %s with %d ranks", s_StatsPlayer.FavouritePartners[i], s_StatsPlayer.BestPartnerFinishes[i]);
				UI()->DoLabel(&PP, aBuf, 17.0f, TEXTALIGN_TL);
				PP.HSplitTop(18.0f, nullptr, &PP);
			}
			{
				char aBuf[32];
				str_format(aBuf, sizeof(aBuf), " Point Last Month: %d", s_StatsPlayer.PLM);
				UI()->DoLabel(&Tpoints2, aBuf, 20.0f, TEXTALIGN_ML);
			}
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
