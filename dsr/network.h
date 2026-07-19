#ifndef DSR_NETWORK_H
#define DSR_NETWORK_H

#include "core.h"

/* GameSpy-compatible network layer for BF1942 multiplayer
 * Based on:
 *   keowu/gamespy — GameSpy SDK RE
 *   bftech.darkstormstudios.co.uk — GameSpy Protocol
 *   aluigi.org — BF2 protocol analysis
 */

#define NET_MAX_PACKET 1472  /* UDP safe payload */
#define NET_MAX_PLAYERS 64
#define NET_MAX_NAME 32

/* GameSpy protocol identifiers */
#define GAMESPY_HEADER  0xFDFE  /* always LE: 0xFE 0xFD */
#define GAMESPY_PROTO_SERVERINFO  0x00
#define GAMESPY_PROTO_PLAYERS     0x01
#define GAMESPY_PROTO_TEAMS       0x02
#define GAMESPY_PROTO_ECHO        0x02  /* proto 02 */
#define GAMESPY_PROTO_HEARTBEAT   0x03
#define GAMESPY_PROTO_KEEPALIVE   0x08

/* Refractor game protocol packet types (from aluigi) */
#define PKT_CLIENT_CONNECT   0x0F  /* 4 bits = 0x0F */
#define PKT_CLIENT_DATA      0x01
#define PKT_SERVER_DATA      0x02
#define PKT_SERVER_SNAPSHOT  0x03

typedef enum {
    NET_DISCONNECTED,
    NET_CONNECTING,
    NET_CONNECTED,
    NET_IN_GAME,
} net_state;

typedef struct {
    u32  ip;
    u16  port;
    char hostname[256];
    char map_name[128];
    char game_name[64];
    u8   player_count;
    u8   max_players;
    u8   team_count;
    u32  ping_ms;
} net_server_info;

/* Bit stream reader/writer (Refractor protocol) */
typedef struct {
    u8   data[NET_MAX_PACKET];
    u32  size;
    u32  bit_pos;
    bool writing;
} bitstream;

void bitstream_init_read(bitstream *bs, const u8 *data, u32 size);
void bitstream_init_write(bitstream *bs);
void bitstream_write_bits(bitstream *bs, u32 value, u32 bits);
u32  bitstream_read_bits(bitstream *bs, u32 bits);

/* Network peer */
typedef struct {
    int           socket_fd;
    net_state     state;
    u32           server_ip;
    u16           server_port;
    u32           client_index;
    /* Game state */
    u32           tick_id;
    u32           seq_local;
    u32           seq_remote;
    u8            buffer[NET_MAX_PACKET];
    /* Callbacks */
    void (*on_connected)(void *user);
    void (*on_disconnected)(void *user);
    void (*on_server_info)(net_server_info *info, void *user);
    void *user_data;
} dsr_network;

/* Create network peer */
dsr_network *network_create(void);

/* Destroy network peer */
void network_destroy(dsr_network *net);

/* Connect to server */
bool network_connect(dsr_network *net, const char *host, u16 port);

/* Disconnect */
void network_disconnect(dsr_network *net);

/* Send game state snapshot (server) */
bool network_send_snapshot(dsr_network *net, const u8 *data, u32 size);

/* Send input (client) */
bool network_send_input(dsr_network *net, const u8 *data, u32 size);

/* Poll network (call each frame) */
void network_poll(dsr_network *net);

/* Host a server */
bool network_host(dsr_network *net, u16 port, const char *server_name, const char *map_name);

/* Master server heartbeat */
bool network_send_heartbeat(dsr_network *net, u32 master_ip, u16 master_port);

/* Query server for info */
bool network_query_server(dsr_network *net, const char *host, u16 port);

#endif /* DSR_NETWORK_H */
