#!/usr/bin/env python3

with open("excludelist", 'r') as f:
    exlist = f.readlines()

exset = set()

for line in exlist:
    if not line.startswith("#") and line.find(".so") > 0:
        l = line[:line.find("#")]
        l = l.strip()
        exset.add(l)

with open("ldd_paths.txt", 'r') as f:
    ldd_paths = f.readlines()

ldd_libs = set()
lib_paths = dict()

for line in ldd_paths:
    l = line.strip()
    lib = l[l.rfind("/")+1:]
    path = l
    ldd_libs.add(lib)
    lib_paths[lib] = path

passlibs = ldd_libs - exset
# print("LEN ORIG:", len(ldd_libs))
# print("LEN PASS:", len(passlibs))
# print(len(ldd_libs) - len(passlibs), "REMOVED")
# print(passlibs)

with open("passlibs.txt", "w") as f:
    for lib in passlibs:
        print(lib, "=>" ,lib_paths[lib])
        f.write(lib_paths[lib] + "\n")

