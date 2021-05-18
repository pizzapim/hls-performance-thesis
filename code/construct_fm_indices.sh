#!/bin/bash
set -x

./construct experiment_data/dna.50MB experiment_data/dna.50MB.fm
./construct experiment_data/proteins.50MB experiment_data/proteins.50MB.fm
./construct experiment_data/dblp.xml.50MB experiment_data/dblp.xml.50MB.fm

./construct experiment_data/dna.100MB experiment_data/dna.100MB.fm
./construct experiment_data/proteins.100MB experiment_data/proteins.100MB.fm
./construct experiment_data/dblp.xml.100MB experiment_data/dblp.xml.100MB.fm

./construct experiment_data/dna.150MB experiment_data/dna.150MB.fm
./construct experiment_data/proteins.150MB experiment_data/proteins.150MB.fm
./construct experiment_data/dblp.xml.150MB experiment_data/dblp.xml.150MB.fm
