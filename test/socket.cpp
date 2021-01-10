/*******************************************************
 * Socket tests -- header file                         *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 14:48 2019-3-14                               *
 *                                                     *
 * Description: Testing the Socket AGS struct          *
 *******************************************************/

#include <cstdlib>
#include <iostream>
#include <string>

#include "agsmock/agsmock.h"
#include "Test.h"

#ifdef _WIN32
	#include <windows.h>
	#define m_sleep(x) Sleep(x)
#else
	#include <sys/socket.h>
	#include <unistd.h>
	#include <signal.h>
	#define m_sleep(x) usleep(x * 1000)
#endif

using std::cout;
using std::endl;

using std::string;

struct Socket
{
	int id; // just for padding, do not use
	AGSMock::ags_t domain, type, protocol;
	AGSMock::ags_t error;
};
struct SockAddr {};
struct SockData {};

// Error constant values returned by AGSEnumerateError, copy from API.h
#define AGSSOCK_NO_ERROR               0
#define AGSSOCK_OTHER_ERROR            1
#define AGSSOCK_ACCESS_DENIED          2
#define AGSSOCK_ADDRESS_NOT_AVAILABLE  3
#define AGSSOCK_PLEASE_TRY_AGAIN       4
#define AGSSOCK_SOCKET_NOT_VALID       5
#define AGSSOCK_DISCONNECTED           6
#define AGSSOCK_INVALID                7
#define AGSSOCK_UNSUPPORTED            8
#define AGSSOCK_HOST_NOT_REACHED       9
#define AGSSOCK_NOT_ENOUGH_RESOURCES  10
#define AGSSOCK_NETWORK_NOT_AVAILABLE 11
#define AGSSOCK_NOT_CONNECTED         12

//------------------------------------------------------------------------------

#define REPORT(x, sock) do { \
	if ((x) == 0) print_socket_error((sock).get()); } while (false)

void print_socket_error(Socket *sock)
{
	using namespace AGSMock;

	if (sock->error == 0)
		return;

	Handle<const char> error =
		Call<const char *>("Socket::ErrorString^0", sock);

	cout << "Error: " << error.get() << endl;
}

//------------------------------------------------------------------------------

Test test1("loading the plugin", []()
{
	using namespace AGSMock;

	LoadPlugin("agssock");

	Handle<Socket> sock = Call<Socket *>("Socket::CreateUDP^0");

	return true;
});

//------------------------------------------------------------------------------

Test test2("local UDP connection", []()
{
	using namespace AGSMock;

	cout << endl;

	// Set up a local connection between two UDP ports 
	Handle<Socket> to = Call<Socket *>("Socket::CreateUDP^0");
	Handle<Socket> from = Call<Socket *>("Socket::CreateUDP^0");
	EXPECT(Call<ags_t>("Socket::get_Valid", to.get()));
	EXPECT(Call<ags_t>("Socket::get_Valid", from.get()));

	{
		Handle<SockAddr> addr = Call<SockAddr *>("SockAddr::CreateFromString^2",
			"0.0.0.0", (ags_t) -1);
		ags_t ret = Call<ags_t>("Socket::Bind^1", to.get(), addr.get());
		REPORT(ret, to);
		EXPECT(ret);
	}

	{
		Handle<SockAddr> addr1 = Call<SockAddr *>("Socket::get_Local",
			to.get());
		ags_t port = Call<ags_t>("SockAddr::get_Port", addr1.get());

		Handle<SockAddr> addr2 = Call<SockAddr *>("SockAddr::CreateIP^2",
			"127.0.0.1", port);

		ags_t ret = Call<ags_t>("Socket::Connect^2", from.get(),
			addr2.get(), (ags_t) 0);
		REPORT(ret, from);
		EXPECT(ret);
	}

	// We send data from on to the other socket
	{
		ags_t ret = Call<ags_t>("Socket::Send^1", from.get(), "Test1234");
		REPORT(ret, from);
		EXPECT(ret);
	}

	// We expect to receive the data eventually
	for (int i = 0; i < 100; ++i)
	{
		Handle<const char> data = Call<const char *>("Socket::Recv^0", &*to);
		REPORT(!!data, to);
		EXPECT(data || to->error == 0);
		if (data)
		{
			EXPECT(string("Test1234") == data.get());
			break;
		}
		m_sleep(10);
	}

	// Close the sockets.
	Call<void>("Socket::Close^0", to.get());
	Call<void>("Socket::Close^0", from.get());

	// The sockets were closed, we expect them to become invalid eventually
	for (int i = 0; i < 100; ++i)
	{
		ags_t valid1 = Call<ags_t>("Socket::get_Valid", to.get());
		ags_t valid2 = Call<ags_t>("Socket::get_Valid", from.get());

		if (!valid1 && !valid2)
			break;

		m_sleep(10);
	}
	{
		ags_t valid1 = Call<ags_t>("Socket::get_Valid", to.get());
		ags_t valid2 = Call<ags_t>("Socket::get_Valid", from.get());
		EXPECT(!valid1 && !valid2);
	}

	return true;
});

