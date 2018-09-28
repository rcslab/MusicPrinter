
#include <unistd.h>

#include "timesync.h"

TimeSync *ts;

int
main(int argc, const char *argv[])
{
    printf("Starting speakerd ...\n");

    ts = new TimeSync();
    ts->start();

    while (1) {
	printf("GET TIME MUTHAFUCKAS : %lu \n", ts->getTime());
        sleep(1);
    }

    ts->stop();
}

