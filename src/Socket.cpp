/*************************************************************
 * Socket interface -- See header file for more information. *
 *************************************************************/

#ifdef DEBUG
	#include <stdio.h>
	#define DEBUG_P(x) puts("\t\t" x);
#else
	#define DEBUG_P(x) ((void *) 0)
#endif

#include <stdlib.h>
#include <string.h>

#include <set>

#include "Socket.h"

#ifdef _WIN32
	#define SET_ERROR(x) (x) = WSAGetLastError();
#else
	#define SET_ERROR(x) (x) = errno;
#endif

/*
 * Todo: (walkthrough)
 *   1. add/remove to pool -- done
 *   2. block/nonblocking -- done
 *   3. local/remote address -- will err invalidate?
 *   4. buffer + mutex -- done
 *   5. sock valid
 *   6. aborting the plugin -- done
 *   7. invalidating sockets (should always remove from pool, see invar II)
 *   8. va style Send?
 *   9. Make string params const in ags header?
 */

namespace AGSSock {

using namespace std;
using namespace AGSSockAPI;

//------------------------------------------------------------------------------
// Invariant I: poolthread->active() == (pool.size() > 0)
// Invariant II: sock->id == INVALID_SOCKET => !pool.count(sock)

set<Socket *> pool; //!< The set of all processed sockets.
Mutex *poollock; //!< Guardes pool, poolsignal and the incoming buffers
Beacon *poolsignal; //!< Signals arrival or departure of sockets in the pool
Thread *poolthread;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

void Run();

void pool_add(Socket *sock);
void pool_remove(Socket *sock);
void pool_clear();

//------------------------------------------------------------------------------

void Initialize()
{
	poollock = new Mutex();
	poolsignal = new Beacon();
	poolthread = new Thread(Run);
}

//------------------------------------------------------------------------------

void Terminate()
{
	// We assume that all managed objects will be disposed of at this point.
	// That means the pool is or soon will be empty thus the read loop stops.
	// Deleting the thread gives it two seconds to do it nicely or else just
	// kills it.
	
	delete poolthread;
	poolthread = NULL;
	
	delete poolsignal;
	poolsignal = NULL;
	
	delete poollock;
	poollock = NULL;
	
	pool.clear();
}

//------------------------------------------------------------------------------

void Run()
{
	set<Socket *>::iterator it;
	SOCKET signal = *poolsignal;
	fd_set read;
	int nfds;
	
	DEBUG_P("Thread started");
	for (;;) { /* event loop */
	
	// Reset FD sets
	FD_ZERO(&read);
	FD_SET(signal, &read);
	nfds = signal;
	
	// Add pool sockets to FD sets
	poollock->lock();
		for (it = pool.begin(); it != pool.end(); ++it)
		{
			FD_SET((*it)->id, &read);
			#ifndef _WIN32
				if (nfds < (*it)->id)
					nfds = (*it)->id;
			#endif
		}
	poollock->unlock();
	
	// Wait for events
	select(nfds + 1, &read, NULL, NULL, NULL);
	// If select errs a socket was most likely closed locally, this is fine.
	// We need to check which one(s) and ignore all 'would block's.
	
	// Process read and error events
	poollock->lock();
		if (FD_ISSET(signal, &read))
		{
			poolsignal->reset();
			signal = *poolsignal;
			DEBUG_P("Thread signalled");
		}
		for (it = pool.begin(); it != pool.end();)
		{
			if (FD_ISSET((*it)->id, &read))
			{
				char buffer[65536];
				int error;
				int ret = recv((*it)->id, buffer, sizeof (buffer), 0);
				SET_ERROR(error);
				
				// We ignore sockets that would block:
				// This is normally filtered by select but a signal could have
				// interrupted select.
				if (ret == SOCKET_ERROR
				#ifdef _WIN32
					&& error == WSAEWOULDBLOCK)
				#else
					&& (error == EAGAIN || error == EWOULDBLOCK))
				#endif
				{
					++it;
					continue;
				}
				
				// If ret == 0 then closed gracefully (for TCP)
				// If ret == SOCKET_ERROR probably closed not so gracefully
				
				if (ret == SOCKET_ERROR)
					(*it)->incoming.error = error;
				else if ((ret > 0) && (*it)->protocol == IPPROTO_TCP)
					(*it)->incoming.append(string(buffer, ret));
				else
					(*it)->incoming.push(string(buffer, ret));
				
				if ((ret == SOCKET_ERROR)
				|| (!ret && (*it)->protocol == IPPROTO_TCP))
				{
					pool.erase(it++); // This socket is done for, stop reading
					continue;
				}	
			}
			++it;
		}
		
		// Close thread if there are no sockets to process anymore
		// Note: This is safe because the thread will be (re)started when
		//       sockets are added to the pool which requires the pool lock.
		if (pool.empty())
		{
			DEBUG_P("Thread finished");
			poolthread->exit();
			poollock->unlock();
			return;
		}
	poollock->unlock();
	
	} /* event loop */
	
