#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef __config_h
#define __config_h

#define OUTPUTFILE "boids.bin"
#define N 1000
#define WIDTH 1000
#define HEIGHT 800
#define MARGIN 200

#define PI 3.14159265358979323846

#define TIMESTEPS 1000

#define VRANGE 40
#define PRANGE 8

#define COHERENCE 0.0005 // centering
#define SEPERATION 0.1 // avoiding
#define ALIGNMENT 0.1 // matching
#define TURNFACTOR 0.2

#define FOV 20

#define MAXSPEED 6
#define MINSPEED 3

typedef struct Boid_t {
    double px;
    double py;
    double vx;
    double vy;
} Boid;


#endif
