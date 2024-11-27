#include "config.h"
#include "processing.h"
#include "clock.h"


#define OUTPUTFILE "boids_omp_grid_%03d.bin"
#define MODULO(a, n) fmod((a), (n)) + (((a) < 0) * (n))
#define CELL_SIZE (VRANGE)
#define GRID_ROWS (int)(HEIGHT/CELL_SIZE) 
#define GRID_COLS (int)(WIDTH/CELL_SIZE) 

typedef struct Boid_t {
    double px;
    double py;
    double vx;
    double vy;
    int next;
    int prev;
} Boid;


int distance_checks = 0;
double rand_double();
int can_see(Boid *boid, Boid *other);
void update_boids(int grid[GRID_ROWS+2][GRID_COLS+2], Boid *old_boids, Boid *new_boids);

void grid_insert(int grid[GRID_ROWS+2][GRID_COLS+2], Boid *boids, int boid) {
    int i = fmin(GRID_ROWS, fmax(boids[boid].py / CELL_SIZE + 1,0)); 
    int j = fmin(GRID_COLS, fmax(boids[boid].px / CELL_SIZE + 1,0)); 

    boids[boid].prev = -1;
    boids[boid].next = grid[i][j];
    if (grid[i][j] != -1)
        boids[grid[i][j]].prev = boid;
    
    grid[i][j] = boid;
}

void grid_recalculate(int grid[GRID_ROWS+2][GRID_COLS+2], Boid *boids, int new_boid, int old_px, int old_py) {
    int old_i = fmin(GRID_ROWS, fmax(old_py / CELL_SIZE + 1,0)); 
    int old_j = fmin(GRID_COLS, fmax(old_px / CELL_SIZE + 1,0)); 
    int i = fmin(GRID_ROWS, fmax(boids[new_boid].py / CELL_SIZE + 1,0)); 
    int j = fmin(GRID_COLS, fmax(boids[new_boid].px / CELL_SIZE + 1,0)); 

    if (i != old_i || j != old_j) {
        // we must remove from old list
        #pragma omp critical
        {
            if (boids[new_boid].prev == -1) // boid is head of list
                grid[old_i][old_j] = boids[new_boid].next;

            if (boids[new_boid].prev != -1)
                boids[boids[new_boid].prev].next = boids[new_boid].next;
            
            if (boids[new_boid].next != -1)
                boids[boids[new_boid].next].prev = boids[new_boid].prev;

            // insert in new cell
            grid_insert(grid, boids, new_boid);
        }
    }
}

int main(int argc, char** argv) {
    srand(0);
    int grid[GRID_ROWS+2][GRID_COLS+2];
    for (int i = 0; i < GRID_ROWS+2; i++) {
        for (int j = 0; j < GRID_COLS+2; j++) {
            grid[i][j] = -1;
        }
    }
    Boid *old_boids = new Boid[N];
    Boid *new_boids = new Boid[N];

    char filename[100];
    sprintf(filename, OUTPUTFILE, 0);
    FILE *file = fopen(filename, "wb");

    struct timeval timer;
    tick(&timer);
    // initialize each boid
    for (int i = 0; i < N; i++) {
        Boid *b = &old_boids[i];
        b->px = WIDTH * rand_double(); 
        b->py = HEIGHT * rand_double(); 

        double theta = rand_double() * 2* PI;
        b->vx = cos(theta)*MINSPEED;
        b->vy = sin(theta)*MINSPEED;
        b->next = -1;
        b->prev = -1;
        grid_insert(grid, old_boids, i);
    }

    for (int i = 0; i < N; i++) {
        new_boids[i] = old_boids[i];
    }

    // update loop
    printf("sizeof %ld %ld %ld\n", sizeof(double),sizeof(int),sizeof(Boid));
    for (int i = 0; i < TIMESTEPS; i++) {
        printf("time %d\n", i);
        for (int row = 0; row < GRID_ROWS+2; row++) {
            for (int col = 0; col < GRID_COLS+2; col++) {
                int ind = grid[row][col];
                int count = 0;
                while (ind != -1) {
                    count++;
                    ind = old_boids[ind].next;
                }
                printf("%4d,", count);
            }
            printf("\n");
        }
        if (i > 0 && i % 100 == 0) {
            fclose(file);
            sprintf(filename, OUTPUTFILE, (int)(i/100));
            file = fopen(filename, "wb");
        }
        dump(file, old_boids, sizeof(Boid), N);
        update_boids(grid, old_boids, new_boids);
    }

    fclose(file);
    printf("simulation took: %fs\n", tock(&timer));

    create_gnuplot_script(OUTPUTFILE);
    free(new_boids);
    free(old_boids);
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

void update_boids(int grid[GRID_ROWS+2][GRID_COLS+2], Boid *old_boids, Boid *new_boids) {
    int old_grid[GRID_ROWS+2][GRID_COLS+2];

    #pragma omp parallel for default(none) shared(old_grid,grid) collapse(2)
    for (int i = 0; i < GRID_ROWS+2; i++) {
        for (int j = 0; j < GRID_COLS+2; j++) {
            old_grid[i][j] = grid[i][j];
        }
    }
    #pragma omp parallel for default(none) shared(old_grid,grid,old_boids,new_boids) collapse(2) schedule(dynamic)
    for (int i = 1; i <= GRID_ROWS; i++) {
        for (int j = 1; j <= GRID_COLS; j++) {
            for (int boid_ind = old_grid[i][j]; boid_ind != -1; boid_ind = old_boids[boid_ind].next) {
                Boid *boid = &old_boids[boid_ind];
                Boid *new_boid = &new_boids[boid_ind];
                new_boid->px = boid->px;
                new_boid->py = boid->py;
                new_boid->vx = boid->vx;
                new_boid->vy = boid->vy;
                double neighbouring_boids = 0;
                double xpos_avg=0, ypos_avg=0, xvel_avg=0, yvel_avg=0, close_dx=0, close_dy=0;

                // iterate through grid cells
                for (int r = -1; r <= 1; r++) {
                    for (int c = -1; c <= 1; c++) {
                        for (int other_ind = old_grid[i+r][j+c]; other_ind != -1; other_ind = old_boids[other_ind].next) {
                            if (other_ind == boid_ind) continue;
                            Boid *other = &old_boids[other_ind];
                            double dx = boid->px - other->px; 
                            double dy = boid->py - other->py; 

                            // if within protected range
                            double dst_sqr = dx*dx + dy*dy;

                            if (dst_sqr < PRANGE*PRANGE) {
                                if (can_see(boid, other)) {
                                    close_dx += dx;
                                    close_dy += dy;
                                }
                            } else if (dst_sqr < VRANGE*VRANGE) {
                                if (can_see(boid, other)) {
                                    xpos_avg += other->px;
                                    ypos_avg += other->py;
                                    xvel_avg += other->vx;
                                    yvel_avg += other->vy;

                                    neighbouring_boids++;
                                }
                            }
                        }
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
                grid_recalculate(grid, new_boids, boid_ind, boid->px, boid->py);
            }
        }
    }

    for (int i = 0; i < N; i++) {
        old_boids[i] = new_boids[i];
    }
}