/*******************************************************
 * Socket interface -- header file                     *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 16:44 29-6-2012                               *
 *                                                     *
 * Decription: Implements a socket object and exposes  *
 *             BSD sockets functionality.              *
 *******************************************************/

#ifndef _SOCKET_H
#define _SOCKET_H

#include <string>

#include "API.h"
#include "Buffer.h"
#include "SockAddr.h"
#include "SockData.h"
#include "version.h"

//! A BSD sockets wrapper plugin for AGS
//! \warning Assumes the API has successfully been initialized.
namespace AGSSock {

//------------------------------------------------------------------------------

void Initialize(); //!< Initializes the interface so it is ready to be used
void Terminate();  //!< Resets the interface to its initial state

//------------------------------------------------------------------------------

struct Socket
{
	// Exposed: <<<DO NOT CHANGE THE ORDER!!!>>>
	SOCKET id;
	int domain, type, protocol;
	int error;
	
	// Internal:
	SockAddr *local, *remote;
	std::string tag;
	Buffer incoming; // This design does not feature an outgoing buffer
};

AGS_DEFINE_CLASS(Socket)

//------------------------------------------------------------------------------

Socket *Socket_Create(ags_t domain, ags_t type, ags_t protocol);
Socket *Socket_CreateUDP();
Socket *Socket_CreateTCP();
Socket *Socket_CreateUDPv6();
Socket *Socket_CreateTCPv6();

ags_t Socket_get_Valid(Socket *);
const char *Socket_get_Tag(Socket *);
void Socket_set_Tag(Socket *, const char *);
SockAddr *Socket_get_Local(Socket *);
SockAddr *Socket_get_Remote(Socket *);
ags_t Socket_ErrorValue(Socket *sock);
const char *Socket_ErrorString(Socket *);

ags_t Socket_Bind(Socket *, const SockAddr *);
ags_t Socket_Listen(Socket *, ags_t backlog);
ags_t Socket_Connect(Socket *, const SockAddr *, ags_t async);
Socket *Socket_Accept(Socket *);
void Socket_Close(Socket *);

ags_t Socket_Send(Socket *, const char *);
ags_t Socket_SendData(Socket *, const SockData *);
ags_t Socket_SendTo(Socket *, const SockAddr *, const char *);
ags_t Socket_SendDataTo(Socket *, const SockAddr *, const SockData *);
const char *Socket_Recv(Socket *);
SockData *Socket_RecvData(Socket *);
const char *Socket_RecvFrom(Socket *, SockAddr *);
SockData *Socket_RecvDataFrom(Socket *, SockAddr *);

ags_t Socket_GetOption(Socket *, ags_t level, ags_t option);
void Socket_SetOption(Socket *, ags_t level, ags_t option, ags_t value);

//------------------------------------------------------------------------------

} /* namespace AGSSock */

//------------------------------------------------------------------------------

