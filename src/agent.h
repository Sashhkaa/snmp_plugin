#include <string.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include "algo.h"

/* TODO: сделать че то с этим кринжем. */
const char *passphrase = "SecurePass123";

extern struct conn_hash *conns_hash;
extern int n_conns;
extern int n_capacity;

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
void init_connetcion();
void destroy_connection();

/* functions related to global vars. */
void init_connections();
void destroy_connection();