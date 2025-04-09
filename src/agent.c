#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/library/transform_oids.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "agent.h"
#include <errno.h>
#include <sys/file.h>

/* TODO: сделать че то с этим кринжем. */
const char *passphrase = "SecurePass123";

/* uthash hashmap initialization. */
struct conn_hash *conns_hash = NULL;

/* hosts that we have not completed */
volatile sig_atomic_t active_hosts = 1;

FILE *db = NULL;

#define PEERNAME_LEN 256

struct conn_entry_serialized {
    int conn_id;
    char peername[PEERNAME_LEN];
};

static void
send_pdu(struct client_conn *client)
{
    struct snmp_pdu *pdu;
    size_t anOID_len = MAX_OID_LEN; 
    char oid_str[MAX_OID_LEN];
    oid anOID[MAX_OID_LEN];

    pdu = snmp_pdu_create(SNMP_MSG_GET);
    if (!pdu) {
        snmp_log(LOG_ERR, "Failed to create PDU\n");
        return;
    }

    snprintf(oid_str, MAX_OID_LEN, ".1.3.6.1.2.1.2.2.1.%d.%d",
             client->sett.direction == TX_TRANSMIT ? 16 : 10,
             client->sett.int_id);

    if (!read_objid(oid_str, anOID, &anOID_len)) {
        snmp_log(LOG_ERR, "Invalid OID format: %s\n", oid_str);
        snmp_free_pdu(pdu);
        return;
    }

    snmp_add_null_var(pdu, anOID, anOID_len);

    if (!client->ss) {
        snmp_log(LOG_ERR, "SNMP session is not open.\n");
        snmp_free_pdu(pdu);
        return;
    }

    int send_result = snmp_send(client->ss, pdu);
    if (send_result == 0) {
        snmp_log(LOG_ERR, "Error while sending PDU to client. SNMP error: %s\n", snmp_errstring(snmp_errno));
        snmp_free_pdu(pdu);
        return;
    } else {
        snmp_log(LOG_DEBUG, "PDU successfully sent.\n");
    }
}


double
extract_oid_value(netsnmp_pdu *pdu)
{
    netsnmp_variable_list *vars = pdu->variables;

    while (vars) {
        switch(vars->type) {
            case ASN_INTEGER:
            case ASN_COUNTER:
            case ASN_GAUGE:
            case ASN_TIMETICKS:
                return (double)*vars->val.integer;
            case ASN_COUNTER64:
                uint64_t counter_value = ((uint64_t)vars->val.counter64->high << 32) | vars->val.counter64->low;
                return (double)counter_value;
            case ASN_OCTET_STR:
                return atof((const char*)vars->val.string);
            default:
                    fprintf(stderr, "Unsupported ASN type: %d\n", vars->type);
                    return 0;
            }
        vars = vars->next_variable;
    }
    fprintf(stderr, "OID not found in response\n");
    return 0;
}

/* Callback function. */
int
asynch_response(int operation, struct snmp_session *sp,
                int reqid, netsnmp_pdu *pdu, void *magic)
{
    struct client_conn *client = (struct client_conn *)magic;
    double data;
    (void)sp;
    (void)reqid;

    if (operation == RECEIVED_MESSAGE) {
        data = extract_oid_value(pdu);
        
        add_data(&client->curve, data);
        process_data(&client->curve);
    } else {
        snmp_log(LOG_ERR, "Ошибка при получении сообщения.\n");
    }

    send_pdu(client);
    return SNMP_ERR_NOERROR;
}

static void
gen_serts(struct snmp_session *session)
{
    if (generate_Ku(session->securityAuthProto,
                    session->securityAuthProtoLen,
                    (u_char *) passphrase,
                    strlen(passphrase),
                    session->securityAuthKey,
                    &session->securityAuthKeyLen) != SNMPERR_SUCCESS) {
        snmp_log(LOG_ERR,
                 "Error generating Ku from authentication pass phrase. \n");
        snmp_shutdown("snmpapp");
        exit(1);
        return;
    }
}

static void
setup_security_param(struct snmp_session *session)
{

    session->version = SNMP_VERSION_3; 

    session->securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;

    session->securityAuthProto = usmHMACMD5AuthProtocol;
    session->securityAuthProtoLen = sizeof(usmHMACMD5AuthProtocol)/sizeof(oid);
    session->securityAuthKeyLen = USM_AUTH_KU_LEN;

    gen_serts(session);
}

