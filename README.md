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
## Pipeline overview
DPCstruct takes as input a set of local structural alignments generated with the software Foldseek.

(see [How to generate all-vs-all](#generation-of-all-vs-all-alignments))

The output of the pipeline is a 'tsv' file containing DPCstruct classification: protIndex, dom-start, dom-end, metaclusterID.

The pipeline consists of the following modules, each with a dedicated help section explaining the different options available.

```
dpcstruct <module> -h

Step 1) prefilters: applies a series of filters to the alignments found with Foldseek.
Step 2) primarycluster: clusters local alignments per query sequence.
Step 3) secondarycluster: clusters the primary clusters.
Step 4) traceback: traces back the alignments to the original sequences.
Step 5) postfilters: removes redundancies from the secondary clusters.

```

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



## Publications
[Barone, F., Laio, A., Punta, M., Cozzini, S., Ansuini, A., & Cazzaniga, A. (2024). Unsupervised domain classification of AlphaFold2-predicted protein structures. bioRxiv, 2024-08.](https://doi.org/10.1101/2024.08.21.608992)

