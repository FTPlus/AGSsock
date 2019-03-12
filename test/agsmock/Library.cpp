/*****************************************************************************
 * Dynamic linked library interface -- See header file for more information. *
 *****************************************************************************/

#ifdef _WIN32
	#include <windows.h>
#else
	#include <dlfcn.h>
	#include <string>
#endif

#include "Library.h"

namespace AGSMock {

//------------------------------------------------------------------------------

Library::Library(const char *name)
#ifdef _WIN32
	: handle((void *) LoadLibrary(name))
#else
	: handle(dlopen((std::string("./lib") + name + ".so").c_str(), RTLD_LAZY))
#endif
 	{}

Library::~Library()
{
	if (handle != nullptr)
	{
	#ifdef _WIN32
		FreeLibrary((HMODULE) handle);
	#else
		dlclose(handle);
	#endif
	}
}

bool Library::bind(void **ptr, const char *name) const
{
	void *addr = nullptr;
#ifdef _WIN32
	if ((addr = (void *) GetProcAddress((HMODULE) handle, name)) == nullptr)
		return false;
#else
	dlerror();
	addr = dlsym(handle, name);
	if (dlerror() != nullptr)
		return false;
#endif
	*ptr = addr;
	return true;
}

//------------------------------------------------------------------------------

} // namespace AGSMock

//..............................................................................
