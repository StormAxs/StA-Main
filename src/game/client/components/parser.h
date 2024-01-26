
#ifndef GAME_CLIENT_COMPONENTS_STATS_H
#define GAME_CLIENT_COMPONENTS_STATS_H

#include "engine/shared/protocol.h"
#include <engine/shared/http.h>
#include <engine/shared/jobs.h>
#include <game/client/component.h>

enum
{
	MOST_PLAYED_LENGTH = 128
};

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
	int timestamp;
	char aMap[11][MOST_PLAYED_LENGTH];
	float aTime[11];
	int totalPlaytime[15];
	int pPlaytime;
	int RankPoints;

	char aJson[7][128];

	//DDNet
	char dPlayer[MAX_NAME_LENGTH];
	int PLM;
	int PointCategoryDDR;
	int RankInWorld;
	char LastFinish[7][MOST_PLAYED_LENGTH];
	float LastFinishTime[7];
	char FavouritePartners[5][MAX_NAME_LENGTH];
	int BestPartnerFinishes[5];
	char PlayTimeLocation[MOST_PLAYED_LENGTH];

	void Reset()
	{
		StatsParsed = false;
		m_pGetStatsDDStats.reset();
		m_pGetStatsDDNet.reset();
	}
};

class CStats : public CComponent
{
public:
	CStats();
	int Sizeof() const override { return sizeof(*this); }
	void FetchPlayer(CStatsPlayer *pStatsDest, const char *pPlayer);
	void ParseJSON(CStatsPlayer *pStatsDest);
};

#endif // GAME_CLIENT_COMPONENTS_STATS_H