//------------------------------------------------------------------------------

Test test3("local TCP connection", []()
{
	using namespace AGSMock;

	cout << endl;

	Handle<Socket> server = Call<Socket *>("Socket::CreateTCP^0");
	EXPECT(Call<ags_t>("Socket::get_Valid", server.get()));

	{
		Handle<SockAddr> addr = Call<SockAddr *>("SockAddr::CreateFromString^2",
			"0.0.0.0:8024", (ags_t) -1);
		ags_t ret = Call<ags_t>("Socket::Bind^1", server.get(), addr.get());
		REPORT(ret, server);
		EXPECT(ret);
	}

	{
		ags_t ret = Call<ags_t>("Socket::Listen^1", server.get(), (ags_t) 10);
		REPORT(ret, server);
		EXPECT(ret);
	}

	Handle<SockAddr> serv_addr;
	{
		Handle<SockAddr> addr = Call<SockAddr *>("Socket::get_Local", &*server);
		ags_t port = Call<ags_t>("SockAddr::get_Port", addr.get());
		serv_addr = Call<SockAddr *>("SockAddr::CreateIP^2", "127.0.0.1", port);
	}

	Handle<Socket> client = Call<Socket *>("Socket::CreateTCP^0");
	EXPECT(Call<ags_t>("Socket::get_Valid", client.get()));

	{
		ags_t ret = Call<ags_t>("Socket::Connect^2", client.get(),
			serv_addr.get(), (ags_t) 0);
		REPORT(ret, client);
		EXPECT(ret);
	}

	Handle<Socket> conn = Call<Socket *>("Socket::Accept^0", server.get());
	EXPECT(!!conn);

	{
		ags_t ret = Call<ags_t>("Socket::Send^1", client.get(), "Test1234");
		REPORT(ret, client);
		EXPECT(ret);
	}

	for (int i = 0; i < 100; ++i)
	{
		Handle<const char> data = Call<const char *>("Socket::Recv^0", &*conn);
		REPORT(!!data, conn);
		EXPECT(data || conn->error == 0);
		if (data)
		{
			EXPECT(string("Test1234") == data.get());
			break;
		}
		m_sleep(10);
	}

	{
		ags_t ret = Call<ags_t>("Socket::Send^1", conn.get(), "12345678");
		REPORT(ret, conn);
		EXPECT(ret);
	}

	for (int i = 0; i < 100; ++i)
	{
		Handle<const char> data = Call<const char *>("Socket::Recv^0", 
			client.get());
		REPORT(!!data, client);
		EXPECT(data || client->error == 0);
		if (data)
		{
			EXPECT(string("12345678") == data.get());
			break;
		}
		m_sleep(10);
	}

	Call<void>("Socket::Close^0", client.get());

	for (int i = 0; i < 1000; ++i)
	{
		Handle<const char> data = Call<const char *>("Socket::Recv^0", &*conn);
		REPORT(!!data, conn);
		EXPECT(data || conn->error == 0);
		if (data)
		{
			EXPECT(string() == data.get());
			EXPECT(!Call<ags_t>("Socket::get_Valid", conn.get()));
			break;
		}
		m_sleep(10);
	}

	Call<void>("Socket::Close^0", server.get());

	{
		Handle<Socket> client = Call<Socket *>("Socket::CreateTCP^0");
		ags_t ret = Call<ags_t>("Socket::Connect^2", client.get(),
			serv_addr.get(), (ags_t) 0);
		EXPECT(!ret);
	}

	return true;
});

//------------------------------------------------------------------------------

