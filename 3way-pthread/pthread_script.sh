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

# params
size=$1        # e.g. "60M"
threads=$2     # e.g. "4"

# prepare output files
out_csv="analysis/pthread_${size}_${threads}_runs.csv"
summary_txt="analysis/pthread_${size}_${threads}_summary.txt"

# add a header to the output csv
echo "run,task_clock_ms,wall_s,cpu_pct,max_rss_kb" > "$out_csv"

# loop for N=10 trials so that we can get an average and standard deviation for each combination of input size and core count
for run in $(seq 1 10); do
  # perf stat
  perf_out="analysis/perf_run${run}.csv"
  perf stat -x, -e task-clock \
    -o "$perf_out" -- ./pthread "$dumpfile" "$threads"
  # grab the first field of the first non-comment line
  task_clock_ms=$(awk -F, '/^[0-9]/{print $1; exit}' "$perf_out")

  # /usr/bin/time for wall time, cpu%, maxrss (i.e. memory utilization)
  time_out="analysis/time_run${run}.txt"
  /usr/bin/time -f "WALL=%e\nCPU_PCT=%P\nMAXRSS=%M" \
    ./pthread "$dumpfile" "$threads" 2> "$time_out"
  # pull out values
  wall_s=$(awk -F= '/^WALL=/ {print $2}' "$time_out")
  cpu_pct=$(awk -F= '/^CPU_PCT=/ {print $2}' "$time_out" | tr -d '%')
  max_rss_kb=$(awk -F= '/^MAXRSS=/ {print $2}' "$time_out")

  # append to the master csv file
  echo "$run,$task_clock_ms,$wall_s,$cpu_pct,$max_rss_kb" \
    >> "$out_csv"
done

# remove the dump files
rm -f "$dumpfile" analysis/perf_run*.csv analysis/time_run*.txt

# compute statistics (i.e. the mean and standard deviation) for each column using awk
awk -F, '
  NR>1 {
    tc[NR-1]=$2; # task clock values
    ws[NR-1]=$3; # wall time values
    cp[NR-1]=$4; # CPU% values
    mr[NR-1]=$5; # max RSS values
    sum_tc+=$2; # running total of task clock
    sum_ws+=$3; # running total of wall time
    sum_cp+=$4; # running total of CPU%
    sum_mr+=$5; # running total of max RSS
  }


  END {
    n=NR-1 # number of data rows

    # computing the means
    mean_tc = sum_tc/n
    mean_ws = sum_ws/n
    mean_cp = sum_cp/n
    mean_mr = sum_mr/n

    # accumulate the squared deviations for each metric
    for(i = 1; i <= n; i++)
    {
      sd_tc += (tc[i]-mean_tc)^2
      sd_ws += (ws[i]-mean_ws)^2
      sd_cp += (cp[i]-mean_cp)^2
      sd_mr += (mr[i]-mean_mr)^2
    }

    # divide by n and take the square root to get the standard deviation
    sd_tc = sqrt(sd_tc/n)
    sd_ws = sqrt(sd_ws/n)
    sd_cp = sqrt(sd_cp/n)
    sd_mr = sqrt(sd_mr/n)

    # printing a table of results into the statistics summary file
    printf("Metric      Mean        StdDev\n") > "'"$summary_txt"'"
    printf("task_clock_ms  %.2f ms    %.2f ms\n", mean_tc, sd_tc) >> "'"$summary_txt"'"
    printf("wall_s         %.3f s     %.3f s\n",  mean_ws, sd_ws) >> "'"$summary_txt"'"
    printf("cpu_pct        %.1f%%       %.1f%%\n",  mean_cp, sd_cp) >> "'"$summary_txt"'"
    printf("max_rss_kb     %.0f KB     %.0f KB\n",   mean_mr, sd_mr) >> "'"$summary_txt"'"
  }
' "$out_csv"
