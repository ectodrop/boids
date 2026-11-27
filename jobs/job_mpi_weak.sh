#!/bin/bash
#SBATCH --job-name=proj_slurm
#SBATCH --cpus-per-task=16
#SBATCH --ntasks-per-node=1
#SBATCH --nodes=4
#SBATCH --time=1:00:00
#SBATCH --output=output_%j.log
cd $SLURM_SUBMIT_DIR

module load TeachEnv/2022a gnu-parallel gcc/13.2.0 gnuplot openmpi/4.1.4


HOSTS=$(scontrol show hostnames $SLURM_NODELIST | tr '\n' ,)
echo $HOSTS

num_boids=0
for i in $(seq 1 16)
do
num_boids=$((num_boids+2500))
	echo $i $(mpirun --quiet -np 4 --host $HOSTS -x OMP_NUM_THREADS=$i ./bin/main_omp_mpi_grid $num_boids)
done

# parallel --workdir $SLURM_SUBMIT_DIR -j $SLURM_NTASKS_PER_NODE -S $HOSTS \
#	'echo {} $(OMP_NUM_THREADS={} ./gameoflife/gameoflife 1000 1000 1000 0.183 1 2>&1 > /dev/null) >> ./q5.dat' ::: {1..16}


