#include <stdio.h>

#include "pbmp.h"

int main(void)
{
    char request[4096];
    char response[16384];

    if (!fgets(request, sizeof(request), stdin))
        return 2;
    if (ambnc_pbmp_handle(request, response, sizeof(response)) < 0)
        return 3;
    fputs(response, stdout);
    return 0;
}
