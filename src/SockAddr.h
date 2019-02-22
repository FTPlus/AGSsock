/*******************************************************
 * Socket address interface -- header file             *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 17:23 29-6-2012                               *
 *                                                     *
 * Description: Implements BSD sockets style socket    *
 *              address representation.                *
 *******************************************************/

#ifndef _SOCKADDR_H
#define _SOCKADDR_H

#include "API.h"
#include "SockData.h"

namespace AGSSock {

//------------------------------------------------------------------------------

struct SockAddr : public SOCKADDR_STORAGE
{
};

AGS_DEFINE_CLASS(SockAddr)

//------------------------------------------------------------------------------

#define ADDR_SIZE (sizeof (SockAddr))
#define ADDR(x) (reinterpret_cast<sockaddr *> (x))
#define CONST_ADDR(x) (reinterpret_cast<const sockaddr *> (x))

SockAddr *SockAddr_Create(ags_t type);
SockAddr *SockAddr_CreateFromString(const char *, ags_t type);
SockAddr *SockAddr_CreateFromData(const SockData *);
SockAddr *SockAddr_CreateIP(const char *addr, ags_t port);
SockAddr *SockAddr_CreateIPv6(const char *addr, ags_t port);

ags_t SockAddr_get_Port(SockAddr *);
void SockAddr_set_Port(SockAddr *, ags_t);
const char *SockAddr_get_Address(SockAddr *);
void SockAddr_set_Address(SockAddr *, const char *);
const char *SockAddr_get_IP(SockAddr *);
void SockAddr_set_IP(SockAddr *, const char *);

SockData *SockAddr_GetData(SockAddr *);

//------------------------------------------------------------------------------

} /* namespace AGSSock */

//------------------------------------------------------------------------------
//                           Plugin interface


#define SOCKADDR_HEADER \
	"#define IPv4 " STRINGIFY(AF_INET) "\r\n" \
	"#define IPv6 " STRINGIFY(AF_INET6) "\r\n" \
	"\r\n" \
	"managed struct SockAddr\r\n" \
	"{\r\n" \
	"  /// Creates an empty socket address. (advanced: set type to IPv6 if you're using IPv6).\r\n" \
	"  import static SockAddr *Create(int type = IPv4);                           // $AUTOCOMPLETESTATICONLY$\r\n" \
	"  /// Creates a socket address from a string. (for example: \"http://www.adventuregamestudio.co.uk\")\r\n" \
	"  import static SockAddr *CreateFromString(String address, int type = IPv4); // $AUTOCOMPLETESTATICONLY$\r\n" \
	"  /// Creates a socket address from raw data. (advanced)\r\n" \
	"  import static SockAddr *CreateFromData(SockData *);                        // $AUTOCOMPLETEIGNORE$\r\n" \
	"  /// Creates a socket address from an IP-address. (for example: \"127.0.0.1\")\r\n" \
	"  import static SockAddr *CreateIP(String address, int port);                // $AUTOCOMPLETESTATICONLY$\r\n" \
	"  /// Creates a socket address from an IPv6-address. (for example: \"::1\")\r\n" \
	"  import static SockAddr *CreateIPv6(String address, int port);              // $AUTOCOMPLETESTATICONLY$\r\n" \
	"  \r\n" \
	"  import attribute int Port;\r\n" \
	"  import attribute String Address;\r\n" \
	"  import attribute String IP;\r\n" \
	"  \r\n" \
	"  /// Returns a SockData object that contains the raw data of the socket address. (advanced)\r\n" \
	"  import SockData *GetData()\r\n" \
	"};\r\n" \
	"\r\n"

#define SOCKADDR_ENTRY	                         \
	AGS_CLASS (SockAddr)                         \
	AGS_METHOD(SockAddr, Create, 1)              \
	AGS_METHOD(SockAddr, CreateFromString, 2)    \
	AGS_METHOD(SockAddr, CreateFromData, 1)      \
	AGS_METHOD(SockAddr, CreateIP, 2)            \
	AGS_METHOD(SockAddr, CreateIPv6, 2)          \
	AGS_MEMBER(SockAddr, Port)                   \
	AGS_MEMBER(SockAddr, Address)                \
	AGS_MEMBER(SockAddr, IP)                     \
	AGS_METHOD(SockAddr, GetData, 0)

//------------------------------------------------------------------------------

#endif /* _SOCKADDR_H */

//..............................................................................
