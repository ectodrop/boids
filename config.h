#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef __config_h
#define __config_h

// dump to the datafile every PER frames 
#define PER 0

// #define N 60000
#define WIDTH 800
#define HEIGHT 800
#define MARGIN 100 // buffer from the borders of the space in which a turning force is applied

#define PI 3.14159265358979323846

#define TIMESTEPS 1000 // number of timesteps to simulate
#define TIMECHUNK 100 // every TIMECHUNK timesteps, output a data file

#define VRANGE 40 // boid visual range
#define PRANGE 8 // boid avoidance range

#define COHERENCE 0.0005 // how strong is the "centering" force
#define SEPERATION 0.09 // how strong is the "avoidance" force
#define ALIGNMENT 0.1 // how quickly boids align velocities
#define TURNFACTOR 0.5 // how much turning force is applied to the boid before reaching the border

#define FOV 120 // field-of-view in degrees, particles outside the fov of a particle are invisible

// particles capped speed
#define MAXSPEED 6 
#define MINSPEED 3


#endif
