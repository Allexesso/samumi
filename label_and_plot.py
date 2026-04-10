#!/usr/bin/env python3
"""
label_and_plot.py – Post-processing for the SamUMI simulation pipeline.

1. Adds a ground-truth `Correct` column to every famsum (*_famsum.tsv) and
   UMIHaploCollect (*_haplo.tsv) output file.
2. Combines all labelled samples into:
      combined_umi.tsv   – for UMIRanForTrain.R
      combined_haplo.tsv – for HapRanForTrain.R
3. Generates evaluation plots (PNG) in the data directory.

`Correct` definition:
  UMI level  – the called Primary_Length matches the length of at least one of
               the individual's true alleles at that locus.
  Haplo level – same criterion applied to the haplotype-level Primary_Length.

Usage:
    python label_and_plot.py --data-dir sim_data --primer example_anchor/PrimedAnchors.txt
"""

import argparse
import csv
import glob
import os
import sys

# ---------------------------------------------------------------------------
# Ground truth helpers
# ---------------------------------------------------------------------------

def load_truth(truth_path):
    """
    Load truth.tsv produced by simulate_reads.py.

    Returns a dict:  (sample, locus) -> truth_entry

    For single-source samples the entry contains:
      {allele1_len, allele2_len, allele1_seq, allele2_seq}

    For mixture samples (two contributors) the entry additionally contains:
      {allele3_len, allele4_len, allele3_seq, allele4_seq}
    """
    truth = {}
    with open(truth_path) as fh:
        reader = csv.DictReader(fh, delimiter="\t")
        for row in reader:
            key = (row["sample"], row["locus"])
            entry = {
                "allele1_seq": row["allele1_seq"],
                "allele2_seq": row["allele2_seq"],
                "allele1_len": int(row["allele1_len"]),
                "allele2_len": int(row["allele2_len"]),
            }
            # Mixture samples have alleles from a second contributor.
            if row.get("allele3_seq"):
                entry["allele3_seq"] = row["allele3_seq"]
                entry["allele4_seq"] = row["allele4_seq"]
                entry["allele3_len"] = int(row["allele3_len"])
                entry["allele4_len"] = int(row["allele4_len"])
            truth[key] = entry
    return truth


def _correct_flag(called_len, truth_entry):
    """Return 1 if called_len matches any true allele length, else 0.

    For single-source samples the true allele lengths are allele1_len and
    allele2_len.  For mixture samples (two contributors) allele3_len and
    allele4_len are also checked so that alleles from the minor contributor
    are correctly classified as True positives.
    """
    try:
        n = int(called_len)
    except (TypeError, ValueError):
        return 0
    true_lengths = {truth_entry["allele1_len"], truth_entry["allele2_len"]}
    if "allele3_len" in truth_entry:
        true_lengths.add(truth_entry["allele3_len"])
        true_lengths.add(truth_entry["allele4_len"])
    return 1 if n in true_lengths else 0


# ---------------------------------------------------------------------------
# Labelling
# ---------------------------------------------------------------------------

def label_file(in_path, sample_name, truth, out_path):
    """
    Read a TSV file, add Sample + Correct columns, write labelled output.
    Returns list of row dicts (with Sample and Correct added).
    """
    import io
    rows = []
    with open(in_path, newline="") as fh:
        # Robustly locate the TSV header: find the first line that contains a
        # tab-separated field list starting with a known column name.  This
        # tolerates any non-TSV lines (e.g., debug prints) that may precede the
        # real header.
        raw_lines = fh.readlines()

    header_idx = None
    for i, line in enumerate(raw_lines):
        fields = line.rstrip("\n").split("\t")
        if len(fields) > 1 and fields[0].strip() in ("Locus", "Sample"):
            header_idx = i
            break
    if header_idx is None:
        return rows

    reader = csv.DictReader(
        io.StringIO("".join(raw_lines[header_idx:])), delimiter="\t"
    )
    if not reader.fieldnames:
        return rows
    for row in reader:
        locus = row.get("Locus", "")
        t = truth.get((sample_name, locus))
        row["Sample"] = sample_name
        row["Correct"] = (
            _correct_flag(row.get("Primary_Length", ""), t)
            if t is not None else ""
        )
        rows.append(row)

    if not rows:
        return rows

    out_fields = ["Sample"] + list(rows[0].keys() - {"Sample", "Correct"}) + ["Correct"]
    # Preserve original column order; add Sample first, Correct last
    orig_fields = list(rows[0].keys())
    ordered = (
        ["Sample"]
        + [f for f in orig_fields if f not in ("Sample", "Correct")]
        + ["Correct"]
    )

    with open(out_path, "w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=ordered, delimiter="\t",
                                extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)

    return rows


