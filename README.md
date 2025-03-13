[![Platform](https://img.shields.io/badge/platform-Linux-blue.svg)](https://www.linux.org/)

<img align="right" src="logo.png" alt="Logo" width="150"/>
<div id="toc">
    <ul style="list-style: none;">
    <summary>
      <h1>DPCStruct</h1>
    </summary>
  </ul>
</div>

Unsupervised clustering algorithm for identifying and classifying protein domains based on structural similarity.

DPCstruct uses the [moodycamel::ConcurrentQueue library](https://github.com/cameron314/concurrentqueue) freely available provided citation (Simplified BSD license). 


### Prerequisites
- GNU compiler (version 13.0 or higher)
- CMake build system
- [Foldseek](https://github.com/steineggerlab/foldseek) (version 9 or higher)


## Installation
To install DPCstruct, clone the repository and run the following commands:
```
git clone https://github.com/RitAreaSciencePark/DPCstruct.git
cd DPCstruct
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..
make -j 4
make install
``````
# DPCstruct Pipeline Overview

DPCstruct is designed to process local structural alignments produced by Foldseek, enabling comprehensive identification and classification of protein domains based on structural similarity. This guide explains each module in the pipeline and how it contributes to the overall process.

## Generating Compatible Alignments

To ensure compatibility with DPCstruct, please verify that your local alignments have the correct format.
For detailed instructions on how to generate DPCstruct-compatible all-vs-all alignments, please refer to the [How to generate all-vs-all alignments](#generation-of-all-vs-all-alignments) section.

## Pipeline Workflow

The pipeline is organized into five main modules. Each module has dedicated commands and help documentation for a smooth execution:

```
dpcstruct <module> -h

Modules:

Step 1) prefilters: applies a series of filters to the alignments found with Foldseek.
Step 2) primarycluster: clusters local alignments per query sequence.
Step 3) secondarycluster: clusters the primary clusters.
Step 4) traceback: traces back the alignments to the original sequences.
Step 5) postfilters: removes redundancies from the secondary clusters.
```

## Output Format

The final output is a TSV file which includes the following columns:
- **protIndex:** Protein identifier.
- **dom-start:** Starting index of the domain.
- **dom-end:** Ending index of the domain.
- **metaclusterID:** Identifier for the assigned structural metacluster.

## Usage example
The folder `example` contains a toy example to test the pipeline.
To run the example you can simply execute `run_example.sh`.

| File | Description |
| ---  | --- |
| proteins.tsv | list of proteins and their corresponding index |
| alns.zip |  set of all-vs-all local alignments from where to start |  

In the following section we provide a quick guide on how to generate local alignments as input to DPCstruct.

## Generation of all-vs-all alignments

Given a folder containing a set of pdbs [*.pdb], the standard procedure to generate the local alignments is the following.
For more information check [Foldseek](https://github.com/steineggerlab/foldseek) repo.

```
# generate protein index table
ls ${pbsDir}/*.pdb | awk '{print NR,$1}' > ${fsdbDir}/${proteinLookup}

# change pdbs filenames to indexes (this step can be performed after all vs. all)
while IFS=' ' read -r index file; do mv "${file}.pdb" "${index}"; done < ${fsdbDir}/${proteinLookup}

# create database
foldseek createdb ${pdbsDir}$/ ${fsdbDir}/pdbs_db

# run local alns (adjust e-value as required)
foldseek search ${queryDB} ${targetDB} ${alns} ${tmpDir} -a --threads ${SLURM_CPUS_PER_TASK} 
foldseek convertalis ${queryDB} ${targetDB} ${alns} ${alnsConverted} --format-mode 4 --format-output query,target,qstart,qend,tstart,tend,qlen,tlen,alnlen,pident,evalue,bits,alntmscore,lddt
```
## How to download Alphafold pLDDTs files

If you are using protein structure predictions from AlphaFold, you will need to download the per-residue pLDDT values for each protein in your dataset.
In the `./build/util/` folder, we provide an auxiliary script `download_plddts.sh` to download this information from the AlphaFold Database hosted on [Google Cloud Public Datasets](https://github.com/google-deepmind/alphafold/blob/main/afdb/README.md) and store it in a binary format compatible with the prefiltering module.

## Publications
[Barone, F., Laio, A., Punta, M., Cozzini, S., Ansuini, A., & Cazzaniga, A. (2024). Unsupervised domain classification of AlphaFold2-predicted protein structures. bioRxiv, 2024-08.](https://doi.org/10.1101/2024.08.21.608992)

