#include "parser.h"
#include "base/system.h"
#include "engine/engine.h"
#include "engine/shared/http.h"
#include <engine/external/json-parser/json.h>

CStats::CStats() = default;

void CStats::FetchPlayer(CStatsPlayer *pStatsDest, const char *pPlayer)
{
	char aUrl_DDStats[256];
	char aUrl_DDNet[256];
	char aEscapedName[MAX_NAME_LENGTH];
	EscapeUrl(aEscapedName, sizeof(aEscapedName), pPlayer);
	str_format(aUrl_DDStats, sizeof(aUrl_DDStats), "%s%s", STATS_URL_DDSTATS, aEscapedName);
	pStatsDest->m_pGetStatsDDStats = HttpGet(aUrl_DDStats);

	str_format(aUrl_DDNet, sizeof(aUrl_DDNet), "%s%s", STATS_URL_DDNET, aEscapedName);
	pStatsDest->m_pGetStatsDDNet = HttpGet(aUrl_DDNet);

	// 10 seconds connection timeout, lower than 8KB/s for 10 seconds to fail.
	pStatsDest->m_pGetStatsDDStats->Timeout(CTimeout{10000, 0, 8000, 10});
	Engine()->AddJob(pStatsDest->m_pGetStatsDDStats);

	pStatsDest->m_pGetStatsDDNet->Timeout(CTimeout{10000, 0, 8000, 10});
	Engine()->AddJob(pStatsDest->m_pGetStatsDDNet);
	pStatsDest->StatsParsed = false;
}

void CStats::ParseJSON(CStatsPlayer *pStatsDest)
{
	// TODO error type validation
	json_value *pPlayerStats = pStatsDest->m_pGetStatsDDStats->ResultJson();
	if(!pPlayerStats)
	{
		dbg_msg("stats", "Invalid JSON received");
		return;
	}

	json_value &PlayerStats = *pPlayerStats;
	//since player is in the first column - do it like this:
	const json_value &Player = PlayerStats["player"];
	str_copy(pStatsDest->aPlayer, Player);

	//get the total points of the PointsCategory in DDStats
	const json_value &PointsCategories = PlayerStats["points"];
	const json_value &PointsCategory = PointsCategories["points"];
	const json_value &Points = PointsCategory["total"];
	const json_value &PointsTotal = Points["points"];

	pStatsDest->Points = PointsTotal.u.integer;

/*
	json_value &MostPlayedMaps = *pPlayerStats;
	//get the total points of the PointsCategory in DDStats
	const json_value &MPM = MostPlayedMaps["mostPlayedMaps"];
	const json_value &FirstMap = MPM["map"];

	pStatsDest->aMap = FirstMap.u.object;
*/

//rankpoints
	const json_value &PointsRankCategories = PlayerStats["points"];
	const json_value &PointsRankCategory = PointsRankCategories["rankpoints"];
	const json_value &RankPoints = PointsRankCategory["total"];
	const json_value &RankPointsTotal = RankPoints["points"];

	pStatsDest->RankPoints = RankPointsTotal.u.integer;

	json_value_free(pPlayerStats);

	pStatsDest->StatsParsed = true;



/*
 	//same for ddnet.org
	json_value *pPlayerDDNet = pStatsDest->m_pGetStatsDDNet->ResultJson();
	if(!pPlayerDDNet)
	{
		dbg_msg("stats", "Invalid JSON received");
		return;
	}

	json_value &PlayerDDNet = *pPlayerDDNet;
	//since player is in the first column - do it like this:
	const json_value &PlayerD = PlayerDDNet["player"];
	str_copy(pStatsDest->aPlayer, PlayerD);


	const json_value &LastFinishes = PlayerD["last_finishes"];

	json_value_free(pPlayerDDNet);
	pStatsDest->StatsParsed = true;
*/
}