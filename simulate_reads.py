#!/usr/bin/env python3
"""
simulate_reads.py – Simulate paired-end UMI-tagged STR amplicon sequencing reads
for the SamUMI pipeline.

Read structure (paired-end):
  R1 (forward):  [primer][repeat_seq][anchor][random padding]       (isRC = 0)
                 [RC(primer)][RC(repeat_seq)][RC(anchor)][padding]  (isRC = 1)
  R2 (reverse):  [UMI (umi_len bp)][common_seq][random padding]

This matches the experimental design described in the paper:
  - 2×301 cycle paired-end sequencing (default --read-len 301)
  - 64 single-source individuals (default --samples 64)
  - 15 pairs of mixed-DNA samples at 1:1 and 1:9 contributor ratios
    (enabled via --mix-pairs, default 15)

Usage:
  python simulate_reads.py [options]

Outputs (all in --out-dir):
  sample_NNN_R1.fq            – forward reads for each simulated individual
  sample_NNN_R2.fq            – reverse reads for each simulated individual
  mix_NNN_1to1_R1.fq          – 1:1 mixture reads for pair NNN
  mix_NNN_1to1_R2.fq
  mix_NNN_1to9_R1.fq          – 1:9 mixture reads for pair NNN
  mix_NNN_1to9_R2.fq
  truth.tsv                   – ground truth allele assignments per sample × locus
                                (includes allele3/allele4 columns for mixture samples)
"""

import argparse
import csv
import os
import random
import sys

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

# R2 (reverse read) contains the UMI and common adapter; it is sequenced from
# the relatively clean adapter end of the molecule and therefore tends to have
# fewer errors than R1 (which spans the STR repeat region).  The R2 error rate
# is set to this fraction of the R1 rate by default.
R2_ERROR_RATE_FACTOR = 0.3

# Within a UMI family, some reads originate from PCR-slippage stutter molecules
# that are one repeat unit shorter (n-1) than the true allele.  Forward stutter
# (n+1) occurs at roughly this fraction of the backward (n-1) stutter rate.
# These constants mirror values typical for forensic 4-bp STR loci.
STUTTER_DEFAULT_RATE = 0.10   # fraction of reads per family that are n-1 stutter
STUTTER_PLUS_FACTOR  = 0.30   # n+1 stutter rate = stutter_rate * this factor
# ---------------------------------------------------------------------------
# Known forensic STR repeat units (forward strand, canonical direction)
# ---------------------------------------------------------------------------
REPEAT_UNITS = {
    "TH01":     "AATG",
    "vWA":      "TCTG",
    "CSF1PO":   "AGAT",
    "FGA":      "CTTT",
    "D3S1358":  "TCTA",
    "D7S820":   "GATA",
    "D8S1179":  "TCTA",
    "D13S317":  "TATC",
    "D18S51":   "AGAA",
    "D19S433":  "AAGG",
    "D16S539":  "GATA",
    "D2S1338":  "TGCC",
    "D5S818":   "AGAT",
    "D1S1656":  "CCTA",
    "D10S1248": "GGAA",
    "D12S391":  "AGAT",
    "D22S1045": "ATT",
    "PentaD":   "AAAGA",
    "PentaE":   "AAAGA",
    "SE33":     "TCTA",
    "D2S441":   "TCTA",
    "D14S1434": "TATC",
    "TPOX":     "AATG",
    "D6S1043":  "AGAT",
    "D17S1301": "TCTA",
    "D1S1677":  "TCTA",
    "D9S1122":  "AGAT",
    "D12ATA63": "ATA",
    "D9S2157":  "ATA",
    "D2S1776":  "TCTA",
    "D3S4529":  "TCTA",
    "D4S2408":  "TATC",
    "D5S2500":  "TCTA",
    "D5S2800":  "AGAT",
    "D6S474":   "AGAT",
    "D20S482":  "AGAT",
    "D21S11":   "TCTA",
}

