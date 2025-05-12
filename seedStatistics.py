import math
import re
import struct
import sys
import matplotlib.pyplot as plt

def stats_from_freq_dict(freq_dict: dict[int|float, int]):
    sorted_items = sorted(freq_dict.items())
    
    total_count = sum(freq_dict.values())
    
    total_sum = sum(val * freq for val, freq in sorted_items)
    mean = total_sum / total_count

    variance = sum(freq * ((val - mean) ** 2) for val, freq in sorted_items) / total_count
    std_dev = math.sqrt(variance)

    midpoint = total_count / 2
    cumulative = 0
    median = None
    for val, freq in sorted_items:
        cumulative += freq
        if cumulative >= midpoint:
            if total_count % 2 == 1:
                median = val
                break
            else:
                if cumulative - freq < midpoint < cumulative:
                    median = val
                else:
                    next_val = next(v for v, f in sorted_items if cumulative - freq < midpoint < cumulative + f)
                    median = (val + next_val) / 2
                break

    return mean, median, std_dev

def combine_dicts(d1: dict, d2: dict) -> dict:
    d = d1.copy()
    for k, v in d2.items():
        if k in d:
            d[k] += v
        else:
            d[k] = v
    return d

def main():
    with open("statistics-results/thread-0/round-141_cash.bin", 'rb') as f:
        data = {}
        bytes = f.read(8)
        while bytes:
            count = int.from_bytes(bytes[:4], sys.byteorder)
            cash50 = int.from_bytes(bytes[4:], sys.byteorder)
            data[cash50/50] = count
            bytes = f.read(8)

    mean, median, std_dev = stats_from_freq_dict(data)
    sorted_items = sorted(data.items())
    bins, counts = list(zip(*sorted_items))
    print(mean, median, std_dev)
    mv = bins[counts.index(max(counts))]
    print(mv)
    plt.scatter(bins, counts)
    plt.xlabel("r141 cash")
    plt.ylabel("frequency (from 100k seeds)")
    plt.show()

if __name__ == '__main__':
    main()