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
	char aEscapedNameNoSpaces[MAX_NAME_LENGTH * 4];
	EscapeUrl(aEscapedName, sizeof(aEscapedName), pPlayer);
	int j = 0;
	for (int i = 0; aEscapedName[i] != '\0'; i++) {
		if (aEscapedName[i] != ' ') {
			aEscapedNameNoSpaces[j++] = aEscapedName[i]; // copy non-space characters
		}
	}
	aEscapedNameNoSpaces[j] = '\0'; // null-terminate the new string

	str_format(aUrl_DDStats, sizeof(aUrl_DDStats), "%s%s", STATS_URL_DDSTATS, aEscapedNameNoSpaces);
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
	if(!pStatsDest->m_pGetStatsDDStats) {
        dbg_msg("stats", "Failed to fetch stats(DDStats)");
        return;
    }

    if(!pStatsDest->m_pGetStatsDDNet) {
	    dbg_msg("stats", "Failed to fetch stats(DDrace)");
	    return;
    }

    json_value *pPlayerStats = pStatsDest->m_pGetStatsDDStats->ResultJson();

    if (!pPlayerStats || pPlayerStats->type!= json_object) {
        dbg_msg("stats", "Invalid JSON received or unexpected type from DDStats");
        return;
    }

    json_value &PlayerStats = *pPlayerStats;
    //DDStats

    const json_value &Profile = PlayerStats["profile"];
    const json_value &pts = Profile["points"];
    pStatsDest->Points = pts.u.integer;

    json_value &Partner = *pPlayerStats;

    const json_value &FavPartners = Partner["favourite_teammates"];
    for(int i = 0; i < 5 && i < static_cast<int>(FavPartners.u.array.length); ++i) // Ensure not to exceed array bounds
    {
	    const json_value &BestPartner = FavPartners[i];
	    if(BestPartner["name"].type == json_string && BestPartner["ranks_together"].type == json_integer)
	    {
		    str_copy(pStatsDest->FavouritePartners[i], BestPartner["name"].u.string.ptr);
		    pStatsDest->BestPartnerFinishes[i] = BestPartner["ranks_together"].u.integer;
	    }
	    else
		    pStatsDest->BestPartnerFinishes[i] = 0;
    }

    const json_value &MostPlayedMaps = PlayerStats["most_played_maps"];
    if (MostPlayedMaps.type!= json_array) {
	    dbg_msg("stats", "Expected array for MostPlayedMaps, got unexpected type");
	    return;
    }

    for(int i = 0; i < 11; ++i)
    {
	    const json_value &map = MostPlayedMaps[i];
	    if (map.type!= json_object || map["map_name"].type!= json_string || map["seconds_played"].type!= json_integer) {
		    dbg_msg("stats", "Missing or incorrect type in MostPlayedMaps entry %d", i);
		    return;
	    }
	    str_copy(pStatsDest->aMap[i], map["map_name"].u.string.ptr);
	    pStatsDest->aTime[i] = map["seconds_played"].u.integer / 60 / 60;
    }
    
    json_value &mostPlayedGametypes = *pPlayerStats;

    // Parse each "seconds_played" value
    const json_value &MPG = mostPlayedGametypes["most_played_gametypes"];
    for (int i = 0; i < 15; ++i)
    {
	    const json_value &gametype = MPG[i];
	    pStatsDest->totalPlaytime[i] = gametype["seconds_played"].u.dbl;
    }


    double totalPlaytimeSum = 0.0;
    for (int i = 0; i < 20; ++i)
    {
	    const json_value &gametype = MPG[i];
	    pStatsDest->totalPlaytime[i] = gametype["seconds_played"].u.dbl;
	    totalPlaytimeSum += pStatsDest->totalPlaytime[i];
    }
    pStatsDest->pPlaytimeHRS = totalPlaytimeSum / 60 / 60;


    //DDNet
    json_value *dPlayerStats = pStatsDest->m_pGetStatsDDNet->ResultJson();
    if(dPlayerStats == NULL)
	    return;


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
    pStatsDest->RankInWorld = CurrentTopWRLD.u.integer;

    const json_value &PointsInWorld = RankInWorld["points"];
    pStatsDest->RankPoints = PointsInWorld.u.integer;

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
    }


}
