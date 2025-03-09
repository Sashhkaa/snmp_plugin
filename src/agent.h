#include <string.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include "algo.h"
#include "../uthash/src/uthash.h"

#define CLEAR_SESSION_DATA(session)\
    do {                           \
        free(session);             \
        (session) = NULL;          \
    } while (0)

#define CHECK_SESSION(session, func_name)  \
    do {                                   \
        if (!(session)) {                  \
            CLEAR_SESSION_DATA(session);   \
            snmp_log(LOG_ERR,              \
                    "Error while init "    \
                    "connection in %s !\n",\
                    (func_name));          \
            return;                        \
        }                                  \
    } while (0)

enum direction {
    TX_TRANSMIT,
    RX_RECIVE
};
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

/* functions related to client initialization. */
struct client_conn *init_client();
void destroy_clinet(struct client_conn *client);

/* functions related to client snmp connections. */
void init_connetcion(struct client_conn *client);
void destroy_connection();

/* functions related to global vars. */
void init_connections();
void destroy_connections();

static void
attach_to_connections_list(struct client_conn *client);

static void
detach_from_connections_list(int conn_id);
