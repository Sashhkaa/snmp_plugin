#include <stdlib.h>
#include <string.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

struct curve {
    /* to do: dynamic. */
    unsigned int curve[10];
    unsigned int rate;
    unsigned int critical_lower;
    unsigned int critical_upper;
    
    int util;
};

struct ctl_context {
    int argc;
    char **argv;
};

struct client {
    /* to do: dynamic. */
    struct snmap_session *session;
    struct curve *curve;
};

const char *passphrase = "SecurePass123";

void
__init_connetcion(struct ctl_context *ctx);