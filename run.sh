#!/bin/bash

# name of the scenario
scenario=$1

# duration of the simulation in seconds
simtime=$2

inpath=topologies/${scenario}.txt
outpath=output/${scenario}_sim.out

echo "Starting simulation for $scenario for $simtime seconds ..."

./runner $inpath $simtime > $outpath
sleep 10

echo -e "Done.\nParsing the output ..."
python scripts/parse_sim_output.py $outpath

echo -e "Done.\nPlotting the exit rate ..."
python scripts/plot_exit_rate.py ${outpath/sim.out/exrates.txt}

echo "All done."
