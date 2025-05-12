from io import BufferedReader
from sys import byteorder
import ctypes
import struct
import zlib
from typing import TypedDict
from time import perf_counter as pfc

sizeof_size_t = ctypes.sizeof(ctypes.c_size_t)

def readChunk(file: BufferedReader) -> int:
    size = file.read(sizeof_size_t)
    data = file.read(struct.unpack("@N", size)[0])
    return int.from_bytes(zlib.decompress(data), byteorder)

class SeedData(TypedDict):
    seed: int
    bads: list[int]
    fbads: list[int]
    cash: list[float]

umax = lambda x: 2 ** x - 1
u6 = umax(6)
u21 = umax(21)
u32 = umax(32)

def processChunk(data: int) -> list[SeedData]:
    all_seeds: list[SeedData] = []
    while data:
        seed = data & u32
        data >>= 32
        this_seed: SeedData = {
            "seed": seed,
            "bads": [],
            "fbads": [],
            "cash": []
        }
        for i in range(141, 1000):
            fbads = data & u6
            data >>= 6
            bads = data & u6
            data >>= 6
            cash50 = data & u21
            data >>= 21
            this_seed["bads"].append(bads)
            this_seed["fbads"].append(fbads)
            this_seed["cash"].append(cash50 / 50)
        all_seeds.append(this_seed)
    return all_seeds

def main():
    with open("database-results/thread-0/seeds_0-999.bin", 'rb') as file:
        chunk = readChunk(file)
        start = pfc()
        data1 = processChunk(chunk)
        end = pfc()
        print(f"took {end-start}s")
        pass
    with open("database-results/thread-0/seeds_0-999.bin", 'rb') as file:
        chunk = readChunk(file)
        start = pfc()
        data2 = processChunk(chunk)
        end = pfc()
        print(f"took {end-start}s")
        pass
    eq = data1 == data2
    print(eq)
    pass

if __name__ == '__main__':
    main()
