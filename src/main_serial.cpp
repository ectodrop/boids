#include "config.h"
#include "processing.h"
#include "clock.h"

#define OUTPUTFILE "boids_serial.bin"
#define MODULO(a, n) fmod((a), (n)) + (((a) < 0) * (n))

typedef struct Boid_t {
    double px;
    double py;
    double vx;
    double vy;
} Boid;

double rand_double();
int can_see(Boid *boid, Boid *other);
void update_boids(Boid *boids, const int N);

int main(int argc, char** argv) {
    srand(0);
    if (argc < 2) {
        fprintf(stderr, "missing argument 1: num_boids\n");
        exit(1);
    }
    const int N = atoi(argv[1]);
    Boid boids[N];
    FILE *file;
    
    if (PER != 0) file = fopen(OUTPUTFILE, "wb");

    struct timeval timer;
    tick(&timer);
    // initialize each boid
    for (int i = 0; i < N; i++) {
        Boid *b = &boids[i];
        b->px = WIDTH * rand_double(); 
        b->py = HEIGHT * rand_double(); 

        double theta = rand_double() * 2* PI;
        b->vx = cos(theta)*MINSPEED;
        b->vy = sin(theta)*MINSPEED;
    }

    // update loop
    for (int i = 0; i < TIMESTEPS; i++) {
        if (PER != 0 && i % PER == 0)
            dump(file, boids, sizeof(Boid), N);
        update_boids(&boids[0], N);
    }
    if (PER != 0)
        fclose(file);
    printf("simulation took: %fs\n", tock(&timer));

    tick(&timer);
}


double rand_double() {
    return (double)rand() / RAND_MAX;
}

int can_see(Boid *boid, Boid *other) {
    double dx = other->px - boid->px;
    double dy = other->py - boid->py;
    double dot = dx*boid->vx + dy*boid->vy; 

    if (dot < 0)
        return 0;

    double theta = cos(FOV*PI/180);
    double dnorm = dx*dx + dy*dy;
    double vnorm = boid->vx*boid->vx + boid->vy*boid->vy;
    return ((dot*dot)/(dnorm*vnorm) > theta*theta);
}


void update_boids(Boid *boids, const int N) {
    Boid old_boids[N];
    for (int i = 0; i < N; i++) {
        old_boids[i] = boids[i];
    }
    for (int i = 0; i < N; i++) {
        double neighbouring_boids = 0;
        double xpos_avg=0, ypos_avg=0, xvel_avg=0, yvel_avg=0, close_dx=0, close_dy=0;

        Boid *boid = &old_boids[i];
        Boid *new_boid = &boids[i];
        for (int j = 0; j < N; j++) {
            if (i == j) continue;
            Boid *other = &old_boids[j];

            double dx = boid->px - other->px; 
            double dy = boid->py - other->py; 

            // if within protected range
            double dst_sqr = dx*dx + dy*dy;
            if (!can_see(boid, other)) continue;

            if (dst_sqr < PRANGE*PRANGE) {
                close_dx += dx;
                close_dy += dy;
            } else if (dst_sqr < VRANGE*VRANGE) {
                xpos_avg += other->px;
                ypos_avg += other->py;
                xvel_avg += other->vx;
                yvel_avg += other->vy;

                neighbouring_boids++;
            }
        }

        if (neighbouring_boids > 0) {
            xpos_avg /= neighbouring_boids;
            ypos_avg /= neighbouring_boids;
            xvel_avg /= neighbouring_boids;
            yvel_avg /= neighbouring_boids;

            new_boid->vx += ((xpos_avg - boid->px) * COHERENCE + (xvel_avg - boid->vx) * ALIGNMENT);
            new_boid->vy += ((ypos_avg - boid->py) * COHERENCE + (yvel_avg - boid->vy) * ALIGNMENT);
        }

        new_boid->vx += (close_dx * SEPERATION);
        new_boid->vy += (close_dy * SEPERATION);

        // check if in margins
        if (boid->px < MARGIN) new_boid->vx += TURNFACTOR;
        if (boid->px > WIDTH-MARGIN) new_boid->vx -= TURNFACTOR;

        if (boid->py < MARGIN) new_boid->vy += TURNFACTOR;
        if (boid->py > HEIGHT-MARGIN) new_boid->vy -= TURNFACTOR;

        double speed = sqrt(new_boid->vx*new_boid->vx + new_boid->vy*new_boid->vy);

        if (speed < MINSPEED) {
            new_boid->vx = (new_boid->vx/speed)*MINSPEED;
            new_boid->vy = (new_boid->vy/speed)*MINSPEED;
        }
        if (speed > MAXSPEED) {
            new_boid->vx = (new_boid->vx/speed)*MAXSPEED;
            new_boid->vy = (new_boid->vy/speed)*MAXSPEED;
        }

        new_boid->px = fmin(WIDTH, fmax(new_boid->px + new_boid->vx, 0));
        new_boid->py = fmin(HEIGHT, fmax(new_boid->py + new_boid->vy, 0));
    }
}