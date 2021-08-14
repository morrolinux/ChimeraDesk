#!/usr/bin/env python3

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("-i", default="ldd_libs.txt", help="input list of linked libraries")
parser.add_argument("-e", default="excludelist", help="list of linked libraries to be excluded")
parser.add_argument("-o", default="pass_libs.txt", help="output filtered of the exclusion list")
args = parser.parse_args()

# Read exclusion list
with open(args.e, 'r') as f:
    exlist = f.readlines()

exset = set()

for line in exlist:
    if not line.startswith("#") and line.find(".so") > 0:
        l = line[:line.find("#")]
        l = l.strip()
        exset.add(l)

# Read input (ldd) libs
with open(args.i, 'r') as f:
    ldd_paths = f.readlines()

ldd_libs = set()
lib_paths = dict()

for line in ldd_paths:
    l = line.strip()
    lib = l[l.rfind("/")+1:]
    path = l
    ldd_libs.add(lib)
    lib_paths[lib] = path

# Filter out libs
passlibs = ldd_libs - exset

# print("LEN ORIG:", len(ldd_libs))
# print("LEN PASS:", len(passlibs))
# print(len(ldd_libs) - len(passlibs), "REMOVED")
# print(passlibs)

# Write results to file
with open(args.o, "w") as f:
    for lib in passlibs:
        print(lib, "=>" ,lib_paths[lib])
        f.write(lib_paths[lib] + "\n")

