set -euo pipefail

MODE="${MODE:-pthread}"                       # pthread or mpi
BASEFILE="${BASEFILE:-~dan/625/wiki_dump.txt}"
OUTDIR="${OUTDIR:-bench/raw_${MODE}}"
RUNS="${RUNS:-3}"

# Sizes (bytes) and cores/ranks grid --------------------------------------
SIZES=(60M 120M 240M full)
if [[ $MODE == "pthread" ]]; then
  CORES=(1 2 4 8 16 32 40)
  BINARY="${BINARY:-./hw4_pthread}"          # argv: <file> <threads>
else
  CORES=(1 2 4 8 16 20 40)                   # 40 = 2 nodes × 20
  BINARY="${BINARY:-./hw4_mpi}"              # used under srun -n <ranks>
fi

mkdir -p "${OUTDIR}"

# Helper: produce temp slice ----------------------------------------------
make_slice () {
  local bytes=$1 dest=$2
  if [[ $bytes == "full" ]]; then echo "${BASEFILE}"
  else
    local file=${dest}/slice_${bytes}.txt
    head --bytes=${bytes} "${BASEFILE}" > "${file}"
    echo "${file}"
  fi
}

# -------------------------------------------------------------------------
echo "MODE=${MODE}"
echo "Results ==> ${OUTDIR}"
echo

for size in "${SIZES[@]}"; do
  for cores in "${CORES[@]}"; do
    for ((r=1; r<=RUNS; r++)); do

      outfile="${OUTDIR}/${MODE}_${size}_${cores}c_r${r}.csv"
      slice=$(make_slice "${size}" "${OUTDIR}")

      echo "→ ${MODE}  size=${size}  cores=${cores}  run=${r}"

      # Build perf + time command -----------------------------------------
      perf_metrics="task-clock,cycles,instructions,cache-misses,branch-misses"
      if [[ $MODE == "pthread" ]]; then
        cmd="/usr/bin/time -f \
'size,cores,rep,wall_s,user_s,sys_s,rss_kB,cs_vol,cs_invol'\
 -o ${outfile} -a \
 perf stat -e ${perf_metrics} -x , -- \
 ${BINARY} ${slice} ${cores}"
      else
        cmd="/usr/bin/time -f \
'size,cores,rep,wall_s,user_s,sys_s,rss_kB,cs_vol,cs_invol'\
 -o ${outfile} -a \
 perf stat -e ${perf_metrics} -x , -- \
 srun --mpi=pmi2 -n ${cores} ${BINARY} ${slice}"
      fi

      # Optional: lightweight CPU util sampling with kstat ----------------
      # PID recorded after fork so we can 'kstat -p <PID> 1 > tmp &' later
      eval ${cmd} 2> >(grep -E 'task-clock|cycles|instructions|cache' \
                       | sed "s/^/${size},${cores},${r},/g" >> ${outfile})
    done
  done
done