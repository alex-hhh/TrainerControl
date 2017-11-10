/**
 *  NetTools -- convenient wrappers for socket functions
 *  Copyright (C) 2017 Alex Harsanyi (AlexHarsanyi@gmail.com)
 * 
 * This program is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "stdafx.h"
#include <ws2tcpip.h>
#include "NetTools.h"
#include "Tools.h"
#include <sstream>
#include <algorithm>

#pragma comment (lib, "ws2_32.lib")

SOCKET tcp_listen(int port)
{
    WSADATA wsaData;
    memset(&wsaData, 0, sizeof(wsaData));
    HRESULT r = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (r != 0 )
        throw Win32Error("WSAStartup()", r);

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (s == INVALID_SOCKET)
        throw Win32Error("socket()", WSAGetLastError());

    BOOL reuse_addr = TRUE;
    r = setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                   (char*)&reuse_addr, sizeof(reuse_addr));
    if (r != 0)
        throw Win32Error("setsockopt(SO_REUSEADDR)", WSAGetLastError());

    struct addrinfo *result, hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    std::ostringstream port_msg;
    port_msg << port;
    r = getaddrinfo(NULL, port_msg.str().c_str(), &hints, &result);
    if (r != 0)
        throw Win32Error("getaddrinfo()", WSAGetLastError());

    r = bind(s, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);

    if (r == SOCKET_ERROR)
        throw Win32Error("bind()", WSAGetLastError());

    r = listen(s, /* backlog = */ 5);
    if (r == SOCKET_ERROR)
        throw Win32Error("listen()", WSAGetLastError());

    return s;
}

SOCKET tcp_accept(SOCKET server)
{
    SOCKET client = accept (server, NULL, NULL);

    if (client == INVALID_SOCKET)
        throw Win32Error ("accept()", WSAGetLastError());

    // Disable send delay.
    unsigned long flag = 1;
    int r = setsockopt(client, IPPROTO_TCP, TCP_NODELAY,
                       reinterpret_cast<const char*>(&flag), sizeof(flag));
    if (r == SOCKET_ERROR)
        throw Win32Error("setsockopt()", WSAGetLastError());

    return client;
}

std::string get_peer_name (SOCKET s)
{
    struct sockaddr_in addr;
    int len = sizeof(struct sockaddr_in);
    int r = getpeername(s, (struct sockaddr*)&addr, &len);
    if (r == SOCKET_ERROR)
    {
        throw Win32Error ("getpeername()", WSAGetLastError());
    }

    char hostname[NI_MAXHOST];
    char servname[NI_MAXSERV];
    r = getnameinfo ((struct sockaddr*)&addr, sizeof(struct sockaddr_in),
                     hostname, NI_MAXHOST,
                     servname, NI_MAXSERV,
                     NI_NUMERICSERV);
    if (r == 0) // success
    {
        std::ostringstream n;
        n << hostname << " at " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
        return n.str();
    }
    else
    {
        std::ostringstream n;
        n << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
        return n.str();
    }
}

// timeout in milliseconds
std::vector<uint8_t> get_socket_status(const std::vector<SOCKET> &sockets, uint64_t timeout)
{
    if (sockets.size() >= FD_SETSIZE)
        throw std::exception("get_socket_status: too many sockets");

    std::vector<uint8_t> result(sockets.size()); // initialized to 0

    fd_set read_fds, write_fds, except_fds;
    FD_ZERO (&read_fds);
    FD_ZERO (&write_fds);
    FD_ZERO (&except_fds);

    std::for_each (begin(sockets), end(sockets),
                   [&](SOCKET s) {
                       FD_SET (s, &read_fds);
                       FD_SET (s, &write_fds);
                       FD_SET (s, &except_fds);
                   });

    auto seconds = timeout / 1000;
    auto useconds = (timeout - (seconds * 1000)) * 1000;

    struct timeval to;
    to.tv_sec = static_cast<unsigned long>(seconds);
    to.tv_usec = static_cast<unsigned long>(useconds);

    int r = select (sockets.size(), &read_fds, &write_fds, &except_fds, &to);

    if (r == SOCKET_ERROR) {
        throw Win32Error ("select()", WSAGetLastError());
    }

    // r == 0 means that no sockets have messages, saves the trouble of
    // checking them individually
    if (r == 0)
        return result;

    for (unsigned i = 0; i < sockets.size(); i++) {
        uint8_t val = 0;
        if (FD_ISSET (sockets[i], &read_fds)) {
            val |= SK_READ;
        }
        if (FD_ISSET (sockets[i], &write_fds)) {
            val |= SK_WRITE;
        }
        if (FD_ISSET (sockets[i], &except_fds)) {
            val |= SK_EXCEPT;
        }
        result[i] = val;
    }

    return std::move (result);
}
