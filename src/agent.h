#include <string.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include "../uthash/src/uthash.h"
#include "algo.h"

enum direction {
    TX_TRANSMIT,
    RX_RECIVE
};
typedef enum {
    RECEIVED_MESSAGE = 1,
    SEND_MESSAGE = 2,
} operation_types_t;

struct conn_settings {
    enum direction direction;

    u_int8_t timeout;
    u_int8_t int_id;
};
struct client_conn {
    struct snmp_session session;
    struct snmp_session *ss;

    struct curve curve;

    struct conn_settings sett;

    int conn_id;
};

struct conn_hash {
    struct client_conn *client;

    int conn_id;

    UT_hash_handle hh; /* handler for hash table. */
};

struct client_conn *init_client();
void destroy_clinet(struct client_conn *client);
void init_connection(struct client_conn *client);
void destroy_connection();
void destroy_connections();
void attach_to_connections_list(struct client_conn *client);
void detach_from_connections_list(int conn_id);
void main_loop(void);