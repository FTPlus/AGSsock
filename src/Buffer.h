/*******************************************************
 * Data buffer class -- header file                    *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 15:29 23-8-2012                               *
 *                                                     *
 * Description: Provides a way to store and retrieve   *
 *              arbitrary sized data packages or       *
 *              streams.                               *
 *******************************************************/

#ifndef _BUFFER_H
#define _BUFFER_H

#include <cstddef>
#include <queue>
#include <string>

namespace AGSSock {

//------------------------------------------------------------------------------

//! Socket buffer

//! A data structure that enqueues both packet based and streaming data.
class Buffer
{
	using string = std::string;

	std::queue<string> queue_;

	public:
	int error; //!< A potential error code the last operation caused
	
	Buffer() : error(0) {}

	//! Access the first element of the buffer
	inline string &front()
		{ return queue_.front(); }
	inline const string &front() const
		{ return queue_.front(); }

	//! Returns if the buffer is empty
	inline bool empty() const
		{ return queue_.empty(); }

	//! Adds a new data-string to the buffer (back)
	inline void push(const char *data, size_t count)
		{ queue_.push(string(data, count)); }

	//! Removes the first element of the buffer
	inline void pop()
		{ queue_.pop(); }
	
	//! Appends a data-string to the (last element of the) buffer
	//! \note zero-length strings indicate EoF,
	//! and are stored in a fresh buffer element
	inline void append(const char *data, size_t count)
	{
		if (queue_.empty() || count == 0)
			queue_.push(string(data, count));
		else
			queue_.back().append(data, count);
	}
	
	//! Removes the first zero-terminated string from the buffer.
	//! \note Spurrious null-characters are also removed.
	//! \warning The buffer should not be empty.
	void extract();
};

//------------------------------------------------------------------------------

} /* namespace AGSSock */

#endif /* _BUFFER_H */

//..............................................................................
