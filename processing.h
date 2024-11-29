#include "config.h"

#ifndef __processing_h
#define __processing_h

void create_gif();
void create_gnuplot_script(const char *);
void dump(FILE *file, void *boids, size_t s, int n);
void aggregate_partial(const int mpi_size, const char *pattern, const char *outputfile);
#endif