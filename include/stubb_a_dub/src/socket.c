#include "sad_gdb_internal.h"
#include "stubb_a_dub.h"
#include <fcntl.h>
#include <unistd.h>

extern SAD_Stub server_g;

stub_ret sad_socket_init(uint16_t port) {
    int opt = 1;
    stub_ret ret = STUB_OK;
    struct sockaddr_in address;

    if ((server_g.server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        goto socket_err;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (setsockopt(server_g.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        goto close_socket;

    if (bind(server_g.server_fd, (struct sockaddr *) &address, sizeof(address)) == -1)
        goto close_socket;

    // set backlog to 1 (hint on number of outstanding connections
    // in the socket's listen queue)
    if (listen(server_g.server_fd, 1) == -1)
        goto close_socket;

    server_g.sad_socket = accept(server_g.server_fd, (struct sockaddr *) &server_g.address, &server_g.addrlen);
    if (server_g.sad_socket == -1)
        goto close_socket;

    return ret;

close_socket:
    close(server_g.server_fd);
socket_err:
    ret = STUB_SOCKET;
    return ret;
}

int sad_socket_blocking(bool blocking) {
    int flags = fcntl(server_g.sad_socket, F_GETFL, 0);

    if (flags == -1)
        return -1;

    if (blocking)
        flags &= ~O_NONBLOCK; // clear nonblocking flag
    else
        flags |= O_NONBLOCK; // set nonblocking flag

    return fcntl(server_g.sad_socket, F_SETFL, flags);
}
