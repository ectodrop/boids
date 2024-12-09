
if (!exists("question")) question='default'
if (!exists("serial")) serial=0.5

filename=sprintf('%s.dat',question)

set term png
set output sprintf('%s_plot.png',question)

set multiplot layout 2,1
set key left top

first_value = NaN
stats filename using 2 nooutput
first_value = STATS_max

set xrange [0<*:]
set yrange [0<*:]




set xlabel 'omp threads'
set ylabel 'runtime (s)'
set title sprintf('%s times', question)
plot filename w l title question

set xlabel 'omp threads'
set ylabel 'speedup'
set title sprintf('%s speedup', question)
plot filename using 1:(first_value/$2) title sprintf('%s speedup',question) w l, filename using 1:(1/(serial+(1-serial)/$1)) title sprintf('amdahls curve(s=%.2f)',serial) w l

unset multiplot

