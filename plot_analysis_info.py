#!/usr/bin/env python3
import matplotlib # need this to plot
matplotlib.use('Agg') # select the Agg backend so we can save PNGs without a display
import matplotlib.pyplot as plt # the usually pyplot interface, aliased it as plt

import pandas as pd # need this for dataframing
import glob, re # glob for file search, need re for regular expressions

# scans our analysis files for 
def load_summaries(metric):
    rows = []
    # match ANY path ending in analysis/<impl>_<size>_<cores>_summary.txt
    pat = re.compile(r'.*/analysis/([^_]+)_(\d+M)_(\d+)_summary\.txt$')
    # find two floats on the metric line
    line_re = re.compile(rf'^{metric}\s+([\d\.]+).+?([\d\.]+)')

    # recursive glob for any summary file under ANY analysis/ directory
    for fn in glob.glob("**/*_summary.txt", recursive=True):
        m = pat.match(fn) # skips files that don't live in an analysis directory or name doesn't follow the pattern
        if not m:
            continue
        impl, size, cores = m.groups()
        cores = int(cores)

        with open(fn) as f: # reads each file line by line until it finds one that starts with our metric name
            for line in f:
                line = line.strip()
                if not line.startswith(metric):
                    continue
                lm = line_re.match(line) # extracts the first two numeric tokens on that line (the mean, then the standard deviation)
                if not lm:
                    print(f"warning: couldn’t parse '{line}' in {fn}")
                else:
                    mean = float(lm.group(1))
                    std  = float(lm.group(2))
                    rows.append((impl, size, cores, 1, mean, std)) # builds a tuple of all the values 
                break   # only the first matching line

    if not rows:
        print(f"warning: no data found for {metric}")
    return pd.DataFrame(rows, columns=["impl","size","cores","nodes","mean","std"]) # turns the list of tuples into a nice-to-work-with data frame

# for each metric, draws one plot per input size, overlaying each implementation
def plot_metric(df, metric, ylabel):
    for size, grp in df.groupby("size"): # groups the data frame by input size
        plt.figure() # new figure, just like matlab
        for impl, g in grp.groupby("impl"): # within that size, plot one line per implementation
            g = g.sort_values("cores") # make sure it’s in ascending-core order
            plt.errorbar(
                g["cores"], # x is the number of cores
                g["mean"], # y is the mean value
                yerr=g["std"], # error bars are the standard deviation
                marker="o",
                capsize=3,
                linestyle="-",
                label=impl, # the legend entries are the different implementations
            )
        # make axes, labels, legends and tidy things up
        plt.title(f"{metric} vs cores (size={size})")
        plt.xlabel("Cores")
        plt.ylabel(ylabel)
        plt.legend()
        plt.tight_layout()
        outfn = f"plots/{metric}_{size}.png"
        plt.savefig(outfn)
        print(f" → saved {outfn}")
        plt.close()

# for each of the metrics, load the data then plot it
if __name__=="__main__":
    for metric, ylabel in [
        ("wall_s",        "Wall-clock time (s)"),
        ("task_clock_ms", "Task-clock (ms)"),
        ("cpu_pct",       "CPU efficiency (%)"),
        ("max_rss_kb",    "Max RSS (KB)"),
    ]:
        df = load_summaries(metric)
        if df.empty:
            print(f"warning: no data found for {metric}")
        else:
            plot_metric(df, metric, ylabel)
