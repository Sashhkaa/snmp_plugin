#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include "agent.h"

#define MAX_OID_LEN 128

/*  Cхема работы: 
 *  SNMP-клиент  <--UDP:161-->  SNMP-агент (snmpd)  <---->  SNMP-менеджер (Плагин (C))
 *    
 */

/* TODO: query for connections 
         securityLevel and securityInfo 
         reties, timeout, some auntification.
*/


/* Глобальная очередь запросов */
struct queue request_queue;

int active_hosts; 

struct conn {
  char *name;
  char *ipaddr;
  char *community;
} conns[] = {
  { "test1", "192.168.0.101", "public" },
  { "test2", "192.168.0.102", "public" },
  { "test3", "192.168.0.103", "public" },
  { "test4", "192.168.0.104", "public" },
  { NULL }
};

struct oid {
  char *Name;
  oid Oid[MAX_OID_LEN];
  int OidLen;
} oids[] = {
  { "system.sysDescr.0" },
  { "interfaces.ifNumber.1" },
  { "interfaces.ifNumber.0" },
  { NULL }
};

struct session {
  struct snmp_session *sess; 
  struct oid *current_oid;
} sessions[4];

void
print_reply(netsnmp_pdu *pdu)
{
    netsnmp_variable_list *var;

    for (var = pdu->variables; var; var = var->next_variable) {
        char buf[1024];
        
        snprint_objid(buf, sizeof(buf), var->name,
                      var->name_length);
        printf("OID: %s, Value: %s\n", buf,
                      var->val.string);
    }
}

static char *
__init_client_connection(struct snmp_session *session,
                         struct conn *conn)
{
    /* TODO: error hendler. */

    snmp_sess_init(session);

    session->version=SNMP_VERSION_3;

    session->peername = strdup(conn->ipaddr);
    session->securityName = strdup(conn->community);
    session->securityNameLen = strlen(session->securityName);
    
    /* TODO: think about some encryption. */
    session->securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;

    return NULL;
}

static struct snmp_session *
create_connection(struct conn *conn,
                  struct snmp_session *session)
{
    struct snmp_session *sp;

    char *error = __init_client_connection(session, conn);

    sp = snmp_open(session);

    if (!sp)
    {
        snmp_log(LOG_ERR, "uuu beda!!!\n");
        exit(2);
    }

    return sp;
}

static struct snmp_pdu *
send_pdu_mess(struct snmp_session *ss,
              struct oid *op)
{
    struct snmp_pdu *pdu, *response;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len = MAX_OID_LEN;
    int status;

    /* create snmpget request. */
    pdu = snmp_pdu_create(SNMP_MSG_GET);

    /* we set it to null value in request message. */
    snmp_add_null_var(pdu, op->Oid, op->OidLen);

    status = snmp_synch_response(ss, pdu, &response);

    if (status == STAT_SUCCESS &&
        response->errstat == SNMP_ERR_NOERROR) {
        snmp_log(LOG_INFO, "error in connection!!!\n");
    }

    if (response) {
        snmp_free_pdu(response);
    }

    snmp_close(ss);

    return pdu;
}

static int
asynch_response(int operation, struct snmp_session *sp,
                int reqid, struct snmp_pdu *pdu,
                void *magic)
{
    struct session *host = (struct session *)magic;
    struct snmp_pdu *req;

    if (operation == RECEIVED_MESSAGE) {
        print_reply(pdu);
        host->current_oid++;                      /* send next GET (if any) */
        if (host->current_oid->Name) {

            req = snmp_pdu_create(SNMP_MSG_GET);
            snmp_add_null_var(req, host->current_oid->Oid,
                              host->current_oid->OidLen);
            
            if (snmp_send(host->sess, req)) {
                return 1;
            } else {
                snmp_perror("snmp_send");
                snmp_free_pdu(req);
            }
    
        }
    } else {
        print_reply(pdu);
    }

    active_hosts--;
    return 1;
}

static void
working_process(void)
{
    struct session *hs;
    struct conn *hp;

    /* startup all hosts */
    for (hs = sessions, hp = conns; hp->name; hs++, hp++) { 
        struct snmp_session session, *sp;
        struct snmp_pdu *request;

        sp = create_connection(hp, &session);

        session.callback = asynch_response;
        session.callback_magic = hs;
        hs->current_oid = oids;

        request = send_pdu_mess(sp, hs->current_oid);
    }
}

int
main(int argc, char **argv)
{ 
    init_agent("custom_snmp_module");
     
    init_master_agent();
    
    working_process(); // асинхронный процесс

    shutdown_master_agent();
     
    return 0;
}