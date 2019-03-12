/*****************************************************************
 * Mockup AGS interface -- See header file for more information. *
 *****************************************************************/

#include <memory>
#include <string>
#include <list>
#include <utility>

#include "agsmock.h"
#include "engine.h"
#include "Library.h"

namespace AGSMock {

using std::string;
using std::unique_ptr;
using std::list;

//------------------------------------------------------------------------------

static MockEditor *editor = nullptr;
static MockEngine *engine = nullptr;
static list<unique_ptr<Library>> plugins;

void Initialize()
{
	editor = new MockEditor();
	engine = new MockEngine();
}

void Terminate()
{
	UnloadPlugins();
	delete engine;
	engine = nullptr;
	delete editor;
	editor = nullptr;
}

//------------------------------------------------------------------------------

void LoadPlugin(const char *name)
{
	unique_ptr<Library> library(new Library(name));
	if (!*library)
		throw PluginError("unable to load dynamic library.");

	int (*PluginV2)();
	if (!library->bind(&PluginV2, "AGS_PluginV2"))
		throw PluginError("dynamic library does not appear to be an "
			"AGS plugin (version 2).");

	int (*EditorStartup)(IAGSEditor *);
	int (*EditorShutdown)(void);
	void (*EngineStartup)(IAGSEngine *);
	void (*EngineShutdown)(void);

	if (!library->bind(&EditorStartup, "AGS_EditorStartup")
		|| !library->bind(&EditorShutdown, "AGS_EditorShutdown")
		|| !library->bind(&EngineStartup, "AGS_EngineStartup")
		|| !library->bind(&EngineShutdown, "AGS_EngineShutdown"))
		throw PluginError("dynamic library has missing symbols.");

	EditorStartup(editor);
	EngineStartup(engine);

	plugins.push_front(std::move(library));
}

void UnloadPlugins()
{
	for (unique_ptr<Library> &plugin : plugins)
	{
		int (*EditorShutdown)(void);
		void (*EngineShutdown)(void);

		plugin->bind(&EditorShutdown, "AGS_EditorShutdown");
		plugin->bind(&EngineShutdown, "AGS_EngineShutdown");

		EngineShutdown();
		EditorShutdown();
	}
	plugins.clear();
}

//------------------------------------------------------------------------------

void *GetFunction(const char *name)
{
	return engine->get_function(name);
}

void Free(void *ptr)
{
	engine->free(ptr);
}

//==============================================================================

Unimplemented::Unimplemented(const char *name)
	: logic_error(string("Unimplemented AGSMock function called: ") + name) {}

PluginError::PluginError(const char *reason)
	: runtime_error(string("AGSMock failed to load plug-in: ") + reason) {}

MissingFunction::MissingFunction(const char *name)
	: runtime_error(string("Tried to call undefined script function: ") + name) {}

GameAborted::GameAborted(const char *reason)
	: runtime_error(string("Plugin called 'AbortGame': ") + reason) {}

//------------------------------------------------------------------------------

} // namespace AGSMock

//..............................................................................
