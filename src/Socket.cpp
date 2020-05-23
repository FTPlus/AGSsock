/*************************************************************
 * Socket interface -- See header file for more information. *
 *************************************************************/

#include <cstdint>
#include <cstring>

#include "Pool.h"
#include "Socket.h"

namespace AGSSock {

using namespace AGSSockAPI;

using std::string;
using std::int32_t;

//------------------------------------------------------------------------------

Pool *pool;

void Initialize()
{
	pool = new Pool();
}

void Terminate()
{
	// We assume that all managed objects will be disposed of at this point.
	// That means the pool is or soon will be empty thus the read loop stops.
	// Deleting the pool gives it two seconds to do it nicely or else just
	// kills it.
	
	delete pool;
	pool = nullptr;
}

inline void CheckPoolInvariant()
{
	if (!*pool)
		AGSAbort("The AGS Sockets plug-in has experienced an "
			"unrecoverable failure: pool invariant violated.");
}

//==============================================================================

int AGSSocket::Dispose(const char *ptr, bool force)
{
	Socket *sock = (Socket *) ptr;
	
	if (sock->id != SOCKET_ERROR)
	{
		// Invalidate socket, forced close.
		pool->remove(sock);
		closesocket(sock->id);
		sock->id = SOCKET_ERROR;
	}
	
	if (sock->local != nullptr)
	{
		AGS_RELEASE(sock->local);
		sock->local = nullptr;
	}
	
	if (sock->remote != nullptr)
	{
		AGS_RELEASE(sock->remote);
		sock->remote = nullptr;
	}

	delete sock;
	return 1;
}

//------------------------------------------------------------------------------

#pragma pack(push, 1)
	struct AGSSocketSerial
	{
		int32_t domain, type, protocol;
		int32_t error;
		int32_t local, remote;
	};
#pragma pack(pop)

//------------------------------------------------------------------------------
// Note: Sockets will not survive serialization, they are but a distant memory.
// We do store the address info so a developer could potentially resuscitate
// them.

int AGSSocket::Serialize(const char *ptr, char *buffer, int length)
{
	Socket *sock = (Socket *) ptr;
	AGSSocketSerial serial =
	{
		(int32_t) sock->domain, (int32_t) sock->type, (int32_t) sock->protocol,
		(int32_t) sock->error,
		AGS_TO_KEY(sock->local), AGS_TO_KEY(sock->remote)
	};
	
	int size = MIN(length, sizeof (AGSSocketSerial));
	memcpy(buffer, &serial, size);
	return sock->tag.copy(buffer + size, (size_t) length - size) + size;
}

//------------------------------------------------------------------------------
// Note: if unserialization happens in the wrong order we get a potential
// memory leak: if the addresses are unserialized after the socket the socket
// will get null references to them while the addresses are still in the pool 
// and are thus never released.
// This needs to be tested someday even though saving sockets is generally a
// bad idea!!!

void AGSSocket::Unserialize(int key, const char *buffer, int length)
{
	AGSSocketSerial serial;
	int size = MIN(length, sizeof (AGSSocketSerial));
	memcpy(&serial, buffer, size);

	string tag;
	if (length - size > 0)
		tag = string(buffer + size, (size_t) length - size);
	
	Socket *sock = new Socket
	{
		INVALID_SOCKET,
		serial.domain, serial.type, serial.protocol,
		serial.error,
		AGS_FROM_KEY(SockAddr, serial.local),
		AGS_FROM_KEY(SockAddr, serial.remote),
		tag
	};
	
	AGS_RESTORE(Socket, sock, key);
}

//==============================================================================

Socket *Socket_Create(ags_t domain, ags_t type, ags_t protocol)
{
	RESET_ERROR(); // Bug: errno is sometimes not reset on Linux
	SOCKET id = socket(domain, type, protocol);
	ags_t error = GET_ERROR();

	// The entire plugin is nonblocking except for:
	//     1. connections in sync mode (async = false)
	//     2. address lookups
	setblocking(id, false);

	Socket *sock = new Socket
	{
		id,
		(int) domain, (int) type, (int) protocol,
		(int) error,
		nullptr, nullptr
	};
	AGS_OBJECT(Socket, sock);
	
	return sock;
}

//------------------------------------------------------------------------------

Socket *Socket_CreateUDP()
{
	return Socket_Create(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

//------------------------------------------------------------------------------

Socket *Socket_CreateTCP()
{
	return Socket_Create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

//------------------------------------------------------------------------------

Socket *Socket_CreateUDPv6()
{
	return Socket_Create(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
}

//------------------------------------------------------------------------------

Socket *Socket_CreateTCPv6()
{
	return Socket_Create(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
}

//==============================================================================

ags_t Socket_get_Valid(Socket *sock)
{
	return (sock->id != INVALID_SOCKET ? 1 : 0);
}

//------------------------------------------------------------------------------

const char *Socket_get_Tag(Socket *sock)
{
	return AGS_STRING(sock->tag.c_str());
}

//------------------------------------------------------------------------------

void Socket_set_Tag(Socket *sock, const char *str)
{
	sock->tag = str;
}

//------------------------------------------------------------------------------

inline void Socket_update_Local(Socket *sock)
{
	ADDRLEN addrlen = sizeof (SockAddr);
	getsockname(sock->id, ADDR(sock->local), &addrlen);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

SockAddr *Socket_get_Local(Socket *sock)
{
	if (sock->local == nullptr)
	{
		sock->local = SockAddr_Create(sock->type);
		AGS_HOLD(sock->local);
		
		Socket_update_Local(sock);
		sock->error = GET_ERROR();
	}
	return sock->local;
}

//------------------------------------------------------------------------------

inline void Socket_update_Remote(Socket *sock)
{
	ADDRLEN addrlen = sizeof (SockAddr);
	getpeername(sock->id, ADDR(sock->remote), &addrlen);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

SockAddr *Socket_get_Remote(Socket *sock)
{
	if (sock->remote == nullptr)
	{
		sock->remote = SockAddr_Create(sock->type);
		AGS_HOLD(sock->remote);
		
		Socket_update_Remote(sock);
		sock->error = GET_ERROR();
	}
	return sock->remote;
}

//------------------------------------------------------------------------------

ags_t Socket_ErrorValue(Socket *sock)
{
	return AGSEnumerateError(sock->error);
}

//------------------------------------------------------------------------------

const char *Socket_ErrorString(Socket *sock)
{
	return AGSFormatError(sock->error);
}

//==============================================================================

ags_t Socket_Bind(Socket *sock, const SockAddr *addr)
{
	int ret = bind(sock->id, CONST_ADDR(addr), ADDR_SIZE(addr));
	sock->error = GET_ERROR();
	if (sock->local != nullptr)
		Socket_update_Local(sock);

	// Faux connection UDP support
	if (ret != SOCKET_ERROR && sock->protocol == IPPROTO_UDP)
	{
		pool->add(sock);
		CheckPoolInvariant();
	}
	return ret == SOCKET_ERROR ? 0 : 1;
}

//------------------------------------------------------------------------------

ags_t Socket_Listen(Socket *sock, ags_t backlog)
{
	if (backlog < 0)
		backlog = SOMAXCONN;
	int ret = listen(sock->id, backlog);
	sock->error = GET_ERROR();
	return ret == SOCKET_ERROR ? 0 : 1;
}

//------------------------------------------------------------------------------
// This will also work for UDP since Berkeley sockets fake a connection for UDP
// by binding a remote address to the socket. We will complete this illusion by
// adding the socket to the pool.

ags_t Socket_Connect(Socket *sock, const SockAddr *addr, ags_t async)
{
	int ret;
	
	if (!async) // Sync mode: do a blocking connect
	{
		setblocking(sock->id, true);
		ret = connect(sock->id, CONST_ADDR(addr), ADDR_SIZE(addr));
		setblocking(sock->id, false);
	}
	else
		ret = connect(sock->id, CONST_ADDR(addr), ADDR_SIZE(addr));
	
	// In async mode: returning false but with error == 0 is: try again
	sock->error = GET_ERROR();
	if (ALREADY(sock->error)) // If already trying to connect
		sock->error = 0;
	
	if (ret != SOCKET_ERROR)
	{
		if (sock->remote != nullptr)
			Socket_update_Remote(sock);
		pool->add(sock);
		CheckPoolInvariant();
	}

	return (ret == SOCKET_ERROR ? 0 : 1);
}

//------------------------------------------------------------------------------
// Accept is nonblocking:
// If it returns nullptr and the error is also 0: try again!
Socket *Socket_Accept(Socket *sock)
{
	sockaddr addr;
	ADDRLEN addrlen = sizeof (addr);
	
	SOCKET conn = accept(sock->id, &addr, &addrlen);
	sock->error = GET_ERROR();
	if (WOULD_BLOCK(sock->error))
		sock->error = 0;

	if (conn == INVALID_SOCKET)
		return nullptr;
	
	Socket *sock2 = new Socket
	{
		conn,
		sock->domain, sock->type, sock->protocol,
		0,
		// It might be more efficient to use the local and returned address, but
		// I rather let the API re-resolve them when needed (less error prone).
		nullptr, nullptr
	};
	AGS_OBJECT(Socket, sock2);
	
	setblocking(conn, false);
	pool->add(sock2);
	CheckPoolInvariant();
	
	return sock2;
}

//------------------------------------------------------------------------------

void Socket_Close(Socket *sock)
{
	if (sock->type == SOCK_STREAM)
	{
		// Graceful shutdown, the poolthread will detect if it succeeded.
		shutdown(sock->id, SD_SEND);
		
		// Wait for a response to prevent race conditions
		fd_set read;
		timeval timeout = {0, 500}; // Half a second fudge time
		FD_ZERO(&read);
		
		FD_SET(sock->id, &read);
		if (select(sock->id + 1, &read, nullptr, nullptr, &timeout) > 0)
			return;
			
		// Select failed or timeout: we force close
	}
	
	// Invalidate socket
	pool->remove(sock);
	closesocket(sock->id);
	sock->id = INVALID_SOCKET;
	sock->error = GET_ERROR();
}

//==============================================================================

// Send is nonblocking:
// If it returns 0 and the error is also 0: try again!

inline ags_t send_impl(Socket *sock, const char *buf, size_t count)
{
	long ret = 0;
	
	while (count > 0)
	{
		ret = send(sock->id, buf, count, 0);
		if (ret == SOCKET_ERROR)
			break;
		buf += ret;
		count -= ret;
	}
	
	sock->error = GET_ERROR();
	if (WOULD_BLOCK(sock->error))
		sock->error = 0;

	return (ret == SOCKET_ERROR ? 0 : 1);
}

ags_t Socket_Send(Socket *sock, const char *str)
{
	return send_impl(sock, str, strlen(str));
}

ags_t Socket_SendData(Socket *sock, const SockData *data)
{
	return send_impl(sock, data->data.data(), data->data.size());
}

//------------------------------------------------------------------------------

inline ags_t sendto_impl(Socket *sock, const SockAddr *addr,
	const char *buf, size_t count)
{
	long ret = 0;
	
	while (count > 0)
	{
		ret = sendto(sock->id, buf, count, 0, CONST_ADDR(addr), ADDR_SIZE(addr));
		if (ret == SOCKET_ERROR)
			break;
		buf += ret;
		count -= ret;
	}
	
	sock->error = GET_ERROR();
	if (WOULD_BLOCK(sock->error))
		sock->error = 0;
	
	return (ret == SOCKET_ERROR ? 0 : 1);
}

ags_t Socket_SendTo(Socket *sock, const SockAddr *addr, const char *str)
{
	return sendto_impl(sock, addr, str, strlen(str));
}

ags_t Socket_SendDataTo(Socket *sock, const SockAddr *addr, const SockData *data)
{
	return sendto_impl(sock, addr, data->data.data(), data->data.size());
}

//------------------------------------------------------------------------------

// Receives and removes a chunk of data from a buffer and returns it
// Comes in a AGS String and SockData flavour
template <typename T> inline T *recv_extract(Buffer &buffer, bool stream);

template <> inline const char *recv_extract(Buffer &buffer, bool stream)
{
	// Get a truncated (zero terminated) version of the received data.
	const char *data = AGS_STRING(buffer.front().c_str());

	// If the connection is streaming we clear the buffer till the first
	// zero-character; otherwise we are dealing with packets: we remove the
	// current packet from the buffer.
	if (stream)
		buffer.extract();
	else
		buffer.pop();

	return data;
}

template <> inline SockData *recv_extract(Buffer &buffer, bool stream)
{
	// For SockData output, we don't have to worry about zero-characters,
	// thus we receive everything and then clear the buffer.
	SockData *data = new SockData();
	AGS_OBJECT(SockData, data);
	data->data.swap(buffer.front());
	buffer.pop();
	return data;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

// Returns whether a chunk of data is empty (the empty string) or not
// Comes in a AGS String and SockData flavour
template <typename T> inline bool recv_empty(T *data);

template <> inline bool recv_empty(const char *data)
{
	return data[0] == '\0';
}

template <> inline bool recv_empty(SockData *data)
{
	return data->data.empty();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

template <typename T> inline T *recv_impl(Socket *sock)
{
	T *data;
	
	{
		Mutex::Lock lock(*pool);
	
		if (sock->incoming.empty())
		{
			// Read buffer is empty: either nothing or an error occurred.
			// In both cases we return null, the error code will tell.
			sock->error = sock->incoming.error;
			
			if (sock->error)
			{
				// Invalidate socket in case of error
				closesocket(sock->id);
				sock->id = INVALID_SOCKET;
				// The read loop itself will remove it from the pool
			}
			
			return nullptr;
		}
		
		data = recv_extract<T>(sock->incoming, sock->type == SOCK_STREAM);
	}
	
	sock->error = 0;

	if (recv_empty(data) && sock->type == SOCK_STREAM)
	{
		// TCP socket was closed, invalidate it.
		closesocket(sock->id);
		sock->id = INVALID_SOCKET;
		// The read loop itself will remove it from the pool
	}
	
	return data;
}

const char *Socket_Recv(Socket *sock)
{
	// An empty string indicates 'end of stream' but an input starting with a
	// zero-character false-triggers this; we still close the socket in this
	// case. NB: the `RecvData` function does not have this limitation. So,
	// in case a protocol may send zero-characters point users to the
	// `RecvData` function. However, most protocols do not.
	return recv_impl<const char>(sock);
}

SockData *Socket_RecvData(Socket *sock)
{
	return recv_impl<SockData>(sock);
}

//------------------------------------------------------------------------------

template <typename T> inline T *recvfrom_return(const char *buf, size_t count);

template <> inline const char *recvfrom_return(const char *buf, size_t count)
{
	return AGS_STRING(buf);
}

template <> inline SockData *recvfrom_return(const char *buf, size_t count)
{
	SockData *data = new SockData();
	AGS_OBJECT(SockData, data);
	data->data = string(buf, count);
	return data;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

template <typename T> inline T *recvfrom_impl(Socket *sock, SockAddr *addr)
{
	char buffer[65536];
	buffer[sizeof (buffer) - 1] = 0;

	ADDRLEN addrlen = sizeof (SockAddr);
	long ret = recvfrom(sock->id, buffer, sizeof (buffer) - 1, 0,
		ADDR(addr), &addrlen);
	sock->error = GET_ERROR();
	
	if (ret == SOCKET_ERROR)
		return nullptr;
	
	return recvfrom_return<T>(buffer, ret);
}

const char *Socket_RecvFrom(Socket *sock, SockAddr *addr)
{
	return recvfrom_impl<const char>(sock, addr);
}

SockData *Socket_RecvDataFrom(Socket *sock, SockAddr *addr)
{
	return recvfrom_impl<SockData>(sock, addr);
}

//==============================================================================

// Unimplemented, there is probably no use for this

ags_t Socket_GetOption(Socket *, ags_t level, ags_t option)
{
	return 0;
}

//------------------------------------------------------------------------------

void Socket_SetOption(Socket *, ags_t level, ags_t option, ags_t value)
{
}

//------------------------------------------------------------------------------

} /* namespace AGSSock */

//..............................................................................
