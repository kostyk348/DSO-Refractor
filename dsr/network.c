#include "network.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <netdb.h>
#  define INVALID_SOCKET -1
#  define SOCKET_ERROR -1
#  define closesocket close
#endif

/* --- Bitstream --- */

void bitstream_init_read(bitstream *bs, const u8 *data, u32 size) {
    memcpy(bs->data, data, size < NET_MAX_PACKET ? size : NET_MAX_PACKET);
    bs->size = size;
    bs->bit_pos = 0;
    bs->writing = false;
}

void bitstream_init_write(bitstream *bs) {
    memset(bs->data, 0, NET_MAX_PACKET);
    bs->size = 0;
    bs->bit_pos = 0;
    bs->writing = true;
}

void bitstream_write_bits(bitstream *bs, u32 value, u32 bits) {
    for (u32 i = 0; i < bits; i++) {
        u32 byte_idx = bs->bit_pos >> 3;
        u32 bit_idx  = bs->bit_pos & 7;
        if (byte_idx >= NET_MAX_PACKET) return;
        if (value & (1u << (bits - 1 - i))) {
            bs->data[byte_idx] |= (u8)(1u << (7 - bit_idx));
        } else {
            bs->data[byte_idx] &= (u8)~(1u << (7 - bit_idx));
        }
        bs->bit_pos++;
    }
    bs->size = (bs->bit_pos + 7) >> 3;
}

u32 bitstream_read_bits(bitstream *bs, u32 bits) {
    u32 value = 0;
    for (u32 i = 0; i < bits; i++) {
        if (bs->bit_pos >= bs->size * 8) return value;
        u32 byte_idx = bs->bit_pos >> 3;
        u32 bit_idx  = bs->bit_pos & 7;
        value <<= 1;
        if (bs->data[byte_idx] & (1u << (7 - bit_idx))) {
            value |= 1;
        }
        bs->bit_pos++;
    }
    return value;
}

/* --- Socket helpers --- */

static int udp_socket_create(void) {
    int fd = (int)socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == INVALID_SOCKET) return INVALID_SOCKET;
#ifdef _WIN32
    unsigned long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
#endif
    return fd;
}

static int resolve_host(const char *host, u32 *out_ip) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(host, NULL, &hints, &res) != 0) return -1;
    if (res) {
        struct sockaddr_in *addr = (struct sockaddr_in*)res->ai_addr;
        *out_ip = addr->sin_addr.s_addr;
        freeaddrinfo(res);
        return 0;
    }
    return -1;
}

/* --- Network API --- */

dsr_network *network_create(void) {
    dsr_network *net = (dsr_network*)calloc(1, sizeof(dsr_network));
    if (!net) return NULL;

    net->socket_fd = udp_socket_create();
    if (net->socket_fd == INVALID_SOCKET) {
        free(net);
        return NULL;
    }
    net->state = NET_DISCONNECTED;
    return net;
}

void network_destroy(dsr_network *net) {
    if (!net) return;
    if (net->socket_fd != INVALID_SOCKET)
        closesocket(net->socket_fd);
    free(net);
}

bool network_connect(dsr_network *net, const char *host, u16 port) {
    if (!net || net->socket_fd == INVALID_SOCKET) return false;

    if (resolve_host(host, &net->server_ip) != 0) return false;
    net->server_port = port;

    /* Send connect packet (Refractor protocol) */
    bitstream bs;
    bitstream_init_write(&bs);
    bitstream_write_bits(&bs, PKT_CLIENT_CONNECT, 4);  /* 0x0F = connect */
    bitstream_write_bits(&bs, 0, 8);                    /* player index = 0 */
    bitstream_write_bits(&bs, 1, 6);                    /* sequence */
    bitstream_write_bits(&bs, 1, 6);                    /* ack */

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = net->server_ip;
    addr.sin_port = htons(port);

    sendto(net->socket_fd, (const char*)bs.data, bs.size, 0,
           (struct sockaddr*)&addr, sizeof(addr));

    net->state = NET_CONNECTING;
    return true;
}

void network_disconnect(dsr_network *net) {
    if (!net) return;
    net->state = NET_DISCONNECTED;
    if (net->on_disconnected)
        net->on_disconnected(net->user_data);
}

bool network_send_snapshot(dsr_network *net, const u8 *data, u32 size) {
    if (net->state != NET_IN_GAME) return false;
    /* TODO: Refractor snapshot protocol */
    (void)data;
    (void)size;
    return false;
}

bool network_send_input(dsr_network *net, const u8 *data, u32 size) {
    if (net->state != NET_IN_GAME) return false;
    (void)data;
    (void)size;
    return false;
}

void network_poll(dsr_network *net) {
    if (!net || net->socket_fd == INVALID_SOCKET) return;

    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    i32 received = (i32)recvfrom(net->socket_fd, (char*)net->buffer,
                                  NET_MAX_PACKET, 0,
                                  (struct sockaddr*)&from, &from_len);
    if (received <= 0) return;

    /* Process packet */
    bitstream bs;
    bitstream_init_read(&bs, net->buffer, (u32)received);

    u32 pkt_type = bitstream_read_bits(&bs, 4);
    u32 player_idx = bitstream_read_bits(&bs, 8);
    u32 seq = bitstream_read_bits(&bs, 6);
    (void)player_idx;
    (void)seq;

    if (pkt_type == PKT_SERVER_SNAPSHOT) {
        /* Full server snapshot */
        net->state = NET_IN_GAME;
        if (net->on_connected)
            net->on_connected(net->user_data);
    }
}

bool network_host(dsr_network *net, u16 port, const char *server_name, const char *map_name) {
    if (!net || net->socket_fd == INVALID_SOCKET) return false;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(net->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        return false;
    }

    net->state = NET_CONNECTED;
    /* TODO: store server info */
    (void)server_name;
    (void)map_name;
    return true;
}

bool network_send_heartbeat(dsr_network *net, u32 master_ip, u16 master_port) {
    if (!net) return false;

    bitstream bs;
    bitstream_init_write(&bs);
    /* GameSpy heartbeat packet */
    bs.data[0] = 0xFE;
    bs.data[1] = 0xFD;
    bs.data[2] = GAMESPY_PROTO_HEARTBEAT;
    bs.data[3] = 0x00; /* instance? */
    bs.size = 4;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = master_ip;
    addr.sin_port = htons(master_port);

    sendto(net->socket_fd, (const char*)bs.data, bs.size, 0,
           (struct sockaddr*)&addr, sizeof(addr));
    return true;
}

bool network_query_server(dsr_network *net, const char *host, u16 port) {
    if (!net) return false;

    u32 ip;
    if (resolve_host(host, &ip) != 0) return false;

    /* GameSpy server info query */
    u8 query[] = {
        0xFE, 0xFD,       /* header */
        0x00,             /* protocol 00: server info */
        0x00, 0x00, 0x00, 0x00, /* timestamp */
        0x01,             /* sub-protocol: basic */
        0x00, 0x00, 0x00, 0x00  /* challenge? */
    };

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = htons(port);

    sendto(net->socket_fd, (const char*)query, sizeof(query), 0,
           (struct sockaddr*)&addr, sizeof(addr));
    return true;
}
