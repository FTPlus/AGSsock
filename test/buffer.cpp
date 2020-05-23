/*******************************************************
 * Socket buffer tests -- header file                  *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 15:36 2019-2-8                                *
 *                                                     *
 * Description: Testing the socket buffer class        *
 *******************************************************/

#include <cstdlib>
#include <iostream>

#include "Buffer.h"
#include "Test.h"

using namespace AGSSock;

//------------------------------------------------------------------------------

Test test1("buffers with datagram inputs", []()
{
	Buffer buffer;
	EXPECT(buffer.empty());

	buffer.push("ABC", 3);
	EXPECT(!buffer.empty());
	buffer.push("DEF", 3);
	buffer.push(nullptr, 0);
	buffer.push("XYZ", 3);

	EXPECT(buffer.front() == "ABC");
	buffer.pop();
	EXPECT(!buffer.empty());

	EXPECT(buffer.front() == "DEF");
	buffer.pop();
	EXPECT(!buffer.empty());

	EXPECT(buffer.front().size() == 0);
	buffer.pop();
	EXPECT(!buffer.empty());

	EXPECT(buffer.front() == "XYZ");
	buffer.pop();
	EXPECT(buffer.empty());

	return true;
});

//------------------------------------------------------------------------------

Test test2("buffers with stream inputs", []()
{
	Buffer buffer;
	EXPECT(buffer.empty());

	buffer.append("ABC", 3);
	EXPECT(!buffer.empty());
	buffer.append("DEF\0XYZ\0\0Q", 10);
	buffer.append("\0\0", 2);
	buffer.append(nullptr, 0);

	// We expect the buffer to contain the concatenated data so far,
	// including null-characters.
	EXPECT(buffer.front().size() == 15);
	// But the first string read back should end at the first null-character
	EXPECT(std::string(buffer.front().c_str()) == "ABCDEF");
	buffer.extract();

	EXPECT(!buffer.empty());
	EXPECT(buffer.front().size() == 8);
	EXPECT(std::string(buffer.front().c_str()) == "XYZ");
	buffer.extract();

	EXPECT(!buffer.empty());
	EXPECT(buffer.front().size() == 3);
	EXPECT(std::string(buffer.front().c_str()) == "Q");
	buffer.extract();

	EXPECT(!buffer.empty());
	EXPECT(buffer.front().size() == 0);
	buffer.extract();

	EXPECT(buffer.empty());

	return true;
});

int main(int argc, char const *argv[])
{
	return Test::run_tests() ? EXIT_SUCCESS : EXIT_FAILURE;
}

//..............................................................................
