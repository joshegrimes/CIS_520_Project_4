#!/bin/bash -l
#SBATCH --job-name=pthread                     # name of the job
#SBATCH --output=slurm-%j.out                  # %j expands to the jobID
#SBATCH --time=5:00:00                         # HH:MM:SS, here 1 hour
#SBATCH --mem-per-cpu=1536M                    # 1.5 GB per core
#SBATCH --constraint=moles            	       # only run on “mole” nodes
#SBATCH --nodes=1                              # single node

# Load the GNU compiler toolchain (includes gcc and pthreads support)
module load foss/2022a

# Go to the directory where this script lives
cd "${SLURM_SUBMIT_DIR}"

# compile the pthreads version
gcc -Wall -O2 -pthread pthread.c -o pthread

# ensure it really is executable
chmod +x pthread

# $1 = size spec (e.g. 60M), $2 = thread count
# create a temp file with the first $1 bytes of the dump
dumpfile="dump_${1}.txt"
head -c "$1" ~dan/625/wiki_dump.txt > "$dumpfile"

# name the perf output file by size, thread-count
perf_out="analysis/perf_pthread_${1}_${2}.csv"

# run 10 repetitions of perf stat, CSV-style, measuring a few key events
perf stat -x, -r 10 \
  -e task-clock \
  -o "$perf_out" \
  -- ./pthread "$dumpfile" "$2"

# wrap the same binary once in /usr/bin/time -v to capture MaxRSS & “real” time
time_out="analysis/time_pthread_${1}_${2}.txt"
/usr/bin/time -f \
"WALL=%e\nCPU_PCT=%P\nMAXRSS=%M" \
./pthread "$dumpfile" "$2" 2> "$time_out"

# record elapsed seconds and max RSS (in KB) via sacct
sacct -j ${SLURM_JOB_ID}.batch \
  -n -P \
  --format=JobID,ElapsedRaw,MaxRSS \
  > analysis/sacct_pthread_${1}_${2}.csv

# cleanup
rm -f "$dumpfile"
