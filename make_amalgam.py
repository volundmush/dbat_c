from pathlib import Path

f = open("amalgam.c", mode="w")

for entry in sorted(Path("include").iterdir(), key=lambda x: x.name):
    if entry.name.endswith(".h"):
        f.write(f'#include "{entry.name}"\n')

for entry in sorted(Path("src").iterdir(), key=lambda x: x.name):
    if entry.name.endswith(".c"):
        f.write(f'#include "src/{entry.name}"\n')

f.close()