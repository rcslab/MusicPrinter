
#include "timesync.h"

TimeSync sync = TimeSync();

int
main(int argc, const char *argv[])
{
    printf("Starting speakerd ...\n");

    sync.start();

    // XXX: Wait on signal to exit
}

