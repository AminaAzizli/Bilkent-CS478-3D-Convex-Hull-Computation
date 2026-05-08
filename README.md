# 3D Convex Hull Project
## Gift Wrapping (GW) and Newton Apple Wrapper (NAW)

This project contains implementations of two different 3D convex hull algorithms:

1. **Gift Wrapping / Jarvis March extension to 3D (GW)**
2. **Newton Apple Wrapper style incremental convex hull construction (NAW)**

The project also includes:

- Runtime benchmarking scripts
- Automated graph generation
- Visualization and animation utilities

---

# Project Structure

```text
Project/
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
└── README.md
```

---

# Compilation

## Main Visualization Version

```bash
cd animation
g++ -O2 -std=c++17 merged_algorithms.cpp -o merged_algorithms
```

## Timing-Only Benchmark Version

```bash
cd measuring_time
g++ -O2 -std=c++17 merged_hulls.cpp -o merged_hulls
```

---

# Running the Main Program

## General Format

```bash
./merged_algorithms <N> <seed> <distribution> <algorithm>
```

## Arguments

| Argument | Description |
|---|---|
| `N` | Number of input points |
| `seed` | Random seed |
| `distribution` | Point distribution type |
| `algorithm` | `gw`, `naw`, or `both` |

---

# Examples

## Run Both Algorithms on Uniform Distribution

```bash
./merged_algorithms 1000 123 uniform both
```

## Run Only Gift Wrapping

```bash
./merged_algorithms 5000 123 sphere gw
```

## Run Only NAW

```bash
./merged_algorithms 10000 123 paraboloid naw
```

---

# Supported Distributions

- `uniform`
- `gaussian`
- `sphere`
- `clustered`
- `paraboloid`
- `ring`
- `shell`
- `ball`

---

# Supported Algorithms

- `gw`
- `naw`
- `both`

---

# Generated Output Files

The main program generates the following files:

```text
points_<distribution>_<N>_<algorithm>.txt
hull_steps_<distribution>_<N>_<algorithm>.txt
hull_final_<distribution>_<N>_<algorithm>.txt
```

## Example

```text
points_uniform_1000_gw.txt
hull_steps_uniform_1000_gw.txt
hull_final_uniform_1000_gw.txt
```

For large inputs, step snapshots may be disabled automatically to reduce memory usage.

---

# Visualization

Go to the visualization folder:

```bash
cd animation
```

## Install Required Python Libraries

```bash
pip install plotly numpy pandas matplotlib
```

## Run the Visualizer

```bash
python visualizer.py
```

The visualization utility animates the convex hull construction process and displays the generated hull surfaces.

---

# Runtime Benchmarking

The `measuring_time/` folder contains scripts for:

- Automated benchmarking
- Runtime data collection
- Graph generation

## Run Benchmark Collection

```bash
cd measuring_time
python run_benchmarks.py
```

All benchmark results are stored in:

```text
benchmark_results.csv
```

## Generate Runtime Graphs

```bash
python plot_benchmarks.py
```

---

# Notes

- The project was implemented in **C++17**.
- Runtime measurements are performed using high-resolution clocks.
- Euler characteristic verification is used to validate generated hulls.
- Large datasets may require substantial memory and execution time, especially for dense hull distributions.

---

# Authors

- Amina Azizli
- Team Members of the CS478 Convex Hull Project
