#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/library/transform_oids.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "agent.h"


/* TODO: сделать че то с этим кринжем. */
const char *passphrase = "SecurePass123";

/* uthash hashmap initialization. */
struct conn_hash *conns_hash = NULL;

/* hosts that we have not completed */
int active_hosts;

static void
send_pdu(struct client_conn *client)
{
    struct snmp_pdu *pdu;
    size_t anOID_len = MAX_OID_LEN; 
    char oid_str[MAX_OID_LEN];
    oid anOID[MAX_OID_LEN];

    pdu = snmp_pdu_create(SNMP_MSG_GET);

    snprintf(oid_str, MAX_OID_LEN, ".1.3.6.1.2.1.2.2.1.%d.%d",
             client->sett.direction == TX_TRANSMIT ? 16 : 10,
             client->sett.int_id);

    if (!read_objid(oid_str, anOID, &anOID_len)) {
        snmp_log(LOG_ERR, "Неверный OID\n");
        snmp_free_pdu(pdu);
        return;
    }

    snmp_add_null_var(pdu, anOID, anOID_len);

    if (snmp_send(client->ss, pdu) == 0) {
        snmp_log(LOG_ERR, "error while send pdu to client\n");
        snmp_free_pdu(pdu);
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
asynch_response(int operation, struct snmp_session *sp, int reqid,
                netsnmp_pdu *pdu, void *magic)
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
        snmp_log(LOG_ERR, "error while receiving message.\n");
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
attach_to_connections_list(struct client_conn *client)
{
    struct conn_hash *conn_hash;

    HASH_FIND_INT(conns_hash, &client->conn_id, conn_hash);

    if (conn_hash) {
        snmp_log(LOG_ERR, "Connection already exists.\n");
        return;
    }

    conn_hash = (struct conn_hash *)calloc(1, sizeof(struct conn_hash));
    conn_hash->conn_id = client->conn_id;
    conn_hash->client = client;

    HASH_ADD_INT(conns_hash, conn_id, conn_hash);
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
    struct conn_hash *conn_hash;

    HASH_FIND_INT(conns_hash, &conn_id, conn_hash);

    return conn_hash;
}

void
detach_from_connections_list(int conn_id)
{
    struct conn_hash *conn_hash;

    conn_hash = lookup_connection(conn_id);

    if (!conn_hash) {
        snmp_log(LOG_ERR, "Connetction doesn't exists. \n");
        return;
    }

    active_hosts--;
    
    HASH_DEL(conns_hash, conn_hash);
    
    free(conn_hash->client->ss);
    free(conn_hash->client);
    free(conn_hash);
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
        HASH_DEL(conns_hash, current);
        free(current->client->ss);
        free(current);
    }


}

void
main_loop()
{
    while (active_hosts) {
        int fds = 0, block = 1;
        fd_set fdset;
        struct timeval timeout;

        FD_ZERO(&fdset);
        snmp_select_info(&fds, &fdset, &timeout, &block);

        
        fds = select(fds, &fdset, NULL, NULL, block ? NULL : &timeout);

        if (fds) {
            snmp_read(&fdset);
        } else { 
            snmp_timeout();
        }
    }
}