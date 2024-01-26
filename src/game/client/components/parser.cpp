#include "parser.h"
#include "base/system.h"
#include "engine/engine.h"
#include "engine/shared/http.h"
#include <chrono>
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
	// since player is in the first column - do it like this:
	const json_value &Player = PlayerStats["player"];
	str_copy(pStatsDest->aPlayer, Player);

	// get the total points of the PointsCategory in DDStats
	const json_value &PointsCategories = PlayerStats["points"];
	const json_value &PointsCategory = PointsCategories["points"];
	const json_value &Points = PointsCategory["total"];
	const json_value &PointsTotal = Points["points"];

	pStatsDest->Points = PointsTotal.u.integer;
	// MPM===============================================
	const json_value &MostPlayedMaps = PlayerStats["mostPlayedMaps"];

	for(int i = 0; i < 11; ++i)
	{
		const json_value &map = MostPlayedMaps[i];
		str_copy(pStatsDest->aMap[i], map["map"].u.string.ptr);
		pStatsDest->aTime[i] = map["Playtime"].u.integer / 60 / 60;
	}
	//

	// rankpoints
	const json_value &PointsRankCategories = PlayerStats["points"];
	const json_value &PointsRankCategory = PointsRankCategories["rankpoints"];
	const json_value &RankPoints = PointsRankCategory["total"];
	const json_value &RankPointsTotal = RankPoints["points"];

	pStatsDest->RankPoints = RankPointsTotal.u.integer;
	// total playtime
	const json_value &PlayerTime = PlayerStats["playtimeGametypes"];

	for(int i = 0; i < 15; i++)
	{
		const json_value &pTime = PlayerTime[i];
		pStatsDest->totalPlaytime[i] = pTime["Playtime"].u.integer;
	}

	// F***ING
	const json_value &PlayTimeLocateCategory = PlayerStats["playtimeLocation"];

	if(PlayTimeLocateCategory.type == json_array)
	{
		const json_value &PlayTimeLoc = PlayTimeLocateCategory[0];

		if(PlayTimeLoc["location"].type == json_string && PlayTimeLoc["location"].u.string.ptr != nullptr)
		{
			str_copy(pStatsDest->PlayTimeLocation, PlayTimeLoc["location"].u.string.ptr);
		}
	}

	json_value *dPlayerStats = pStatsDest->m_pGetStatsDDNet->ResultJson();

	if(!dPlayerStats)
	{
		dbg_msg("DDnet.org", "Invalid JSON received");
		return;
	}

	json_value &dPlayerStat = *dPlayerStats;
	const json_value &ddrPlayer = dPlayerStat["player"];
	str_copy(pStatsDest->dPlayer, ddrPlayer);

	// get rank lead
	const json_value &PointCategoryDDR = dPlayerStat["points"];
	const json_value &CurrentPRank = PointCategoryDDR["rank"];
	pStatsDest->PointCategoryDDR = CurrentPRank.u.integer; // Rank ddnet
	// world rank
	const json_value &RankInWorld = dPlayerStat["rank"];
	const json_value &CurrentTopWRLD = RankInWorld["rank"];
	pStatsDest->RankInWorld = CurrentTopWRLD.u.integer; // Rank pts
	//PTS for last month
	const json_value &PLM = dPlayerStat["points_last_month"];
	const json_value &PointsMonth = PLM["points"];
	pStatsDest->PLM = PointsMonth.u.integer; // pts last month
	// Last Finishes
	json_value &LastFinishes = *dPlayerStats;
	// get the total points of the PointsCategory in DDStats
	const json_value &LastFinish = LastFinishes["last_finishes"];

	for(int i = 0; i < 7; ++i)
	{
		const json_value &LFM = LastFinish[i];
		str_copy(pStatsDest->LastFinish[i], LFM["map"].u.string.ptr);
	}

	for(int i = 0; i < 7; ++i)
	{
		const json_value &LFM = LastFinish[i];
		pStatsDest->LastFinishTime[i] = LFM["time"].u.dbl / 60;

		// Assuming you have a Unix timestamp
		time_t unixTimestamp = static_cast<time_t>(LFM["time"].u.dbl);

		// Convert Unix timestamp to struct tm in UTC
		struct tm *timeinfo = std::gmtime(&unixTimestamp);

		// Format the time as a string
		std::strftime(pStatsDest->aJson[i], sizeof(pStatsDest->aJson[i]), "%H:%M:%S", timeinfo);
		//thanks, GPT lol
	}

	json_value &Partner = *dPlayerStats;

	const json_value &FavPartners = Partner["favorite_partners"];
	for(int i = 0; i < 5; ++i)
	{
		const json_value &BestParner = FavPartners[i];
		str_copy(pStatsDest->FavouritePartners[i], BestParner["name"].u.string.ptr);
		pStatsDest->BestPartnerFinishes[i] = BestParner["finishes"].u.integer;
	}
}
