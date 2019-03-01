/*******************************************************
 * Socket pool tests -- header file                    *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 14:00 2019-2-22                               *
 *                                                     *
 * Description: Testing the socket pool class          *
 *******************************************************/

#include <algorithm>
#include <iostream>

#include "Socket.h"
#include "Pool.h"
#include "API.h"
#include "Test.h"

#ifdef _WIN32
	#include <windows.h>
	#define m_sleep(x) Sleep(x)
#else
	#include <unistd.h>
	#define m_sleep(x) usleep(x * 1000)
#endif

#define REPORT(x) do { \
	if ((x) == SOCKET_ERROR) print_socket_error(); } while (false)

using namespace AGSSock;

using AGSSockAPI::Mutex;

//------------------------------------------------------------------------------

void print_socket_error()
{
	using namespace std;

#ifdef _WIN32
	LPSTR msg = nullptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	              0, WSAGetLastError(), 0, (LPSTR)&msg, 0, 0);
	cout << msg << endl;
	LocalFree(msg);
#else
	cout << strerror(errno) << endl;
#endif
}

//------------------------------------------------------------------------------

Socket create_udp_socket()
{
	SOCKET id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	ags_t error = GET_ERROR();

	Socket sock
	{
		id,
		AF_INET, SOCK_DGRAM, IPPROTO_UDP,
		error,
		nullptr, nullptr,"",{}
	};

	return sock;
}

//------------------------------------------------------------------------------

