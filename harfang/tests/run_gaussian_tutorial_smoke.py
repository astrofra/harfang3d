#!/usr/bin/env python3
"""Exercise preloading, GUI switching and cleanup with an installed or built Lua runtime."""
import argparse
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lua', type=Path, required=True)
    parser.add_argument('--module', type=Path, help='harfang module directory; defaults to the Lua executable directory')
    args = parser.parse_args()
    lua = args.lua.resolve()
    module = args.module.resolve() if args.module else lua.parent
    repo = Path(__file__).resolve().parents[2]
    runner = Path(__file__).with_name('gaussian_splatting_runner.lua').resolve()
    result = subprocess.run([str(lua), str(runner), str(module), 'gaussian_splatting.lua', '120'],
                            cwd=repo / 'tutorials', text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=60)
    if result.returncode or 'PASS actual tutorial' not in result.stdout:
        raise RuntimeError(result.stdout)
    print(result.stdout.splitlines()[-1])


if __name__ == '__main__':
    main()
