#include "config.h"
#include "processing.h"
#include "clock.h"


#define OUTPUTFILE "boids_omp_grid_%03d.bin"
#define MODULO(a, n) fmod((a), (n)) + (((a) < 0) * (n))
#define CELL_SIZE (VRANGE)
#define GRID_ROWS (int)(HEIGHT/CELL_SIZE) 
#define GRID_COLS (int)(WIDTH/CELL_SIZE) 

#define IMAGE_HEIGHT 90
#define IMAGE_WIDTH 120
typedef struct Boid_t {
    double px;
    double py;
    double vx;
    double vy;
    int next;
    int prev;
} Boid;

int *buf;

double rand_double();
int can_see(Boid *boid, Boid *other);
void update_boids(int grid[GRID_ROWS+2][GRID_COLS+2], Boid *old_boids, Boid *new_boids, const int N);

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
    FILE *f;
    if (PER != 0) f = fopen("./apple/apple.bin", "rb");
    int frames = 6562;
    int buf_size = sizeof(int)*IMAGE_HEIGHT*IMAGE_WIDTH*frames;
    int *buffer = (int*)malloc(buf_size);
    if (argc < 2) {
        fprintf(stderr, "missing argument 1: num_boids\n");
        exit(1);
    }
    const int N = atoi(argv[1]);
    buf = buffer;
    fread(buf, buf_size, 1, f);
    buf += IMAGE_HEIGHT*IMAGE_WIDTH*30;
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
    for (int i = 0; i < TIMESTEPS; i++) {
        if (i % 2 == 0) {
            buf += IMAGE_HEIGHT*IMAGE_WIDTH;
        }
        if (i > 0 && i % TIMECHUNK == 0) {
            fclose(file);
            sprintf(filename, OUTPUTFILE, (int)(i/TIMECHUNK));
            file = fopen(filename, "wb");
        }
        if (PER != 0 && i % PER == 0)
            dump(file, old_boids, sizeof(Boid), N);
        update_boids(grid, old_boids, new_boids, N);
        
        for (int i = 0; i < N; i++) {
            old_boids[i] = new_boids[i];
        }
    }

    if (PER != 0)
        fclose(file);
    printf("simulation took: %fs\n", tock(&timer));

    free(new_boids);
    free(old_boids);
    free(buffer);
}


double rand_double() {
    return (double)rand() / RAND_MAX;
}


int can_see(Boid *boid, Boid *other) {
    static const double theta = cos(FOV*PI/180);
    double dx = other->px - boid->px;
    double dy = other->py - boid->py;
    double dot = dx*boid->vx + dy*boid->vy; 

    double dnorm = dx*dx + dy*dy;
    double vnorm = boid->vx*boid->vx + boid->vy*boid->vy;
    return (dot/(sqrt(dnorm)*sqrt(vnorm)) > theta);
}

void update_boids(int grid[GRID_ROWS+2][GRID_COLS+2], Boid *old_boids, Boid *new_boids, const int N) {
    int old_grid[GRID_ROWS+2][GRID_COLS+2];

    #pragma omp parallel for default(none) shared(old_grid,grid) collapse(2)
    for (int i = 0; i < GRID_ROWS+2; i++) {
        for (int j = 0; j < GRID_COLS+2; j++) {
            old_grid[i][j] = grid[i][j];
        }
    }
    #pragma omp parallel for default(none) shared(buf,old_grid,grid,old_boids,new_boids) collapse(2) schedule(dynamic)
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


                int image_x = (boid->px / WIDTH) * IMAGE_WIDTH;
                int image_y = IMAGE_HEIGHT - (boid->py / HEIGHT) * IMAGE_HEIGHT;
                int index = image_x + image_y*IMAGE_WIDTH;
                if (buf[index] != -1) {
                    int b_x = buf[index] % IMAGE_WIDTH;
                    int b_y = (buf[index] - (buf[index] % IMAGE_WIDTH)) / IMAGE_WIDTH;
                    int dx = b_x - image_x;
                    int dy = b_y - image_y;
                    if (dx != 0 || dy != 0) {
                        new_boid->vx += dx*6;//(sqrt(dx*dx+dy*dy));
                        new_boid->vy += dy*6;//(sqrt(dx*dx+dy*dy));
                    }
                }

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
}