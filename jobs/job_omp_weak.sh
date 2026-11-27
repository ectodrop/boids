#!/bin/bash
#SBATCH --job-name=proj_slurm
#SBATCH --cpus-per-task=16
#SBATCH --ntasks-per-node=1
#SBATCH --nodes=1
#SBATCH --time=4:00:00
#SBATCH --output=output_%j.log
cd $SLURM_SUBMIT_DIR

module load TeachEnv/2022a gnu-parallel gcc/13.2.0 gnuplot openmpi/4.1.4


num_boids=0
for i in $(seq 1 16)
do
num_boids=$((num_boids+3000))
echo $i $(OMP_NUM_THREADS=$i ./bin/main_omp_grid $num_boids)
done

