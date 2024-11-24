#include "processing.h"

void create_gif() {
    // Generate PNGs using gnuplot
    system("gnuplot animation.gnuplot");

    // Combine PNGs into a GIF using ImageMagick
    system("convert -delay 1x30 -loop 0 frame_*.png animation.gif");

    // Clean up intermediate files
    system("rm frame_*.dat frame_*.png animation.gnuplot");
}

void create_gnuplot_script() {
    FILE *file = fopen("animation.gnuplot", "w");
    if (!file) {
        perror("Error creating gnuplot script");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "set terminal gif animate delay 4\n");
    fprintf(file, "set xrange [0:%d]\n", WIDTH);
    fprintf(file, "set yrange [0:%d]\n", HEIGHT);
    fprintf(file, "file = '%s'\n", OUTPUTFILE);
    fprintf(file, "unset key\n");
    fprintf(file, "set style line 1 pt 7 ps 0.1 lc rgb 'blue'\n");

    fprintf(file, "do for [i=0:%d-1] {\n", TIMESTEPS);
    fprintf(file, "  set output sprintf('plot_window_%%d.png', i)");
    fprintf(file, "  plot file binary format='%%double%%double%%double%%double' every :::i::i using 1:2 with points ls 1\n", N);
    fprintf(file, "}\n");
    fclose(file);
}


void dump(FILE *file, Boid *boids, size_t n) {
    fwrite(boids, sizeof(Boid), n, file);
    fprintf(file, "\n");
}
