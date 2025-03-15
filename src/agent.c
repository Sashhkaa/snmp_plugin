#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/library/transform_oids.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "agent.h"
#include "../../uthash/src/uthash.h"

static void
send_pdu(struct client_conn *client)
{
    struct snmp_pdu *pdu, *response;
    size_t anOID_len = MAX_OID_LEN; 
    char *oid_str;
    oid anOID[MAX_OID_LEN];

    pdu = snmp_pdu_create(SNMP_MSG_GET);

    sprintf(oid_str, ".1.3.6.1.2.1.2.2.1.%s.%d",
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

    __gen_serts(session);
}

void
init_connection(struct client_conn *client)
{
    snmp_sess_init(&client->session);

    setup_security_param(&client->session);

    /* нужно ли для каждого ново отправленного сообщения
        заново создавать соединение или мы можем беспрерывно
        поддерживать изначально иницализированное ? */
    client->ss = snmp_open(&client->session);
    
    if (!client->ss) {
        snmp_log(LOG_ERR, "Не удалось открыть SNMP-сессию. \n");
        exit(1);
    }
}

void
destroy_connection(int conn_id)
{
 
}

static void
attach_to_connections(int conn_id)
{
    n_conns++;
    
    if (n_conns > n_capacity) {
        conns_hash = (struct conn_hash *)realloc(sizeof(struct conn_hash *), 10);
    }

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

void
main_loop()
{

}

void
init_connections(struct client_conn *client)
{
    conns_hash = (struct conn_hash *)calloc(10, sizeof(struct conn_hash));

    if (!conns_hash) {
        perror("Failed to allocate memory for connection_ids");
        exit(1);
    }
    
    n_capacity = 10;
    n_conns = 0;
}

void
destroy_connections()
{
    free(conns_hash);
}
