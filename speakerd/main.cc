
#include <stdio.h>
#include <unistd.h>

#include "timesync.h"

TimeSync *ts;
int listen_to_commands(TimeSync *ts);

#define SECOND 1000000
int
main(int argc, const char *argv[])
{
    printf("Starting speakerd ...\n");

    ts = new TimeSync();
    ts->start();

    listen_to_commands(ts); 

    ts->stop();
}

