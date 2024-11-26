#include <mpi.h>
#include "config.h"
#include "processing.h"
#include "clock.h"
#include <bits/stdc++.h>


#define OUTPUTFILE "boids_omp_grid_%02d_%03d.bin"
#define MODULO(a, n) fmod((a), (n)) + (((a) < 0) * (n))
#define CELL_SIZE (VRANGE)
#define GRID_ROWS (int)(HEIGHT/CELL_SIZE) 
#define GRID_COLS (int)(WIDTH/CELL_SIZE) 
#define LOCAL_GRID_ROWS (GRID_ROWS/2)
#define LOCAL_GRID_COLS (GRID_COLS/2)

#define MPI_WIDTH (WIDTH/2)
#define MPI_HEIGHT (HEIGHT/2)
#define MPI_ROWS 2
#define MPI_COLS 2

#define MPI_ROW(r) (((r)-(r)%MPI_ROWS)/MPI_ROWS)
#define MPI_COL(r) ((r)%MPI_ROWS)
#define MPI_RANK(i,j) ((j)+(i)*MPI_COLS)


typedef struct Boid_t {
    double px;
    double py;
    double vx;
    double vy;
    int id;
    int next;
    int prev;
} Boid;


int distance_checks = 0;
int num_boids = 0;
double rand_double();
int can_see(Boid *boid, Boid *other);
void update_boids(int rank, int grid[LOCAL_GRID_ROWS+2][LOCAL_GRID_COLS+2], Boid *old_boids, Boid *new_boids);

void grid_insert(int rank,int grid[LOCAL_GRID_ROWS+2][LOCAL_GRID_COLS+2], Boid *boids, int boid) {
    int mi = MPI_ROW(rank); // mpi row index for current process
    int mj = MPI_COL(rank); // mpi col index for current process
  
    int i = fmin(LOCAL_GRID_ROWS, fmax((int)(boids[boid].py - mi*MPI_HEIGHT) / CELL_SIZE + 1,1));
    int j = fmin(LOCAL_GRID_COLS, fmax((int)(boids[boid].px - mj*MPI_WIDTH) / CELL_SIZE + 1,1)); 

    boids[boid].prev = -1;
    boids[boid].next = grid[i][j];
    if (grid[i][j] != -1)
        boids[grid[i][j]].prev = boid;
    
    grid[i][j] = boid;
}

void grid_remove(int rank, int grid[LOCAL_GRID_ROWS+2][LOCAL_GRID_COLS+2], Boid *boids, int boid) {
    int mi = MPI_ROW(rank); // mpi row index for current process
    int mj = MPI_COL(rank); // mpi col index for current process
  
    int i = fmin(LOCAL_GRID_ROWS, fmax((int)(boids[boid].py - mi*MPI_HEIGHT) / CELL_SIZE + 1,1));
    int j = fmin(LOCAL_GRID_COLS, fmax((int)(boids[boid].px - mj*MPI_WIDTH) / CELL_SIZE + 1,1)); 

    if (boids[boid].prev == -1) // boid is head of list
        grid[i][j] = boids[boid].next;

    if (boids[boid].prev != -1)
        boids[boids[boid].prev].next = boids[boid].next;
    
    if (boids[boid].next != -1)
        boids[boids[boid].next].prev = boids[boid].prev;
    
    boids[boid].next = -1;
    boids[boid].prev = -1;
}

void grid_recalculate(int rank, int grid[LOCAL_GRID_ROWS+2][LOCAL_GRID_COLS+2], Boid *boids, int new_boid, int old_px, int old_py) {
    int mi = MPI_ROW(rank); // mpi row index for current process
    int mj = MPI_COL(rank); // mpi col index for current process

    int old_i = fmin(LOCAL_GRID_ROWS, fmax((old_py-mi*MPI_HEIGHT) / CELL_SIZE + 1,1)); 
    int old_j = fmin(LOCAL_GRID_COLS, fmax((old_px-mj*MPI_WIDTH) / CELL_SIZE + 1,1)); 
    int i = fmin(LOCAL_GRID_ROWS, fmax((boids[new_boid].py - mi*MPI_HEIGHT) / CELL_SIZE + 1,1)); 
    int j = fmin(LOCAL_GRID_COLS, fmax((boids[new_boid].px - mj*MPI_WIDTH) / CELL_SIZE + 1,1)); 

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
            grid_insert(rank, grid, boids, new_boid);
        }
    }
}

