/*******************************************************
 * Input pool -- header file                           *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 14:41 2019-2-9                                *
 *                                                     *
 * Description: Provides a threaded read cycle that    *
 *              fills the incoming data buffer for a   *
 *              pool of sockets.                       *
 *******************************************************/

#ifndef _POOL_H
#define _POOL_H

#include <unordered_set>

#include "API.h"
#include "Socket.h"

namespace AGSSock {

//------------------------------------------------------------------------------

//! Sockets pool

//! Allows sockets to be registered to a pool for which the incomming data is
//! processed by a threaded read cycle.
//! \warning Lock the pool when using id, protocol or the incomming buffer of
//! a socket when it is registered to the pool to prevent race-conditions.
class Pool
{
	using Mutex = AGSSockAPI::Mutex;
	using Beacon = AGSSockAPI::Beacon;
	using Thread = AGSSockAPI::Thread;
	using Sockets = std::unordered_set<Socket *>;

	// Note: the order ensures the destructors are called in the right order.
	// Failing to do so may cause race-codnitions.
	Sockets sockets_; //!< The set of all registered sockets.
	Mutex guard_;     //!< Guards the pool, pool signal and the incoming buffers
	Beacon beacon_;   //!< Signals addition or removal of sockets in the pool
	Thread thread_;   //!< Thread that processes incoming data of pool sockets

	void run(); //!< Read cycle for pool sockets

	public:
	Pool() : thread_([this]() { run(); }) {}

	void add(Socket *);    //!< Registers a socket at the pool for processing
	void remove(Socket *); //!< Unregisters a proviously added socket
	void clear();          //!< Unregisters all pool sockets

	//! Allows the pool to be used in a Mutex lock object
	operator Mutex &() { return guard_; }

	Pool(const Pool &) = delete;
	void operator =(const Pool &) = delete;
};

//------------------------------------------------------------------------------

} // namespace AGSSock

#endif // _POOL_H

//..............................................................................
