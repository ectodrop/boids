#include "processing.h"

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
    fprintf(file, "set output 'animation.gif'\n");
    fprintf(file, "set xrange [0:%d]\n", WIDTH);
    fprintf(file, "set yrange [0:%d]\n", HEIGHT);
    fprintf(file, "file = '%s'\n", output_file);
    fprintf(file, "N = %d\n", N);
    fprintf(file, "unset key\n");
    fprintf(file, "set style line 1 pt 7 ps 0.1 lc rgb 'blue'\n");

    fprintf(file, "do for [i=0:%d-1] {\n", TIMESTEPS);
    fprintf(file, "  plot file binary format='%%double%%double' every ::(i*N)::((i+1)*N) using 1:2 with points ls 1\n");
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
