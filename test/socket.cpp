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
	EXPECT(Call<ags_ret_t>("Socket::get_Valid", to.get()));
	EXPECT(Call<ags_ret_t>("Socket::get_Valid", from.get()));

	{
		Handle<SockAddr> addr = Call<SockAddr *>("SockAddr::CreateFromString^2",
			"0.0.0.0", (ags_t) AF_INET);
		int ret = Call<ags_ret_t>("Socket::Bind^1", to.get(), addr.get());
		REPORT(ret, to);
		EXPECT(ret);
	}

	{
		Handle<SockAddr> addr1 = Call<SockAddr *>("Socket::get_Local",
			to.get());
		int port = Call<ags_ret_t>("SockAddr::get_Port", addr1.get());

		Handle<SockAddr> addr2 = Call<SockAddr *>("SockAddr::CreateIP^2",
			"127.0.0.1", port);

		int ret = Call<ags_ret_t>("Socket::Connect^2", from.get(),
			addr2.get(), (ags_t) 0);
		REPORT(ret, from);
		EXPECT(ret);
	}

	// We send data from on to the other socket
	{
		int ret = Call<ags_ret_t>("Socket::Send^1", from.get(), "Test1234");
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
		int valid1 = Call<ags_ret_t>("Socket::get_Valid", to.get());
		int valid2 = Call<ags_ret_t>("Socket::get_Valid", from.get());

		if (!valid1 && !valid2)
			break;

		m_sleep(10);
	}
	{
		int valid1 = Call<ags_ret_t>("Socket::get_Valid", to.get());
		int valid2 = Call<ags_ret_t>("Socket::get_Valid", from.get());
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
	EXPECT(Call<ags_ret_t>("Socket::get_Valid", server.get()));

	{
		Handle<SockAddr> addr = Call<SockAddr *>("SockAddr::CreateFromString^2",
			"0.0.0.0:7200", (ags_t) AF_INET);
		int ret = Call<ags_ret_t>("Socket::Bind^1", server.get(), addr.get());
		REPORT(ret, server);
		EXPECT(ret);
	}

	{
		int ret = Call<ags_ret_t>("Socket::Listen^1", server.get(), (ags_t) 10);
		REPORT(ret, server);
		EXPECT(ret);
	}

	Handle<SockAddr> serv_addr;
	{
		Handle<SockAddr> addr = Call<SockAddr *>("Socket::get_Local", &*server);
		int port = Call<ags_ret_t>("SockAddr::get_Port", addr.get());
		serv_addr = Call<SockAddr *>("SockAddr::CreateIP^2", "127.0.0.1", port);
	}

	Handle<Socket> client = Call<Socket *>("Socket::CreateTCP^0");
	EXPECT(Call<ags_t>("Socket::get_Valid", client.get()));

	{
		int ret = Call<ags_ret_t>("Socket::Connect^2", client.get(),
			serv_addr.get(), (ags_t) 0);
		REPORT(ret, client);
		EXPECT(ret);
	}

	Handle<Socket> conn = Call<Socket *>("Socket::Accept^0", server.get());
	EXPECT(!!conn);

	{
		int ret = Call<ags_ret_t>("Socket::Send^1", client.get(), "Test1234");
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
		int ret = Call<ags_ret_t>("Socket::Send^1", conn.get(), "12345678");
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
			EXPECT(!Call<ags_ret_t>("Socket::get_Valid", conn.get()));
			break;
		}
		m_sleep(10);
	}

	Call<void>("Socket::Close^0", server.get());

	{
		Handle<Socket> client = Call<Socket *>("Socket::CreateTCP^0");
		int ret = Call<ags_ret_t>("Socket::Connect^2", client.get(),
			serv_addr.get(), (ags_t) 0);
		EXPECT(!ret);
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
