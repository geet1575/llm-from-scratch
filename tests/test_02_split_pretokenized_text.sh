#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PROMPT_DIR="${REPO_ROOT}/test_prompts"
REFERENCE_OUT="/tmp/reference.txt"
MAIN_OUT="/tmp/main.txt"

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <model-dir>"
    exit 1
fi
MODEL_DIR="$1"

if [[ -z "${MODEL_DIR}" ]]; then
    echo "Error: model directory path cannot be empty."
    exit 1
fi

if [[ ! -d "${MODEL_DIR}" ]]; then
    echo "Error: model directory '${MODEL_DIR}' does not exist."
    exit 1
fi

passed=0
failed=0
failed_tests=()

make -C "${REPO_ROOT}" build/main
build_status=$?

if [[ ${build_status} -ne 0 ]]; then
    echo "Failed to build build/main"
    exit "${build_status}"
fi

shopt -s nullglob
prompt_files=("${PROMPT_DIR}"/*.txt)

if [[ ${#prompt_files[@]} -eq 0 ]]; then
    echo "No test prompt files found in ${PROMPT_DIR}"
    exit 1
fi

for prompt_file in "${prompt_files[@]}"; do
    test_name="$(basename "${prompt_file}")"

    rm -f "${REFERENCE_OUT}" "${MAIN_OUT}"

    python3 "${REPO_ROOT}/reference/run.py" \
        --model-dir "${MODEL_DIR}" \
        --input-file "${prompt_file}" \
        --print-split-pretokenized-text \
        --output-file "${REFERENCE_OUT}"
    reference_status=$?

    "${REPO_ROOT}/build/main" \
        --input-file "${prompt_file}" \
        --print-split-pretokenized-text \
        --output-file "${MAIN_OUT}"
    main_status=$?

    if [[ ${reference_status} -ne 0 ]]; then
        echo "FAIL ${test_name}: reference implementation exited with ${reference_status}"
        failed=$((failed + 1))
        failed_tests+=("${test_name}")
        continue
    fi

    if [[ ${main_status} -ne 0 ]]; then
        echo "FAIL ${test_name}: build/main exited with ${main_status}"
        failed=$((failed + 1))
        failed_tests+=("${test_name}")
        continue
    fi

    if diff "${REFERENCE_OUT}" "${MAIN_OUT}" >/dev/null; then
        echo "PASS ${test_name}"
        passed=$((passed + 1))
    else
        echo "FAIL ${test_name}: Split pretokenized text differs"
        diff "${REFERENCE_OUT}" "${MAIN_OUT}"
        failed=$((failed + 1))
        failed_tests+=("${test_name}")
    fi
done

echo
echo "Split pretokenization test results:"
echo "  Passed: ${passed}"
echo "  Failed: ${failed}"

if [[ ${failed} -gt 0 ]]; then
    echo "Failed tests:"
    for failed_test in "${failed_tests[@]}"; do
        echo "  ${failed_test}"
    done
    exit 1
fi

exit 0
