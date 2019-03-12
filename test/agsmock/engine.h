/*******************************************************
 * Mockup AGS engine -- header file                    *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 15:47 2019-3-10                               *
 *                                                     *
 * Description: A mockup AGS Engine implementation     *
 *******************************************************/

#ifndef _ENGINE_H
#define _ENGINE_H

#include "agsmock.h"
#include "agsplugin.h"

namespace AGSMock {

//------------------------------------------------------------------------------

class MockEditor : public IAGSEditor
{
	public:
	MockEditor();

	AGSIFUNC(void) RegisterScriptHeader(const char *header);
	AGSIFUNC(void) UnregisterScriptHeader(const char *header);
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

class MockEngine : public IAGSEngine
{
	struct Data;
	Data *data_;

	public:
	MockEngine();
	~MockEngine();

	void *get_function(const char *);
	void free(void *object, bool force = false);
	void free_all();

	AGSIFUNC(void) AbortGame(const char *reason);
	AGSIFUNC(void) RegisterScriptFunction(const char *name, void *address);

	AGSIFUNC(int) RegisterManagedObject(const void *object, IAGSScriptManagedObject *callback);
	AGSIFUNC(void) AddManagedObjectReader(const char *typeName, IAGSManagedObjectReader *reader);

	AGSIFUNC(const char*) CreateScriptString(const char *fromText);
};

//------------------------------------------------------------------------------

} // namespace AGSMock

#endif // _ENGINE_H

//..............................................................................
