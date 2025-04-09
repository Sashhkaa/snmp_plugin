#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/library/transform_oids.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include "agent.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

static volatile sig_atomic_t keep_running = 1;

void handle_signal(int signal) {
    if (signal == SIGTERM || signal == SIGINT) {
        fprintf(stderr, "Received signal %d, shutting down...\n", signal);
        keep_running = 0;
        active_hosts = 0;
    }
}

int
main()
{
    /* register signal handler. */
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    /* main loop. */
    while (keep_running) {
        main_loop(keep_running);
    }

    destroy_connections();

    snmp_shutdown("snmpapp");

}