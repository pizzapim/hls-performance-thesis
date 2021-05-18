#!/bin/bash
set -x

./construct experiment_data/dna.10MB experiment_data/dna.10MB.fm
./construct experiment_data/proteins.10MB experiment_data/proteins.10MB.fm
./construct experiment_data/dblp.xml.10MB experiment_data/dblp.xml.10MB.fm

./construct experiment_data/dna.15MB experiment_data/dna.15MB.fm
./construct experiment_data/proteins.15MB experiment_data/proteins.15MB.fm
./construct experiment_data/dblp.xml.15MB experiment_data/dblp.xml.15MB.fm

./construct experiment_data/dna.20MB experiment_data/dna.20MB.fm
./construct experiment_data/proteins.20MB experiment_data/proteins.20MB.fm
./construct experiment_data/dblp.xml.20MB experiment_data/dblp.xml.20MB.fm
