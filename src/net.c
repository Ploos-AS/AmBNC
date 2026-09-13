#include <string.h>

#include <exec/libraries.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/bsdsocket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "net.h"

struct Library *SocketBase = 0;

int ambnc_net_open(void)
{
    if (SocketBase != 0) return 0;
    SocketBase = (struct Library *)OpenLibrary((CONST_STRPTR)"bsdsocket.library", 0);
    return SocketBase != 0 ? 0 : -1;
}

void ambnc_net_close(void)
{
    if (SocketBase != 0) {
        CloseLibrary(SocketBase);
        SocketBase = 0;
    }
}

int ambnc_net_connect_ipv4(const char *host, unsigned short port)
{
    const struct hostent *resolved;
    struct sockaddr_in address;
    int sock;

    if (host == 0 || host[0] == '\0') return -1;
    resolved = gethostbyname((STRPTR)host);
    if (resolved == 0 || resolved->h_addrtype != AF_INET ||
        resolved->h_length < (int)sizeof(address.sin_addr) ||
        resolved->h_addr_list == 0 || resolved->h_addr_list[0] == 0)
        return -1;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    memcpy(&address.sin_addr, resolved->h_addr_list[0], sizeof(address.sin_addr));

    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        CloseSocket(sock);
        return -1;
    }
    return sock;
}

int ambnc_net_send_all(int sock, const char *data, unsigned int length)
{
    unsigned int sent = 0;
    while (sent < length) {
        int rc = send(sock, (STRPTR)(data + sent), length - sent, 0);
        if (rc <= 0) return -1;
        sent += (unsigned int)rc;
    }
    return 0;
}

int ambnc_net_recv(int sock, char *buffer, unsigned int length)
{
    return recv(sock, buffer, length, 0);
}

void ambnc_net_close_socket(int sock)
{
    if (sock >= 0) CloseSocket(sock);
}
