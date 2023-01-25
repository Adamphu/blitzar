#!/bin/bash
EXEC_CMD="bazel run -c opt //benchmark/multi_commitment:benchmark -- "
EXEC_CMD_CALLGRIND="bazel run -c opt --define callgrind=true //benchmark/multi_commitment:benchmark_callgrind -- "

############################################

results_dir="benchmark/multi_commitment/.results"

read_field() {
    # $1 --> column
    # $2 --> file 
    IFS=':'; read -a line_arr <<< $(awk '{if(NR=='${1}') print $0}' ${2}); 

    echo ${line_arr[1]}
}

run() {
    word_sizes=$1
    execs=$2
    backend=$3
    use_callgrind=$4
    num_samples=$5
    verbose=$6

    # Run all the benchmarks
    for ws in ${word_sizes[@]}; do
        for ex in ${execs[@]}; do
            c=$(echo $ex | cut -f1 -d-)
            r=$(echo $ex | cut -f2 -d-)

            base_dir="${results_dir}/${c}_commits/${ws}_wordsize"

            mkdir -p ${base_dir}

            curr_benchmark_file="${base_dir}/${backend}_${c}_commits_${r}_length_${ws}_wordsize_${num_samples}_samples"

            if [ "${use_callgrind}" == "1" ]; then
                ${EXEC_CMD_CALLGRIND} ${backend} ${c} ${r} ${ws} ${verbose} ${num_samples}
                mv -f *.svg ${curr_benchmark_file}.svg
            else
                ${EXEC_CMD} ${backend} ${c} ${r} ${ws} ${verbose} ${num_samples} > ${curr_benchmark_file}.out
            fi
        done
    done
}

spreadsheet() {
    word_sizes=$1
    execs=$2
    backend=$3

    c0="Commitments\t"
    c1="Commitment Length\t"
    c2="Word Size\t"
    c3="Table Size (KB)\t"
    c4="Total Exponentiations\t"
    c5="Duration (s)\t"
    c6="Std. Deviation (s)\t"
    c7="Throughput (Exponentiations / s)\t"

    base_spreadsheet="${results_dir}"
    spreadsheet_file="${results_dir}/summary_${backend}.txt"

    cat /dev/null > ${spreadsheet_file}
    echo -e $c0$c1$c2$c3$c4$c5$c6$c7$c8$c9$c10 >> ${spreadsheet_file}
    
    prev="";

    # Extract the duration time of each benchmark
    for ws in ${word_sizes[@]}; do
        for ex in ${execs[@]}; do
            c=$(echo $ex | cut -f1 -d-)
            r=$(echo $ex | cut -f2 -d-)

            base_dir="${results_dir}/${c}_commits/${ws}_wordsize"

            mkdir -p ${base_dir}

            curr_benchmark_file="${base_dir}/${backend}_${c}_commits_${r}_length_${ws}_wordsize_${num_samples}_samples.out"

            table_size=$(read_field 7 $curr_benchmark_file);
            num_exps=$(read_field 8 $curr_benchmark_file);
            duration=$(read_field 10 $curr_benchmark_file);
            std_deviation=$(read_field 11 $curr_benchmark_file);
            throughput=$(read_field 12 $curr_benchmark_file);

            if [ "$prev" != "$c" ]; then
                echo -e "-- \t-- \t-- \t-- \t-- \t-- \t-- \t-- " >> ${spreadsheet_file}
            fi

            echo -e "$c \t $r \t $ws \t $table_size \t $num_exps \t $duration \t $std_deviation \t $throughput" >> ${spreadsheet_file}

            prev=$c;
        done
    done
}

backend="$1"
RUN_BENCHMARK="$2"
RUN_BENCHMARK_WITH_CALLGRIND="$3"

############################################
############################################
# RUN BENCHMARK
############################################
############################################
if [ "${RUN_BENCHMARK}" == "1" ]; then
    verbose=1
    num_samples=3
    word_sizes=("0" "1" "4" "8" "32")

    ############################################
    ############################################
    # RUN STANDARD EXECUTION
    ############################################
    ############################################
    execs=()

    ############################################
    # Number of Commitments - Small
    ############################################

    NC="1"
    execs+=(
        "${NC}-1" "${NC}-10" "${NC}-100" "${NC}-1000"
        "${NC}-10000" "${NC}-100000"
    )

    ############################################
    # Number of Commitments - Medium 1
    ############################################

    NC="10"
    execs+=(
        "${NC}-1" "${NC}-10" "${NC}-100"
        "${NC}-1000" "${NC}-10000"
    )

    ############################################
    # Number of Commitments - Medium 2
    ############################################

    NC="100"
    execs+=(
        "${NC}-1" "${NC}-10" "${NC}-100"
        "${NC}-1000"
    )

    ############################################
    # Number of Commitments - Large
    ############################################

    NC="1000"
    execs+=(
        "${NC}-1" "${NC}-10" "${NC}-100"
    )


    run $word_sizes $execs $backend 0 $num_samples $verbose # run specified function

    spreadsheet $word_sizes $execs $backend # run specified function
fi

############################################
############################################
# RUN CALLGRIND BENCHMARK
############################################
############################################
if [ "${RUN_BENCHMARK_WITH_CALLGRIND}" == "1" ]; then
    execs=()
    
    verbose=0
    num_samples=1
    word_sizes=("0" "32")

    ############################################
    # Number of Commitments - Small
    ############################################

    NC="1"
    execs+=("1-10000")

    ############################################
    # Number of Commitments - Medium 2
    ############################################

    NC="100"
    execs+=("${NC}-100")

    run $word_sizes $execs $backend 1 $num_samples $verbose # run specified function
fi