# Realistic diploid allele ranges (number of repeats) for each locus
ALLELE_RANGES = {
    "TH01":     (5,  11),
    "vWA":      (14, 20),
    "CSF1PO":   (7,  13),
    "FGA":      (18, 26),
    "D3S1358":  (13, 19),
    "D7S820":   (7,  13),
    "D8S1179":  (10, 16),
    "D13S317":  (8,  14),
    "D18S51":   (13, 20),
    "D19S433":  (12, 17),
    "D16S539":  (9,  14),
    "D2S1338":  (19, 26),
    "D5S818":   (9,  14),
    "D1S1656":  (12, 18),
    "D10S1248": (13, 19),
    "D12S391":  (16, 24),
    "D22S1045": (11, 17),
    "PentaD":   (9,  17),
    "PentaE":   (7,  18),
    "SE33":     (15, 30),
    "D2S441":   (10, 15),
    "D14S1434": (13, 20),
    "TPOX":     (6,  13),
    "D6S1043":  (11, 18),
    "D17S1301": (8,  15),
    "D1S1677":  (11, 16),
    "D9S1122":  (10, 16),
    "D12ATA63": (11, 18),
    "D9S2157":  (13, 18),
    "D2S1776":  (17, 24),
    "D3S4529":  (10, 16),
    "D4S2408":  (8,  14),
    "D5S2500":  (7,  13),
    "D5S2800":  (8,  14),
    "D6S474":   (10, 16),
    "D20S482":  (8,  14),
    "D21S11":   (25, 35),
}

# ---------------------------------------------------------------------------
# Sequence utilities
# ---------------------------------------------------------------------------
_COMP = str.maketrans("ACGTacgt", "TGCAtgca")


def reverse_complement(seq):
    """Return the reverse complement of a DNA sequence."""
    return seq.translate(_COMP)[::-1]


def _introduce_errors(seq, error_rate, rng):
    """
    Randomly substitute bases to simulate sequencing errors.
    Returns (new_seq, set_of_error_positions).
    """
    bases = list(seq)
    error_positions = set()
    for i in range(len(bases)):
        if rng.random() < error_rate:
            orig = bases[i].upper()
            alt = rng.choice([b for b in "ACGT" if b != orig])
            bases[i] = alt
            error_positions.add(i)
    return "".join(bases), error_positions


def _make_quality(length, error_positions, base_q=35, error_q=12):
    """Build a Phred+33 ASCII quality string."""
    quals = []
    for i in range(length):
        q = error_q if i in error_positions else base_q
        quals.append(chr(q + 33))
    return "".join(quals)


def _simulate_read(raw_seq, error_rate, rng, base_q=35, error_q=12):
    """Apply sequencing errors and return (final_seq, qual_string)."""
    seq, err_pos = _introduce_errors(raw_seq, error_rate, rng)
    qual = _make_quality(len(seq), err_pos, base_q=base_q, error_q=error_q)
    return seq, qual


# ---------------------------------------------------------------------------
# Primer file parsing
# ---------------------------------------------------------------------------
def parse_primer_file(path):
    """
    Parse the PrimedAnchors TSV (7 tab-separated columns, no header):
      locus  chrom  position  is_rc  primer  anchor  period
    Returns a list of dicts.
    """
    loci = []
    with open(path) as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            parts = line.split("\t")
            if len(parts) < 7:
                continue
            loci.append({
                "locus":    parts[0].strip(),
                "chrom":    parts[1].strip(),
                "position": int(parts[2].strip()),
                "is_rc":    int(parts[3].strip()),
                "primer":   parts[4].strip(),
                "anchor":   parts[5].strip(),
                "period":   int(parts[6].strip()),
            })
    return loci


# ---------------------------------------------------------------------------
# Allele helpers
# ---------------------------------------------------------------------------
def _repeat_unit_for(locus_name, period):
    """
    Return the repeat unit for this locus.
    Uses known forensic data where available; falls back to an ACGT cycle.
    """
    unit = REPEAT_UNITS.get(locus_name)
    if unit and len(unit) == period:
        return unit
    # Generic fallback: cycle through ACGT
    return "".join("ACGT"[i % 4] for i in range(period))


def _allele_range_for(locus_name, period, read_len, primer, anchor):
    """
    Return (lo, hi) repeat-count range for this locus.
    Ensures that primer + max_allele + anchor fits within read_len.
    """
    lo, hi = ALLELE_RANGES.get(locus_name, (5, 20))
    max_by_read = (read_len - len(primer) - len(anchor)) // max(period, 1)
    hi = min(hi, max(max_by_read, lo))
    return lo, hi


def _random_umi(length, rng):
    return "".join(rng.choice("ACGT") for _ in range(length))


# ---------------------------------------------------------------------------
# Per-sample simulation
# ---------------------------------------------------------------------------

