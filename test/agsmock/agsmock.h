/*******************************************************
 * Mockup AGS interface -- header file                 *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 12:19 2019-2-14                               *
 *                                                     *
 * Description: A framework that implements the        *
 *              minimal functionality of AGS so AGS    *
 *              plugins can be tested in a consistent  *
 *              manner.                                *
 *******************************************************/

#ifndef _AGSMOCK_H
#define _AGSMOCK_H

#include <stdexcept>

namespace AGSMock {

#ifdef HAVE_INTPTR_T
typedef std::intptr_t ags_t;
#else
typedef long ags_t;
#endif

//------------------------------------------------------------------------------

class Unimplemented : public std::logic_error
{
	public:
	Unimplemented(const char *name);
};

class PluginError : public std::runtime_error
{
	public:
	PluginError(const char *reason);
};

class MissingFunction : public std::runtime_error
{
	public:
	MissingFunction(const char *name);
};

class GameAborted : public std::runtime_error
{
	public:
	GameAborted(const char *reason);
};

//------------------------------------------------------------------------------

void Initialize();
void Terminate();

void LoadPlugin(const char *name);
void UnloadPlugins();

void *GetFunction(const char *name);
template <typename T, typename... Args> T Call(const char *name, Args... args)
{
	void *ptr = GetFunction(name);
	if (ptr == nullptr) throw MissingFunction(name);
	T (*func)(Args...);
	*(reinterpret_cast<void **> (&func)) = ptr;
	return func(args...);
}

void Free(void *);
template <typename T> class Handle
{
	T *ptr_;

	public:
	Handle() : ptr_(nullptr) {}
	Handle(T *ptr) : ptr_(ptr) {}
	Handle(const Handle &) = delete;
	Handle(Handle &&other) : ptr_(other.release()) {}
	~Handle() { reset(); }

	void reset()
		{ if (ptr_ != nullptr) Free((void *) ptr_); ptr_ = nullptr; }
	T *get() const
		{ return ptr_; }
	T *release()
		{ T *ptr = ptr_; ptr_ = nullptr; return ptr; }

	Handle &operator =(const Handle &) = delete;
	Handle &operator =(Handle &&other)
		{ reset(); ptr_ = other.release(); return *this; }
	operator bool() const
		{ return ptr_ != nullptr; }
	T &operator *() const
		{ return *ptr_; }
	T *operator ->() const
		{ return ptr_; }
};

#define AGS_METHOD(classname, name, arity) #classname "::" #name "^" #arity
#define AGS_GET(classname, name) #classname "::get_" #name
#define AGS_SET(classname, name) #classname "::set_" #name
#define AGS_GET_INDEX(classname, name) #classname "::geti_" #name
#define AGS_SET_INDEX(classname, name) #classname "::seti_" #name

//------------------------------------------------------------------------------

} // namespace AGSMock

#endif // _AGSMOCK_H

//..............................................................................
