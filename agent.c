#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/library/transform_oids.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "agent.h"

#define CLEAR_SESSION_DATA(session) \
    do {                             \
        free(session);                \
        (session) = NULL;              \
    } while (0)

#define CHECK_SESSION(session, func_name) \
    do {                                   \
        if (!(session)) {                   \
            CLEAR_SESSION_DATA(session);     \
            snmp_log(LOG_ERR,                 \
                    "Error while init "        \
                    "connection in %s !\n",     \
                    (func_name));                \
            return;                               \
        }                                          \
    } while (0)

static void
__setup_timeouts(struct snmp_session *session)
{
    CHECK_SESSION(session, __func__);

    session->timeout = 1000000;
}

static void
__gen_serts(struct ctl_context *ctx,
            struct snmp_session *session)
{
    CHECK_SESSION(session, __func__);

    if (generate_Ku(session->securityAuthProto,
                    session->securityAuthProtoLen,
                    (u_char *) passphrase,
                    strlen(passphrase),
                    session->securityAuthKey,
                    &session->securityAuthKeyLen) != SNMPERR_SUCCESS) {
        snmp_perror(ctx->argv[0]);
        snmp_log(LOG_ERR,
                 "Error generating Ku from authentication pass phrase. \n");
        snmp_shutdown("snmpapp");
        exit(1);
        return;
    }
}

static void
__setup_security_param(struct ctl_context *ctx,
                       struct snmp_session *session)
{
    CHECK_SESSION(session, __func__);

    session->version = SNMP_VERSION_3; 

    session->securityName = strdup("MD5User");
    session->securityNameLen = strlen(session->securityName);

    session->securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;

    session->securityAuthProto = usmHMACMD5AuthProtocol;
    session->securityAuthProtoLen = sizeof(usmHMACMD5AuthProtocol)/sizeof(oid);
    session->securityAuthKeyLen = USM_AUTH_KU_LEN;

    __gen_serts(ctx, session);
}

static void
__setup_configuration_param(struct snmp_session *session)
{
    CHECK_SESSION(session, __func__);

    const char *ip_addr = " ";

    session->peername = strdup(ip_addr);
}

static void
get_info(struct snmp_pdu *pdu,
          struct curve *curve)
{
    for (struct variable_list *vars = pdu->variables; vars;
         vars = vars->next_variable) {
        if (vars->type == ASN_COUNTER) {
            unsigned long bytes = *vars->val.integer;
            snmp_log(LOG_ALERT, "Counter value: %lu\n", bytes);
            curve->curve[curve->util] = bytes;
            curve->util++;
        } else if (vars->type == ASN_COUNTER64) {
            uint64_t bytes = (uint64_t)vars->val.counter64->high << 32 | vars->val.counter64->low;
        } else {
            snmp_log(LOG_ALERT, "Unsupported type: %d\n", vars->type);
        }
    }
}

static void
__send_pdu(struct snmp_session *session,
           struct curve *curve)
{
    CHECK_SESSION(session, __func__);

    struct snmp_pdu *pdu, *response;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len = MAX_OID_LEN; 
    
    pdu = snmp_pdu_create(SNMP_MSG_GET);

    if (!read_objid(".1.3.6.1.2.1.2.2.1.10.1", anOID, &anOID_len)) {
        snmp_log(LOG_ERR, "Неверный OID\n");
        snmp_free_pdu(pdu);
        return;
    }

    snmp_add_null_var(pdu, anOID, anOID_len);

    int status = snmp_synch_response(session, pdu, &response);
    
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
        get_info(response, curve);
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
__disable_connection(struct snmp_session *session)
{
    snmp_close(session);
}

/* if session inition wasn't sec we need to clear mem ? */
void
__init_connetcion(struct ctl_context *ctx)
{
    struct snmp_session session, *ss;
    struct snmp_pdu *pdu;
    struct snmp_pdu *response;
    struct curve curve = {0};

    snmp_sess_init(&session);

    __setup_configuration_param(&session);
    __setup_timeouts(&session);
    __setup_security_param(ctx, &session);

    ss = snmp_open(&session);
    
    if (!ss) {
        snmp_log(LOG_ERR, "Не удалось открыть SNMP-сессию. \n");
        exit(1);
    }

    for (size_t i = 0; i < 10; i++) {
        __send_pdu(ss, &curve);
    }

    __disable_connection(ss);
}   

int
main(int argc, char ** argv)
{
    init_snmp("snmpapp");
    
    struct ctl_context ctx = {argc, argv};

    __init_connetcion(&ctx);

    snmp_shutdown("snmpapp");

}