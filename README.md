# 3D Convex Hull Project
# Gift Wrapping (GW) and Newton Apple Wrapper (NAW)

This project contains implementations of two different 3D convex hull algorithms:

1. Gift Wrapping / Jarvis March extension to 3D (GW)
2. Newton Apple Wrapper style incremental convex hull construction (NAW)

The project also includes:
- runtime benchmarking scripts,
- automated graph generation,
- and visualization/animation utilities.

------------------------------------------------------------
PROJECT STRUCTURE
------------------------------------------------------------

Project/
│
│
├── animation/
│   ├── merged_algorithms.cpp
│   ├── visualizer.py
│
├── measuring_time/
│   ├── merged_hulls.cpp
│   ├── run_benchmarks.py
│   ├── plot_benchmarks.py
│
└── README.txt

------------------------------------------------------------
COMPILATION
------------------------------------------------------------

Compile the main convex hull implementation using:

cd animation
g++ -O2 -std=c++17 merged_algorithms.cpp -o merged_algorithms

For the timing-only version:

cd measuring_time
g++ -O2 -std=c++17 merged_hulls.cpp -o merged_hulls

------------------------------------------------------------
RUNNING THE MAIN PROGRAM
------------------------------------------------------------

General format:

./merged_algorithms <N> <seed> <distribution> <algorithm>

Arguments:

N              -> number of input points
seed           -> random seed
distribution   -> point distribution type
algorithm      -> gw, naw, or both

------------------------------------------------------------
EXAMPLES
------------------------------------------------------------

Run both algorithms on uniform distribution:

./merged_algorithms 1000 123 uniform both

Run only Gift Wrapping:

./merged_algorithms 5000 123 sphere gw

Run only NAW:

./merged_algorithms 10000 123 paraboloid naw

------------------------------------------------------------
SUPPORTED DISTRIBUTIONS
------------------------------------------------------------

uniform
gaussian
sphere
clustered
paraboloid
ring
shell
ball

------------------------------------------------------------
SUPPORTED ALGORITHMS
------------------------------------------------------------

gw
naw
both

------------------------------------------------------------
GENERATED OUTPUT FILES
------------------------------------------------------------

The main program generates:

points_<distribution>_<N>_<algorithm>.txt
hull_steps_<distribution>_<N>_<algorithm>.txt
hull_final_<distribution>_<N>_<algorithm>.txt

Example:

points_uniform_1000_gw.txt
hull_steps_uniform_1000_gw.txt
hull_final_uniform_1000_gw.txt

For large inputs, step snapshots may be disabled automatically to reduce memory usage.

------------------------------------------------------------
VISUALIZATION
------------------------------------------------------------
Go to folder:
cd animation

Install required Python libraries:

pip install plotly numpy pandas matplotlib

Run visualization scripts using:

python visualizer.py


------------------------------------------------------------
RUNTIME BENCHMARKING
------------------------------------------------------------

The MeasuringTime folder contains scripts for:
- automated benchmarking,
- runtime data collection,
- and graph generation.

Run the benchmark script:
Go to the folder
cd measuring_time

python run_benchmarks.py

Generate runtime graphs using:

python plot_benchmarks.py
