#include "config.h"

#ifndef __processing_h
#define __processing_h

void create_gif();
void create_gnuplot_script();
void dump(FILE *file, Boid boids[N], size_t n);
#endif