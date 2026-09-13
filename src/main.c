#include <stdio.h>
#include "ambnc.h"

int ambnc_run(void)
{
    puts(AMBNC_NAME " " AMBNC_VERSION);
    puts("M0 foundation build");
    puts("ARexx port reserved: " AMBNC_REXX_PORT);
    return 0;
}

int main(void)
{
    return ambnc_run();
}
