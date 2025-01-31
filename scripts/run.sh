#!/bin/bash

set -e
# Define the cleanup function
cleanup() {
    echo "Performing cleanup..."
    rm -rf ./example/alns_filtered  
    rm -rf ./example/primaryclusters  
    rm -rf ./example/secondaryclusters  
    rm -rf ./example/output  
    echo "Cleanup complete."
}
trap cleanup EXIT

# This file contains a step-by-step guide on how to run the DPCstruct pipeline.
# For this purpose we will use a small dataset of aroun 1250 proteins as an example.
# To keep the repository light we will assume the proteins are already compressed into a Foldseek database using the following commands:

## 0. Preprocessing
# protLookup="./example/proteins.tsv"
# change pdbs filenames to indexes (this step can be performed after all vs. all)
# while IFS=' ' read -r index file; do mv "${file}.pdb" "${index}"; done < ${protLookup}

### create db
# foldseek createdb example_pdbs/ example_db/example_db

## 1. Search all vs. all
# queryDB="./example/database/example_db"
# targetDB="./example/database/example_db"
# alns="./example/alns/alns"
# alnsConverted="./example/alns/alns.tsv"
# mkdir -p "$(dirname "$alns")"
# tmpDir="./tmp"
# mkdir ${tmpDir}

### foldseek search (--max-seqs=100 000 for bigger datasets)
# foldseek search ${queryDB} ${targetDB} ${alns} ${tmpDir}  -s 7.5 --max-seqs 1000 -e 0.001 -a --threads ${SLURM_CPUS_PER_TASK} 
# foldseek convertalis ${queryDB} ${targetDB} ${alns} ${alnsConverted} --format-mode 4 --format-output query,target,qstart,qend,tstart,tend,qlen,tlen,alnlen,pident,evalue,bits,alntmscore,lddt

# We start the pipeline from the prefilter step:
protLookup="./example/proteins.tsv"
alnsConverted="./example/alns/alns.tsv"
unzip ./example/alns/alns.zip -d ./example/alns
# Check if the file was successfully extracted
if [[ -f "$alnsConverted" ]]; then
    echo "File $alnsConverted successfully extracted."
else
    echo "Error: File $alnsConverted does not exist or failed to extract."
    exit 1
fi

## 2. Prefilters
plddtFiles="./example/database/plddts/plddts_compressed"
alnsFilteredDir="./example/alns_filtered"
alnsFiltered="${alnsFilteredDir}/alns_filtered.tsv"
mkdir -p "${alnsFilteredDir}"

./build/bin/prefilters ${alnsConverted} -m ${protLookup} -p ${plddtFiles} -o ${alnsFiltered}

## 3. Primary clustering
pcsDir="./example/primaryclusters"
pcsFile="${pcsDir}/pcs.bin"
mkdir -p "${pcsDir}"

./build/bin/primarycluster -i ${alnsFiltered} -o ${pcsFile} -t 4

## 4. Secondary clustering
scDistDir="./example/secondaryclusters/distance"
scLabelsDir="./example/secondaryclusters/classification"
scDistFile="${scDistDir}/distance_matrix.bin"
scLabelsFile="${scLabelsDir}/sc_classification.txt"
mkdir -p "${scDistDir}"
mkdir -p "${scLabelsDir}"

./build/bin/secondarycluster distance -i ${pcsFile} -j ${pcsFile} -o ${scDistFile} -p 1 -c 4 
./build/bin/secondarycluster classify -i ${scDistFile} -o ${scLabelsFile}

## 5. Traceback
tracebackDir="./example/output/binary"
mkdir -p "${tracebackDir}"
./build/bin/traceback ${pcsFile} -l ${scLabelsFile} -o ${tracebackDir}

## 6. Postfilter
outputDir="./example/output"
mkdir -p "./example/output"
./build/bin/postfilters -i ${tracebackDir}/sequence-labels_1.bin -o ${outputDir}/sequence-labels_1.txt 