int main(int argc, char** argv) {
    srand(0);
    int grid[LOCAL_GRID_ROWS+2][LOCAL_GRID_COLS+2];
    for (int i = 0; i < LOCAL_GRID_ROWS+2; i++) {
        for (int j = 0; j < LOCAL_GRID_COLS+2; j++) {
            grid[i][j] = -1;
        }
    }
    int rank, size;
    const int nfields = 7;
    MPI_Datatype boid_types[nfields] = { MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_INT, MPI_INT, MPI_INT };
    int boid_lengths[nfields] = { 1,1,1,1,1,1,1 };
    MPI_Aint offsets[nfields] = {
        offsetof(Boid,px),
        offsetof(Boid,py),
        offsetof(Boid,vx),
        offsetof(Boid,vy),
        offsetof(Boid,id),
        offsetof(Boid,next),
        offsetof(Boid,prev)};
    MPI_Datatype MPI_BOID;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int mi = MPI_ROW(rank);
    const int mj = MPI_COL(rank);
    MPI_Type_create_struct(nfields, boid_lengths, offsets, boid_types, &MPI_BOID);
    MPI_Type_commit(&MPI_BOID);

    Boid *old_boids = new Boid[N];
    Boid *new_boids = new Boid[N];

    Boid *send_boids = new Boid[5*N];
    Boid *recv_boids = new Boid[5*N];

    char filename[100];
    sprintf(filename, OUTPUTFILE, rank,0);
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
        b->id = i;

        int boid_mi = fmin(MPI_ROWS-1, fmax((int)(b->py / MPI_HEIGHT), 0));
        int boid_mj = fmin(MPI_COLS-1, fmax((int)(b->px / MPI_WIDTH), 0)); 

        if (mi == boid_mi && mj == boid_mj) {
            // this is our boid
            num_boids++;
            grid_insert(rank, grid, old_boids, i);
        }
    }

    for (int i = 0; i < N; i++) {
        new_boids[i] = old_boids[i];
    }

    int top_rank = MPI_RANK(mi-1,mj);
    if (mi == 0) top_rank = MPI_PROC_NULL;

    int bot_rank = MPI_RANK(mi+1,mj);
    if (mi == MPI_ROWS-1) bot_rank = MPI_PROC_NULL;

    int left_rank = MPI_RANK(mi,mj-1);
    if (mj == 0) left_rank = MPI_PROC_NULL;

    int right_rank = MPI_RANK(mi,mj+1);
    if (mj == MPI_COLS-1) right_rank = MPI_PROC_NULL;

    int corner = MPI_PROC_NULL;
    if (rank == 0) corner = 3;
    else if (rank == 1) corner = 2;
    else if (rank == 2) corner = 1;
    else if (rank == 3) corner = 0;

    const int neighbours[5] = { top_rank, bot_rank, left_rank, right_rank, corner};
    // update loop
    for (int i = 0; i < TIMESTEPS; i++) {
        if (i > 0 && i % 100 == 0) {
            fclose(file);
            sprintf(filename, OUTPUTFILE, rank, (int)(i/100));
            file = fopen(filename, "wb");
        }
        update_boids(rank, grid, old_boids, new_boids);
        // rank == 0 // top left

        int counts[5] = { 0 };

        int ind, j, boid_mi, boid_mj;

        for (j = 1; j <= LOCAL_GRID_COLS; j++) {
            if (top_rank != MPI_PROC_NULL) {
                ind = grid[1][j];
                while (ind != -1) {
                    int tmp = new_boids[ind].next; 
                    send_boids[N*0 + counts[0]++] = new_boids[ind];
                    boid_mi = fmin(MPI_ROWS-1, fmax((int)(new_boids[ind].py / MPI_HEIGHT), 0));
                    boid_mj = fmin(MPI_COLS-1, fmax((int)(new_boids[ind].px / MPI_WIDTH), 0)); 
                    if (mi != boid_mi && mj == boid_mj) grid_remove(rank, grid, new_boids, ind);
                    ind = tmp;
                }
            }
            if (bot_rank != MPI_PROC_NULL) {
                ind = grid[LOCAL_GRID_ROWS][j];
                while (ind != -1) {
                    int tmp = new_boids[ind].next; 
                    send_boids[N*1 + counts[1]++] = new_boids[ind];
                    boid_mi = fmin(MPI_ROWS-1, fmax((int)(new_boids[ind].py / MPI_HEIGHT), 0));
                    boid_mj = fmin(MPI_COLS-1, fmax((int)(new_boids[ind].px / MPI_WIDTH), 0)); 
                    if (mi != boid_mi && mj == boid_mj) grid_remove(rank, grid, new_boids, ind);
                    ind = tmp;
                }
            }
        }

        for (j = 1; j <= LOCAL_GRID_ROWS; j++) {
            if (left_rank != MPI_PROC_NULL) {
                ind = grid[j][1];
                while (ind != -1) {
                    int tmp = new_boids[ind].next; 
                    send_boids[N*2 + counts[2]++] = new_boids[ind];
                    boid_mi = fmin(MPI_ROWS-1, fmax((int)(new_boids[ind].py / MPI_HEIGHT), 0));
                    boid_mj = fmin(MPI_COLS-1, fmax((int)(new_boids[ind].px / MPI_WIDTH), 0)); 
                    if (mi == boid_mi && mj != boid_mj) grid_remove(rank, grid, new_boids, ind);
                    ind = tmp;
                }
            }
            if (right_rank != MPI_PROC_NULL) {
                ind = grid[j][LOCAL_GRID_COLS];
                while (ind != -1) {
                    int tmp = new_boids[ind].next; 
                    send_boids[N*3 + counts[3]++] = new_boids[ind];
                    boid_mi = fmin(MPI_ROWS-1, fmax((int)(new_boids[ind].py / MPI_HEIGHT), 0));
                    boid_mj = fmin(MPI_COLS-1, fmax((int)(new_boids[ind].px / MPI_WIDTH), 0)); 
                    if (mi == boid_mi && mj != boid_mj) grid_remove(rank, grid, new_boids, ind);
                    ind = tmp;
                }
            }
        }
        if (corner != MPI_PROC_NULL) {
            int row,col;
            if (rank == 0) {
                row = LOCAL_GRID_ROWS;
                col = LOCAL_GRID_COLS;
            } else if (rank == 1) {
                row = LOCAL_GRID_ROWS;
                col = 1;
            } else if (rank == 2) {
                row = 1;
                col = LOCAL_GRID_COLS;
            } else if (rank == 3) {
                row = 1;
                col = 1;
            }
            ind = grid[row][col];
            while (ind != -1) {
                int tmp = new_boids[ind].next; 
                send_boids[N*4 + counts[4]++] = new_boids[ind];
                boid_mi = fmin(MPI_ROWS-1, fmax((int)(new_boids[ind].py / MPI_HEIGHT), 0));
                boid_mj = fmin(MPI_COLS-1, fmax((int)(new_boids[ind].px / MPI_WIDTH), 0)); 
                if (mi != boid_mi && mj != boid_mj) grid_remove(rank, grid, new_boids, ind);
                ind = tmp;
            }
        }
        MPI_Request recv_requests[5], send_requests[5];
        for (j = 0; j < 5; j++) {
            int n = neighbours[j];

            MPI_Isend(&send_boids[j*N], counts[j], MPI_BOID, n, 0, MPI_COMM_WORLD, &send_requests[j]);
            MPI_Irecv(&recv_boids[j*N], N, MPI_BOID, n, 0, MPI_COMM_WORLD, &recv_requests[j]);
        }

        for (j = 0; j < 5; j++) {
            MPI_Status recv_status;
            MPI_Wait(&recv_requests[j], &recv_status);
            int count;
            MPI_Get_count(&recv_status, MPI_BOID, &count);
            for (int k = 0; k < count; k++) {
                Boid b = recv_boids[j*N+k];
                new_boids[b.id] = b;

                boid_mi = fmin(MPI_ROWS-1, fmax((int)(new_boids[b.id].py / MPI_HEIGHT), 0));
                boid_mj = fmin(MPI_COLS-1, fmax((int)(new_boids[b.id].px / MPI_WIDTH), 0)); 
                if (mi == boid_mi && mj == boid_mj) {
                    new_boids[b.id].prev = -1;
                    new_boids[b.id].next = -1;
                    grid_insert(rank, grid, new_boids, b.id);
                } else if (b.prev == -1) { // the head of a list
                    int bi = fmin(LOCAL_GRID_ROWS+1, fmax((b.py - mi*MPI_HEIGHT) / CELL_SIZE + 1,0));  
                    int bj = fmin(LOCAL_GRID_COLS+1, fmax((b.px - mj*MPI_WIDTH) / CELL_SIZE + 1,0));  
                    grid[bi][bj] = -1;
                }
            }

        }
        MPI_Waitall(5, send_requests, MPI_STATUSES_IGNORE);

        for (int i = 0; i < N; i++) {
            old_boids[i] = new_boids[i];
        }

        dump(file, old_boids, sizeof(Boid), N);
    }

    fclose(file);
    printf("simulation took: %fs, creating plot script %d\n", tock(&timer), distance_checks);

    create_gnuplot_script(OUTPUTFILE);

    free(new_boids);
    free(old_boids);

    MPI_Finalize();
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

void update_boids(int rank, int grid[LOCAL_GRID_ROWS+2][LOCAL_GRID_COLS+2], Boid *old_boids, Boid *new_boids) {
    int old_grid[LOCAL_GRID_ROWS+2][LOCAL_GRID_COLS+2];

    #pragma omp parallel for default(none) shared(old_grid,grid) collapse(2)
    for (int i = 0; i < LOCAL_GRID_ROWS+2; i++) {
        for (int j = 0; j < LOCAL_GRID_COLS+2; j++) {
            old_grid[i][j] = grid[i][j];
        }
    }
    #pragma omp parallel for default(none) shared(old_grid,grid,old_boids,new_boids,rank) collapse(2) schedule(dynamic)
    for (int i = 1; i <= LOCAL_GRID_ROWS; i++) {
        for (int j = 1; j <= LOCAL_GRID_COLS; j++) {
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
                grid_recalculate(rank, grid, new_boids, boid_ind, boid->px, boid->py);
            }
        }
    }
}