Test test4("error values", []()
{
	using namespace AGSMock;

	{
		Handle<Socket> sock = Call<Socket *>("Socket::CreateTCP^0");
		EXPECT(Call<ags_t>("Socket::get_Valid", sock.get()));
		ags_t error = Call<ags_t>("Socket::ErrorValue^0", sock.get());
		EXPECT(error == AGSSOCK_NO_ERROR);
	}

	{
		Handle<Socket> sock = Call<Socket *>("Socket::CreateUDP^0");
		EXPECT(Call<ags_t>("Socket::get_Valid", sock.get()));
		Handle<SockAddr> addr = Call<SockAddr *>("SockAddr::CreateIP^2",
			"255.255.255.255", (ags_t) 8024);
		Call<ags_t>("Socket::SendTo^2", sock.get(), addr.get(), "Test1234");
		ags_t error = Call<ags_t>("Socket::ErrorValue^0", sock.get());
		EXPECT(error == AGSSOCK_ACCESS_DENIED);
	}

	{
		Handle<SockAddr> addr1 = Call<SockAddr *>("SockAddr::CreateIP^2",
			"0.0.0.0", (ags_t) 8024);
		Handle<SockAddr> addr2 = Call<SockAddr *>("SockAddr::CreateIP^2",
			"0.0.0.0", (ags_t) 8024);
		Handle<Socket> sock1 = Call<Socket *>("Socket::CreateTCP^0");
		EXPECT(Call<ags_t>("Socket::get_Valid", sock1.get()));
		Call<ags_t>("Socket::Bind^1", sock1.get(), addr1.get());
		Handle<Socket> sock2 = Call<Socket *>("Socket::CreateTCP^0");
		Call<ags_t>("Socket::Bind^1", sock2.get(), addr2.get());
		EXPECT(Call<ags_t>("Socket::get_Valid", sock2.get()));
		ags_t error = Call<ags_t>("Socket::ErrorValue^0", sock2.get());
		EXPECT(error == AGSSOCK_ADDRESS_NOT_AVAILABLE);
	}

	{
		Handle<SockAddr> addr1 = Call<SockAddr *>("SockAddr::CreateIP^2",
			"0.0.0.0", (ags_t) 0);
		Handle<SockAddr> addr2 = Call<SockAddr *>("SockAddr::CreateIP^2",
			"0.0.0.0", (ags_t) 8024);
		Handle<Socket> sock = Call<Socket *>("Socket::CreateUDP^0");
		EXPECT(Call<ags_t>("Socket::get_Valid", sock.get()));
		Call<ags_t>("Socket::Bind^1", sock.get(), addr1.get());
		Call<SockData *>("Socket::RecvDataFrom^1", sock.get(), addr2.get());
		ags_t error = Call<ags_t>("Socket::ErrorValue^0", sock.get());
		EXPECT(error == AGSSOCK_PLEASE_TRY_AGAIN);
	}

	{
		Handle<SockAddr> addr1 = Call<SockAddr *>("SockAddr::CreateIP^2",
			"0.0.0.0", (ags_t) 0);
		Handle<Socket> sock = Call<Socket *>("Socket::Create^3", 1, 2, 3);
		Call<ags_t>("Socket::Bind^1", sock.get(), addr1.get());
		ags_t error = Call<ags_t>("Socket::ErrorValue^0", sock.get());
		EXPECT(error == AGSSOCK_SOCKET_NOT_VALID);
		EXPECT(!Call<ags_t>("Socket::get_Valid", sock.get()));
	}

	// AGSSOCK_DISCONNECTED: difficult to test a connection reset

	{
		Handle<SockAddr> addr1 = Call<SockAddr *>("SockAddr::CreateIP^2",
			"0.0.0.0", (ags_t) 0);
		Handle<SockAddr> addr2 = Call<SockAddr *>("SockAddr::CreateIP^2",
			"0.0.0.0", (ags_t) 0);
		Handle<Socket> sock = Call<Socket *>("Socket::CreateTCP^0");
		EXPECT(Call<ags_t>("Socket::get_Valid", sock.get()));
		Call<ags_t>("Socket::Bind^1", sock.get(), addr1.get());
		Call<ags_t>("Socket::Bind^1", sock.get(), addr2.get());
		ags_t error = Call<ags_t>("Socket::ErrorValue^0", sock.get());
		EXPECT(error == AGSSOCK_INVALID);
	}

	{
		Handle<Socket> sock = Call<Socket *>("Socket::CreateUDP^0");
		EXPECT(Call<ags_t>("Socket::get_Valid", sock.get()));
		Handle<Socket> conn = Call<Socket *>("Socket::Accept^0", sock.get());
		ags_t error = Call<ags_t>("Socket::ErrorValue^0", sock.get());
		EXPECT(error == AGSSOCK_UNSUPPORTED);
	}

	// Not tested:
	// AGSSOCK_HOST_NOT_REACHED
	// AGSSOCK_NOT_ENOUGH_RESOURCES
	// AGSSOCK_NETWORK_NOT_AVAILABLE

	{
#if defined(__unix__) || defined(__APPLE__)
		// Temporarily disable the SIGPIPE signal
		const struct sigaction ignore_handler{SIG_IGN};
		struct sigaction default_handler;
		sigaction(SIGPIPE, &ignore_handler, &default_handler);
#endif
		Handle<Socket> sock = Call<Socket *>("Socket::CreateTCP^0");
		EXPECT(Call<ags_t>("Socket::get_Valid", sock.get()));
		Call<ags_t>("Socket::Send^1", sock.get(), "Test1234");
		ags_t error = Call<ags_t>("Socket::ErrorValue^0", sock.get());
		EXPECT(error == AGSSOCK_NOT_CONNECTED);
#if defined(__unix__) || defined(__APPLE__)
		sigaction(SIGPIPE, &default_handler, nullptr);
#endif
	}

	return true;
});

//------------------------------------------------------------------------------

int main(int argc, char const *argv[])
{
	AGSMock::Initialize();
	bool result = Test::run_tests();
	AGSMock::Terminate();
	return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

//..............................................................................
