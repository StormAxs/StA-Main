#include "parser.h"

void CJsonFetcher::Webgrab(const char *PlayerName)
{
	m_PlayerName = PlayerName;
	std::stringstream GrabURL;
	GrabURL << "https://ddnet.org/players/?json2=" << m_PlayerName;
	std::string jsonData = FetchJsonData(GrabURL.str());
	json_value *parsedJson = ParseJsonData(jsonData);
	FreeJsonData(parsedJson);
	dbg_msg("stats", "Webgrab done successfully");
}

std::string CJsonFetcher::FetchJsonData(const std::string &url)
{
	std::string jsonData;
	CURL *curl = curl_easy_init();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallBack);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonData);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	return jsonData;
}

json_value *CJsonFetcher::ParseJsonData(const std::string &jsonData)
{
	json_settings settings;
	memset(&settings, 0, sizeof(settings));
	settings.settings = json_enable_comments;
	char error[256];
	json_value *json = json_parse_ex(&settings, jsonData.c_str(), jsonData.length(), error);
	if(json == nullptr)
	{
		dbg_msg("stats", "Error parsing json: %s", error);
	}
	if(json != nullptr && json->type == json_object)
	{
		json_value *playerValue = nullptr;
		json_value *pointsValue = nullptr;

		// find "player" "points" and "maps" values
		for(unsigned int i = 0; i < json->u.object.length; ++i)
		{
			if(strcmp(json->u.object.values[i].name, "player") == 0)
			{
				playerValue = json->u.object.values[i].value;
				const char *player = playerValue->u.string.ptr;
				m_PlayerName = player;

			}
			else if(strcmp(json->u.object.values[i].name, "points") == 0)
			{
				pointsValue = json->u.object.values[i].value;

			}
		}
		if(playerValue != nullptr && playerValue->type == json_object)
		{
			json_value *nameValue = nullptr;

			// Find the "player" value within the object
			for(unsigned int i = 0; i < playerValue->u.object.length; ++i)
			{
				if(strcmp(playerValue->u.object.values[i].name, "player") == 0)
				{
					nameValue = playerValue->u.object.values[i].value;
				}
			}
			{
				const char *name = nameValue->u.string.ptr;
				m_PlayerName = name;
				dbg_msg("stats", "Current Name: %s", name);
			}
		}
		if(pointsValue != nullptr && pointsValue->type == json_object)
		{
			json_value *totalValue = nullptr;

			// Find the "total" value within the "points" object
			for(unsigned int i = 0; i < pointsValue->u.object.length; ++i)
			{
				if(strcmp(pointsValue->u.object.values[i].name, "points") == 0)
				{
					totalValue = pointsValue->u.object.values[i].value;
				}
			}
			{
				int totalPoints = totalValue->u.integer;
				m_Points = totalPoints;
				dbg_msg("stats", "Current Points: %d", totalPoints);
			}
		}
	}

	return json;
}

int CJsonFetcher::GetPoints()
{
	return m_Points;
}

std::string CJsonFetcher::GetPlayerName()
{
	return m_PlayerName;
}

void CJsonFetcher::FreeJsonData(json_value *parsedJson)
{
	json_value_free(parsedJson);
}

size_t CJsonFetcher::WriteCallBack(const void *contents, size_t size, size_t nmemb, std::string *output)
{
	size_t totalSize = size * nmemb;
	output->append(static_cast<const char *>(contents), totalSize);
	return totalSize;
}