/**************************************************************
 * Mockup AGS engine -- See header file for more information. *
 **************************************************************/

#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

#include "engine.h"

namespace AGSMock {

using std::string;
using std::unordered_map;
using std::map;

//------------------------------------------------------------------------------

struct StringType
	: public IAGSScriptManagedObject, public IAGSManagedObjectReader
{
	virtual const char *GetType()
		{ return "String"; }
	virtual int Dispose(const char *address, bool force)
		{ free(const_cast<char *> (address)); return 1; }

	// We do not serialize strings as they are pseudo objects
	virtual int Serialize(const char *address, char *buffer, int bufsize)
		{ return 0; }
	virtual void Unserialize(int key, const char *serializedData, int dataSize)
		{}
} ScriptString;

//------------------------------------------------------------------------------

MockEditor::MockEditor()
{
	version = 9999;
	pluginId = 0;
}

void MockEditor::RegisterScriptHeader(const char *header) {}
void MockEditor::UnregisterScriptHeader(const char *header) {}

//------------------------------------------------------------------------------

struct MockEngine::Data
{
	struct Resource
	{
		int count;
		int key;
		IAGSScriptManagedObject *callback;
	};
	unordered_map<string, IAGSManagedObjectReader *> readers;
	unordered_map<string, void *> functions;
	unordered_map<void *, Resource> objects;

