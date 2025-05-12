import ctypes
from time import perf_counter as pfc
from typing import TypedDict
import sys
from glob import glob

num = 1000 - 141

class SeedData(TypedDict):
    seed: int
    bads: list[int]
    fbads: list[int]
    cash: list[float]
    
class Result(ctypes.Structure):
    seed: int
    fbads: ctypes.Array
    bads: ctypes.Array
    cash: ctypes.Array
    _fields_ = [
        ("seed", ctypes.c_uint32),
        ("fbads", ctypes.c_ubyte * num),
        ("bads", ctypes.c_ubyte * num),
        ("cash", ctypes.c_double * num),
    ]

class DatabaseFile(ctypes.Structure):
    _fields_ = [
        ("file", ctypes.c_void_p),
        ("buffer", ctypes.POINTER(ctypes.c_ubyte)),
        ("buffersize", ctypes.c_size_t),
        ("index", ctypes.c_size_t),
    ]

class Reader(ctypes.CDLL):
    openFile: "ctypes._FuncPointer"
    closeFile: "ctypes._FuncPointer"
    allocateResults: "ctypes._FuncPointer"
    freeptr: "ctypes._FuncPointer"
    read: "ctypes._FuncPointer"
    readSeeds: "ctypes._FuncPointer"
    freeResults: "ctypes._FuncPointer"

paths = glob("./*readerCDLL*.*")
if len(paths) > 1:
    raise RuntimeError("More than one CDLL path was found")

reader = Reader(paths[0])
reader.openFile.argtypes = [ctypes.POINTER(DatabaseFile), ctypes.c_char_p, ctypes.c_char_p, ctypes.c_size_t]
reader.openFile.restype = ctypes.c_int
reader.closeFile.argtypes = [ctypes.POINTER(DatabaseFile)]
reader.closeFile.restype = None
reader.allocateResults.argtypes = []
reader.allocateResults.restype = ctypes.POINTER(Result)
reader.freeptr.argtypes = [ctypes.c_void_p]
reader.freeptr.restype = None
reader.freeResults.argtypes = [ctypes.POINTER(Result)]
reader.freeResults.restype = None
reader.read.argtypes = [ctypes.POINTER(DatabaseFile), ctypes.POINTER(Result)]
reader.read.restype = ctypes.c_int
reader.readSeeds.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
reader.readSeeds.restype = ctypes.c_void_p

def read(file: DatabaseFile) -> list[SeedData]:
    result_ptr = reader.allocateResults()
    res = reader.read(file, result_ptr)
    if res:
        raise RuntimeError("Error while calling ctypes library")
    results: list[Result] = result_ptr[:100]
    all_results = []
    for result in results:
        data: SeedData = {
            "seed": result.seed,
            "fbads": result.fbads[:],
            "bads": result.bads[:],
            "cash": result.cash[:]
        }
        all_results.append(data)
    reader.freeptr(ctypes.cast(result_ptr, ctypes.c_void_p))
    return all_results

def readSeeds(low: int, high: int):
    return_ptr = reader.readSeeds(ctypes.c_uint32(low), ctypes.c_uint32(high))
    if return_ptr is not None:
        results_ptr = ctypes.cast(return_ptr, ctypes.POINTER(Result))
        results: list[Result] = results_ptr[:(high-low+1)]
        all_results = []
        for result in results:
            data: SeedData = {
                "seed": result.seed,
                "fbads": result.fbads[:],
                "bads": result.bads[:],
                "cash": result.cash[:]
            }
            all_results.append(data)
        reader.freeResults(results_ptr)
        return all_results
    else:
        print("NULL ptr error", file=sys.stderr)

def main():
    s1, s2 = 99, 1010
    start = pfc()
    data = readSeeds(s1, s2)
    end = pfc()
    print(f"Took {end-start}s to read {s2 - s1 + 1} seeds")
    start = pfc()
    file = DatabaseFile()
    res = reader.openFile(ctypes.pointer(file), "database-results/thread-0/seeds_0-999999.bin".encode(), "rb".encode(), 355538)
    if res:
        raise RuntimeError("Error opening file")
    results = read(file)
    results2 = read(file)
    reader.closeFile(file)
    end = pfc()
    #print(results[0])
    print(f"took {end-start}s")

if __name__ == '__main__':
    main()