def _emit_allele_reads(
    sample_name, locus, primer, anchor, is_rc,
    allele_n, allele_seq, repeat_unit,
    n_families, reads_per_family,
    r1_error_rate, r2_error_rate, stutter_rate,
    umi_len, common_seq, read_len, rng,
    r1_fh, r2_fh, read_idx,
):
    """
    Emit FASTQ reads for one diploid allele copy into open file handles.

    For each UMI family a fraction *stutter_rate* of reads are replaced with
    PCR-slippage stutter molecules: n-1 repeats (backward stutter, the dominant
    artefact in STR sequencing) at probability *stutter_rate*, and n+1 repeats
    (forward stutter) at probability *stutter_rate* * STUTTER_PLUS_FACTOR.
    Stutter reads share the same UMI as their parent family and therefore
    appear as minority alleles within that family – exactly the signal that the
    SamUMI random-forest models are trained to recognise and correct.

    Returns the updated read_idx (so the caller can chain calls without
    colliding read names, e.g. when writing two contributors to the same file).
    """
    stutter_plus_rate = stutter_rate * STUTTER_PLUS_FACTOR
    stutter_minus_seq = repeat_unit * (allele_n - 1) if allele_n > 1 else None
    stutter_plus_seq  = repeat_unit * (allele_n + 1)

    for _ in range(n_families):
        umi = _random_umi(umi_len, rng)

        for _ in range(reads_per_family):
            read_name = f"{sample_name}.{locus}.{read_idx}"
            read_idx += 1

            # ── Choose allele sequence for this read ──────────────────────
            # With probability stutter_rate the read comes from an n-1
            # PCR-slippage molecule (backward stutter); with probability
            # stutter_plus_rate from an n+1 molecule (forward stutter);
            # otherwise the true allele.
            roll = rng.random()
            if stutter_minus_seq is not None and roll < stutter_rate:
                read_allele_seq = stutter_minus_seq
            elif roll < stutter_rate + stutter_plus_rate:
                read_allele_seq = stutter_plus_seq
            else:
                read_allele_seq = allele_seq

            # ── Build R1 core ─────────────────────────────────────────────
            if is_rc:
                # On the reverse strand the read spans:
                # RC(primer) + RC(allele) + RC(anchor)
                core_r1 = (
                    reverse_complement(primer)
                    + reverse_complement(read_allele_seq)
                    + reverse_complement(anchor)
                )
            else:
                core_r1 = primer + read_allele_seq + anchor

            pad_r1 = max(0, read_len - len(core_r1))
            raw_r1 = (
                core_r1
                + "".join(rng.choice("ACGT") for _ in range(pad_r1))
            )[:read_len]
            r1_seq, r1_qual = _simulate_read(raw_r1, r1_error_rate, rng)

            # ── Build R2 core ─────────────────────────────────────────────
            # UMI sits immediately before the common sequence on R2.
            core_r2 = umi + common_seq
            pad_r2 = max(0, read_len - len(core_r2))
            raw_r2 = (
                core_r2
                + "".join(rng.choice("ACGT") for _ in range(pad_r2))
            )[:read_len]
            r2_seq, r2_qual = _simulate_read(raw_r2, r2_error_rate, rng)

            # Write FASTQ entries
            r1_fh.write(f"@{read_name}/1\n{r1_seq}\n+\n{r1_qual}\n")
            r2_fh.write(f"@{read_name}/2\n{r2_seq}\n+\n{r2_qual}\n")

    return read_idx


def simulate_sample(
    sample_name,
    loci,
    common_seq,
    umi_len,
    read_len,
    families_per_allele,
    reads_per_family,
    r1_error_rate,
    r2_error_rate,
    stutter_rate,
    rng,
    r1_fh,
    r2_fh,
    read_offset=0,
):
    """
    Generate all FASTQ reads for one simulated single-source individual.

    Returns a list of ground-truth dicts:
      {sample, locus, primer, allele1_seq, allele2_seq, allele1_len, allele2_len}
    """
    truth_records = []
    read_idx = read_offset

    for loc in loci:
        locus     = loc["locus"]
        primer    = loc["primer"]
        anchor    = loc["anchor"]
        is_rc     = loc["is_rc"]
        period    = loc["period"]

        repeat_unit = _repeat_unit_for(locus, period)
        lo, hi      = _allele_range_for(locus, period, read_len, primer, anchor)

        # Assign diploid alleles (two independent draws)
        allele1_n = rng.randint(lo, hi)
        allele2_n = rng.randint(lo, hi)

        # Canonical allele sequence = repeat region on forward strand
        allele1_seq = repeat_unit * allele1_n
        allele2_seq = repeat_unit * allele2_n

        truth_records.append({
            "sample":      sample_name,
            "locus":       locus,
            "primer":      primer,
            "allele1_seq": allele1_seq,
            "allele2_seq": allele2_seq,
            "allele1_len": len(allele1_seq),
            "allele2_len": len(allele2_seq),
        })

        # Generate reads for each allele copy (diploid = 2 copies)
        for allele_n, allele_seq in (
            (allele1_n, allele1_seq),
            (allele2_n, allele2_seq),
        ):
            read_idx = _emit_allele_reads(
                sample_name, locus, primer, anchor, is_rc,
                allele_n, allele_seq, repeat_unit,
                families_per_allele, reads_per_family,
                r1_error_rate, r2_error_rate, stutter_rate,
                umi_len, common_seq, read_len, rng,
                r1_fh, r2_fh, read_idx,
            )

    return truth_records


