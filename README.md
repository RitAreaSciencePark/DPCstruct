# DPCstruct
Unsupervised clustering algorithm for identifying and classifying protein domains based on structural similarity.

## Installation
To install DPCstruct, clone the repository and run the following command in the root directory:
```
mkdir build
cd build
cmake ..
make -j 4
```
## Usage
DPCstruct is a pipeline consisting of a series of sequentail modules that are used to identify and classify protein domains based on structural similarity. The pipeline consists of the following steps:
```
prefilters: applies a series of filters to the alignments found with Foldseek.
primarycluster: clusters the alignments per query sequence.
secondarycluster: clusters the primary clusters.
postfilters: removes redundancies from the secondary clusters.
traceback: traces back the alignments to the original sequences.
```
We've also included a script that runs the entire pipeline on an example dataset.

## Publications
[Barone, F., Laio, A., Punta, M., Cozzini, S., Ansuini, A., & Cazzaniga, A. (2024). Unsupervised domain classification of AlphaFold2-predicted protein structures. bioRxiv, 2024-08.](https://doi.org/10.1101/2024.08.21.608992)

