
#include <stdio.h>
#include <unistd.h>

#include "timesync.h"

TimeSync *ts;

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
    while (1) {
	ts->sleepUntil(2 * SECOND + ts->getTime());
    }

    ts->stop();
}

