#!/usr/bin/env python
from pathlib import Path
from yaml import dump
import re

ACMD = re.compile(r"^\s*(extern\s+)?(ACMD|SPECIAL)\((?P<funcname>\w+)\)\s*;\s*$")


def reader(files):
    names = set()

    for f in files:
        with f.open(mode="r") as r:
            for l in r.readlines():
                if (a := ACMD.match(l)):
                    names.add(a.group("funcname"))

    return names


def make_section(name: str):
    return

def main():
    files = list()
    include = Path("include")
    src = Path("src")
    files.extend(include.glob("*.h"))
    files.extend(src.glob("*.c"))

    idents = list()
    for name in reader(files):
        idents.append({"name": name,
                       "fields": [
                           {"name": "argument",
                            "type": "string"}
                       ]})

    out = {"ident": idents}
    dumped = dump(out)
    print(dumped)



if __name__ == "__main__":
    main()