/*******************************************************
 * Input pool -- See header file for more information. *
 *******************************************************/

#ifdef NDEBUG
	#define DEBUG_P(x) ((void) 0)
#else
	#include <cstdio>
	#define DEBUG_P(x) std::puts("\t\t" x);
#endif

#include "Pool.h"

namespace AGSSock {

using namespace AGSSockAPI;

// Invariant I: thread_->active() == (sockets_.size() > 0)
// Invariant II: (sock->id == INVALID_SOCKET) => !sockets_.count(sock)

//------------------------------------------------------------------------------

void Pool::run()
{
	SOCKET signal = beacon_;
	fd_set read;
	int nfds;
	
	DEBUG_P("Thread started");
	for (;;) { /* event loop */
	
	// Reset FD sets
	FD_ZERO(&read);
	FD_SET(signal, &read);
	nfds = signal;
	
	// Add pool sockets to FD sets
	{
		Mutex::Lock lock(guard_);
		
		for (Socket *sock : sockets_)
		{
			FD_SET(sock->id, &read);
			// Windows ignores the nfds parameter, can we default to this?
			#ifndef _WIN32
				if (nfds < sock->id)
					nfds = sock->id;
			#endif
		}
	}
	
	// Wait for events
	select(nfds + 1, &read, nullptr, nullptr, nullptr);
	// If select errs a socket was most likely closed locally, this is fine.
	// We need to check which one(s) and ignore all 'would block's.
	
	// Process read and error events
	{
		Mutex::Lock lock(guard_);
		
		if (FD_ISSET(signal, &read))
		{
			beacon_.reset();
			signal = beacon_;
			DEBUG_P("Thread signalled");
		}

		for (Sockets::iterator it = sockets_.begin(); it != sockets_.end();)
		{
			Socket *sock = *it;

			if (FD_ISSET(sock->id, &read))
			{
				char buffer[65536];
				int ret = recv(sock->id, buffer, sizeof (buffer), 0);
				int error = GET_ERROR();
				
				// We ignore sockets that would block:
				// This is normally filtered by select but a signal could have
				// interrupted select.
				if (ret == SOCKET_ERROR && WOULD_BLOCK(error))
				{
					++it;
					continue;
				}
				
				// If ret == 0 then closed gracefully (for TCP)
				// If ret == SOCKET_ERROR probably closed not so gracefully
				
				if (ret == SOCKET_ERROR)
					sock->incoming.error = error;
				else if (sock->protocol == IPPROTO_TCP)
					sock->incoming.append(buffer, ret);
				else
					sock->incoming.push(buffer, ret);
				
				if ((ret == SOCKET_ERROR)
					|| (!ret && sock->protocol == IPPROTO_TCP))
				{
					sockets_.erase(it++); // This socket is done for, stop reading
					continue;
				}	
			}

			++it;
		}
		
		// Close thread if there are no sockets to process anymore
		// Note: This is safe because the thread will be (re)started when
		//       sockets are added to the pool which requires the pool lock.
		if (sockets_.empty())
		{
			DEBUG_P("Thread finished");
			thread_.exit();
			return;
		}
	}
	
	} /* event loop */
	
	// Todo: chain the two critical sections for efficiency (one loop)
	
	DEBUG_P("Thread cancelled");
	thread_.exit();
	return;
}

//------------------------------------------------------------------------------

void Pool::add(Socket *sock)
{
	Mutex::Lock lock(guard_);

	if (sockets_.insert(sock).second && sockets_.size() == 1)
		thread_.start();
	else
		beacon_.signal();
}

void Pool::remove(Socket *sock)
{
	Mutex::Lock lock(guard_);

	if (sockets_.erase(sock))
		beacon_.signal();

	// Signalling might not be necessary for windows: closing sockets might
	// already trigger select.
}

void Pool::clear()
{
	Mutex::Lock lock(guard_);

	sockets_.clear();
	beacon_.signal();
}

//------------------------------------------------------------------------------

} // namespace AGSSock

//..............................................................................
