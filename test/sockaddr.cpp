/*******************************************************
 * SockAddr tests -- header file                       *
 *                                                     *
 * Author: Ferry "Wyz" Timmers                         *
 *                                                     *
 * Date: 16:42 2019-3-8                                *
 *                                                     *
 * Description: Testing the SockAddr AGS struct        *
 *******************************************************/

#include <cstdlib>
#include <iostream>
#include <string>

#include "agsmock/agsmock.h"
#include "Test.h"

#ifdef _WIN32
	#include <windows.h>
#else
	#include <sys/socket.h>
#endif

using std::string;

struct SockAddr {};
struct SockData {};

//------------------------------------------------------------------------------

Test test1("loading the plugin", []()
{
	using namespace AGSMock;

	LoadPlugin("agssock");

	Handle<SockAddr> addr = Call<SockAddr *>("SockAddr::Create^1", (ags_t) -1);

	return true;
});

//------------------------------------------------------------------------------

Test test2("plain IPv4 addresses", []()
{
	using namespace AGSMock;

	Handle<SockAddr> addr = Call<SockAddr *>("SockAddr::CreateIP^2", 
		"127.0.0.1", (ags_t) 0x1234);

	{
		int port = Call<ags_t>("SockAddr::get_Port", addr.get());
		EXPECT(port == 0x1234);

		Handle<const char> ip = Call<const char *>("SockAddr::get_IP", &*addr);
		EXPECT(string("127.0.0.1") == ip.get());
	}

	Call<void>("SockAddr::set_Port", addr.get(), (ags_t) 0x5678);
	Call<void>("SockAddr::set_IP", addr.get(), "12.34.56.78");

	{
		int port = Call<ags_t>("SockAddr::get_Port", addr.get());
		EXPECT(port == 0x5678);

		Handle<const char> ip = Call<const char *>("SockAddr::get_IP",
			addr.get());
		EXPECT(string("12.34.56.78") == ip.get());
	}

	return true;
});

//------------------------------------------------------------------------------

Test test3("plain IPv6 addresses", []()
{
	using namespace AGSMock;
	
	Handle<SockAddr> addr = Call<SockAddr *>("SockAddr::CreateIPv6^2", 
		"::1", (ags_t) 0x1234);

	{
		int port = Call<ags_t>("SockAddr::get_Port", addr.get());
		EXPECT(port == 0x1234);

		Handle<const char> ip = Call<const char *>("SockAddr::get_IP",
			addr.get());
		EXPECT(string("::1") == ip.get());
	}

	const char *long_ip = "0:1234::5678:9:abcd:ef";
	Call<void>("SockAddr::set_Port", addr.get(), (ags_t) 0x5678);
	Call<void>("SockAddr::set_IP", addr.get(), long_ip);

	{
		int port = Call<ags_t>("SockAddr::get_Port", addr.get());
		EXPECT(port == 0x5678);

		Handle<const char> ip = Call<const char *>("SockAddr::get_IP",
			addr.get());
		EXPECT(string(long_ip) == ip.get());
	}

	return true;
});

//------------------------------------------------------------------------------

Test test4("resolving addresses", []()
{
	using namespace AGSMock;

	Handle<SockAddr> addr = Call<SockAddr *>("SockAddr::CreateFromString^2",
		"http://localhost", (ags_t) AF_INET);

	{
		int port = Call<ags_t>("SockAddr::get_Port", addr.get());
		EXPECT(port == 80);

		Handle<const char> ip = Call<const char *>("SockAddr::get_IP", &*addr);
		EXPECT(string("127.0.0.1") == ip.get());
	}

	Call<void>("SockAddr::set_Address", addr.get(), "irc://localhost:6667");

	{
		int port = Call<ags_t>("SockAddr::get_Port", addr.get());
		EXPECT(port == 6667);

		Handle<const char> ip = Call<const char *>("SockAddr::get_IP", &*addr);
		EXPECT(string("127.0.0.1") == ip.get());
	}

	return true;
});

//------------------------------------------------------------------------------

Test test5("reverse resolving **internet access required**", []()
{
	using namespace AGSMock;

	Handle<SockAddr> addr = Call<SockAddr *>("SockAddr::Create^1",
		(ags_t) AF_INET);

	Call<void>("SockAddr::set_Address", addr.get(), "8.8.8.8:53");

	{
		int port = Call<ags_t>("SockAddr::get_Port", addr.get());
		EXPECT(port == 53);

		Handle<const char> ip = Call<const char *>("SockAddr::get_IP", &*addr);
		EXPECT(string("8.8.8.8") == ip.get());

		Handle<const char> str = Call<const char *>("SockAddr::get_Address",
			addr.get());
		EXPECT(string("domain://dns.google") == str.get());
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
