#!/bin/bash

# if any command in this script returns a non-zero (i.e. “error”) exit status, immediately stop the script
set -e

# ensures the logs and analysis directories exists
mkdir -p logs
mkdir -p analysis

# list of sizes (bytes)
sizes=(60M 120M 240M 720M 1440M 1700M)
# list of ranks to test
cores=(1 2 4 8 16 20)

last_jid=""

for sz in "${sizes[@]}"; do
  for rk in "${cores[@]}"; do
    # generate a distinctive job name
    jobname="mpi_${sz}_${rk}"

    sbatch_cmd=( sbatch
      --job-name="$jobname"
      --ntasks="$rk"
      --cpus-per-task=1
      --output="logs/slurm_${sz}_${rk}.out"
    )

    # if this isn’t the first job, add the dependency
    if [[ -n $last_jid ]]; then
      sbatch_cmd+=( --dependency=afterok:"$last_jid" )
    fi

    sbatch_cmd+=( mpi_script.sh "$sz" )

    # launch and capture the new job’s ID
    jid=$("${sbatch_cmd[@]}" | awk '{print $4}')
    last_jid=$jid

  done
done