	// Todo: chain the two critical sections for efficiency (one loop)
	
	DEBUG_P("Thread cancelled");
	poolthread->exit();
	return;
}

//==============================================================================

void pool_add(Socket *sock)
{
	poollock->lock();
		if (pool.insert(sock).second && pool.size() == 1)
			poolthread->start();
		else
			poolsignal->signal();
	poollock->unlock();
}

//------------------------------------------------------------------------------

void pool_remove(Socket *sock)
{
	poollock->lock();
		if (pool.erase(sock))
			poolsignal->signal();
		// Signalling might not be necessary for windows: closing sockets might
		// already trigger select.
	poollock->unlock();
}

//------------------------------------------------------------------------------

void pool_clear()
{
	poollock->lock();
		pool.clear();
		poolsignal->signal();
	poollock->unlock();
}

//==============================================================================

int AGSSocket::Dispose(const char *ptr, bool force)
{
	Socket *sock = (Socket *) ptr;
	
	if (sock->id != SOCKET_ERROR)
	{
		// Invalidate socket, forced close.
		pool_remove(sock);
		closesocket(sock->id);
		sock->id = SOCKET_ERROR;
	}
	
	if (sock->local != NULL)
		AGS_RELEASE(sock->local);
	
	if (sock->remote != NULL)
		AGS_RELEASE(sock->remote);
		
	delete sock;
	return 1;
}

//------------------------------------------------------------------------------

#pragma pack(push, 1)
	struct AGSSocketSerial
	{
		long domain, type, protocol;
		long error;
		int local, remote;
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
		sock->domain,
		sock->type,
		sock->protocol,
		sock->error,
		AGS_TO_KEY(sock->local),
		AGS_TO_KEY(sock->remote)
	};
	
	int size = MIN(length, sizeof (AGSSocketSerial));
	memcpy(buffer, &serial, size);
	return sock->tag.copy(buffer + size, length - size) + size;
}

//------------------------------------------------------------------------------
// Note: if unserialization happens in the wrong order we get a potential
// memory leak: if the addresses are unserialized after the socket the socket
// will get null references to them while the adresses are still in the pool 
// and are thus never released.
// This needs to be tested someday eventhough saving sockets is generally a
// bad idea!!!

void AGSSocket::Unserialize(int key, const char *buffer, int length)
{
	AGSSocketSerial serial;
	int size = MIN(length, sizeof (AGSSocketSerial));
	memcpy(&serial, buffer, size);
	
	Socket *sock = new Socket;
	sock->id = INVALID_SOCKET;
	sock->domain = serial.domain;
	sock->type = serial.type;
	sock->protocol = serial.protocol;
	sock->error = serial.error;
	sock->local = AGS_FROM_KEY(SockAddr, serial.local);
	sock->remote = AGS_FROM_KEY(SockAddr, serial.remote);
	if (length - size > 0)
		sock->tag = string(buffer + size, length - size);
	
	AGS_RESTORE(Socket, sock, key);
}

//==============================================================================

