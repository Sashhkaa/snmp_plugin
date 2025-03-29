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

/* callback function for response messages. */
static void
asynch_response(int operation, struct snmp_session *sp, int reqid,
                struct snmp_pdu *pdu, void *data)
{
    struct client_conn *conn = (struct session *)data;
    struct snmp_pdu *response;

    if (operation == RECEIVED_MESSAGE) {
        get_info(response, conn->curve);
    } else {
        /* TODO: check retries. */
        snmp_log(LOG_ERR, "Error while receiving info. \n");
        detach_from_connections_list(conn->conn_id);
    }    

    /* initilize new run. */
    /* TODO: timer. */
    send_pdu(conn);
}

static void
send_pdu(struct client_conn *client)
{
    struct snmp_pdu *pdu, *response;
    size_t anOID_len = MAX_OID_LEN; 
    char oid_str[MAX_OID_LEN];
    oid anOID[MAX_OID_LEN];

    pdu = snmp_pdu_create(SNMP_MSG_GET);

    snprintf(oid_str, MAX_OID_LEN, ".1.3.6.1.2.1.2.2.1.%d.%d",
                      client->sett.direction == TX_TRANSMIT ? "16" : "10",
                      client->sett.int_id);

    if (!read_objid(oid_str, anOID, &anOID_len)) {
        snmp_log(LOG_ERR, "Неверный OID\n");
        snmp_free_pdu(pdu);
        return;
    }

    snmp_add_null_var(pdu, anOID, anOID_len);

    int status = snmp_synch_response(client->ss, pdu, &response);
    
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
        //get_info(response, curve);
    } else {
        snmp_log(LOG_ERR, "Ошибка при выполнении запроса. Код ошибки: %d\n", status);
        if (response) {
            snmp_log(LOG_ERR, "Код ошибки SNMP: %ld\n", response->errstat);
        }
    }

    if (response) {
        snmp_free_pdu(response);
    }
}

static void
disable_connection(struct snmp_session *session)
{
    snmp_close(session);
}

static void
gen_serts(struct snmp_session *session)
{
    CHECK_SESSION(session, __func__);

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
    CHECK_SESSION(session, __func__);

    /* TODO: add v1 and v2.*/
    session->version = SNMP_VERSION_3; 

    session->securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;

    session->securityAuthProto = usmHMACMD5AuthProtocol;
    session->securityAuthProtoLen = sizeof(usmHMACMD5AuthProtocol)/sizeof(oid);
    session->securityAuthKeyLen = USM_AUTH_KU_LEN;

    gen_serts(session);
}

void
init_connection(struct client_conn *client)
{
    snmp_sess_init(&client->session);

    setup_security_param(&client->session);

    client->ss = snmp_open(&client->session);

    client->session.callback = asynch_response;

    if (!client->ss) {
        snmp_log(LOG_ERR, "Не удалось открыть SNMP-сессию. \n");
        exit(1);
    }
}

void
destroy_connection()
{

}

static void
attach_to_connections_list(struct client_conn *client)
{
    struct conn_hash *conn_hash;

    HASH_FIND_INT(conns_hash, &client->conn_id, conn_hash);

    if (conn_hash) {
        snmp_log(LOG_ERR, "Connection already exists. \n");
        return;
    }

    conn_hash = (struct conn_hash *)calloc(1, sizeof(struct conn_hash));
    conn_hash->conn_id = client->conn_id;

    /* should be const.*/
    conns_hash->client = client;

    HASH_ADD_INT(conns_hash, client->conn_id, conn_hash);
}

static struct conn_hash *
lookup_connection(int conn_id)
{
    struct conn_hash *conn_hash;

    HASH_FIND_INT(conns_hash, &conn_id, conn_hash);

    if (!conn_hash) {
        snmp_log(LOG_ERR, "Connetction already exists. \n");
        return;
    }

    return conn_hash;
}

static void
detach_from_connections_list(int conn_id)
{
    struct conn_hash *conn_hash;

    conn_hash = lookup_connection(conn_id);

    if (!conn_hash) {
        snmp_log(LOG_ERR, "Connetction doesn't exists. \n");
        return;
    }

    HASH_DEL(conns_hash, conn_hash);
    
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
destroy_client(struct client_conn *client)
{
    free(client->ss);

    free(client);   
}

static unsigned int
get_clients_num()
{
    return HASH_COUNT(conns_hash);
}

void
main_loop()
{
    struct conn_hash *current, tmp;

    HASH_ITER(hh, conns_hash, current, tmp) {

    }
}

void
destroy_connections()
{
    struct conn_hash *current, tmp;

    HASH_ITER(hh, conns_hash, current, tmp) {
        HASH_DEL(conns_hash, current);
        free(current->client->ss);
        free(current);
    }
}
