#include "monitor.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <interface>\n", argv[0]);
        return 1;
    }

    const char *ifname = argv[1];

    if (monitor_mode_on(ifname) != 0) {
        fprintf(stderr, "Failed to enable monitor mode\n");
        return 1;
    }

    printf("Monitor mode active. Waiting 3 seconds...\n");
    sleep(3);

    if (monitor_mode_off(ifname) != 0) {
        fprintf(stderr, "Failed to disable monitor mode\n");
        return 1;
    }

    return 0;
}