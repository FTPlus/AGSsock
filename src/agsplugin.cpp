/***********************************************************
 * AGS sock                                                *
 *                                                         *
 * Author: Ferry "Wyz" Timmers                             *
 *                                                         *
 * Date: 16-08-11 16:14                                    *
 *                                                         *
 * Description: Network socket support for AGS.            *
 *                                                         *
 ***********************************************************/

#define MIN_EDITOR_VERSION 1
#define MIN_ENGINE_VERSION 18

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#if !defined(_WINDOWS_) && defined(_WIN32)
#define _WINDOWS_
#endif

#define AGSMAIN

#include "API.h"
#include "SockData.h"
#include "SockAddr.h"
#include "Socket.h"
#include "agsplugin.h"
#include "version.h"

DLLEXPORT int AGS_PluginV2() { return 1; }

using namespace AGSSock;

//==============================================================================

// The standard Windows DLL entry point

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
	switch(reason)
	{
		case DLL_PROCESS_ATTACH:
			break;
		
		case DLL_PROCESS_DETACH:
			break;
		
		case DLL_THREAD_ATTACH:
			break;
		
		case DLL_THREAD_DETACH:
			break;
	}
	
	return TRUE;
}

//==============================================================================

// ***** Plugin definition *****

//==============================================================================

// ***** Design time *****

IAGSEditor *editor; // Editor interface

const char *ourScriptHeader = SOCKDATA_HEADER SOCKADDR_HEADER SOCKET_HEADER;

//------------------------------------------------------------------------------

LPCSTR AGS_GetPluginName()
{
	return ("Sockets for AGS");
}

//------------------------------------------------------------------------------

int AGS_EditorStartup(IAGSEditor *lpEditor)
{
	// User has checked the plugin to use it in their game
	
	// If it's an earlier version than what we need, abort.
	if (lpEditor->version < MIN_EDITOR_VERSION)
		return (-1);
	
	editor = lpEditor;
	editor->RegisterScriptHeader(ourScriptHeader);
	
	return (0);
}

//------------------------------------------------------------------------------

void AGS_EditorShutdown()
{
	// User has un-checked the plugin from their game
	editor->UnregisterScriptHeader(ourScriptHeader);
}

//------------------------------------------------------------------------------

void AGS_EditorProperties(HWND parent)
{
	MessageBox(parent, "AGS Sockets plugin by " AUTHORS "; " RELEASE_DATE_STRING
		".", "About", MB_OK | MB_ICONINFORMATION);
}

/*
int AGS_EditorSaveGame(char *buffer, int bufsize) { return (0); }
void AGS_EditorLoadGame(char *buffer, int bufsize) {}
*/

//==============================================================================

// ***** Run time *****

//------------------------------------------------------------------------------

void AGS_EngineStartup(IAGSEngine *lpEngine)
{
	using namespace AGSSockAPI;
	
	engine = lpEngine;
	
	// Make sure it's got the version with the features we need
	if (engine->version < MIN_ENGINE_VERSION)
		engine->AbortGame("Plugin needs engine version " STRINGIFY(MIN_ENGINE_VERSION) " or newer.");
	
	AGSSockAPI::Initialize();
	AGSSock::Initialize();
	
	// Register functions
	SOCKDATA_ENTRY
	SOCKADDR_ENTRY
	SOCKET_ENTRY
}

//------------------------------------------------------------------------------

void AGS_EngineShutdown()
{
	AGSSock::Terminate();
	AGSSockAPI::Terminate();
}

//------------------------------------------------------------------------------
/*
int AGS_EngineOnEvent(int event, int data)
{
	switch (event)
	{
		case AGSE_KEYPRESS:
		case AGSE_MOUSECLICK:
		case AGSE_POSTSCREENDRAW:
		case AGSE_PRESCREENDRAW:
		case AGSE_SAVEGAME:
		case AGSE_RESTOREGAME:
		case AGSE_PREGUIDRAW:
		case AGSE_LEAVEROOM:
		case AGSE_ENTERROOM:
		case AGSE_TRANSITIONIN:
		case AGSE_TRANSITIONOUT:
		case AGSE_FINALSCREENDRAW:
		case AGSE_TRANSLATETEXT:
		case AGSE_SCRIPTDEBUG:
		case AGSE_SPRITELOAD:
		case AGSE_PRERENDER:
		case AGSE_PRESAVEGAME:
		case AGSE_POSTRESTOREGAME:
		default:
			break;
	}
	
	// Return 1 to stop event from processing further (when needed)
	return (0);
}
*/
//------------------------------------------------------------------------------
/*
int AGS_EngineDebugHook(const char *scriptName, int lineNum, int reserved) {}
void AGS_EngineInitGfx(const char *driverID, void *data) {}
*/
//..............................................................................
