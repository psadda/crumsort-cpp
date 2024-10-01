import matplotlib.pyplot as plt
import numpy as np
from tqdm import tqdm
import fileinput

TESTS = {
    'random int' : 'random',
    'random order' : 'random high bits',
    'random half' : 'random half',
    'ascending order' : 'ascending',
    'descending order' : 'descending',
    'ascending saw' : 'ascending saw',
    'ascending tiles' : 'ascending tiles',
    'pipe organ' : 'pipe organ',
    'bit reversal' : 'bit reversal'
}

ALGORITHMS = {
    'crumsort' : 'crumsort (C)',
    'quadsort' : 'quadsort (C)',
    'cxcrumsort' : 'crumsort (C++)',
    'cxquadsort' : 'quadsort (C++)',
    'qsort' : 'qsort',
    'sort' : 'std::sort',
    'stablesort' : 'std::stable_sort',
    'pdqsort' : 'pdqsort',
    'timsort' : 'timsort',
    'skasort' : 'ska_sort',
    'rhsort' : 'rhsort',
    'simdsort' : 'x86-simd-sort (AVX2)',
    'vqsort' : 'vqsort'
}

EXPECTED_X_VALUES = [ 10, 100, 1000, 10000, 100000 ]

benchmark_results = {}

for line in fileinput.input():

    line = line.strip()

    if len(line) > 0 and  line[0] == '|' and line[-1] == '|': # This line actually has a table row

        # Parse the table row
        cells = line.split('|')[1:-1]      # Throw out empty first and last elements
        cells = [c.strip() for c in cells]
        if cells[0] == 'Name' or len(cells[0]) == 0 or cells[0][0] == '-': # Throw out header rows
            continue

        # Extract the benchmark results
        algorithm = cells[0]
        array_len = int(cells[1])
        time      = float(cells[4])
        test_name = cells[-1]

        if not algorithm in ALGORITHMS or not test_name in TESTS:
            continue
        algorithm = ALGORITHMS[algorithm]
        test_name = TESTS[test_name]

        # Store results in the global result table
        benchmark_results.setdefault(test_name, {})
        benchmark_results[test_name].setdefault(algorithm, {})
        benchmark_results[test_name][algorithm][array_len] = time

# Bar graphs
for test in tqdm(TESTS.values()):

    fig, ax = plt.subplots()

    labels = []
    values = []

    for algorithm in reversed(ALGORITHMS.values()):

        if not algorithm in benchmark_results[test]:
            continue

        labels.append(algorithm)
        values.append(benchmark_results[test][algorithm][10000])

    y_pos = np.arange(len(labels))
    plt.barh(y_pos, values, align='center')
    plt.yticks(y_pos, labels)
    plt.xlabel('run time (ns/value)')
    plt.title(test + ' (10,000 elements)')
    plt.tight_layout()

    fig.savefig(test + " 10000.png")
    plt.close()

# Line/scaling graphs
for test in tqdm(TESTS.values()):

    fig, ax = plt.subplots()
    ax.set_xscale('log')
    ax.set_yscale('log')

    ax.set(xlabel='array length', ylabel='run time (ns/value)', title=test)

    for algorithm in reversed(ALGORITHMS.values()):

        if not algorithm in benchmark_results[test]:
            continue

        times = benchmark_results[test][algorithm]
        array_lens = [key for key in times]
        array_lens.sort()
        datapoints = [times[array_len] for array_len in array_lens]
        datapoints = np.array(datapoints)

        ax.plot(array_lens, datapoints, label=algorithm)

    ax.legend()
    fig.savefig(test + ".png")
    plt.close()
