
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

    while (1) {
	ts->sleepUntil(2 * SECOND + ts->getTime());
	printf("WAKE UP DAMN U");

    }

    ts->stop();
}

