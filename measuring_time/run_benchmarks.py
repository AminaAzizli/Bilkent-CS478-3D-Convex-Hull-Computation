import subprocess
import re
import pandas as pd

# Change this if your executable name is different
EXECUTABLE = "./merged_hulls" 

SEED = 123
ALGORITHM = "both"

DISTRIBUTIONS = [
    "uniform",
    "gaussian",
    "clustered" 


]

INPUT_SIZES = [
    100,
    250,
    500,
    750,
    1000,
    2500,
    5000,
    7500,
    10000,
    25000,
    50000,
    75000,
    100000,
    250000,
    500000,
    1000000
]

results = []

def extract_value(pattern, text):
    match = re.search(pattern, text)
    if match:
        return float(match.group(1))
    return None

for dist in DISTRIBUTIONS:
    for n in INPUT_SIZES:
        print(f"Running: {dist}, N={n}")

        cmd = [
            EXECUTABLE,
            str(n),
            str(SEED),
            dist,
            ALGORITHM
        ]

        try:
            completed = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=7200
            )

            output = completed.stdout + completed.stderr

            gw_time = extract_value(r"\[GW\] Wall time ms = ([0-9.eE+-]+)", output)
            naw_time = extract_value(r"\[NAW\] Wall time ms = ([0-9.eE+-]+)", output)

            hull_vertices = extract_value(r"\[GW\] Hull vertices = ([0-9.eE+-]+)", output)
            unique_edges = extract_value(r"\[GW\] Unique edges\s+= ([0-9.eE+-]+)", output)
            hull_faces = extract_value(r"\[GW\] Faces\s+= ([0-9.eE+-]+)", output)
            euler_value = extract_value(r"\[GW\] Euler V-E\+F\s+= ([0-9.eE+-]+)", output)

            results.append({
                "Distribution": dist,
                "InputSize": n,
                "GW_Time_ms": gw_time,
                "NAW_Time_ms": naw_time,
                "HullVertices": hull_vertices,
                "UniqueEdges": unique_edges,
                "HullFaces": hull_faces,
                "EulerValue": euler_value
            })

        except subprocess.TimeoutExpired:
            print(f"Timeout for {dist}, N={n}")

            results.append({
                "Distribution": dist,
                "InputSize": n,
                "GW_Time_ms": None,
                "NAW_Time_ms": None,
                "HullVertices": None,
                "UniqueEdges": None,
                "HullFaces": None,
                "EulerValue": None
            })

df = pd.DataFrame(results)
df.to_csv("benchmark_results.csv", index=False)

print("\nSaved results to benchmark_results.csv")
print(df)