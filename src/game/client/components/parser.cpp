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
	char aEscapedName[MAX_NAME_LENGTH * 4];
	EscapeUrl(aEscapedName, sizeof(aEscapedName), pPlayer);
	str_format(aUrl_DDStats, sizeof(aUrl_DDStats), "%s%s", STATS_URL_DDSTATS, aEscapedName);
	pStatsDest->m_pGetStatsDDStats = HttpGet(aUrl_DDStats);

	str_format(aUrl_DDNet, sizeof(aUrl_DDNet), "%s%s", STATS_URL_DDNET, aEscapedName);
	pStatsDest->m_pGetStatsDDNet = HttpGet(aUrl_DDNet);

	pStatsDest->m_pGetStatsDDStats->Timeout(CTimeout{10000, 0, 8000, 10});
	Engine()->AddJob(pStatsDest->m_pGetStatsDDStats);

	pStatsDest->m_pGetStatsDDNet->Timeout(CTimeout{10000, 0, 8000, 10});
	Engine()->AddJob(pStatsDest->m_pGetStatsDDNet);
	pStatsDest->StatsParsed = false;
}

void CStats::ParseJSON(CStatsPlayer *pStatsDest)
{
	if(!pStatsDest->m_pGetStatsDDStats ||pStatsDest->m_pGetStatsDDNet) {
        dbg_msg("stats", "Failed to fetch stats");
        return;
    }

    json_value *pPlayerStats = pStatsDest->m_pGetStatsDDStats->ResultJson();

    if (!pPlayerStats || pPlayerStats->type!= json_object) {
        dbg_msg("stats", "Invalid JSON received or unexpected type from DDStats");
        return;
    }

    json_value &PlayerStats = *pPlayerStats;
    const json_value &Player = PlayerStats["profile"];
    str_copy(pStatsDest->aPlayer, Player);

    const auto PointsCategories = PlayerStats["points"]["points"]["Total"]["points"];
    if (PointsCategories.type!= json_integer) {
        dbg_msg("stats", "Expected integer for PointsCategories, got unexpected type");
        return;
    }
    const int PointsTotal = PointsCategories.u.integer;

    const json_value &MostPlayedMaps = PlayerStats["most_played_maps"];
    if (MostPlayedMaps.type!= json_array) {
        dbg_msg("stats", "Expected array for MostPlayedMaps, got unexpected type");
        return;
    }

    for(int i = 0; i < 11; ++i)
    {
        const json_value &map = MostPlayedMaps[i];
        if (map.type!= json_object || map["map"].type!= json_string || map["Playtime"].type!= json_integer) {
            dbg_msg("stats", "Missing or incorrect type in MostPlayedMaps entry %d", i);
            return;
        }
        str_copy(pStatsDest->aMap[i], map["map"].u.string.ptr);
        pStatsDest->aTime[i] = map["Playtime"].u.integer / 60 / 60;
    }

	const json_value &PlayTimeLocateCategory = PlayerStats["most_played_locations"];

	if(PlayTimeLocateCategory.type == json_array)
	{
		const json_value &PlayTimeLoc = PlayTimeLocateCategory[0];

		if(PlayTimeLoc["key"].type == json_string && PlayTimeLoc["seconds_played"].u.string.ptr != nullptr)
		{
			str_copy(pStatsDest->PlayTimeLocation, PlayTimeLoc["key"].u.string.ptr);
		}
	}

	json_value *dPlayerStats = pStatsDest->m_pGetStatsDDNet->ResultJson();
    if (!dPlayerStats) {
        dbg_msg("DDnet.org", "Invalid JSON received from DDNet");
        return;
    }
	
	if (dPlayerStats->type!= json_object) {
        dbg_msg("DDnet.org", "Unexpected JSON type from DDNet");
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
	for(int i = 0; i < 5 && i < static_cast<int>(FavPartners.u.array.length); ++i) // Ensure not to exceed array bounds
	{
		const json_value &BestPartner = FavPartners[i];
		if(BestPartner["name"].type == json_string && BestPartner["finishes"].type == json_integer)
		{
			str_copy(pStatsDest->FavouritePartners[i], BestPartner["name"].u.string.ptr);
			pStatsDest->BestPartnerFinishes[i] = BestPartner["finishes"].u.integer;
		}
		else
			pStatsDest->BestPartnerFinishes[i] = 0;
	}
}
