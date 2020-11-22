# AGSsock

AGSsock plugin brings Network Sockets for Adventure Game Studio.

## AGSsock API

The API is how you will interact with AGSsock plugin through AGS Script.

- [SockData](#sockdata) 
- [SockAddr](#sockaddr) 
- [Socket](#socket) 

### `SockData`

#### `SockData.Create`

`static SockData* SockData.Create(int size, char defchar = 0)`

Creates a new data container with specified size (and what character to fill it with)


#### `SockData.CreateEmpty`

`static SockData* SockData.CreateEmpty()`

Creates a new data container of zero size


#### `SockData.CreateFromString`

`static SockData* SockData.CreateFromString(const string str)`

Creates a data container from a string.


#### `SockData.Size`

`attribute int Size`


#### `SockData.Chars`

`attribute char Chars[]`


#### `SockData.AsString`

`String SockData.AsString()`

Makes and returns a string from the data object. 

Warning: anything after a null character will be truncated.


#### `SockData.Clear`

`void SockData.Clear()`

Removes all the data from a socket data object, reducing its size to zero.


### `SockAddr`

#### `SockAddr.Create`

`static SockAddr* SockAddr.Create(int type = IPv4)`

Creates an empty socket address. (advanced: set type to IPv6 if you're using IPv6)


#### `SockAddr.CreateFromString`

`static SockAddr* SockAddr.CreateFromString(const string address, int type = IPv4)`

Creates a socket address from a string. (for example: \"http://www.adventuregamestudio.co.uk\")


#### `SockAddr.CreateFromData`

`static SockAddr* SockAddr.CreateFromData(SockData *)`

Creates a socket address from raw data. (advanced)


#### `SockAddr.CreateIP`

`static SockAddr* SockAddr.CreateIP(const string address, int port)`

Creates a socket address from an IP-address. (for example: "`127.0.0.1`")


#### `SockAddr.CreateIPv6`

`static SockAddr* SockAddr.CreateIPv6(const string address, int port)`

Creates a socket address from an IPv6-address. (for example: "`::1`")


#### `SockAddr.Port`

`attribute int SockAddr.Port` 


#### `SockAddr.Address`

`attribute String SockAddr.Address`


#### `SockAddr.IP`

`attribute String SockAddr.IP`


#### `SockAddr.GetData`

`SockData* SockAddr.GetData()`

Returns a SockData object that contains the raw data of the socket address. (advanced)


### `Socket`

#### `Socket.Create`

`static Socket* Socket.Create(int domain, int type, int protocol = 0)`

Creates a socket for the specified protocol. (advanced)


#### `Socket.CreateUDP`

`static Socket* Socket.CreateUDP()`

Creates a UDP socket. (unreliable, connectionless, message based)


#### `Socket.CreateTCP`

`static Socket* Socket.CreateTCP()`

Creates a TCP socket. (reliable, connection based, streaming)


#### `Socket.CreateUDPv6`

`static Socket* Socket.CreateUDPv6()`

Creates a UDP socket for IPv6. (when in doubt use CreateUDP)


#### `Socket.CreateTCPv6`

`static Socket* Socket.CreateTCPv6()`

Creates a TCP socket for IPv6. (when in doubt use CreateTCP)


#### `Socket.LastError`

`static int Socket.LastError`


#### `Socket.Tag`

`attribute String Tag`


#### `Socket.Local`

`readonly attribute SockAddr *Local`


#### `Socket.Remote`

`readonly attribute SockAddr *Remote`

#### `Socket.Valid`

`readonly attribute bool Valid`


#### `Socket.ErrorValue`

`SockError Socket.ErrorValue()`

Returns the last error observed from this socket as an enumerated value.


#### `Socket.ErrorString`

`String Socket.ErrorString()`

Returns the last error observed from this socket as an human readable string.


#### `Socket.Bind`

`bool Socket.Bind(SockAddr *local)`

Binds the socket to a local address. (generally used  before listening)


#### `Socket.Listen`

`bool Listen(int backlog = 10)`

Makes a socket listen for incoming connection requests. (TCP only) Backlog specifies how many requests can be queued. (optional)


#### `Socket.Connect`

`bool Socket.Connect(SockAddr *host, bool async = false)`

Makes a socket connect to a remote host. (for UDP it will simply bind to a remote address) Defaults to sync which makes it wait; see the manual for async use.


#### `Socket.Accept`

`Socket* Socket.Accept()`

Accepts a connection request and returns the resulting socket when successful. (TCP only)


#### `Socket.Close`

`void Socket.Close()`

Closes the socket. (you can still receive until socket is marked invalid)


#### `Socket.Send`

`bool Socket.Send(const string msg)`

Sends a string to the remote host. Returns whether successful. (no error means: try again later)


#### `Socket.SendTo`

`bool Socket.SendTo(SockAddr *target, const string msg)`

Sends a string to the specified remote host. (UDP only)


#### `Socket.Recv`

`String Socket.Recv()`

Receives a string from the remote host. (no error means: try again later)


#### `Socket.RecvFrom`

`String Socket.RecvFrom(SockAddr *source)`

Receives a string from an unspecified host. The given address object will contain the remote address. (UDP only)


#### `Socket.SendData`

`bool Socket.SendData(SockData *data)`

Sends raw data to the remote host. Returns whether successful. (no error means: try again later)


#### `Socket.SendDataTo`

`bool Socket.SendDataTo(SockAddr *target, SockData *data)`

Sends raw data to the specified remote host. (UDP only)


#### `Socket.RecvData`

`SockData *Socket.RecvData()`

Receives raw data from the remote host. (no error means: try again later)


#### `Socket.RecvDataFrom`

`SockData* Socket.RecvDataFrom(SockAddr *source)`

Receives raw data from an unspecified host. The given address object will contain the remote address. (UDP only)


---

## License and Author

This plugin was created by Ferry "Wyz" Timmers, and it's license is provided in [`LICENSE.txt`](LICENSE.txt).
