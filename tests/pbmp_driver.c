#include <stdio.h>
#include <string.h>

#include "networks.h"
#include "pbmp.h"

int main(void)
{
    char request[4096];
    char response[16384];
    struct ambnc_networks_config networks;

    memset(&networks, 0, sizeof(networks));
    networks.count = 1;
    strcpy(networks.networks[0].name, "Qualification");
    ambnc_network_runtime_reset();
    ambnc_pbmp_set_networks(&networks);

    if (!fgets(request, sizeof(request), stdin))
        return 2;
    if (ambnc_pbmp_handle(request, response, sizeof(response)) < 0)
        return 3;
    fputs(response, stdout);
    return 0;
}