void
dump_connections(FILE *file)
{
    struct conn_hash *current, *tmp;
    struct conn_entry_serialized entry;

    flock(fileno(file), LOCK_EX);

    HASH_ITER(hh, conns_hash, current, tmp) {
        memset(&entry, 0, sizeof(entry));
        entry.conn_id = current->conn_id;
        strncpy(entry.peername, current->client->session.peername, PEERNAME_LEN - 1);

        fwrite(&entry, sizeof(entry), 1, file);
    }

    fflush(file);
    flock(fileno(file), LOCK_UN);
}

/*  to do: add shmem instead. */
void
get_connections(FILE *file)
{
    struct conn_entry_serialized entry;

    flock(fileno(file), LOCK_SH);

    while (fread(&entry, sizeof(entry), 1, file) == 1) {
        struct client_conn *client = init_client();

        client->conn_id = entry.conn_id;
        client->session.peername = strdup(entry.peername);
        attach_to_connections_list(client);
    }

    flock(fileno(file), LOCK_UN);
}

static void
print_connections()
{
    struct conn_hash *current, *tmp;

    HASH_ITER (hh, conns_hash, current, tmp) {
        fprintf(stderr, "client = %s\n", current->client->session.peername);
    }
}

void
attach_to_connections_list(struct client_conn *client)
{
    struct conn_hash *conn_hash;

    HASH_FIND_INT(conns_hash, &client->conn_id, conn_hash);

    if (conn_hash) {
        snmp_log(LOG_ERR, "Connection already exists.\n");
        return;
    }

    conn_hash = calloc(1, sizeof(struct conn_hash));
    conn_hash->conn_id = client->conn_id;
    conn_hash->client = client;

    HASH_ADD_INT(conns_hash, conn_id, conn_hash);

    FILE *db = fopen("/tmp/db2.txt", "wb");
    if (!db) {
        fprintf(stderr, "errno = %d (%s)\n", errno, strerror(errno));
        return;
    }

    dump_connections(db);
    fclose(db);
}

void
sync_connections_from_file()
{
    FILE *db = fopen("/tmp/db2.txt", "rb");
    if (!db) {
        fprintf(stderr, "errno = %d (%s)\n", errno, strerror(errno));
        return;
    }

    get_connections(db);
    fclose(db);

    print_connections();
}

void
init_connection(struct client_conn *client)
{
    if (!client) return;

    snmp_sess_init(&client->session);
    setup_security_param(&client->session);

    client->ss = snmp_open(&client->session);
    if (!client->ss) {
        snmp_log(LOG_ERR, "Error while establishing snmp session.\n");
        exit(1);
    }

    client->session.callback = asynch_response;
    client->session.callback_magic = client;

    active_hosts++;

    init_algorithm(&client->curve, 0.0008, 1.2);

    send_pdu(client);

}

static struct conn_hash *
lookup_connection(int conn_id)
{
    struct conn_hash *current, *tmp;

    HASH_ITER(hh, conns_hash, current, tmp) {
        if (current->conn_id == conn_id) {
            return current;
        }
    }

    return NULL;
}

void
detach_from_connections_list(int conn_id)
{
    struct conn_hash *conn_hash = lookup_connection(conn_id);

    if (!conn_hash) {
        snmp_log(LOG_ERR, "Connection doesn't exist (conn_id = %d).\n", conn_id);
        return;
    }

    if (conn_hash->client) {
        if (conn_hash->client->session.peername) {
            free(conn_hash->client->session.peername);
            conn_hash->client->session.peername = NULL;
        }

        if (conn_hash->client->ss) {
            snmp_close(conn_hash->client->ss);
            conn_hash->client->ss = NULL;
        }

        free(conn_hash->client);
        conn_hash->client = NULL;
    }

    HASH_DEL(conns_hash, conn_hash);
    free(conn_hash);

    active_hosts--;
}

struct client_conn *
init_client(void)
{
    struct client_conn *client = (struct client_conn *)
                    calloc(1, sizeof(struct client_conn));
    
    client->ss = (struct snmp_session *)
                    calloc(1, sizeof(struct snmp_session));

    return client;
}

void
destroy_connections()
{
    struct conn_hash *current, *tmp;

    HASH_ITER(hh, conns_hash, current, tmp) {
        fprintf(stderr, "destroy %s", current->client->session.peername); 
        HASH_DEL(conns_hash, current);
        free(current->client->ss);
        free(current);
    }
}

void
main_loop(int keep_running)
{
    while (active_hosts && keep_running) {
        sync_connections_from_file();
        sleep(5);
    }
}