#!/bin/bash

# if any command in this script returns a non-zero (i.e. “error”) exit status, immediately stop the script
set -e

# ensures the logs directory exists
mkdir -p logs

# list of sizes (bytes)
sizes=(60M 120M 240M 720M 1440M 1700M)
# list of threads
cores=(1 2 4 8 16 20)

last_jid=""

for sz in "${sizes[@]}"; do
  for th in "${cores[@]}"; do
    # generate a distinctive job name
    jobname="pthread_${sz}_${th}"

    sbatch_cmd=( sbatch
      --job-name="$jobname"
      --cpus-per-task="$th"
      --output="logs/slurm_${sz}_${th}.out"
    )

    # if this isn’t the first job, add the dependency
    if [[ -n $last_jid ]]; then
      sbatch_cmd+=( --dependency=afterok:"$last_jid" )
    fi

    sbatch_cmd+=( pthread_script.sh "$sz" "$th" )

    # launch and capture the new job’s ID
    jid=$("${sbatch_cmd[@]}" | awk '{print $4}')
    last_jid=$jid

  done
done
