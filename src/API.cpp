/*******************************************************
 * API module -- See header file for more information. *
 *******************************************************/

#include <stdlib.h>

#include "API.h"

namespace AGSSockAPI {

IAGSEngine *engine = NULL;

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

//==============================================================================

#define ASSERT_DATA(x) if (!data) return x;

struct Thread::Data
{
	bool active;
	Mutex mutex;
	
	Data() : active(false), mutex() {}
	
	#ifdef _WIN32
		HANDLE handle;
		
		static DWORD WINAPI Function(LPVOID data)
		{
			if (!data) return EXIT_FAILURE;
	#else
		pthread_t thread;
		
		static void *Function(void *data)
		{
			if (!data) pthread_exit(NULL);
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	#endif
			Thread *thread = (Thread *)data;
			
			if (thread->func) thread->func();
			
	#ifdef _WIN32
			return EXIT_SUCCESS;
	#else
			pthread_exit(NULL);
	#endif
		}
};

//------------------------------------------------------------------------------

Thread::Thread(Callback callback) : func(callback), data(new Data) {}

//------------------------------------------------------------------------------

Thread::~Thread()
{
	ASSERT_DATA()
	if (active())
	{
		#ifdef _WIN32
			if (WaitForSingleObject(data->handle, 2000) != WAIT_OBJECT_0)
				TerminateThread(data->handle, 0);
			
			CloseHandle(data->handle);
		#else
			pthread_join(data->thread, NULL); // Todo: add a timeout in the future
		#endif
	}
	delete data;
}

//------------------------------------------------------------------------------

void Thread::start()
{
	ASSERT_DATA()
	data->mutex.lock();
		if (!data->active)
		{
			#ifdef _WIN32
				data->handle = CreateThread(NULL, 0, Data::Function, (LPVOID) this, 0, NULL);
				data->active = (data->handle != NULL);
			#else
				data->active = !pthread_create(&data->thread, NULL, Data::Function, (void *) this);
			#endif
		}
	data->mutex.unlock();
}

//------------------------------------------------------------------------------

void Thread::stop()
{
	ASSERT_DATA()
	data->mutex.lock();
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
	data->mutex.unlock();
}

//------------------------------------------------------------------------------

bool Thread::active()
{
	ASSERT_DATA(false)
	data->mutex.lock();
		bool state = data->active;
	data->mutex.unlock();
	return state;
}

//------------------------------------------------------------------------------

void Thread::exit()
{
	ASSERT_DATA();
	data->mutex.lock();
		data->active = false;
	data->mutex.unlock();
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
Beacon::Beacon()
{
	#ifdef _WIN32
	/** /
		data.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		setblocking(data.fd, false);
		
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = 0;
		
		connect(data.fd, (sockaddr *) &addr, sizeof (addr));
	/**/
		data.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		setblocking(data.fd, false);
	/**/
	#else
		pipe(data.fd);
		setblocking(data.fd[0], false);
		setblocking(data.fd[1], false);
	#endif
	// Todo: add error handling
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
	char buffer[8];
	#ifdef _WIN32
	/** /
		while (recv(data.fd, buffer, sizeof (buffer), 0) > 0);
	/**/
		closesocket(data.fd);
		data.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		setblocking(data.fd, false);
	/**/
	#else
		while (read(data.fd[0], buffer, sizeof (buffer)) > 0);
	#endif
}

//------------------------------------------------------------------------------

void Beacon::signal()
{
	const char sig[] = "";
	#ifdef _WIN32
	/** /
		send(data.fd, sig, sizeof (sig), 0);
	/**/
		closesocket(data.fd);
	/**/
	#else
		write(data.fd[1], sig, sizeof (sig));
	#endif
}

//------------------------------------------------------------------------------

} /* namespace AGSSockAPI */

//------------------------------------------------------------------------------

#if (_WIN32_WINNT < 0x600)

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
	if (getnameinfo((const sockaddr *) src, sizeof (SOCKADDR_STORAGE),
		dst, size, NULL, 0, NI_NUMERICHOST))
		return NULL;
	else
		return dst;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

int inet_pton(int af, const char *src, void *dst)
{	
	addrinfo hint, *result;
	memset(&hint, 0, sizeof (addrinfo));
	hint.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED | AI_NUMERICHOST;
	hint.ai_family = af;
	
	if (getaddrinfo(src, NULL, &hint, &result))
		return 0;
	else if (result)
	{
		if (af == AF_INET)
		{
			u_short port = ((sockaddr_in *) dst)->sin_port;
			memcpy(dst, result->ai_addr, result->ai_addrlen);
			((sockaddr_in *) dst)->sin_port = port;
		}
		else if (af == AF_INET6)
		{
			u_short port = ((sockaddr_in6 *) dst)->sin6_port;
			memcpy(dst, result->ai_addr, result->ai_addrlen);
			((sockaddr_in6 *) dst)->sin6_port = port;
		}
		else
			memcpy(dst, result->ai_addr, result->ai_addrlen);
	}
	
	freeaddrinfo(result);
	return 1;
}

#endif

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

//------------------------------------------------------------------------------
/*
	Todo:
		1. Error constants to AGS?
*/
//..............................................................................