def simulate_mixture_sample(
    sample_name,
    loci,
    common_seq,
    umi_len,
    read_len,
    families_c1,
    families_c2,
    reads_per_family,
    r1_error_rate,
    r2_error_rate,
    stutter_rate,
    rng,
    r1_fh,
    r2_fh,
):
    """
    Generate mixed FASTQ reads from two contributors into the same output files.

    The two contributors' reads are interleaved in the output files in proportion
    to their family counts (families_c1 : families_c2), mimicking the physical
    mixing of DNA at a given ratio before library preparation.

    Returns a list of ground-truth dicts with alleles from *both* contributors:
      {sample, locus, primer,
       allele1_seq, allele2_seq, allele1_len, allele2_len,   ← contributor 1
       allele3_seq, allele4_seq, allele3_len, allele4_len}   ← contributor 2
    """
    truth_records = []
    read_idx = 0

    for loc in loci:
        locus     = loc["locus"]
        primer    = loc["primer"]
        anchor    = loc["anchor"]
        is_rc     = loc["is_rc"]
        period    = loc["period"]

        repeat_unit = _repeat_unit_for(locus, period)
        lo, hi      = _allele_range_for(locus, period, read_len, primer, anchor)

        # Assign diploid alleles for each contributor independently
        c1_a1_n = rng.randint(lo, hi)
        c1_a2_n = rng.randint(lo, hi)
        c2_a1_n = rng.randint(lo, hi)
        c2_a2_n = rng.randint(lo, hi)

        c1_a1_seq = repeat_unit * c1_a1_n
        c1_a2_seq = repeat_unit * c1_a2_n
        c2_a1_seq = repeat_unit * c2_a1_n
        c2_a2_seq = repeat_unit * c2_a2_n

        truth_records.append({
            "sample":      sample_name,
            "locus":       locus,
            "primer":      primer,
            "allele1_seq": c1_a1_seq,
            "allele2_seq": c1_a2_seq,
            "allele1_len": len(c1_a1_seq),
            "allele2_len": len(c1_a2_seq),
            "allele3_seq": c2_a1_seq,
            "allele4_seq": c2_a2_seq,
            "allele3_len": len(c2_a1_seq),
            "allele4_len": len(c2_a2_seq),
        })

        # Emit contributor 1 reads (proportional to families_c1)
        for allele_n, allele_seq in (
            (c1_a1_n, c1_a1_seq),
            (c1_a2_n, c1_a2_seq),
        ):
            read_idx = _emit_allele_reads(
                sample_name, locus, primer, anchor, is_rc,
                allele_n, allele_seq, repeat_unit,
                families_c1, reads_per_family,
                r1_error_rate, r2_error_rate, stutter_rate,
                umi_len, common_seq, read_len, rng,
                r1_fh, r2_fh, read_idx,
            )

        # Emit contributor 2 reads (proportional to families_c2)
        for allele_n, allele_seq in (
            (c2_a1_n, c2_a1_seq),
            (c2_a2_n, c2_a2_seq),
        ):
            read_idx = _emit_allele_reads(
                sample_name, locus, primer, anchor, is_rc,
                allele_n, allele_seq, repeat_unit,
                families_c2, reads_per_family,
                r1_error_rate, r2_error_rate, stutter_rate,
                umi_len, common_seq, read_len, rng,
                r1_fh, r2_fh, read_idx,
            )

    return truth_records


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="Simulate UMI-tagged STR amplicon reads for the SamUMI pipeline.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--primer",
        default="example_anchor/PrimedAnchors.txt",
        help="Primer TSV file (locus/chrom/pos/isRC/primer/anchor/period).",
    )
    parser.add_argument(
        "--common",
        default="GAAACAGGATTAGATACCCT",
        help="Fixed common adapter sequence on R2 (immediately after the UMI).",
    )
    parser.add_argument(
        "--umi-len", type=int, default=12,
        help="UMI length in bases.",
    )
    parser.add_argument(
        "--samples", type=int, default=64,
        help="Number of simulated single-source individuals (paper: 64).",
    )
    parser.add_argument(
        "--mix-pairs", type=int, default=15,
        help=(
            "Number of mixture pairs to simulate.  Each pair produces one 1:1 "
            "and one 1:9 mixture FASTQ.  Set to 0 to disable mixture simulation. "
            "Pairs are drawn from the single-source individuals in order "
            "(pair 1 = individuals 1+2, pair 2 = 3+4, …); requires "
            "--samples >= 2 * --mix-pairs.  Paper: 15 pairs."
        ),
    )
    parser.add_argument(
        "--families", type=int, default=10,
        help="UMI families per allele per locus.",
    )
    parser.add_argument(
        "--reads", type=int, default=8,
        help="Reads per UMI family.",
    )
    parser.add_argument(
        "--error-rate", type=float, default=0.01,
        help="Per-base sequencing error rate for R1.",
    )
    parser.add_argument(
        "--r2-error-rate", type=float, default=None,
        help=(
            f"Per-base sequencing error rate for R2 "
            f"(default: {int(R2_ERROR_RATE_FACTOR*100)}%% of --error-rate)."
        ),
    )
    parser.add_argument(
        "--stutter-rate", type=float, default=STUTTER_DEFAULT_RATE,
        help=(
            "Fraction of reads per UMI family that are n-1 PCR-slippage stutter "
            "molecules (backward stutter). Forward stutter (n+1) is generated at "
            f"{int(STUTTER_PLUS_FACTOR*100)}%% of this rate. "
            "Set to 0 to disable stutter simulation."
        ),
    )
    parser.add_argument(
        "--read-len", type=int, default=301,
        help="Read length in bases (paper: 2×301 cycle sequencing).",
    )
    parser.add_argument(
        "--seed", type=int, default=42,
        help="Random seed for reproducibility.",
    )
    parser.add_argument(
        "--out-dir", default="sim_data",
        help="Directory for output files.",
    )
    args = parser.parse_args()

    r2_error_rate = args.r2_error_rate
    if r2_error_rate is None:
        r2_error_rate = args.error_rate * R2_ERROR_RATE_FACTOR

    if args.mix_pairs > 0 and args.samples < 2 * args.mix_pairs:
        print(
            f"ERROR: --mix-pairs {args.mix_pairs} requires at least "
            f"{2 * args.mix_pairs} single-source individuals (--samples), "
            f"but only {args.samples} requested.",
            file=sys.stderr,
        )
        sys.exit(1)

    os.makedirs(args.out_dir, exist_ok=True)

    rng = random.Random(args.seed)

    loci = parse_primer_file(args.primer)
    if not loci:
        print("ERROR: No loci loaded from primer file.", file=sys.stderr)
        sys.exit(1)

    print(f"Loaded {len(loci)} loci from {args.primer}")
    print(f"Simulating {args.samples} individual(s): "
          f"{args.families} families × {args.reads} reads per allele per locus")
    if args.mix_pairs:
        print(f"Simulating {args.mix_pairs} mixture pair(s): "
              f"1:1 and 1:9 ratios → {2 * args.mix_pairs} mixture sample(s)")
    print(f"  R1 error rate    : {args.error_rate}")
    print(f"  R2 error rate    : {r2_error_rate:.4f}")
    print(f"  Stutter rate     : {args.stutter_rate} (n-1) / "
          f"{args.stutter_rate * STUTTER_PLUS_FACTOR:.4f} (n+1)")
    print(f"  Read length      : {args.read_len} bp")
    print()

    truth_path = os.path.join(args.out_dir, "truth.tsv")
    all_truth = []

    # ── Single-source individuals ─────────────────────────────────────────────
    for s in range(1, args.samples + 1):
        sample_name = f"sample_{s:03d}"
        r1_path = os.path.join(args.out_dir, f"{sample_name}_R1.fq")
        r2_path = os.path.join(args.out_dir, f"{sample_name}_R2.fq")

        print(f"  {sample_name} …", end=" ", flush=True)
        with open(r1_path, "w") as r1_fh, open(r2_path, "w") as r2_fh:
            truth = simulate_sample(
                sample_name=sample_name,
                loci=loci,
                common_seq=args.common,
                umi_len=args.umi_len,
                read_len=args.read_len,
                families_per_allele=args.families,
                reads_per_family=args.reads,
                r1_error_rate=args.error_rate,
                r2_error_rate=r2_error_rate,
                stutter_rate=args.stutter_rate,
                rng=rng,
                r1_fh=r1_fh,
                r2_fh=r2_fh,
            )
        all_truth.extend(truth)
        n_reads = len(loci) * 2 * args.families * args.reads
        print(f"{n_reads:,} reads")

    # ── Mixture samples ───────────────────────────────────────────────────────
    # Mirrors the paper: 15 pairs × (1:1 + 1:9) = 30 mixture FASTQs.
    # families_per_allele is split between the two contributors according to
    # the mix ratio so that the total sequencing depth remains constant:
    #   1:1  → each contributor gets families // 2 families
    #   1:9  → contributor 1 gets families // 10,
    #           contributor 2 gets families - families // 10
    # (minimum 1 family per contributor to avoid empty files)
    if args.mix_pairs:
        print()
        for p in range(1, args.mix_pairs + 1):
            ind_a = 2 * p - 1   # individual indices within the already-generated set
            ind_b = 2 * p

            for ratio_label, fam_c1, fam_c2 in (
                ("1to1",
                 max(1, args.families // 2),
                 max(1, args.families - args.families // 2)),
                ("1to9",
                 max(1, args.families // 10),
                 max(1, args.families - args.families // 10)),
            ):
                mix_name = f"mix_{p:03d}_{ratio_label}"
                r1_path = os.path.join(args.out_dir, f"{mix_name}_R1.fq")
                r2_path = os.path.join(args.out_dir, f"{mix_name}_R2.fq")

                print(
                    f"  {mix_name}  "
                    f"(pair {ind_a}/{ind_b}, C1={fam_c1} fam, C2={fam_c2} fam) …",
                    end=" ", flush=True,
                )
                with open(r1_path, "w") as r1_fh, open(r2_path, "w") as r2_fh:
                    truth = simulate_mixture_sample(
                        sample_name=mix_name,
                        loci=loci,
                        common_seq=args.common,
                        umi_len=args.umi_len,
                        read_len=args.read_len,
                        families_c1=fam_c1,
                        families_c2=fam_c2,
                        reads_per_family=args.reads,
                        r1_error_rate=args.error_rate,
                        r2_error_rate=r2_error_rate,
                        stutter_rate=args.stutter_rate,
                        rng=rng,
                        r1_fh=r1_fh,
                        r2_fh=r2_fh,
                    )
                all_truth.extend(truth)
                n_reads = len(loci) * (fam_c1 + fam_c2) * 2 * args.reads
                print(f"{n_reads:,} reads")

    # ── Write ground truth ────────────────────────────────────────────────────
    # Single-source rows contain allele1/allele2 only.
    # Mixture rows additionally contain allele3/allele4 (contributor 2).
    single_source_fields = [
        "sample", "locus", "primer",
        "allele1_seq", "allele2_seq",
        "allele1_len", "allele2_len",
    ]
    mixture_fields = [
        "allele3_seq", "allele4_seq",
        "allele3_len", "allele4_len",
    ]
    all_fields = single_source_fields + mixture_fields

    with open(truth_path, "w", newline="") as fh:
        writer = csv.DictWriter(
            fh,
            fieldnames=all_fields,
            delimiter="\t",
            extrasaction="ignore",
        )
        writer.writeheader()
        for rec in all_truth:
            # Fill missing mixture columns with empty strings for single-source rows
            for col in mixture_fields:
                rec.setdefault(col, "")
            writer.writerow(rec)

    print()
    print(f"Simulation complete.")
    print(f"  Output directory : {args.out_dir}/")
    print(f"  Ground truth TSV : {truth_path}")
    print(f"  Common sequence  : {args.common}")
    print(f"  UMI length       : {args.umi_len}")
    print()
    print("Next: build samumi and run run_pipeline.sh")


if __name__ == "__main__":
    main()
