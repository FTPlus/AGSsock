/*******************************************************
 * API module -- header file                           *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 16:59 29-6-2012                               *
 *                                                     *
 * Description: Sets up various components the plugin  *
 *              uses in a platform independend way.    *
 *******************************************************/

#ifndef _API_H
#define _API_H

#ifdef _WIN32
	#if (_WIN32_WINNT < 0x501)
		#undef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif

	#include <winsock2.h>
	#include <ws2tcpip.h>
	
	// Not supported prior to vista:
	#ifndef AI_ADDRCONFIG
		#define AI_ADDRCONFIG 0x0400
	#endif
	
	#ifndef AI_V4MAPPED
		#define AI_V4MAPPED 0x0800
	#endif
	
	#if (_WIN32_WINNT < 0x600)
		#define _WIN32_WINNT 0x0501
		const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
		int inet_pton(int af, const char *src, void *dst);
	#endif
	
	#ifndef _WINDOWS_
		#define _WINDOWS_
	#endif

	#define WOULD_BLOCK(x) ((x) == WSAEWOULDBLOCK)
	#define ALREADY(x) ((x) == WSAEALREADY || (x) == WSAEINVAL || (x) == WSAEWOULDBLOCK)
	#define GET_ERROR() WSAGetLastError()
#else
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <pthread.h>
	#include <time.h>
	
	#define SOCKET int
	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1
	#define SOCKADDR_STORAGE sockaddr_storage
	#define closesocket close
	#define SD_RECEIVE SHUT_RD
	#define SD_SEND SHUT_WR
	#define SD_BOTH SHUT_RDWR
	#define WOULD_BLOCK(x) ((x) == EAGAIN || (x) == EWOULDBLOCK)
	#define ALREADY(x) ((x) == EINPROGRESS || (x) == EALREADY)
	#define GET_ERROR() errno
#endif

int setblocking(SOCKET sock, bool state);

#ifndef MIN
	#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif

//------------------------------------------------------------------------------

#include <cstdint>
#include <functional>

#include "agsplugin.h"

#define STRINGIFY(s) STRINGIFY_X(s)
#define STRINGIFY_X(s) #s

#define AGS_STRING(x)     AGSSockAPI::engine->CreateScriptString(x)
#define AGS_OBJECT(c,x)   AGSSockAPI::engine->RegisterManagedObject((void *) (x), &ags ## c)
#define AGS_RESTORE(c,x,i)AGSSockAPI::engine->RegisterUnserializedObject((i), (void *) (x), &ags ## c)
#define AGS_HOLD(x)       AGSSockAPI::engine->IncrementManagedObjectRefCount((const char *) (x))
#define AGS_RELEASE(x)    AGSSockAPI::engine->DecrementManagedObjectRefCount((const char *) (x))
#define AGS_TO_KEY(x)     AGSSockAPI::engine->GetManagedObjectKeyByAddress((const char *) (x))
#define AGS_FROM_KEY(c,x) ((c *) AGSSockAPI::engine->GetManagedObjectAddressByKey(x))

