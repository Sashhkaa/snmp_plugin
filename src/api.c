#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "agent.h"

struct ctl_context {
    int argc;
    char **argv;
};

static void
print_help(void)
{
    printf("Usage: snmp-agent command\n");
    printf("add-conn  -- add new snmp client");
    printf("del-conn  -- del snmp client");
}

static int
is_valid_ipv4(const char *ip)
{
    int num = 0, dots = 0;
    const char *ptr = ip;

    if (!ip) {
        return 0;
    }

    while (*ptr) {
        if (*ptr == '.') {
            if (++dots > 3 || num < 0 || num > 255) return 0;
            num = 0;
        } else if (isdigit(*ptr)) {
            num = num * 10 + (*ptr - '0');
        } else {
            return 0;
        }
        ptr++;
    }

    return dots == 3 && num >= 0 && num <= 255;
}

unsigned int
hash_string(const char *str)
{
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

int
generate_id(const char *str1, int der, int num)
{
    unsigned int hash1 = hash_string(str1);
    unsigned int hash2 = (unsigned int) der;
    unsigned int hash3 = (unsigned int) num;

    unsigned int combined = (hash1 ^ (hash2 << 1)) ^ (hash3 << 2);

    return (int)(combined & 0x7FFFFFFF);
}

/*
 * add-conn ip-addr tx/rx int-id timeout "community string"
 */
void
create_conneection(struct ctl_context *ctx)
{
    struct client_conn *client = init_client();

    const char *ip_addr = ctx->argv[2];

    if (!ip_addr || !is_valid_ipv4(ip_addr)) {
        fprintf(stderr, "Error: Invalid IPv4 address\n");
        return;
    }

    client->session.peername = strdup(ip_addr);

    client->sett.direction = !strcmp(ctx->argv[3], "tx") ?
                             TX_TRANSMIT : RX_RECIVE;

    client->sett.int_id = (int8_t)strtol(ctx->argv[4], NULL, 10);

    client->sett.timeout = (int8_t)strtol(ctx->argv[5], NULL, 10);

    char *community = ctx->argv[6];
    if (strlen(community) <= 8) {
        fprintf(stderr, "Community len should have at least 8 symbols. \n");
        return;
    }

    client->session.securityName = community;
    client->session.contextNameLen = strlen(community);

    client->conn_id = generate_id(client->session.peername,
                                  client->sett.direction,
                                  client->sett.int_id);

    attach_to_connections_list(client);

    init_connection(client);
}

/*
 * del-conn ip-addr tx/rx int-id
 */
static void
destroy_connetion(struct ctl_context *ctx)
{
    const char *ip_addr = ctx->argv[2];
    int derection, int_id, conn_id;

    if (ip_addr || !is_valid_ipv4(ip_addr)) {
        fprintf(stderr, "Error: Invalid IPv4 address\n");
        return;
    }

    derection = !strcmp(ctx->argv[3], "tx") ?
                            TX_TRANSMIT : RX_RECIVE;

    int_id = (int8_t)strtol(ctx->argv[4], NULL, 10);

    conn_id = generate_id(ip_addr, derection, int_id);

    detach_from_connections_list(conn_id);
}

int
main(int argc, char **argv)
{
    struct ctl_context ctx = {
        .argc = argc,
        .argv = argv
    };

    if (!strcmp("add-conn", ctx.argv[1])) {
        create_conneection(&ctx);
    } else if (!strcmp("del-conn", ctx.argv[2])) {
        destroy_connetion(&ctx);
    } else if (!strcmp("-h", ctx.argv[1])) {
        print_help();
    }
}