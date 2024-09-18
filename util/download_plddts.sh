#!/bin/bash
#SBATCH --job-name=down_plddts
#SBATCH -p EPYC
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=128
#SBATCH --mem=490G
#SBATCH --time=24:00:00
#SBATCH -o slog-%x.%j.out
#SBATCH -e slog-%x.%j.error
#SBATCH -A lade

cd "$SLURM_SUBMIT_DIR" || exit

BATCH_SIZE=300
METABATCH_SIZE=50
LIST="../example/protein_mapping.tsv"
PLDDTS_DOWN_DIR="../example/database/plddts/plddts_files"
PLDDTS_COMP_DIR="../example/database/plddts/plddts_compressed"

# Create the directory to store the downloaded files and the compressed files
mkdir -p $PLDDTS_DOWN_DIR
mkdir -p $PLDDTS_COMP_DIR

# remove files from previous runs
rm -rf "${PLDDTS_DOWN_DIR:?}/"*

# Split the file LIST into batches
BATCH_COUNT=$(( ($(wc -l $LIST | awk '{print $1}') + BATCH_SIZE - 1) / BATCH_SIZE ))
METABATCH_COUNT=1

# Download files in batches
T_START=$(date +%s)

for ((i = 1; i <= $BATCH_COUNT; i++)); do
    echo "Processing batch $i"
    # elapsed time
    START=$(( (i - 1) * BATCH_SIZE + 1 ))
    END=$(( i * BATCH_SIZE ))
    BATCH_FILES=$(sed -n "${START},${END}p" $LIST | awk '{print "gs://public-datasets-deepmind-alphafold-v4/AF-"$2"-F1-confidence_v4.json"}')

    # Download the batch of files
    gsutil -m cp $BATCH_FILES "$PLDDTS_DOWN_DIR" 2> /dev/null &
    
    if [ $(( i % METABATCH_SIZE)) -eq 0 ]; then
        wait
        # colapse files
        SUFFIX=$METABATCH_COUNT
        ./plddt_json_parser.x "$PLDDTS_DOWN_DIR" "$SUFFIX" "$PLDDTS_COMP_DIR"
        METABATCH_COUNT=$(( METABATCH_COUNT + 1 ))
        rm -rf ${PLDDTS_DOWN_DIR:?}/*
        T_END=$(date +%s)
        echo "elapsed time: " $(( T_END - T_START )) "s"
        T_START=$(date +%s)
    fi
done

wait
SUFFIX=$METABATCH_COUNT
./plddt_json_parser.x "$PLDDTS_DOWN_DIR" "$SUFFIX" "$PLDDTS_COMP_DIR"
METABATCH_COUNT=$(( METABATCH_COUNT + 1 ))
rm -rf "${PLDDTS_DOWN_DIR:?}"
T_END=$(date +%s)
echo "elapsed time: " $(( T_END - T_START )) "s"
echo "Download done!"

