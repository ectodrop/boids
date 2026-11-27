# Render the scaling charts used in the README.
# Usage (from repo root):  gnuplot scripts/plot_scaling.gp
# Inputs : results/*.dat   (column 1 = OpenMP threads, column 2 = runtime in seconds)
# Outputs: docs/media/strong_scaling.png, docs/media/weak_scaling.png

c_omp  = '#257de0'   # OpenMP grid (1 node)
c_mpi  = '#f47230'   # MPI + OpenMP grid (4 nodes)
c_ref  = '#9c9c9c'   # reference lines
c_text = '#191919'
c_mute = '#666666'
c_grid = '#e6e6e6'

set terminal pngcairo size 1200,480 enhanced font 'Helvetica,12' linewidth 1 background '#ffffff'
set border 3 lc rgb c_mute lw 1
set tics nomirror textcolor rgb c_mute out scale 0.5
set grid ytics lc rgb c_grid lw 1 lt 1
set xrange [0.5:16.5]
set xtics 1,1,16
set key top left Left reverse noautotitle textcolor rgb c_text samplen 2 spacing 1.3 box lc rgb '#ffffff'
set style line 1 lc rgb c_omp lw 2 pt 7 ps 0.9
set style line 2 lc rgb c_mpi lw 2 pt 7 ps 0.9
set style line 3 lc rgb c_ref lw 1.5 dt (6,4)

stats 'results/omp_strong.dat' u 2 nooutput
t1 = STATS_max           # 1-thread OpenMP baseline

# ---------------------------------------------------------------- strong
set output 'docs/media/strong_scaling.png'
set multiplot layout 1,2 title "{/:Bold Strong scaling} — 5,000 boids, 1,000 timesteps" font 'Helvetica,15' textcolor rgb c_text

set title 'Runtime' textcolor rgb c_text
set xlabel 'OpenMP threads per process' textcolor rgb c_mute
set ylabel 'seconds' textcolor rgb c_mute
set yrange [0:*]
plot 'results/omp_strong.dat'     u 1:2 w lp ls 1 t 'OpenMP grid (1 node)', \
     'results/mpi_omp_strong.dat' u 1:2 w lp ls 2 t 'MPI + OpenMP grid (4 nodes)'

set title 'Speedup over 1-thread OpenMP' textcolor rgb c_text
set ylabel 'speedup (×)' textcolor rgb c_mute
set yrange [0:16.5]
plot x w l ls 3 t 'ideal (linear)', \
     'results/omp_strong.dat'     u 1:(t1/$2) w lp ls 1 t 'OpenMP grid (1 node)', \
     'results/mpi_omp_strong.dat' u 1:(t1/$2) w lp ls 2 t 'MPI + OpenMP grid (4 nodes)'
unset multiplot

# ---------------------------------------------------------------- weak
stats 'results/omp_weak.dat'     u 2 nooutput
w_omp = STATS_min
stats 'results/mpi_omp_weak.dat' u 2 nooutput
w_mpi = STATS_min

set output 'docs/media/weak_scaling.png'
set multiplot layout 1,2 title "{/:Bold Weak scaling} — problem size grows with thread count" font 'Helvetica,15' textcolor rgb c_text

set title 'Runtime' textcolor rgb c_text
set xlabel 'OpenMP threads per process' textcolor rgb c_mute
set ylabel 'seconds' textcolor rgb c_mute
set yrange [0:*]
plot 'results/omp_weak.dat'     u 1:2 w lp ls 1 t 'OpenMP grid (3,000 boids × threads)', \
     'results/mpi_omp_weak.dat' u 1:2 w lp ls 2 t 'MPI + OpenMP grid (2,500 boids × threads)'

set title 'Weak-scaling efficiency  (T_1 / T_n)' textcolor rgb c_text
set ylabel 'efficiency' textcolor rgb c_mute
set yrange [0:105]
set format y '%.0f%%'
set key inside right center Right noreverse
plot 100 w l ls 3 t 'ideal', \
     'results/omp_weak.dat'     u 1:(100*w_omp/$2) w lp ls 1 t 'OpenMP grid', \
     'results/mpi_omp_weak.dat' u 1:(100*w_mpi/$2) w lp ls 2 t 'MPI + OpenMP grid'
unset multiplot
