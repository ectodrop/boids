#include <sys/time.h>
#include <stdlib.h>

#ifndef __clock_h
#define __clock_h

void tick(struct timeval * t) {
    gettimeofday(t, NULL) ;
}

double tock ( struct timeval * t ) {
    struct timeval now ;
    gettimeofday (&now, NULL) ;
    return ( double ) (now.tv_sec - t->tv_sec) + ((double) (now.tv_usec - t->tv_usec)/1000000) ;
}

#endif