#define AGS_FUNCTION(x)   engine->RegisterScriptFunction(#x, (void *) (x));
#define AGS_METHOD(c,x,a) engine->RegisterScriptFunction(#c "::" #x "^" #a, (void *) (c ## _ ## x));
#define AGS_MEMBER(c,x)   engine->RegisterScriptFunction(#c "::get_" #x, (void *) (c ## _get_ ## x)); \
                          engine->RegisterScriptFunction(#c "::set_" #x, (void *) (c ## _set_ ## x));
#define AGS_READONLY(c,x) engine->RegisterScriptFunction(#c "::get_" #x, (void *) (c ## _get_ ## x));
#define AGS_ARRAY(c,x)    engine->RegisterScriptFunction(#c "::geti_" #x, (void *) (c ## _geti_ ## x)); \
                          engine->RegisterScriptFunction(#c "::seti_" #x, (void *) (c ## _seti_ ## x));
#define AGS_CLASS(c)      engine->AddManagedObjectReader(#c, &ags ## c);

// Note: Unfortunately AGS makes assumptions about the size of 'int' and 'long',
// specifically, long is used to pass around both integral and pointer values
// in the plugin interface. This is non-portable. We introduce a special type
// 'ags_t' for this purpose and implement it as intptr_t if possible. This
// type has been added to the C++ spec for this purpose specifically. When it
// is unavailable we resort to the old implementation.
#ifdef HAVE_INTPTR_T
typedef std::intptr_t ags_t;
#else
typedef long ags_t;
#warning "The 'intptr_t' type is unavailable, resorting to 'long'. Alignment errors may occur in plugin function calls!"
#endif

#ifndef AGSMAIN
	#define AGSMAIN extern
#endif

#define AGS_DEFINE_CLASS(c)                                                      \
struct AGS ## c : public IAGSScriptManagedObject,                                \
                  public IAGSManagedObjectReader                                 \
{                                                                                \
	virtual const char *GetType() { return #c; }                                 \
	virtual int Dispose(const char *address, bool force);                        \
	virtual int Serialize(const char *address, char *buffer, int bufsize);       \
	virtual void Unserialize(int key, const char *serializedData, int dataSize); \
} AGSMAIN ags ## c;

//! Functions providing a platform independent API for the plugin
namespace AGSSockAPI {
	
extern IAGSEngine *engine; //!< AGS' engine plugin interface


//! Returns an AGS string containing an human-readable error message for a
//! specific error code.
const char *AGSFormatError(int errnum);
//! Terminate AGS when an unrecoverable error has occurred.
void AGSAbort(const char *msg);

//------------------------------------------------------------------------------

void Initialize(); //!< Initializes the API
void Terminate();  //!< Cleans up the API

//------------------------------------------------------------------------------

//! Mutual exclusion class
class Mutex
{
#ifdef _WIN32
	CRITICAL_SECTION cs;
	
	public:
	Mutex() { InitializeCriticalSection(&cs); }  //!< Initializes the mutex
	~Mutex() { DeleteCriticalSection(&cs); }     //!< Cleans after the mutex
	void lock() { EnterCriticalSection(&cs); }   //!< Acquire mutual exclusion
	void unlock() { LeaveCriticalSection(&cs); } //!< Leaves mutual exclusion
#else
	pthread_mutex_t mutex;
	
	public:
	Mutex() { pthread_mutex_init(&mutex, NULL); }
	~Mutex() { pthread_mutex_destroy(&mutex); }
	void lock() { pthread_mutex_lock(&mutex); }
	void unlock() { pthread_mutex_unlock(&mutex); }
#endif

	//! Scope based lock for a Mutex object
	class Lock
	{
		Mutex &mutex_;

		public:
		Lock(Mutex &mutex) : mutex_(mutex) { mutex_.lock(); }
		~Lock() { mutex_.unlock(); }

		Lock(const Lock &) = delete;
		void operator =(const Lock &) = delete;
	};

	Mutex(const Mutex &) = delete;
	void operator =(const Mutex &) = delete;
};

//------------------------------------------------------------------------------

//! Concurrent execution class
class Thread
{
	public:
	using Callback = std::function<void()>; //!< Asynchronously executed function type
	
	Thread(Callback); //!< Initializes the thread
	~Thread();        //!< Waits for the thread to finish and cleans up after
	
	void start();     //!< Start concurrent execution of the function
	void stop();      //!< Cancels operations of the asynchronously executed function
	bool active();    //!< Returns whether concurrent execution is active
	
	void exit();    //!< Call this before in the callback function before return.

	Thread(const Thread &) = delete;
	void operator =(const Thread &) = delete;
	
	private:
	struct Data;
	
	Callback func;
	Data *data;
};

//------------------------------------------------------------------------------

//! Inter-thread signalling class
class Beacon
{
	public:
	Beacon();
	~Beacon();
	
	//! Presents a socket to the listening party which can be signalled to
	//! \note The socket remains valid at least until it was signalled
	operator SOCKET()
	{
		#ifdef _WIN32
			return data.fd;
		#else
			return data.fd[0];
		#endif
	}
	
	void reset();  //!< Resets the beacon after use for re-use
	void signal(); //!< Signals the listening party

	Beacon(const Beacon &) = delete;
	void operator =(const Beacon &) = delete;
	
	private:
	struct Data
	{
		#ifdef _WIN32
			SOCKET fd;
		#else
			int fd[2];
		#endif
	} data;
};

//------------------------------------------------------------------------------

} /* namespace AGSSockAPI */

#endif /* _API_H */

//..............................................................................
