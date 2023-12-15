#ifndef GAME_CLIENT_COMPONENTS_STATS_H
#define GAME_CLIENT_COMPONENTS_STATS_H

	#include "engine/shared/protocol.h"
	#include <engine/shared/http.h>
	#include <engine/shared/jobs.h>
	#include <game/client/component.h>
	static constexpr const char *STATS_URL_DDSTATS = "https://ddstats.qwik.space/player/json?player=";
	static constexpr const char *STATS_URL_DDNET = "https://ddnet.org/players/?json2=";
	class CStatsPlayer
	{
	public:
		std::shared_ptr<CHttpRequest> m_pGetStatsDDStats; // profile and player stats
		std::shared_ptr<CHttpRequest> m_pGetStatsDDNet; // map tracking

		char aPlayer[MAX_NAME_LENGTH];
		int Points;
		bool StatsParsed = false;
	};

	class CStats : public CComponent
		       {
	private:
		  public:
				CStats();
		int Sizeof() const override { return sizeof(*this); }
		void FetchPlayer(CStatsPlayer *pStatsDest, const char *pPlayer);
		void ParseJSON(CStatsPlayer *pStatsDest);
       };

	#endif // GAME_CLIENT_COMPONENTS_STATS_H