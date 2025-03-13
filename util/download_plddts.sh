#!/bin/bash
# ------------------------------------------------------------------------------
# Description:
#   Automates the downloading of pLDDT files in manageable batches and compresses
#   the data into a binary format with an associated index for efficient access.
#
# Functionality:
#   - Splits a large protein mapping file into smaller, batch-oriented files.
#   - Downloads the corresponding pLDDT files for each batch.
#   - Parses the downloaded data to generate:
#       • A binary file that stores the pLDDT values.
#       • An index file to facilitate reading and interpretation of the binary data.
#
# Usage:
#   Simply run the script in your shell:
#       ./download_plddts.sh
#
# Requirements:
#   - Ensure access tu gsutil for downloading the pLDDT files.
#   - The pLDDT JSON parser executable (plddt_json_parser.x) should be available. For compilation, 
#       • run: g++ -O3 -o plddt_json_parser.x plddt_json_parser.cc
#   - Confirm that the protein mapping file is correctly configured and accessible.
#
# Note:
#   Modify batch sizes and other script parameters as needed to suit your specific setup.
# ------------------------------------------------------------------------------

# Configuration
BATCH_SIZE=300
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <protein_mapping_file> <plddts_directory>"
    exit 1
fi

PROTEIN_LIST="$1"
PLDDTS_DIR="$2"

# check if executable
if [ ! -x "./plddt_json_parser" ]; then
    echo "Error: JSON parser executable not found: $JSON_PARSER_EXE"
    echo "Please compile the JSON parser executable using: g++ -O3 -o plddt_json_parser.x plddt_json_parser.cc"
    exit 1
fi

# check if gsutil is installed
if ! command -v gsutil &> /dev/null; then
    echo "Error: gsutil is not installed."
    exit 1
fi

if [ ! -f "$PROTEIN_LIST" ]; then
    echo "Error: Protein mapping file not found: $PROTEIN_LIST"
    exit 1
fi

# Create directories if they don't exist
mkdir -p "$PLDDTS_DIR"

# check if the directory is not empty
if [ "$(ls -A "$PLDDTS_DIR")" ]; then
    echo "Error: Output directory must be empty."
    exit 1
fi

# Split the protein mapping PROTEIN_LIST into smaller batch files with numeric suffixes starting at 001
temp_batches=$(mktemp -d ./tmp.XXXXXXXX)
PLDDTS_DOWN_DIR="$temp_batches/plddts_files"
mkdir -p "$PLDDTS_DOWN_DIR"
split -d --numeric-suffixes=1 -a 3 -l "$BATCH_SIZE" "$PROTEIN_LIST" "${temp_batches}/batch_"

# Process each batch file
for batch_path in "${temp_batches}"/batch_*; do
    # grab the batch file name
    batch_filename=$(basename "$batch_path")
    # extract suffix
    batch_suffix="${batch_filename#batch_}"

    echo "Processing batch file: $batch_path"
    # Generate the URLs from the batch file
    urls=$(awk '{print "gs://public-datasets-deepmind-alphafold-v4/AF-"$2"-F1-confidence_v4.json"}' "$batch_path")
    
    # Download the batch of files concurrently using gsutil
    # The -m flag enables parallel multi-threading
    echo "$urls" | gsutil -m cp -I "$PLDDTS_DOWN_DIR" 2> /dev/null
    
    # Run the parser; here we use the batch file name (or part of it) as a unique suffix.
    ./plddt_json_parser "$PLDDTS_DOWN_DIR" "${batch_suffix}" "$PLDDTS_DIR"
    
    # Clean up the download directory for the next batch
    rm -rf "${PLDDTS_DOWN_DIR:?}/"*
done

# Remove temporary batch files
rm -rf "$temp_batches"

echo "Download and processing complete!"
