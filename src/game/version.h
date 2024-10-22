/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VERSION_H
#define GAME_VERSION_H
#ifndef GAME_RELEASE_VERSION
#define GAME_RELEASE_VERSION "17.4.2"
#endif
#define GAME_VERSION "0.6.4, " GAME_RELEASE_VERSION
#define GAME_NETVERSION "0.6 626fce9a778df4d4"
#define DDNET_VERSION_NUMBER 17042
#define GAME_STA_VERSION 001
#define STA_VERSION "0.0.1"
extern const char *GIT_SHORTREV_HASH;
#define GAME_NAME "DDNet"
#define STA_BUILD_DATE __DATE__ ", " __TIME__
#endif
