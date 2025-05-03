#!/bin/bash -l
#SBATCH --job-name=pthread                     # a short name for your job
#SBATCH --output=slurm-%j.out                  # %j expands to the jobID
#SBATCH --time=5:00:00                         # HH:MM:SS, here 1 hour
#SBATCH --mem-per-cpu=1536M                    # 1.5 GB per core
#SBATCH --constraint=moles            	       # only run on “mole” nodes
#SBATCH --nodes=1                              # single node

# Load the GNU compiler toolchain (includes gcc and pthreads support)
module load foss/2022a

# Go to the directory where this script lives
cd "${SLURM_SUBMIT_DIR}"

# Compile the pthreads version
gcc -Wall -O2 -pthread pthread.c -o pthread

# ensure it really is executable
chmod +x pthread

# $1 = size spec (e.g. 60M), $2 = thread count
# create a temp file with the first $1 bytes of the dump
dumpfile="dump_${1}.txt"
head -c "$1" ~dan/625/wiki_dump.txt > "$dumpfile"

# run
./pthread "$dumpfile" "$2"

# cleanup
rm -f "$dumpfile"