	static int get_unique_key()
	{
		static int key = 0;
		return ++key;
	}
};

MockEngine::MockEngine()
	: data_(new Data())
{
	version = 9999;
	pluginId = 0;
}

MockEngine::~MockEngine()
{
	free_all();
	delete data_;
	data_ = nullptr;
}

void *MockEngine::get_function(const char *name)
{
	if (data_->functions.count(name) < 1)
		return nullptr;
	return data_->functions[name];
}

void MockEngine::free(void *object, bool force)
{
	decltype(data_->objects)::iterator it = data_->objects.find(object);

	if (it == data_->objects.end())
		return;

	if (!force && --it->second.count > 0)
		return;

	if (!it->second.callback->Dispose((const char *) object, force ? 1 : 0))
		return;

	data_->objects.erase(it);
}

void MockEngine::free_all()
{
	map<int, void *> object_list;
	for (auto &object : data_->objects)
		object_list[object.second.key] = object.first;

	for (auto &object : object_list)
		free(object.second);

	if (data_->objects.size() > 0)
	{
		using namespace std;
		cout << endl << "Warning: some resource persisted disposal." << endl;

		object_list.clear();
		for (auto &object : data_->objects)
			object_list[object.second.key] = object.first;

		for (auto &object : object_list)
			free(object.second, true);
	}

	data_->objects.clear();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

void MockEngine::AbortGame(const char *reason)
{
	throw GameAborted(reason);
}

void MockEngine::RegisterScriptFunction(const char *name, void *address)
{
	data_->functions[name] = address;
}

int MockEngine::RegisterManagedObject(const void *object, IAGSScriptManagedObject *callback)
{
	int key = Data::get_unique_key();
	data_->objects[const_cast<void *> (object)] = {1, key, callback};
	return key;
}

void MockEngine::AddManagedObjectReader(const char *typeName, IAGSManagedObjectReader *reader)
{
	data_->readers[typeName] = reader;
}

const char *MockEngine::CreateScriptString(const char *fromText)
{
	char *str = strdup(fromText);
	data_->objects[(void *) str] = {1, Data::get_unique_key(), &ScriptString};
	return str;
}

int MockEngine::IncrementManagedObjectRefCount(const char *address)
{
	if (data_->objects.count((void *) address) < 1)
		return 0;

	return ++data_->objects[(void *) address].count;
}

int MockEngine::DecrementManagedObjectRefCount(const char *address)
{
	if (data_->objects.count((void *) address) < 1)
		return -1;

	Data::Resource &res = data_->objects[(void *) address];
	int count = --res.count;

	if (count < 1)
		free((void *) address, false);

	return count;
}

//------------------------------------------------------------------------------

} // namespace AGSMock

//==============================================================================

#define EDITOR_NOT_IMPL(name, args) \
	IAGSEditor::name args { throw AGSMock::Unimplemented(#name); }

#define NOT_IMPL(name, args) \
	IAGSEngine::name args { throw AGSMock::Unimplemented(#name); }

//------------------------------------------------------------------------------

HWND EDITOR_NOT_IMPL(GetEditorHandle, ());
HWND EDITOR_NOT_IMPL(GetWindowHandle, ());
void EDITOR_NOT_IMPL(RegisterScriptHeader, (const char *header));
void EDITOR_NOT_IMPL(UnregisterScriptHeader, (const char *header));

//------------------------------------------------------------------------------

void NOT_IMPL(AbortGame, (const char *reason));

const char* NOT_IMPL(GetEngineVersion, ());

void NOT_IMPL(RegisterScriptFunction, (const char *name, void *address));

#ifdef WINDOWS_VERSION
	HWND NOT_IMPL(GetWindowHandle, ());
	LPDIRECTDRAW2 NOT_IMPL(GetDirectDraw2, ());
	LPDIRECTDRAWSURFACE2 NOT_IMPL(GetBitmapSurface, (BITMAP *));
#endif
BITMAP * NOT_IMPL(GetScreen, ());

void NOT_IMPL(RequestEventHook, (int32 event));
int NOT_IMPL(GetSavedData, (char *buffer, int32 bufsize));

BITMAP * NOT_IMPL(GetVirtualScreen, ());
void NOT_IMPL(DrawText, (int32 x, int32 y, int32 font, int32 color, char *text));
void NOT_IMPL(GetScreenDimensions, (int32 *width, int32 *height, int32 *coldepth));
unsigned char** NOT_IMPL(GetRawBitmapSurface, (BITMAP *));
void NOT_IMPL(ReleaseBitmapSurface, (BITMAP *));
void NOT_IMPL(GetMousePosition, (int32 *x, int32 *y));

int NOT_IMPL(GetCurrentRoom, ());
int NOT_IMPL(GetNumBackgrounds, ());
int NOT_IMPL(GetCurrentBackground, ());
BITMAP * NOT_IMPL(GetBackgroundScene, (int32));
void NOT_IMPL(GetBitmapDimensions, (BITMAP *bmp, int32 *width, int32 *height, int32 *coldepth));

int NOT_IMPL(FWrite, (void *, int32, int32));
int NOT_IMPL(FRead, (void *, int32, int32));
void NOT_IMPL(DrawTextWrapped, (int32 x, int32 y, int32 width, int32 font, int32 color, const char *text));
void NOT_IMPL(SetVirtualScreen, (BITMAP *));
int NOT_IMPL(LookupParserWord, (const char *word));
void NOT_IMPL(BlitBitmap, (int32 x, int32 y, BITMAP *, int32 masked));
void NOT_IMPL(PollSystem, ());

int NOT_IMPL(GetNumCharacters, ());
AGSCharacter* NOT_IMPL(GetCharacter, (int32));
AGSGameOptions* NOT_IMPL(GetGameOptions, ());
AGSColor* NOT_IMPL(GetPalette,());
void NOT_IMPL(SetPalette, (int32 start, int32 finish, AGSColor*));

int NOT_IMPL(GetPlayerCharacter, ());
void NOT_IMPL(RoomToViewport, (int32 *x, int32 *y));
void NOT_IMPL(ViewportToRoom, (int32 *x, int32 *y));
int NOT_IMPL(GetNumObjects, ());
AGSObject* NOT_IMPL(GetObject, (int32));
BITMAP * NOT_IMPL(GetSpriteGraphic, (int32));
BITMAP * NOT_IMPL(CreateBlankBitmap, (int32 width, int32 height, int32 coldep));
void NOT_IMPL(FreeBitmap, (BITMAP *));

BITMAP * NOT_IMPL(GetRoomMask, (int32));

AGSViewFrame * NOT_IMPL(GetViewFrame, (int32 view, int32 loop, int32 frame));
int NOT_IMPL(GetWalkbehindBaseline, (int32 walkbehind));
void * NOT_IMPL(GetScriptFunctionAddress, (const char * funcName));
int NOT_IMPL(GetBitmapTransparentColor, (BITMAP *));
int NOT_IMPL(GetAreaScaling,  (int32 x, int32 y));
int NOT_IMPL(IsGamePaused, ());

int NOT_IMPL(GetRawPixelColor, (int32 color));

int NOT_IMPL(GetSpriteWidth, (int32));
int NOT_IMPL(GetSpriteHeight, (int32));
void NOT_IMPL(GetTextExtent, (int32 font, const char *text, int32 *width, int32 *height));
void NOT_IMPL(PrintDebugConsole, (const char *text));
void NOT_IMPL(PlaySoundChannel, (int32 channel, int32 soundType, int32 volume, int32 loop, const char *filename));
int NOT_IMPL(IsChannelPlaying, (int32 channel));

void NOT_IMPL(MarkRegionDirty, (int32 left, int32 top, int32 right, int32 bottom));
AGSMouseCursor * NOT_IMPL(GetMouseCursor, (int32 cursor));
void NOT_IMPL(GetRawColorComponents, (int32 coldepth, int32 color, int32 *red, int32 *green, int32 *blue, int32 *alpha));
int NOT_IMPL(MakeRawColorPixel, (int32 coldepth, int32 red, int32 green, int32 blue, int32 alpha));
int NOT_IMPL(GetFontType, (int32 fontNum));
int NOT_IMPL(CreateDynamicSprite, (int32 coldepth, int32 width, int32 height));
void NOT_IMPL(DeleteDynamicSprite, (int32 slot));
int NOT_IMPL(IsSpriteAlphaBlended, (int32 slot));

void NOT_IMPL(UnrequestEventHook, (int32 event));
void NOT_IMPL(BlitSpriteTranslucent, (int32 x, int32 y, BITMAP *, int32 trans));
void NOT_IMPL(BlitSpriteRotated, (int32 x, int32 y, BITMAP *, int32 angle));

#ifdef WINDOWS_VERSION
	LPDIRECTSOUND NOT_IMPL(GetDirectSound, ());
#endif
void NOT_IMPL(DisableSound, ());
int NOT_IMPL(CanRunScriptFunctionNow, ());
int NOT_IMPL(CallGameScriptFunction, (const char *name, int32 globalScript, int32 numArgs, int32 arg1, int32 arg2, int32 arg3));

void NOT_IMPL(NotifySpriteUpdated, (int32 slot));
void NOT_IMPL(SetSpriteAlphaBlended, (int32 slot, int32 isAlphaBlended));
void NOT_IMPL(QueueGameScriptFunction, (const char *name, int32 globalScript, int32 numArgs, int32 arg1, int32 arg2));
int NOT_IMPL(RegisterManagedObject, (const void *object, IAGSScriptManagedObject *callback));
void NOT_IMPL(AddManagedObjectReader, (const char *typeName, IAGSManagedObjectReader *reader));
void NOT_IMPL(RegisterUnserializedObject, (int key, const void *object, IAGSScriptManagedObject *callback));

void* NOT_IMPL(GetManagedObjectAddressByKey, (int key));
int NOT_IMPL(GetManagedObjectKeyByAddress, (const char *address));

const char* NOT_IMPL(CreateScriptString, (const char *fromText));

int NOT_IMPL(IncrementManagedObjectRefCount, (const char *address));
int NOT_IMPL(DecrementManagedObjectRefCount, (const char *address));
void NOT_IMPL(SetMousePosition, (int32 x, int32 y));
void NOT_IMPL(SimulateMouseClick, (int32 button));
int NOT_IMPL(GetMovementPathWaypointCount, (int32 pathId));
int NOT_IMPL(GetMovementPathLastWaypoint, (int32 pathId));
void NOT_IMPL(GetMovementPathWaypointLocation, (int32 pathId, int32 waypoint, int32 *x, int32 *y));
void NOT_IMPL(GetMovementPathWaypointSpeed, (int32 pathId, int32 waypoint, int32 *xSpeed, int32 *ySpeed));

const char* NOT_IMPL(GetGraphicsDriverID, ());

int NOT_IMPL(IsRunningUnderDebugger, ());
void NOT_IMPL(BreakIntoDebugger, ());
void NOT_IMPL(GetPathToFileInCompiledFolder, (const char* fileName, char* buffer));

#ifdef WINDOWS_VERSION
	LPDIRECTINPUTDEVICE NOT_IMPL(GetDirectInputKeyboard, ());
	LPDIRECTINPUTDEVICE NOT_IMPL(GetDirectInputMouse, ());
#endif
IAGSFontRenderer* NOT_IMPL(ReplaceFontRenderer, (int fontNumber, IAGSFontRenderer* newRenderer));

//..............................................................................
