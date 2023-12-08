#ifndef DDNET_PARSER_H
#define DDNET_PARSER_H

#include <curl/curl.h>
#include <engine/external/json-parser/json.h>
// #include <engine/shared/config.h>

#include <base/math.h>
#include <base/system.h>
#include <iostream>
#include <sstream>

class CJsonFetcher
{
public:
	std::string m_PlayerName;
	int m_Points;

	void Webgrab(const char *PlayerName);
	std::string FetchJsonData(const std::string &url);
	json_value *ParseJsonData(const std::string &jsonData);
	void FreeJsonData(json_value *parsedJson);
	static size_t WriteCallBack(const void *contents, size_t size, size_t nmemb, std::string *output);
	//getter functions
	std::string GetPlayerName();
	int GetPoints();
};


#endif // DDNET_PARSER_H