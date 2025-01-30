#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>

#define MAX_MESSAGES 1000

static int arrival_times[MAX_MESSAGES] = {0, 10, 20, 30, 50, 70};
static int n = 6;
static int sigma = 1, T = 10;
static double rates[MAX_MESSAGES];
static int num_rates = 0;
pthread_mutex_t lock;

void
detect_outgoing_and_update_critical(int *arrival_times,
                                    int n, double lower_curve,
                                    double upper_curve, double rate,
                                    int *outgoing, int *critical_lower,
                                    int *critical_upper)
{
    if (!arrival_times || n <= 0 || !outgoing || !critical_lower || !critical_upper) {
        fprintf(stderr, "Invalid input parameters to detect_outgoing_and_update_critical\n");
        return;
    }
    
    int m = 0;
    *critical_lower = m;
    *critical_upper = m;
    *outgoing = -1;

    for (int i = 1; i < n; i++) {
        if (arrival_times[i] < 0 || arrival_times[i] > INT_MAX) {
            fprintf(stderr, "Invalid arrival time at index %d\n", i);
            continue;
        }

        if (arrival_times[i] < lower_curve || arrival_times[i] > upper_curve) {
            *outgoing = i;
            break;
        }

        if (i * rate - arrival_times[i] > (*critical_lower) * rate - arrival_times[*critical_lower]) {
            *critical_lower = i;
        }
        
        if (i * rate - arrival_times[i] < (*critical_upper) * rate - arrival_times[*critical_upper]) {
            *critical_upper = i;
        }
    }
}

void
compute_arrival_curves()
{
    pthread_mutex_lock(&lock);
    if (n <= 1 || arrival_times[1] == 0) {
        fprintf(stderr, "Invalid arrival times or insufficient data for curve computation\n");
        pthread_mutex_unlock(&lock);
        return;
    }
    
    int m = 0;
    double rate = 1.0 / arrival_times[1];
    int critical_lower = 0, critical_upper = 0;
    num_rates = 0;

    for (int i = 1; i < n; i++) {
        int outgoing;
        detect_outgoing_and_update_critical(arrival_times, n, sigma, T, rate, &outgoing, &critical_lower, &critical_upper);
        
        if (outgoing != -1) {
            if (arrival_times[outgoing] - arrival_times[critical_lower] == 0) {
                fprintf(stderr, "Division by zero detected in rate computation\n");
                pthread_mutex_unlock(&lock);
                return;
            }
            
            rate = (double)(arrival_times[outgoing] - arrival_times[critical_lower]) / (outgoing - critical_lower);
            rates[num_rates++] = rate;
            m = outgoing;
        }
    }
    pthread_mutex_unlock(&lock);
}

int
handle_snmp_request(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info *reqinfo,
                    netsnmp_request_info *requests)
{
    if (!requests || !requests->requestvb) {
        fprintf(stderr, "Invalid SNMP request\n");
        return SNMP_ERR_GENERR;
    }
    
    switch (reqinfo->mode) {
        case MODE_GET:
            compute_arrival_curves();
            if (num_rates <= 0) {
                fprintf(stderr, "No valid rates computed\n");
                return SNMP_ERR_GENERR;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, (u_char *)rates, sizeof(double) * num_rates);
            break;
        default:
            fprintf(stderr, "Unsupported SNMP request mode\n");
            return SNMP_ERR_GENERR;
    }
    return SNMP_ERR_NOERROR;
}

void
init_custom_snmp_module(void)
{
    static oid custom_oid[] = {1,3,6,1,4,1,99999,1};
    
    netsnmp_register_scalar(netsnmp_create_handler_registration("customOID", handle_snmp_request, custom_oid, OID_LENGTH(custom_oid), HANDLER_CAN_RONLY));
}

void *
snmp_agent_thread(void *arg)
{
    while (1) {
        agent_check_and_process(1);
    }
    return NULL;
}

int
main(int argc, char **argv)
{
    pthread_t snmp_thread;
    
    pthread_mutex_init(&lock, NULL);
    
    netsnmp_enable_subagent();
    
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_NO_TOKEN_WARNINGS, 1);
    
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_DONT_PERSIST_STATE, 1);
    
    init_agent("custom_snmp_module");
    
    init_custom_snmp_module();
    
    init_snmp("custom_snmp_module");
    
    pthread_create(&snmp_thread, NULL, snmp_agent_thread, NULL);
    
    pthread_join(snmp_thread, NULL);
    
    snmp_shutdown("custom_snmp_module");
    
    pthread_mutex_destroy(&lock);
    
    return 0;
}
