#!/usr/bin/env bash
BASE=~dan/625/wiki_dump.txt
SIZES=(60M 120M 240M full)
PROCS=(1 2 4 8 16 20 40)    # 40 ranks = 2 nodes × 20
REPS=3                      # fewer reps than threads grid to save time

make_slice () {
  local bytes=$1 dest=$2
  if [[ $bytes == "full" ]]; then echo "$BASE"
  else
    local file=$dest/slice_${bytes}.txt
    head --bytes=$bytes "$BASE" > "$file"; echo "$file"
  fi
}

mkdir -p raw_mpi
for sz in "${SIZES[@]}";    do
for pp in "${PROCS[@]}";    do
  for ((r=1;r<=REPS;r++));  do
    slice=$(make_slice "$sz" raw_mpi)
    out="raw_mpi/m${sz}_${pp}P_r${r}.csv"
    /usr/bin/time -f \
        "size,ranks,rep,wall_s,user_s,sys_s,rss_kB,ctxsw_vol,ctxsw_invol\n\
$sz,$pp,$r,%e,%U,%S,%M,%c,%w" \
        srun --mpi=pmi2 -n $pp ./hw4_mpi "$slice" 2> "$out" > /dev/null
  done
done; done