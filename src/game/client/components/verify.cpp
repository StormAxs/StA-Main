#include <engine/graphics.h>
#include <engine/shared/config.h>

#include "engine/engine.h"
#include <game/client/animstate.h>
#include <game/client/render.h>
#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <game/client/gameclient.h>

#include "verify.h"

bool CVerify::verified = {false};
void CVerify::OnRender()
{
	if(g_Config.m_ClAutoVerify)
	{
		if(verified)
			return;
		
		CTimeout Timeout{10000, 0, 8000, 10};
		CHttpRequest *pGet = HttpGet("https://ger10.ddnet.org/").release();
		pGet->Timeout(Timeout);
		IEngine::RunJobBlocking(pGet);
		
			if(pGet->State() != HTTP_DONE){
			dbg_msg("verify", "Failed to verify client");
			verified = {true};
		}
		else
		{
			unsigned char *cChar[32];
			size_t cSize[32];
			pGet->Result(cChar, cSize);
			dbg_msg("verify", "Verified client to GER10!");
			verified = {true};
		return;
		}
	}
}