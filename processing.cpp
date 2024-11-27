#include "processing.h"
#include <glob.h>

void create_gif() {
    // Generate PNGs using gnuplot
    system("gnuplot animation.gnuplot");

    // Clean up intermediate files
    system("rm animation.gnuplot");
}

void create_gnuplot_script(const char *output_file) {
    FILE *file = fopen("animation.gnuplot", "w");
    if (!file) {
        perror("Error creating gnuplot script");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "set terminal gif animate delay 4\n");
    fprintf(file, "set output ARG3\n");
    fprintf(file, "set xrange [0:%d]\n", WIDTH);
    fprintf(file, "set yrange [0:%d]\n", HEIGHT);
    fprintf(file, "N = %d\n", N);
    fprintf(file, "records = ARG2/(2*8*N)\n");
    fprintf(file, "unset key\n");
    fprintf(file, "set style line 1 pt 7 ps 0.1 lc rgb 'blue'\n");

    fprintf(file, "do for [i=0:records-1] {\n");
    fprintf(file, "  plot ARG1 binary format='%%double%%double' every ::(i*N)::((i+1)*N) using 1:2 with points ls 1\n");
    fprintf(file, "}\n");
    fclose(file);
}


void dump(FILE *file, void *boids, size_t s, int n) {
    size_t offset = 0;
    for (int i = 0; i < n; i++) {
        fwrite((char*)boids + offset, sizeof(double)*2, 1, file);
        offset += s;
    }
}


void aggregate_partial(const int num_mpi, const char *pattern, const char *outputfile) {
    glob_t globbuf;
    FILE *files[num_mpi];
    int block_size[num_mpi];
    double positions[2*N];
    FILE *output = fopen(outputfile, "wb");
    if (glob(pattern, 0, NULL, &globbuf) == 0) {
        char **filenames=globbuf.gl_pathv;
        int num_files = globbuf.gl_pathc;

        for (int i = 0; i < num_files; i++) {
            files[i] = fopen(filenames[i], "rb");
            fread(&block_size[i], sizeof(int), 1, files[i]);
        }

        int timesteps = 0;
        while (!feof(files[0])) {
            int total = 0;
            for (int i = 0; i < num_files; i++) {
                fread(&positions[total*2], sizeof(double)*2,block_size[i],files[i]);
                total += block_size[i];
                timesteps += block_size[i];
                fread(&block_size[i], sizeof(int), 1, files[i]);
            }
            fwrite(positions, 2*sizeof(double), N, output);
        }

        printf("%d\n", timesteps);
        for (int i = 0; i < num_mpi; i++) {
            fclose(files[i]);
            remove(filenames[i]);
        }

        fclose(output);
    }
}