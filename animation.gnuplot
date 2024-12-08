set terminal gif animate delay 4
set output ARG3
set xrange [0:800]
set yrange [0:800]
N = ARG4
records = ARG2/(2*8*N)
unset key
set style line 1 pt 7 ps 0.1 lc rgb 'blue'
do for [i=0:records-1] {
  plot ARG1 binary format='%double%double' every ::(i*N)::((i+1)*N) using 1:2 with points ls 1
}
