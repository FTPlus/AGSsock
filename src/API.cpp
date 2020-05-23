/*******************************************************
 * API module -- See header file for more information. *
 *******************************************************/

#include <cassert>
#include <cstddef>
#include <cstdlib>

#include "API.h"

namespace AGSSockAPI {

IAGSEngine *engine = nullptr;

#ifdef _WIN32
	WSADATA wsa;
#endif

//------------------------------------------------------------------------------

void Initialize()
{
#ifdef _WIN32
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

//------------------------------------------------------------------------------

void Terminate()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

//------------------------------------------------------------------------------

/*
ACCES, PERM:                                      AccessDenied
ADDRINUSE, ADDRNOTAVAIL, AFNOSUPPORT:             AddressNotAvailable
AGAIN, WOULDBLOCK, ALREADY, INPROGRESS, INTR:     PleaseTryAgain
BADF, NOTSOCK:                                    SocketNotValid
CONNABORTED, CONNREFUSED, CONNRESET, NETRESET:    Disconnected
DESTADDRREQ, INVAL, PROTOTYPE, FAULT, ISCONN:     Invalid
OPNOTSUPP, PROTO, PROTONOSUPPORT, SOCKTNOSUPPORT: Unsupported
HOSTUNREACH:                                      HostUnreachable
MFILE, NFILE, NOBUFS, NOMEM:                      NotEnoughResources
NETDOWN, NETUNREACH:                              NetworkUnreachable
NOTCONN, PIPE, SHUTDOWN, TIMEDOUT:                NotConnected
*/

#ifdef _WIN32
	#define ERR(x) WSAE ## x
	#define NOT_WIN(x)
#else
	#define ERR(x) E ## x
	#define NOT_WIN(x) x
#endif

ags_t AGSEnumerateError(int errnum)
{
	switch (errnum)
	{
		case 0:
		                          return AGSSOCK_NO_ERROR;
		case ERR(ACCES):
		NOT_WIN(case ERR(PERM):)
		                          return AGSSOCK_ACCESS_DENIED;
		case ERR(ADDRINUSE):
		case ERR(ADDRNOTAVAIL):
		case ERR(AFNOSUPPORT):
		                          return AGSSOCK_ADDRESS_NOT_AVAILABLE;
#ifndef _WIN32
	#if EAGAIN != EWOULDBLOCK
		case EAGAIN:
	#endif
#endif
		case ERR(WOULDBLOCK):
		case ERR(ALREADY):
		case ERR(INPROGRESS):
		case ERR(INTR):
		                          return AGSSOCK_PLEASE_TRY_AGAIN;
		case ERR(BADF):
		case ERR(NOTSOCK):
		                          return AGSSOCK_SOCKET_NOT_VALID;
		case ERR(CONNABORTED):
		case ERR(CONNREFUSED):
		case ERR(CONNRESET):
		case ERR(NETRESET):
		                          return AGSSOCK_DISCONNECTED;
		case ERR(DESTADDRREQ):
		case ERR(INVAL):
		case ERR(PROTOTYPE):
		case ERR(FAULT):
		case ERR(ISCONN):
		                          return AGSSOCK_INVALID;
		case ERR(OPNOTSUPP):
		NOT_WIN(case ERR(PROTO):)
		case ERR(PROTONOSUPPORT):
		case ERR(SOCKTNOSUPPORT):
		                          return AGSSOCK_UNSUPPORTED;
		case ERR(HOSTUNREACH):
		                          return AGSSOCK_HOST_NOT_REACHED;
		case ERR(MFILE):
		NOT_WIN(case ERR(NFILE):)
		case ERR(NOBUFS):
		NOT_WIN(case ERR(NOMEM):)
		                          return AGSSOCK_NOT_ENOUGH_RESOURCES;
		case ERR(NETDOWN):
		case ERR(NETUNREACH):
		                          return AGSSOCK_NETWORK_NOT_AVAILABLE;
		case ERR(NOTCONN):
		NOT_WIN(case ERR(PIPE):)
		case ERR(SHUTDOWN):
		case ERR(TIMEDOUT):
		                          return AGSSOCK_NOT_CONNECTED;
		default:
		                          return AGSSOCK_OTHER_ERROR;
	}
}

//------------------------------------------------------------------------------

const char *AGSFormatError(int errnum)
{
#ifdef _WIN32
	LPSTR msg = nullptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	              0, errnum, 0, (LPSTR)&msg, 0, 0);
	const char *str = AGS_STRING(msg);
	LocalFree(msg);
	return str;
#else
	return AGS_STRING(strerror(errnum));
#endif
}

void AGSAbort(const char *msg)
{
	engine->AbortGame(msg);
}

//==============================================================================

struct Thread::Data
{
	bool active;
	Mutex mutex;
	
