/*******************************************************
 * Dynamic linked library interface -- header file     *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 12:20 2019-3-11                               *
 *                                                     *
 * Description: Platform independent interface to load *
 *              and use dynamically linked libraries.  *
 *******************************************************/

#ifndef _LIBRARY_H
#define _LIBRARY_H

namespace AGSMock {

//------------------------------------------------------------------------------

class Library
{
	void *handle;

	bool bind(void **, const char *name) const;

	public:
	Library(const char *name);
	~Library();

	operator bool() const
		{ return handle != nullptr; }

	template <typename F> bool bind(F *func, const char *name) const
		{ return bind(reinterpret_cast<void **> (func), name); }
};

//------------------------------------------------------------------------------

} // namespace AGSMock

#endif // _LIBRARY_H

//..............................................................................
