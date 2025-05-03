#!/bin/bash -l
#SBATCH --job-name=mpi                         # name of the job
#SBATCH --output=slurm-%j.out                  # %j expands to the jobID
#SBATCH --time=5:00:00                         # HH:MM:SS, here 1 hour
#SBATCH --mem-per-cpu=1536M                    # 1.5 GB per core
#SBATCH --constraint=moles            	       # only run on “mole” nodes
#SBATCH --nodes=1                              # single node

# compatible C compiler and MPI implementation available in the environment
module load foss/2022a OpenMPI/4.1.4-GCC-11.3.0

# Go to the directory where this script lives
cd "${SLURM_SUBMIT_DIR}"

# compile the mpi version
mpicc -Wall -O2 MPI.c -o mpi

# ensure it really is executable
chmod +x mpi

# slice input
# $1 = size spec (e.g. 60M)
dumpfile="dump_${1}.txt"
head -c "$1" ~dan/625/wiki_dump.txt > "$dumpfile"

# launch MPI
# SLURM_NTASKS == number of ranks
mpirun -np "$SLURM_NTASKS" ./mpi "$dumpfile"

# cleanup
rm -f "$dumpfile"
