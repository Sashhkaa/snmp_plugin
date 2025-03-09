#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/library/transform_oids.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include "agent.h"

volatile sig_atomic_t keep_running = 1;

void handle_signal(int signal) {
    keep_running = 0;
}

int
main()
{
    /* register signal handler. */
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    init_snmp("snmpapp");

    /* main loop. */
    while (keep_running) {

    }

    destroy_connections();

    snmp_shutdown("snmpapp");
}