#define SOCKET_HEADER \
	"#define AGSSOCK " RELEASE_DATE "\r\n\r\n" \
	"enum SockError\r\n" \
	"{\r\n" \
	"	eSockNoError             = " STRINGIFY(AGSSOCK_NO_ERROR) ",\r\n" \
	"	eSockOtherError          = " STRINGIFY(AGSSOCK_OTHER_ERROR) ",\r\n" \
	"	eSockAccessDenied        = " STRINGIFY(AGSSOCK_ACCESS_DENIED) ",\r\n" \
	"	eSockAddressNotAvailable = " STRINGIFY(AGSSOCK_ADDRESS_NOT_AVAILABLE) ",\r\n" \
	"	eSockPleaseTryAgain      = " STRINGIFY(AGSSOCK_PLEASE_TRY_AGAIN) ",\r\n" \
	"	eSockSocketNotValid      = " STRINGIFY(AGSSOCK_SOCKET_NOT_VALID) ",\r\n" \
	"	eSockDisconnected        = " STRINGIFY(AGSSOCK_DISCONNECTED) ",\r\n" \
	"	eSockInvalid             = " STRINGIFY(AGSSOCK_INVALID) ",\r\n" \
	"	eSockUnsupported         = " STRINGIFY(AGSSOCK_UNSUPPORTED) ",\r\n" \
	"	eSockHostNotReached      = " STRINGIFY(AGSSOCK_HOST_NOT_REACHED) ",\r\n" \
	"	eSockNotEnoughResources  = " STRINGIFY(AGSSOCK_NOT_ENOUGH_RESOURCES) ",\r\n" \
	"	eSockNetworkNotAvailable = " STRINGIFY(AGSSOCK_NETWORK_NOT_AVAILABLE) ",\r\n" \
	"	eSockNotConnected        = " STRINGIFY(AGSSOCK_NOT_CONNECTED) "\r\n" \
	"};\r\n\r\n" \
	"managed struct Socket\r\n" \
	"{\r\n" \
	"	/// Creates a socket for the specified protocol. (advanced)\r\n" \
	"	import static Socket *Create(int domain, int type, int protocol = 0); // $AUTOCOMPLETEIGNORE$\r\n" \
	"	/// Creates a UDP socket. (unreliable, connectionless, message based)\r\n" \
	"	import static Socket *CreateUDP();           // $AUTOCOMPLETESTATICONLY$\r\n" \
	"	/// Creates a TCP socket. (reliable, connection based, streaming)\r\n" \
	"	import static Socket *CreateTCP();           // $AUTOCOMPLETESTATICONLY$\r\n" \
	"	/// Creates a UDP socket for IPv6. (when in doubt use CreateUDP)\r\n" \
	"	import static Socket *CreateUDPv6();         // $AUTOCOMPLETESTATICONLY$\r\n" \
	"	/// Creates a TCP socket for IPv6. (when in doubt use CreateTCP)\r\n" \
	"	import static Socket *CreateTCPv6();         // $AUTOCOMPLETESTATICONLY$\r\n" \
	"	\r\n" \
	"	readonly int ID;                             // $AUTOCOMPLETEIGNORE$\r\n" \
	"	readonly int Domain;                         // $AUTOCOMPLETEIGNORE$\r\n" \
	"	readonly int Type;                           // $AUTOCOMPLETEIGNORE$\r\n" \
	"	readonly int Protocol;                       // $AUTOCOMPLETEIGNORE$\r\n" \
	"	readonly int LastError;\r\n" \
	"	\r\n" \
	"	         import attribute String Tag;\r\n" \
	"	readonly import attribute SockAddr *Local;\r\n" \
	"	readonly import attribute SockAddr *Remote;\r\n" \
	"	readonly import attribute bool Valid;\r\n" \
	"	\r\n" \
	"	/// Returns the last error observed from this socket as an enumerated value.\r\n" \
	"	import SockError ErrorValue();\r\n" \
	"	/// Returns the last error observed from this socket as an human readable string.\r\n" \
	"	import String ErrorString();\r\n" \
	"	/// Binds the socket to a local address. (generally used  before listening)\r\n" \
	"	import bool Bind(SockAddr *local);\r\n" \
	"	/// Makes a socket listen for incoming connection requests. (TCP only) Backlog specifies how many requests can be queued. (optional)\r\n" \
	"	import bool Listen(int backlog = 10);\r\n" \
	"	/// Makes a socket connect to a remote host. (for UDP it will simply bind to a remote address) Defaults to sync which makes it wait; see the manual for async use.\r\n" \
	"	import bool Connect(SockAddr *host, bool async = false);\r\n" \
	"	/// Accepts a connection request and returns the resulting socket when successful. (TCP only)\r\n" \
	"	import Socket *Accept();\r\n" \
	"	/// Closes the socket. (you can still receive until socket is marked invalid)\r\n" \
	"	import void Close();\r\n" \
	"	\r\n" \
	"	/// Sends a string to the remote host. Returns whether successful. (no error means: try again later)\r\n" \
	"	import bool Send(const string msg);\r\n" \
	"	/// Sends a string to the specified remote host. (UDP only)\r\n" \
	"	import bool SendTo(SockAddr *target, const string msg);\r\n" \
	"	/// Receives a string from the remote host. (no error means: try again later)\r\n" \
	"	import String Recv();\r\n" \
	"	/// Receives a string from an unspecified host. The given address object will contain the remote address. (UDP only)\r\n" \
	"	import String RecvFrom(SockAddr *source);\r\n" \
	"	\r\n" \
	"	/// Sends raw data to the remote host. Returns whether successful. (no error means: try again later\r\n" \
	"	import bool SendData(SockData *data);\r\n" \
	"	/// Sends raw data to the specified remote host. (UDP only)\r\n" \
	"	import bool SendDataTo(SockAddr *target, SockData *data);\r\n" \
	"	/// Receives raw data from the remote host. (no error means: try again later)\r\n" \
	"	import SockData *RecvData();\r\n" \
	"	/// Receives raw data from an unspecified host. The given address object will contain the remote address. (UDP only)\r\n" \
	"	import SockData *RecvDataFrom(SockAddr *source);\r\n" \
	"	\r\n" \
	"	/// Gets a socket option. (advanced)\r\n" \
	"	import long GetOption(int level, int option);             // $AUTOCOMPLETEIGNORE$\r\n" \
	"	/// Sets a socket option. (advanced)\r\n" \
	"	import bool SetOption(int level, int option, long value); // $AUTOCOMPLETEIGNORE$\r\n" \
	"};\r\n"

#define SOCKET_ENTRY    	                     \
	AGS_CLASS   (Socket)                         \
	AGS_METHOD  (Socket, Create, 3)              \
	AGS_METHOD  (Socket, CreateUDP, 0)           \
	AGS_METHOD  (Socket, CreateTCP, 0)           \
	AGS_METHOD  (Socket, CreateUDPv6, 0)         \
	AGS_METHOD  (Socket, CreateTCPv6, 0)         \
	AGS_MEMBER  (Socket, Tag)                    \
	AGS_READONLY(Socket, Local)                  \
	AGS_READONLY(Socket, Remote)                 \
	AGS_READONLY(Socket, Valid)                  \
	AGS_METHOD  (Socket, ErrorValue, 0)          \
	AGS_METHOD  (Socket, ErrorString, 0)         \
	AGS_METHOD  (Socket, Bind, 1)                \
	AGS_METHOD  (Socket, Listen, 1)              \
	AGS_METHOD  (Socket, Connect, 2)             \
	AGS_METHOD  (Socket, Accept, 0)              \
	AGS_METHOD  (Socket, Close, 0)               \
	AGS_METHOD  (Socket, Send, 1)                \
	AGS_METHOD  (Socket, SendTo, 2)              \
	AGS_METHOD  (Socket, Recv, 0)                \
	AGS_METHOD  (Socket, RecvFrom, 1)            \
	AGS_METHOD  (Socket, SendData, 1)            \
	AGS_METHOD  (Socket, SendDataTo, 2)          \
	AGS_METHOD  (Socket, RecvData, 0)            \
	AGS_METHOD  (Socket, RecvDataFrom, 1)        \
	AGS_METHOD  (Socket, GetOption, 2)           \
	AGS_METHOD  (Socket, SetOption, 3)

//------------------------------------------------------------------------------

#endif /* _SOCKET_H */

//..............................................................................
