#!/usr/bin/env bash
# run_pipeline.sh – Build samumi (if needed) then run the full SamUMI pipeline
# on the simulated data produced by simulate_reads.py.
#
# Prerequisites:
#   g++, zlib-dev, python3
#
# Usage:
#   bash run_pipeline.sh [--no-simulate] [--samples N] [--families N] [--reads N]
#
# Output (all inside sim_data/):
#   *_fuzzfind.tsv, *_fampack.tsv, *_hammrg.tsv, *_famsum.tsv, *_haplo.tsv
#   combined_umi.tsv, combined_haplo.tsv  – ready for random forest training
#   *.png                                 – evaluation plots

set -euo pipefail

# ── Defaults ──────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/source_c/builds/bin_x64_linux"
SAMUMI="${BUILD_DIR}/samumi"
PRIMER="${SCRIPT_DIR}/example_anchor/PrimedAnchors.txt"
DATA_DIR="${SCRIPT_DIR}/sim_data"
PY_HAPLO="${SCRIPT_DIR}/source_py/UMIHaploCollect.py"
PY_LABEL="${SCRIPT_DIR}/label_and_plot.py"

COMMON_SEQ="GAAACAGGATTAGATACCCT"
UMI_LEN=12
RUN_TYPE="SIM"
HAM=1
PFUZZ=2
AFUZZ=2
CFUZZ=1

DO_SIMULATE=1
SAMPLES=20
FAMILIES=10
READS=8

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-simulate)  DO_SIMULATE=0 ;;
        --samples)      SAMPLES="$2";  shift ;;
        --families)     FAMILIES="$2"; shift ;;
        --reads)        READS="$2";    shift ;;
        --data-dir)     DATA_DIR="$2"; shift ;;
        --common)       COMMON_SEQ="$2"; shift ;;
        --umi-len)      UMI_LEN="$2";  shift ;;
        --run-type)     RUN_TYPE="$2"; shift ;;
        --ham)          HAM="$2";      shift ;;
        *)  echo "Unknown argument: $1" >&2; exit 1 ;;
    esac
    shift
done

# ── Step 0: Build samumi ───────────────────────────────────────────────────────
if [[ ! -f "${SAMUMI}" ]]; then
    echo "=== Building samumi ==="
    pushd "${SCRIPT_DIR}/source_c" > /dev/null
    bash Buildit_x64_linux.sh
    popd > /dev/null
    echo "Build complete: ${SAMUMI}"
    echo
fi

# ── Step 1: Simulate reads ─────────────────────────────────────────────────────
if [[ "${DO_SIMULATE}" -eq 1 ]]; then
    echo "=== Simulating reads ==="
    python3 "${SCRIPT_DIR}/simulate_reads.py" \
        --primer   "${PRIMER}" \
        --common   "${COMMON_SEQ}" \
        --umi-len  "${UMI_LEN}" \
        --samples  "${SAMPLES}" \
        --families "${FAMILIES}" \
        --reads    "${READS}" \
        --out-dir  "${DATA_DIR}"
    echo
fi

if [[ ! -f "${DATA_DIR}/truth.tsv" ]]; then
    echo "ERROR: ${DATA_DIR}/truth.tsv not found. Run without --no-simulate first." >&2
    exit 1
fi

# ── Step 2: Run pipeline for every sample ─────────────────────────────────────
echo "=== Running SamUMI pipeline ==="

shopt -s nullglob
R1_FILES=("${DATA_DIR}"/sample_*_R1.fq)
if [[ ${#R1_FILES[@]} -eq 0 ]]; then
    echo "ERROR: No sample_*_R1.fq files found in ${DATA_DIR}" >&2
    exit 1
fi

for R1 in "${R1_FILES[@]}"; do
    SAMPLE="$(basename "${R1}" _R1.fq)"
    R2="${DATA_DIR}/${SAMPLE}_R2.fq"

    if [[ ! -f "${R2}" ]]; then
        echo "  SKIP ${SAMPLE}: missing R2 file"
        continue
    fi

    FUZZ_OUT="${DATA_DIR}/${SAMPLE}_fuzzfind.tsv"
    PACK_OUT="${DATA_DIR}/${SAMPLE}_fampack.tsv"
    HAMM_OUT="${DATA_DIR}/${SAMPLE}_hammrg.tsv"
    SUM_OUT="${DATA_DIR}/${SAMPLE}_famsum.tsv"
    HAP_OUT="${DATA_DIR}/${SAMPLE}_haplo.tsv"

    echo "  Processing ${SAMPLE} …"

    # 2a. fuzzfind: locate primer / anchor / UMI / common in raw reads
    "${SAMUMI}" fuzzfind \
        --forw   "${R1}" \
        --back   "${R2}" \
        --primer "${PRIMER}" \
        --common "${COMMON_SEQ}" \
        --umi    "${UMI_LEN}" \
        --pfuzz  "${PFUZZ}" \
        --afuzz  "${AFUZZ}" \
        --cfuzz  "${CFUZZ}" \
        --out    "${FUZZ_OUT}"

    # 2b. fampack: sort reads into UMI families
    "${SAMUMI}" fampack \
        --in  "${FUZZ_OUT}" \
        --out "${PACK_OUT}"

    # 2c. hammrg: merge nearby UMI families (Hamming ≤ HAM)
    "${SAMUMI}" hammrg \
        --in  "${PACK_OUT}" \
        --ham "${HAM}" \
        --out "${HAMM_OUT}"

    # 2d. famsum: summarise each merged family (allele calls + quality metrics)
    "${SAMUMI}" famsum \
        --in     "${HAMM_OUT}" \
        --primer "${PRIMER}" \
        --type   "${RUN_TYPE}" \
        --ham    "${HAM}" \
        --out    "${SUM_OUT}"

    # 2e. UMIHaploCollect: aggregate families into per-allele haplotype table
    python3 "${PY_HAPLO}" < "${SUM_OUT}" > "${HAP_OUT}"
done

echo

# ── Step 3: Add ground-truth labels and produce plots ─────────────────────────
echo "=== Labelling results and generating evaluation plots ==="
python3 "${PY_LABEL}" \
    --data-dir "${DATA_DIR}" \
    --primer   "${PRIMER}"

echo
echo "=== Pipeline complete ==="
echo "  Data directory     : ${DATA_DIR}/"
echo "  Combined UMI data  : ${DATA_DIR}/combined_umi.tsv"
echo "  Combined hap data  : ${DATA_DIR}/combined_haplo.tsv"
echo "  Plots              : ${DATA_DIR}/*.png"
echo
echo "To train random forest models (requires R + ranger + tidyverse):"
echo "  Rscript source_r/UMIRanForTrain.R  ${DATA_DIR}/umi_model.rds  3 ${DATA_DIR}/combined_umi.tsv"
echo "  Rscript source_r/HapRanForTrain.R  ${DATA_DIR}/hap_model.rds  3 ${DATA_DIR}/combined_haplo.tsv"
