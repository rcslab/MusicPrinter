
#include <stdio.h>
#include <unistd.h>

#include "timesync.h"

TimeSync *ts;
int listen_to_commands();

#define SECOND 1000000
int
main(int argc, const char *argv[])
{
    printf("Starting speakerd ...\n");

    ts = new TimeSync();
    ts->start();
    printf("PRE SLEEP\n");
    sleep(5);
    printf("POST SLEEP\n");

    listen_to_commands(); 

    ts->stop();
}

