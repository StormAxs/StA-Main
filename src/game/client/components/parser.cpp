#include "parser.h"
#include "base/system.h"
#include "engine/engine.h"
#include "engine/shared/http.h"
#include <engine/external/json-parser/json.h>

	CStats::CStats() = default;

	void CStats::FetchPlayer(CStatsPlayer *pStatsDest, const char *pPlayer)
	{
		char aUrl[256];
		char aEscapedName[MAX_NAME_LENGTH];
		EscapeUrl(aEscapedName, sizeof(aEscapedName), pPlayer);
		str_format(aUrl, sizeof(aUrl), "%s%s", STATS_URL, aEscapedName);
    	pStatsDest->m_pGetStats = HttpGet(aUrl);

			// 10 seconds connection timeout, lower than 8KB/s for 10 seconds to fail.
			pStatsDest->m_pGetStats->Timeout(CTimeout{10000, 0, 8000, 10});
		Engine()->AddJob(pStatsDest->m_pGetStats);
		pStatsDest->StatsParsed = false;
}

	void CStats::ParseJSON(CStatsPlayer *pStatsDest)
	{
		// TODO error type validation
			json_value *pPlayerStats = pStatsDest->m_pGetStats->ResultJson();
		if(!pPlayerStats)
			{
				dbg_msg("stats", "Invalid JSON received");
				return;
			}

			json_value &PlayerStats = *pPlayerStats;
		const json_value &Player = PlayerStats["player"];
		str_copy(pStatsDest->aPlayer, Player);

    const json_value &PointsCategories = PlayerStats["points"];
	    const json_value &PointsCategory = PointsCategories["points"];
		const json_value &Points = PointsCategory["total"];
		const json_value &PointsTotal = Points["points"];

			pStatsDest->Points = PointsTotal.u.integer;

			json_value_free(pPlayerStats);
		pStatsDest->StatsParsed = true;
	}