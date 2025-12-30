#!/bin/bash
set -e

############################################################################
# File Name  : run_lab4.sh
# Author     : Yiqing Huang (Modified)
# Description: Extract timing info from findpng3 for different T and M values
############################################################################

PROG="./findpng3"
T="1 10 20"
M="1 10 20 30 40 50 100"
NN=5

exec_producer () 
{
    if [ $# != 4 ]; then
        echo "Usage: $0 <exec_name> -t T -m M <num_runs>" 
        exit 1
    fi

    PROGRAM=$1
    NUM_T=$2
    NUM_M=$3
    X_TIMES=$4
    SEED_URL="http://ece252-1.uwaterloo.ca/lab4"

    O_FILE="T${NUM_T}_M${NUM_M}.dat"
    : > "$O_FILE"  # truncate the file if it exists

    for ((xx=1; xx<=X_TIMES; xx++)); do
        cmd="${PROGRAM} -t ${NUM_T} -m ${NUM_M} ${SEED_URL}"
        str=$(eval "$cmd" | tail -1 | awk -F' ' '{print $4}')
        echo "$str" >> "$O_FILE"
    done
}

gen_data ()
{
    for t in $T; do
        for m in $M; do
            exec_producer "$PROG" "$t" "$m" "$NN"
        done
    done
}

gen_stat_per_pair ()
{
    FNAME_DATA="$1"
    TB1="$2"
    TB2="$3"

    awk -v tb1="$TB1" -v tb2="$TB2" '
    {
        sum += $1
        sumsq += $1 * $1
        n++
    }
    END {
        mean = sum / n
        stddev = sqrt(n / (n - 1) * (sumsq / n - mean * mean))
        printf("%.6f\n", mean) >> tb1
        printf("%.6f\n", stddev) >> tb2
    }' "$FNAME_DATA"
}

gen_table ()
{
    TB1="tb1_$$.txt"
    TB2="tb2_$$.txt"
    echo 'T,M,Time' > "$TB1"
    echo 'T,M,Time' > "$TB2"

    for t in $T; do
        for m in $M; do
            echo "Processing T=$t M=$m"
            echo -n "${t},${m}," >> "$TB1"
            echo -n "${t},${m}," >> "$TB2"
            FNAME_DATA="T${t}_M${m}.dat"
            gen_stat_per_pair "$FNAME_DATA" "$TB1" "$TB2"
        done
    done
}

gen_data
gen_table