# ---------------------------------------------------------------------------
# Plotting
# ---------------------------------------------------------------------------

def make_plots(umi_rows, haplo_rows, out_dir):
    """Generate evaluation PNGs into out_dir."""
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        import collections
    except ImportError:
        print("  matplotlib not available – skipping plots.")
        print("  Install with: pip install matplotlib")
        return

    def savefig(name, tight=True):
        path = os.path.join(out_dir, name)
        if tight:
            plt.tight_layout()
        plt.savefig(path, dpi=150)
        plt.close()
        print(f"  Saved: {path}")

    # ── 1. Overall UMI-family call accuracy ──────────────────────────────────
    correct_vals = [r.get("Correct", "") for r in umi_rows]
    n_correct   = correct_vals.count(1) + correct_vals.count("1")
    n_incorrect = correct_vals.count(0) + correct_vals.count("0")
    n_unlabelled = len(correct_vals) - n_correct - n_incorrect

    fig, ax = plt.subplots(figsize=(5, 5))
    labels = ["Correct", "Incorrect"]
    vals   = [n_correct, n_incorrect]
    colors = ["#2ecc71", "#e74c3c"]
    if n_unlabelled:
        labels.append("Unlabelled")
        vals.append(n_unlabelled)
        colors.append("#95a5a6")
    ax.pie(vals, labels=labels, colors=colors, autopct="%1.1f%%", startangle=90)
    ax.set_title("UMI-family allele-call accuracy\n(all samples, all loci)")
    savefig("plot_01_overall_accuracy.png")

    # ── 2. Per-locus accuracy (UMI level) ────────────────────────────────────
    from collections import defaultdict
    locus_stats = defaultdict(lambda: {"correct": 0, "total": 0})
    for r in umi_rows:
        locus = r.get("Locus", "unknown")
        c = r.get("Correct", "")
        if c in (1, "1", 0, "0"):
            locus_stats[locus]["total"] += 1
            if c in (1, "1"):
                locus_stats[locus]["correct"] += 1

    if locus_stats:
        loci_sorted = sorted(locus_stats.keys())
        accs = [
            locus_stats[l]["correct"] / locus_stats[l]["total"] * 100
            if locus_stats[l]["total"] > 0 else 0
            for l in loci_sorted
        ]
        fig, ax = plt.subplots(figsize=(max(8, len(loci_sorted) * 0.5), 5))
        bars = ax.bar(range(len(loci_sorted)), accs, color="#3498db")
        ax.set_xticks(range(len(loci_sorted)))
        ax.set_xticklabels(loci_sorted, rotation=90, fontsize=8)
        ax.set_ylabel("Accuracy (%)")
        ax.set_title("Per-locus UMI-family call accuracy")
        ax.set_ylim(0, 105)
        ax.axhline(100, color="gray", linestyle="--", linewidth=0.8)
        savefig("plot_02_per_locus_accuracy.png")

    # ── 3. UMI family read-depth distribution ────────────────────────────────
    depths = []
    for r in umi_rows:
        try:
            depths.append(int(r.get("Reads", 0)))
        except (TypeError, ValueError):
            pass

    if depths:
        fig, ax = plt.subplots(figsize=(7, 4))
        ax.hist(depths, bins=range(1, max(depths) + 2), color="#9b59b6", edgecolor="white")
        ax.set_xlabel("Reads per UMI family")
        ax.set_ylabel("Count")
        ax.set_title("UMI family read-depth distribution")
        savefig("plot_03_family_depth.png")

    # ── 4. Primary proportion (purity) distribution ──────────────────────────
    purities = []
    for r in umi_rows:
        try:
            purities.append(float(r.get("Primary_Proportion", 0)))
        except (TypeError, ValueError):
            pass

    if purities:
        fig, ax = plt.subplots(figsize=(7, 4))
        ax.hist(purities, bins=20, range=(0, 1), color="#e67e22", edgecolor="white")
        ax.set_xlabel("Primary proportion (family purity)")
        ax.set_ylabel("Count")
        ax.set_title("UMI family purity distribution")
        ax.set_xlim(0, 1)
        savefig("plot_04_family_purity.png")

    # ── 5. Primary allele length distribution across loci ────────────────────
    locus_lengths = defaultdict(list)
    for r in umi_rows:
        locus = r.get("Locus", "")
        try:
            locus_lengths[locus].append(int(r.get("Primary_Length", 0)))
        except (TypeError, ValueError):
            pass

    if locus_lengths:
        loci_sorted = sorted(locus_lengths.keys())
        data = [locus_lengths[l] for l in loci_sorted]
        fig, ax = plt.subplots(figsize=(max(8, len(loci_sorted) * 0.5), 5))
        ax.boxplot(data, tick_labels=loci_sorted)
        ax.set_xticklabels(loci_sorted, rotation=90, fontsize=8)
        ax.set_ylabel("Primary allele length (bp)")
        ax.set_title("Called allele length distribution per locus")
        savefig("plot_05_allele_lengths.png")

    # ── 6. Haplotype-level accuracy ──────────────────────────────────────────
    if haplo_rows:
        hap_correct   = sum(1 for r in haplo_rows if r.get("Correct") in (1, "1"))
        hap_incorrect = sum(1 for r in haplo_rows if r.get("Correct") in (0, "0"))
        hap_unlabelled = len(haplo_rows) - hap_correct - hap_incorrect

        fig, ax = plt.subplots(figsize=(5, 5))
        labels = ["Correct", "Incorrect"]
        vals   = [hap_correct, hap_incorrect]
        colors = ["#2ecc71", "#e74c3c"]
        if hap_unlabelled:
            labels.append("Unlabelled")
            vals.append(hap_unlabelled)
            colors.append("#95a5a6")
        ax.pie(vals, labels=labels, colors=colors, autopct="%1.1f%%", startangle=90)
        ax.set_title("Haplotype-level allele call accuracy\n(all samples, all loci)")
        savefig("plot_06_haplo_accuracy.png")

    # ── 7. UMI count per allele (haplotype level) ────────────────────────────
    if haplo_rows:
        umi_counts = []
        for r in haplo_rows:
            try:
                umi_counts.append(int(r.get("UMI_Count", 0)))
            except (TypeError, ValueError):
                pass
        if umi_counts:
            fig, ax = plt.subplots(figsize=(7, 4))
            ax.hist(umi_counts, bins=30, color="#1abc9c", edgecolor="white")
            ax.set_xlabel("UMI count per allele")
            ax.set_ylabel("Count")
            ax.set_title("UMI family support per allele (haplotype level)")
            savefig("plot_07_umi_counts.png")

    print("  All plots written.")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Add Correct labels and generate evaluation plots for SamUMI output.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("--data-dir", default="sim_data",
                        help="Directory containing simulation + pipeline output files.")
    parser.add_argument("--primer",   default="example_anchor/PrimedAnchors.txt",
                        help="Primer TSV file (used only for reporting).")
    parser.add_argument("--no-plots", action="store_true",
                        help="Skip generating PNG plots.")
    args = parser.parse_args()

    truth_path = os.path.join(args.data_dir, "truth.tsv")
    if not os.path.isfile(truth_path):
        print(f"ERROR: {truth_path} not found.\n"
              "Run simulate_reads.py first to generate ground truth.", file=sys.stderr)
        sys.exit(1)

    truth = load_truth(truth_path)
    print(f"Loaded ground truth for {len(truth)} (sample, locus) combinations.")

    # ── Find all per-sample output files ─────────────────────────────────────
    famsum_files = sorted(glob.glob(os.path.join(args.data_dir, "*_famsum.tsv")))
    haplo_files  = sorted(glob.glob(os.path.join(args.data_dir, "*_haplo.tsv")))

    if not famsum_files:
        print("WARNING: No *_famsum.tsv files found. Run run_pipeline.sh first.",
              file=sys.stderr)

    all_umi_rows   = []
    all_haplo_rows = []

    # ── Label UMI (famsum) files ──────────────────────────────────────────────
    print(f"\nLabelling {len(famsum_files)} famsum file(s) …")
    for path in famsum_files:
        sample_name = os.path.basename(path).replace("_famsum.tsv", "")
        out_path = os.path.join(args.data_dir, f"{sample_name}_famsum_labelled.tsv")
        rows = label_file(path, sample_name, truth, out_path)
        all_umi_rows.extend(rows)
        n_correct = sum(1 for r in rows if r.get("Correct") in (1, "1"))
        print(f"  {sample_name}: {len(rows)} families, "
              f"{n_correct} correct ({n_correct/len(rows)*100:.1f}%)" if rows else
              f"  {sample_name}: 0 families")

    # ── Label haplotype (haplo) files ─────────────────────────────────────────
    print(f"\nLabelling {len(haplo_files)} haplo file(s) …")
    for path in haplo_files:
        sample_name = os.path.basename(path).replace("_haplo.tsv", "")
        out_path = os.path.join(args.data_dir, f"{sample_name}_haplo_labelled.tsv")
        rows = label_file(path, sample_name, truth, out_path)
        all_haplo_rows.extend(rows)
        n_correct = sum(1 for r in rows if r.get("Correct") in (1, "1"))
        print(f"  {sample_name}: {len(rows)} alleles, "
              f"{n_correct} correct ({n_correct/len(rows)*100:.1f}%)" if rows else
              f"  {sample_name}: 0 alleles")

    # ── Write combined training files ─────────────────────────────────────────
    def write_combined(rows, out_path):
        if not rows:
            return
        fields = list(rows[0].keys())
        with open(out_path, "w", newline="") as fh:
            writer = csv.DictWriter(fh, fieldnames=fields, delimiter="\t",
                                    extrasaction="ignore")
            writer.writeheader()
            writer.writerows(rows)

    umi_combined   = os.path.join(args.data_dir, "combined_umi.tsv")
    haplo_combined = os.path.join(args.data_dir, "combined_haplo.tsv")

    write_combined(all_umi_rows,   umi_combined)
    write_combined(all_haplo_rows, haplo_combined)

    print(f"\nCombined files written:")
    print(f"  {umi_combined}   ({len(all_umi_rows)} rows)")
    print(f"  {haplo_combined} ({len(all_haplo_rows)} rows)")

    # ── Summary stats ─────────────────────────────────────────────────────────
    if all_umi_rows:
        n_correct   = sum(1 for r in all_umi_rows if r.get("Correct") in (1, "1"))
        n_labelled  = sum(1 for r in all_umi_rows if r.get("Correct") in (1, "1", 0, "0"))
        pct = n_correct / n_labelled * 100 if n_labelled else 0
        print(f"\nOverall UMI-family accuracy  : {n_correct}/{n_labelled} = {pct:.1f}%")
    if all_haplo_rows:
        n_correct  = sum(1 for r in all_haplo_rows if r.get("Correct") in (1, "1"))
        n_labelled = sum(1 for r in all_haplo_rows if r.get("Correct") in (1, "1", 0, "0"))
        pct = n_correct / n_labelled * 100 if n_labelled else 0
        print(f"Overall haplotype accuracy   : {n_correct}/{n_labelled} = {pct:.1f}%")

    # ── Plots ─────────────────────────────────────────────────────────────────
    if not args.no_plots:
        print("\nGenerating plots …")
        make_plots(all_umi_rows, all_haplo_rows, args.data_dir)

    print("\nDone.")
    print("\nRandom forest training commands (requires R + ranger + tidyverse):")
    print(f"  Rscript source_r/UMIRanForTrain.R  sim_data/umi_model.rds  3  {umi_combined}")
    print(f"  Rscript source_r/HapRanForTrain.R  sim_data/hap_model.rds  3  {haplo_combined}")


if __name__ == "__main__":
    main()