	Data() : active(false), mutex() {}
	
#ifdef _WIN32
	HANDLE handle;
	
	static DWORD WINAPI Function(LPVOID data)
	{
		assert(data != nullptr);
		Thread &thread = *static_cast<Thread *> (data);
		
		if (thread.func)
			thread.func();
		
		return EXIT_SUCCESS;
	}
#else
	pthread_t thread;
	
	static void *Function(void *data)
	{
		assert(data != nullptr);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);

		Thread &thread = *static_cast<Thread *> (data);
		
		if (thread.func)
			thread.func();
		
		pthread_exit(nullptr);
		return nullptr;
	}
#endif
};

//------------------------------------------------------------------------------

Thread::Thread(Callback callback) : func(callback), data(new Data()) {}

Thread::~Thread()
{
	assert(data != nullptr);

	if (active())
	{
		// Wait for the thread to terminate, or after 2 seconds, exterminate!
	#ifdef _WIN32
		if (WaitForSingleObject(data->handle, 2000) != WAIT_OBJECT_0)
			TerminateThread(data->handle, 0);
		
		CloseHandle(data->handle);
	#elif HAVE_TIMEDJOIN
		timespec ts;
		if (clock_gettime(CLOCK_REALTIME, &ts))
			pthread_cancel(data->thread);
		else
		{
			ts.tv_sec += 2;
			if (pthread_timedjoin_np(data->thread, nullptr, &ts))
				pthread_cancel(data->thread);
		}
	#else
		// We do not have a timed join function, instead
		// simply terminate the thread immediately.
		pthread_cancel(data->thread);
	#endif
	}

	delete data;
	data = nullptr;
}

void Thread::start()
{
	assert(data != nullptr);

	{
		Mutex::Lock lock(data->mutex);

		if (!data->active)
		{
		#ifdef _WIN32
			data->handle = CreateThread(nullptr, 0,
				Data::Function, static_cast<LPVOID> (this), 0, nullptr);
			data->active = (data->handle != nullptr);
		#else
			data->active = !pthread_create(&data->thread, nullptr,
				Data::Function, static_cast<void *> (this));
		#endif
		}
	}
}

void Thread::stop()
{
	assert(data != nullptr);

	{
		Mutex::Lock lock(data->mutex);

		if (data->active)
		{
		#ifdef _WIN32
			TerminateThread(data->handle, 0);
		#else
			pthread_cancel(data->thread);
		#endif
			data->active = false;
		}
		// Why is this not in the conditional?
	#ifdef _WIN32
		CloseHandle(data->handle);
	#endif
	}
}

bool Thread::active()
{
	assert(data != nullptr);

	{
		Mutex::Lock lock(data->mutex);

		return data->active;
	}
}

void Thread::exit()
{
	assert(data != nullptr);

	{
		Mutex::Lock lock(data->mutex);

		data->active = false;
	}
}

//==============================================================================
/*

	There are two possible windows implementation for this;
	both have their advantages and disadvantages:
	
		1. Use a self-connected udp socket.
			Pro: The resource only need to be created once.
			Con: It can cause interfere with other UDP connections.
			
		2. Create and close a udp socket everytime.
			Pro: It does not interfere with other communication.
			Con: It creates sockets many times.
		
		For now I go for the second implementation since it will not be called
		very often.

*/

#define IMPL_MODE 2

