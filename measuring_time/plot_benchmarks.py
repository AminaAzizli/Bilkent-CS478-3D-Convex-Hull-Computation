import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("benchmark_results.csv")

# Plot one graph per distribution
for dist in df["Distribution"].unique():
    sub = df[df["Distribution"] == dist]

    plt.figure(figsize=(8, 5))
    plt.plot(sub["InputSize"], sub["GW_Time_ms"], marker="o", label="Gift Wrapping")
    plt.plot(sub["InputSize"], sub["NAW_Time_ms"], marker="o", label="NAW")

    plt.xlabel("Number of input points")
    plt.ylabel("Runtime (ms)")
    plt.title(f"Runtime Comparison for {dist.capitalize()} Distribution")
    plt.legend()
    plt.grid(True)
    plt.xscale("log")
    plt.yscale("log")

    plt.tight_layout()
    plt.savefig(f"runtime_{dist}.png", dpi=300)
    plt.show()