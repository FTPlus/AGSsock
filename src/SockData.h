/*******************************************************
 * Socket data interface -- header file                *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 16:36 23-8-2012                               *
 *                                                     *
 * Description: Provides a binary data interface for   *
 *              AGS.                                   *
 *******************************************************/

#ifndef _SOCKDATA_H
#define _SOCKDATA_H

#include <string>

namespace AGSSock {

//------------------------------------------------------------------------------
//! Binary data wrapper
struct SockData
{
	std::string data; //!< internal data representation
	
	//! Creates an empty data object
	SockData() {}
	//! Creates a data object of specified length and optionally filled with a specific char value
	SockData(size_t size, char c = '\0') : data(std::string(size, c)) {}
	//! Creates a data object from a string object
	SockData(const std::string &D) : data(D) {}
};

AGS_DEFINE_CLASS(SockData)

//------------------------------------------------------------------------------

SockData *SockData_Create(long, long);
SockData *SockData_CreateEmpty();
SockData *SockData_CreateFromString(const char *);

long SockData_get_Size(SockData *);
void SockData_set_Size(SockData *, long);

// Note: No range checks are preformed for efficiency!
long SockData_geti_Chars(SockData *, long);
void SockData_seti_Chars(SockData *, long, long);

const char *SockData_AsString(SockData *);
void SockData_Clear(SockData *);

//------------------------------------------------------------------------------

} /* namespace AGSSock */

//------------------------------------------------------------------------------
//                           Plugin interface

#define SOCKDATA_HEADER \
	"managed struct SockData\r\n" \
	"{\r\n" \
	"  /// Creates a new data container with specified size (and what character to fill it with).\r\n" \
	"  import static SockData *Create(int size, char defchar = 0); // $AUTOCOMPLETESTATICONLY$\r\n" \
	"  /// Creates a new data container of zero size\r\n" \
	"  import static SockData *CreateEmpty();                      // $AUTOCOMPLETESTATICONLY$\r\n" \
	"  /// Creates a data container from a string.\r\n" \
	"  import static SockData *CreateFromString(String str);       // $AUTOCOMPLETESTATICONLY$\r\n" \
	"  \r\n" \
	"  import attribute int Size;\r\n" \
	"  import attribute char Chars[];\r\n" \
	"  \r\n" \
	"  /// Makes and returns a string from the data object. (Warning: anything after a null character will be truncated)\r\n" \
	"  import String AsString();\r\n" \
	"  /// Removes all the data from a socket data object, reducing its size to zero.\r\n" \
	"  import void Clear();\r\n" \
	"};\r\n" \
	"\r\n"

#define SOCKDATA_ENTRY	                         \
	AGS_CLASS (SockData)                         \
	AGS_METHOD(SockData, Create, 2)              \
	AGS_METHOD(SockData, CreateEmpty, 0)         \
	AGS_METHOD(SockData, CreateFromString, 1)    \
	AGS_MEMBER(SockData, Size)                   \
	AGS_ARRAY (SockData, Chars)                  \
	AGS_METHOD(SockData, AsString, 0)            \
	AGS_METHOD(SockData, Clear, 0)

//------------------------------------------------------------------------------

#endif /* _SOCKDATA_H */

//..............................................................................
