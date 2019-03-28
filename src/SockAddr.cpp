/*********************************************************************
 * Socket address interface -- See header file for more information. *
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "SockAddr.h"

namespace AGSSock {

//------------------------------------------------------------------------------

void decode_type(ags_t &type)
{
	if (type == -1)
		type = AF_INET;
	else if (type == -2)
		type = AF_INET6;
}

//------------------------------------------------------------------------------

int AGSSockAddr::Dispose(const char *addr, bool force)
{
	delete (SockAddr *) addr;
	return 1;
}

//------------------------------------------------------------------------------

int AGSSockAddr::Serialize(const char *addr, char *buffer, int size)
{
	size = MIN(size, sizeof (SockAddr));
	memcpy(buffer, addr, size);
	return size;
}

//------------------------------------------------------------------------------

void AGSSockAddr::Unserialize(int key, const char *buffer, int size)
{
	size = MIN(size, sizeof (SockAddr));
	SockAddr *addr = new SockAddr; // Default-intitalized by design
	memcpy(addr, buffer, size);
	AGS_RESTORE(SockAddr, addr, key);
}

//==============================================================================

SockAddr *SockAddr_Create(ags_t type)
{
	decode_type(type);
	SockAddr *addr = new SockAddr; // by design
	AGS_OBJECT(SockAddr, addr);
	memset(addr, 0, sizeof (SockAddr));
	ADDR_INIT(addr, type); // update definition when new address types are added
	addr->ss_family = type;
	return addr;
}

//------------------------------------------------------------------------------

SockAddr *SockAddr_CreateFromString(const char *str, ags_t type)
{
	decode_type(type);
	SockAddr *addr = SockAddr_Create(type);
	SockAddr_set_Address(addr, str);
	return addr;
}

//------------------------------------------------------------------------------

SockAddr *SockAddr_CreateFromData(const SockData *data)
{
	SockAddr *addr = new SockAddr; // by design
	AGS_OBJECT(SockAddr, addr);
	memcpy(addr, data->data.data(), MIN(data->data.size(), sizeof (SockAddr)));
	return addr;
}

//------------------------------------------------------------------------------

SockAddr *SockAddr_CreateIP(const char *ip, ags_t port)
{
	SockAddr *addr = SockAddr_Create(AF_INET);
	SockAddr_set_IP(addr, ip);
	SockAddr_set_Port(addr, port);
	return addr;
}

//------------------------------------------------------------------------------

SockAddr *SockAddr_CreateIPv6(const char *ip, ags_t port)
{
	SockAddr *addr = SockAddr_Create(AF_INET6);
	SockAddr_set_IP(addr, ip);
	SockAddr_set_Port(addr, port);
	return addr;
}

//------------------------------------------------------------------------------

ags_t SockAddr_get_Port(SockAddr *sa)
{
	if (sa->ss_family == AF_INET)
	{
		sockaddr_in *addr = reinterpret_cast<sockaddr_in *> (sa);
		return ntohs(addr->sin_port);
	}
	else if (sa->ss_family == AF_INET6)
	{
		sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *> (sa);
		return ntohs(addr->sin6_port);
	}
	else
		return 0;
}

//------------------------------------------------------------------------------

void SockAddr_set_Port(SockAddr *sa, ags_t port)
{
	if (sa->ss_family == AF_INET)
	{
		sockaddr_in *addr = reinterpret_cast<sockaddr_in *> (sa);
		addr->sin_port = htons(port);
	}
	else if (sa->ss_family == AF_INET6)
	{
		sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *> (sa);
		addr->sin6_port = htons(port);
	}
}

//------------------------------------------------------------------------------

const char *SockAddr_get_Address(SockAddr *sa)
{
	std::string addr;
	char host[NI_MAXHOST];
	char serv[NI_MAXSERV];
	
	if (getnameinfo(ADDR(sa), ADDR_SIZE(sa),
		host, sizeof (host), serv, sizeof (serv), 0))
	{
		// It failed: let's try without service names
		if (getnameinfo(ADDR(sa), ADDR_SIZE(sa),
			host, sizeof (host), NULL, 0, 0))
		{
			// Handle error:
			// we'll just return an empty string, that will be comforting enough
		}
		else
		{
			sprintf(serv, "%d", SockAddr_get_Port(sa));
			addr = (std::string(host) + ":") + serv;
		}
	}
	else
	{
		if (!serv[0] || (serv[0] == '0' && !serv[1]))
			addr = host;
		else if (atof(serv) == 0.0) // A bit wonky but does the trick
			addr = (std::string(serv) + "://") + host;
		else
			addr = (std::string(host) + ":") + serv;
	}
	
	return AGS_STRING(addr.c_str());
}

//------------------------------------------------------------------------------

void SockAddr_set_Address(SockAddr *sa, const char *addr)
{
	std::string node = addr, service;
	size_t index;
	
	if ((index = node.find("://")) != std::string::npos)
	{
		service = node.substr(0, index);
		node = node.substr(index + 3);
	}
	
	if ((sa->ss_family != AF_INET6)
	&& ((index = node.rfind(':')) != std::string::npos))
	{
		service = node.substr(index + 1);
		node.resize(index);
	}
	
	addrinfo hint, *result = nullptr;
	memset(&hint, 0, sizeof (addrinfo));
	hint.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED | (node.empty() ? AI_PASSIVE : 0);
	hint.ai_family = sa->ss_family ? sa->ss_family : AF_UNSPEC;
	
	if (getaddrinfo(node.c_str(), service.c_str(), &hint, &result) || !result)
	{
		// Handle error:
		// We'll simply do nothing when address resolving failed;
		// Users will figure out that the address object is empty,
		// so the address string supplied is most likely invalid.
		return;
	}

	memcpy(sa, result->ai_addr, result->ai_addrlen);
	freeaddrinfo(result);
}

//------------------------------------------------------------------------------

const char *SockAddr_get_IP(SockAddr *sa)
{
	if (sa->ss_family == AF_INET)
	{
		char buffer[INET_ADDRSTRLEN] = "";
		sockaddr_in *addr = reinterpret_cast<sockaddr_in *> (sa);
		inet_ntop(sa->ss_family, &addr->sin_addr, buffer, sizeof (buffer));
		return AGS_STRING(buffer);
	}
	else if (sa->ss_family == AF_INET6)
	{
		char buffer[INET6_ADDRSTRLEN] = "";
		sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *> (sa);
		inet_ntop(sa->ss_family, &addr->sin6_addr, buffer, sizeof (buffer));
		return AGS_STRING(buffer);
	}
	return nullptr;
}

//------------------------------------------------------------------------------

void SockAddr_set_IP(SockAddr *sa, const char *ip)
{
	if (sa->ss_family == AF_INET)
	{
		sockaddr_in *addr = reinterpret_cast<sockaddr_in *> (sa);
		inet_pton(sa->ss_family, ip, &addr->sin_addr);
	}
	else if (sa->ss_family == AF_INET6)
	{
		sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *> (sa);
		inet_pton(sa->ss_family, ip, &addr->sin6_addr);
	}
}

//------------------------------------------------------------------------------

SockData *SockAddr_GetData(SockAddr *sa)
{
	SockData *data = new SockData();
	AGS_OBJECT(SockData, data);
	data->data = std::string(reinterpret_cast<char *> (sa), ADDR_SIZE(sa));
	return data;
}

//------------------------------------------------------------------------------

} /* namespace AGSSock */

//..............................................................................