bool create_udp_tunnel(Socket &from, Socket &to)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = 0;

	if (bind(to.id, (sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
	{
		print_socket_error();
		return false;
	}

	ADDRLEN addrlen = sizeof (addr);
	if (getsockname(to.id, (sockaddr *) &addr, &addrlen) == SOCKET_ERROR)
	{
		print_socket_error();
		return false;
	}

	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (connect(from.id, (sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
	{
		print_socket_error();
		return false;
	}

	return true;
}

//==============================================================================

Test test1("pool generic construction/destruction", []()
{
	using namespace std;

	cout << endl;
	{
		Pool pool;
		EXPECT(pool);

		Socket sock = create_udp_socket();
		EXPECT(sock.id != INVALID_SOCKET);

		pool.add(&sock);
		EXPECT(pool);

		pool.remove(&sock);
		EXPECT(pool);

		// The pool is empty, the thread should shut itself down eventually
		for (int i = 0; i < 100; ++i)
		{
			if (!pool.active())
				break;
			m_sleep(10);
		}
		EXPECT(!pool.active());

		closesocket(sock.id);
	}

	return true;
});

//------------------------------------------------------------------------------

Test test2("pool read cycle", []()
{
	using namespace std;

	cout << endl;
	{
		Pool pool;
		EXPECT(pool);

		Socket sock_in = create_udp_socket();
		EXPECT(sock_in.id != INVALID_SOCKET);

		Socket sock_out = create_udp_socket();
		EXPECT(sock_out.id != INVALID_SOCKET);

		EXPECT(create_udp_tunnel(sock_in, sock_out) == true);
		setblocking(sock_out.id, false);

		pool.add(&sock_out);
		EXPECT(pool);

		char data[4] = {0x12, 0x34, 0x56, 0x78};
		int ret = send(sock_in.id, data, sizeof (data), 0);
		REPORT(ret);
		EXPECT(ret != SOCKET_ERROR);

		// We sent data, the read cycle should put it in the buffer eventually
		for (int i = 0; i < 100; ++i)
		{
			{
				Mutex::Lock lock(pool);

				if (!sock_out.incoming.empty())
					break;
			}
			m_sleep(10);
		}
		{
			Mutex::Lock lock(pool);

			EXPECT(!sock_out.incoming.empty());
			EXPECT(std::equal(data, data + sizeof(data),
				sock_out.incoming.front().data()));

			sock_out.incoming.pop();
		}

		pool.remove(&sock_out);
		EXPECT(pool);

		closesocket(sock_out.id);
		closesocket(sock_in.id);
	}

	return true;
});

//------------------------------------------------------------------------------

Test test3("pool read cycle interruptions", []()
{
	using namespace std;

	cout << endl;
	{
		Pool pool;
		EXPECT(pool);

		Socket sock_in[2] = {create_udp_socket(), create_udp_socket()};
		Socket sock_out[2] = {create_udp_socket(), create_udp_socket()};
		EXPECT(sock_in[0].id != INVALID_SOCKET);
		EXPECT(sock_in[1].id != INVALID_SOCKET);
		EXPECT(sock_out[0].id != INVALID_SOCKET);
		EXPECT(sock_out[1].id != INVALID_SOCKET);

		EXPECT(create_udp_tunnel(sock_in[0], sock_out[0]) == true);
		EXPECT(create_udp_tunnel(sock_in[1], sock_out[1]) == true);
		setblocking(sock_out[0].id, false);
		setblocking(sock_out[1].id, false);

		// The first addition to the pool should activate the read cycle. There,
		// it waits for input of this socket. We immediately add another one;
		// now waiting for input should be interrupted so we can instead wait
		// for input of both sockets.
		pool.add(&sock_out[0]);
		pool.add(&sock_out[1]);

		char data[4] = {0x12, 0x34, 0x56, 0x78};
		int ret = send(sock_in[1].id, data, sizeof (data), 0);
		REPORT(ret);
		EXPECT(ret != SOCKET_ERROR);

		// We sent data to the second socket, it should be read eventually
		for (int i = 0; i < 100; ++i)
		{
			{
				Mutex::Lock lock(pool);

				if (!sock_out[1].incoming.empty())
					break;
			}
			m_sleep(10);
		}
		{
			Mutex::Lock lock(pool);

			EXPECT(sock_out[0].incoming.empty());
			EXPECT(!sock_out[1].incoming.empty());
			EXPECT(std::equal(data, data + sizeof(data),
				sock_out[1].incoming.front().data()));

			sock_out[1].incoming.pop();
		}

		pool.clear();
		EXPECT(pool);

		// The pool is empty, the thread should shut itself down eventually
		for (int i = 0; i < 100; ++i)
		{
			if (!pool.active())
				break;
			m_sleep(10);
		}
		EXPECT(!pool.active());

		closesocket(sock_out[1].id);
		closesocket(sock_out[0].id);
		closesocket(sock_in[1].id);
		closesocket(sock_in[0].id);
	}

	return true;
});

//------------------------------------------------------------------------------

Test test4("pool read errors", []()
{
	using namespace std;

	cout << endl;
	{
		Pool pool;
		EXPECT(pool);

		Socket sock_in = create_udp_socket();
		Socket sock_out = create_udp_socket();
		EXPECT(sock_in.id != INVALID_SOCKET);
		EXPECT(sock_out.id != INVALID_SOCKET);

		EXPECT(create_udp_tunnel(sock_in, sock_out) == true);
		setblocking(sock_out.id, false);

		pool.add(&sock_out);
		EXPECT(pool);

		char data[4] = {0x12, 0x34, 0x56, 0x78};
		int ret = send(sock_in.id, data, sizeof (data), 0);
		REPORT(ret);
		EXPECT(ret != SOCKET_ERROR);

		// We expect the sent data to be in the buffer, eventually
		for (int i = 0; i < 100; ++i)
		{
			{
				Mutex::Lock lock(pool);

				if (!sock_out.incoming.empty())
					break;
			}
			m_sleep(10);
		}
		{
			Mutex::Lock lock(pool);

			EXPECT(!sock_out.incoming.empty());
			EXPECT(std::equal(data, data + sizeof(data),
				sock_out.incoming.front().data()));

			sock_out.incoming.pop();
		}

		// closing the sockets should cause read errors
		closesocket(sock_out.id);
		closesocket(sock_in.id);
		pool.add(&sock_out); // force the read cycle to be signalled
		// Note: Windows signals the read cycle automatically when sockets close

		// We expect the read error to be in the buffer eventually
		for (int i = 0; i < 100; ++i)
		{
			{
				Mutex::Lock lock(pool);

				if (sock_out.incoming.error != 0)
					break;
			}
			m_sleep(10);
		}
		{
			Mutex::Lock lock(pool);
			
			EXPECT(sock_out.incoming.error != 0);
		}
		EXPECT(pool);

		// The pool should have removed the faulted socket, thus become empty
		// and should shut itself down, eventually
		for (int i = 0; i < 100; ++i)
		{
			if (!pool.active())
				break;
			m_sleep(10);
		}
		EXPECT(!pool.active());
	}

	return true;
});

//------------------------------------------------------------------------------

int main(int argc, char const *argv[])
{
	using namespace std;
	AGSSockAPI::Initialize();
	bool result = Test::run_tests();
	AGSSockAPI::Terminate();
	return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

//..............................................................................