Beacon::Beacon()
{
#if defined(_WIN32) && (IMPL_MODE == 1)
	data.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	setblocking(data.fd, false);
	
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;
	
	connect(data.fd, reinterpret_cast<sockaddr *> (&addr), sizeof (addr));
#elif defined(_WIN32) && (IMPL_MODE == 2)
	data.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	setblocking(data.fd, false);
#else
	pipe(data.fd);
	setblocking(data.fd[0], false);
	setblocking(data.fd[1], false);
#endif
}

//------------------------------------------------------------------------------

Beacon::~Beacon()
{
#ifdef _WIN32
	closesocket(data.fd);
#else
	close(data.fd[0]);
	close(data.fd[1]);
#endif
}

//------------------------------------------------------------------------------

void Beacon::reset()
{
#if defined(_WIN32) && (IMPL_MODE == 1)
	char buffer[8];
	while (recv(data.fd, buffer, sizeof (buffer), 0) > 0);
#elif defined(_WIN32) && (IMPL_MODE == 2)
	closesocket(data.fd);
	data.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	setblocking(data.fd, false);
#else
	char buffer[8];
	while (read(data.fd[0], buffer, sizeof (buffer)) > 0);
#endif
}

//------------------------------------------------------------------------------

void Beacon::signal()
{
	const char sig[] = "";
#if defined(_WIN32) && (IMPL_MODE == 1)
	send(data.fd, sig, sizeof (sig), 0);
#elif defined(_WIN32) && (IMPL_MODE == 2)
	closesocket(data.fd);
#else
	write(data.fd[1], sig, sizeof (sig));
#endif
}

//------------------------------------------------------------------------------

} /* namespace AGSSockAPI */

//------------------------------------------------------------------------------

#if defined(_WIN32) && (_WIN32_WINNT < 0x600)

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
	SOCKADDR_STORAGE sa;
	memset(&sa, 0, sizeof (SOCKADDR_STORAGE));

	if (af == AF_INET)
	{
		sockaddr_in *addr = reinterpret_cast<sockaddr_in *> (&sa);
		addr->sin_family = af;
		memcpy(&addr->sin_addr, src, sizeof (in_addr));
	}
	else if (af = AF_INET6)
	{
		sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *> (&sa);
		addr->sin6_family = af;
		memcpy(&addr->sin6_addr, src, sizeof (in6_addr));
	}
	else
		return nullptr;

	if (getnameinfo(reinterpret_cast<const sockaddr *> (&sa),
		sizeof (SOCKADDR_STORAGE), dst, size, nullptr, 0, NI_NUMERICHOST))
		return nullptr;
	else
		return dst;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

int inet_pton(int af, const char *src, void *dst)
{	
	addrinfo hint, *result = nullptr;
	memset(&hint, 0, sizeof (addrinfo));
	hint.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED | AI_NUMERICHOST;
	hint.ai_family = af;
	
	if (getaddrinfo(src, nullptr, &hint, &result))
		return 0;

	if (result == nullptr)
		return 0;

	int ret = 0;
	if (af == AF_INET)
	{
		if (result->ai_addrlen >= sizeof (sockaddr_in))
		{
			sockaddr_in *addr = reinterpret_cast<sockaddr_in *>
				(result->ai_addr);
			memcpy(dst, &addr->sin_addr, sizeof (in_addr));
			ret = 1;
		}
	}
	else if (af == AF_INET6)
	{
		if (result->ai_addrlen >= sizeof (sockaddr_in6))
		{
			sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *>
				(result->ai_addr);
			memcpy(dst, &addr->sin6_addr, sizeof (in6_addr));
			ret = 1;
		}
	}
	else
		ret = -1;
	
	freeaddrinfo(result);
	return ret;
}

#endif /* defined(_WIN32) && (_WIN32_WINNT < 0x600)*/

//------------------------------------------------------------------------------

int setblocking(SOCKET sock, bool state)
{
#ifdef _WIN32
	unsigned long mode = state ? 0 : 1;
	return ioctlsocket(sock, FIONBIO, &mode);
#else
	int flags = fcntl(sock, F_GETFL, 0);
	if (state)
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;
	return fcntl(sock, F_SETFL, flags);
#endif
}

//..............................................................................