Socket *Socket_Create(long domain, long type, long protocol)
{
	Socket *sock = new Socket;
	AGS_OBJECT(Socket, sock);
	sock->id = socket(domain, type, protocol);
	SET_ERROR(sock->error);
	
	// The entire plugin is nonblocking except for:
	//     1. connections in sync mode (async = false)
	//     2. address lookups
	setblocking(sock->id, false);
	
	sock->domain = domain;
	sock->type = type;
	sock->protocol = protocol;
	
	sock->local = NULL;
	sock->remote = NULL;
	
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

long Socket_get_Valid(Socket *sock)
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
	int addrlen = sizeof (SockAddr);
	getsockname(sock->id, (sockaddr *) sock->local, &addrlen);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

SockAddr *Socket_get_Local(Socket *sock)
{
	if (sock->local == NULL)
	{
		sock->local = SockAddr_Create(sock->type);
		AGS_HOLD(sock->local);
		
		Socket_update_Local(sock);
		SET_ERROR(sock->error);
	}
	return sock->local;
}

//------------------------------------------------------------------------------

inline void Socket_update_Remote(Socket *sock)
{
	int addrlen = sizeof (SockAddr);
	getpeername(sock->id, (sockaddr *) sock->remote, &addrlen);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

SockAddr *Socket_get_Remote(Socket *sock)
{
	if (sock->remote == NULL)
	{
		sock->remote = SockAddr_Create(sock->type);
		AGS_HOLD(sock->remote);
		
		Socket_update_Remote(sock);
		SET_ERROR(sock->error);
	}
	return sock->remote;
}

//------------------------------------------------------------------------------

const char *Socket_ErrorString(Socket *sock)
{
	#ifdef _WIN32
		LPSTR msg = NULL;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		              0, sock->error, 0, (LPSTR)&msg, 0, 0);
		const char *str = AGS_STRING(msg);
		LocalFree(msg);
		return str;
	#else
		return AGS_STRING(strerror(sock->error));
	#endif
}

//==============================================================================

long Socket_Bind(Socket *sock, const SockAddr *addr)
{
	long ret = (bind(sock->id, (sockaddr *) addr, sizeof (SockAddr)) == SOCKET_ERROR ? 0 : 1);
	SET_ERROR(sock->error);
	if (sock->local != NULL)
		Socket_update_Local(sock);
	return ret;
}

//------------------------------------------------------------------------------

long Socket_Listen(Socket *sock, long backlog)
{
	if (backlog < 0)
		backlog = SOMAXCONN;
	long ret = (listen(sock->id, backlog) == SOCKET_ERROR ? 0 : 1);
	SET_ERROR(sock->error);
	return ret;
}

//------------------------------------------------------------------------------
// This will also work for UDP since Berkeley sockets fake a connection for UDP
// by binding a remote address to the socket. We will complete this illusion by
// adding the socket to the pool.

long Socket_Connect(Socket *sock, const SockAddr *addr, long async)
{
	int ret;
	
	if (!async) // Sync mode: do a blocking connect
	{
		setblocking(sock->id, true);
		ret = connect(sock->id, (sockaddr *) addr, sizeof (SockAddr));
		setblocking(sock->id, false);
	}
	else
		ret = connect(sock->id, (sockaddr *) addr, sizeof (SockAddr));
	
	// In async mode: returning false but with error == 0 is: try again
	SET_ERROR(sock->error);
	#ifdef _WIN32
		// Note: rumours are that timeouts are reported incorrectly: TEST THIS
		if (sock->error == WSAEALREADY
		|| sock->error == WSAEINVAL || sock->error == WSAEWOULDBLOCK)
			sock->error = 0;
	#else
		if (sock->error == EINPROGRESS || sock->error == EALREADY)
			sock->error = 0;
	#endif
	
	if (ret != SOCKET_ERROR)
	{
		if (sock->remote != NULL)
			Socket_update_Remote(sock);
		pool_add(sock);
	}
		
	return (ret == SOCKET_ERROR ? 0 : 1);
}

//------------------------------------------------------------------------------
// Accept is nonblocking:
// If it returns NULL and the error is also 0: try again!
Socket *Socket_Accept(Socket *sock)
{
	SockAddr addr;
	int addrlen = sizeof (SockAddr);
	
	SOCKET conn = accept(sock->id, (sockaddr *) &addr, &addrlen);
	SET_ERROR(sock->error);
	#ifdef _WIN32
		if (sock->error == WSAEWOULDBLOCK)
			sock->error = 0;
	#else
		if (sock->error == EAGAIN || sock->error == EWOULDBLOCK)
			sock->error = 0;
	#endif
	if (conn == INVALID_SOCKET)
		return NULL;
	
	Socket *sock2 = new Socket;
	AGS_OBJECT(Socket, sock2);
	
	sock2->id = conn;
	sock2->error = 0;
	
	sock2->domain = sock->domain;
	sock2->type = sock->type;
	sock2->protocol = sock->protocol;
	
	// It might be more efficient to use the local and returned address but I
	// rather let the API re-resolve them when needed (less error prone).
	sock2->local = NULL;
	sock2->remote = NULL;
	
	setblocking(sock2->id, false);
	pool_add(sock2);
	
	return sock2;
}

//------------------------------------------------------------------------------

void Socket_Close(Socket *sock)
{
	if (sock->protocol == IPPROTO_TCP)
	{
		// Graceful shutdown, the poolthread will detect if it succeeded.
		shutdown(sock->id, SD_SEND);
		
		// Wait for a response to prevent race conditions
		fd_set read;
		timeval timeout = {0, 500}; // Half a second fudge time
		FD_ZERO(&read);
		
		FD_SET(sock->id, &read);
		if (select(sock->id + 1, &read, NULL, NULL, &timeout) > 0)
			return;
			
		// Select failed or timeout: we force close
	}
	
	// Invalidate socket
	closesocket(sock->id);
	sock->id = INVALID_SOCKET;
	SET_ERROR(sock->error);
}

//==============================================================================
// Send is nonblocking:
// If it returns 0 and the error is also 0: try again!
long Socket_Send(Socket *sock, const char *str)
{
	long len = strlen(str);
	long ret = 0;
	
	while (len > 0)
	{
		if ((ret = send(sock->id, str, len, 0)) == SOCKET_ERROR)
			break;
		str += ret;
		len -= ret;
	}
	
	SET_ERROR(sock->error);
	#ifdef _WIN32
		if (sock->error == WSAEWOULDBLOCK)
			sock->error = 0;
	#else
		if (sock->error == EAGAIN || sock->error == EWOULDBLOCK)
			sock->error = 0;
	#endif
	return (ret == SOCKET_ERROR ? 0 : 1);
}

//------------------------------------------------------------------------------

long Socket_SendTo(Socket *sock, const SockAddr *addr, const char *str)
{
	long len = strlen(str);
	long ret = 0;
	
	while (len > 0)
	{
		if ((ret = sendto(sock->id, str, len, 0,
			(sockaddr *) addr, sizeof (SockAddr))) == SOCKET_ERROR)
			break;
		str += ret;
		len -= ret;
	}
	
	SET_ERROR(sock->error);
	#ifdef _WIN32
		if (sock->error == WSAEWOULDBLOCK)
			sock->error = 0;
	#else
		if (sock->error == EAGAIN || sock->error == EWOULDBLOCK)
			sock->error = 0;
	#endif
	return (ret == SOCKET_ERROR ? 0 : 1);
}

//------------------------------------------------------------------------------

const char *Socket_Recv(Socket *sock)
{
	string str;
	
	poollock->lock();
		if (sock->incoming.empty())
		{
			// Read buffer is empty: either nothing or an error occurred.
			// In both cases we return null, the error code will tell.
			sock->error = sock->incoming.error;
			poollock->unlock();
			
			if (sock->error)
			{
				// Invalidate socket in case of error
				closesocket(sock->id);
				sock->id = INVALID_SOCKET;
				// The read loop itself will remove it from the pool
			}
			
			return NULL;
		}
		
		// Get a truncated (zero terminated) version of the received data.
		str = sock->incoming.front().c_str();
		
		if (sock->protocol == IPPROTO_TCP)
			sock->incoming.extract();
		else
			sock->incoming.pop();
	poollock->unlock();
	
	sock->error = 0;
	if (!str.size() && sock->protocol == IPPROTO_TCP)
	{
		// TCP socket was closed, invalidate it.
		closesocket(sock->id);
		sock->id = INVALID_SOCKET;
		// The read loop itself will remove it from the pool
	}
	
	return AGS_STRING(str.c_str());
}

//------------------------------------------------------------------------------

const char *Socket_RecvFrom(Socket *sock, SockAddr *addr)
{
	char buffer[65536];
	int addrlen = sizeof (SockAddr);
	long ret = recvfrom(sock->id, buffer, sizeof (buffer), 0,
		(sockaddr *) addr, &addrlen);
	SET_ERROR(sock->error);
	
	if (ret == SOCKET_ERROR)
		return NULL;
	
	buffer[MIN(ret, sizeof (buffer) - 1)] = 0;
	return AGS_STRING(buffer);
}

//==============================================================================

long Socket_SendData(Socket *, const SockData *)
{
	return 0; // Todo: implement
}

//------------------------------------------------------------------------------

long Socket_SendDataTo(Socket *, const SockAddr *, const SockData *)
{
	return 0; // Todo: implement
}

//------------------------------------------------------------------------------

SockData *Socket_RecvData(Socket *)
{
	return NULL; // Todo: implement
}

//------------------------------------------------------------------------------

SockData *Socket_RecvDataFrom(Socket *, SockAddr *)
{
	return NULL; // Todo: implement
}

//==============================================================================

long Socket_GetOption(Socket *, long level, long option)
{
	return 0; // Todo: implement
}

//------------------------------------------------------------------------------

void Socket_SetOption(Socket *, long level, long option, long value)
{
	// Todo: implement
}

//------------------------------------------------------------------------------

} /* namespace AGSSock */

//..............................................................................
