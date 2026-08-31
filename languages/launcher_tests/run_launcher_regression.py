#!/usr/bin/env python3

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def parse_args():
    parser = argparse.ArgumentParser(description="Run a launcher regression fixture.")
    parser.add_argument("--language", required=True, choices=["lua", "squirrel"])
    parser.add_argument("--mode", required=True, choices=["folder", "legacy"])
    parser.add_argument("--expected-source", required=True, choices=["folder", "legacy"])
    parser.add_argument("--launcher", required=True)
    parser.add_argument("--fixture-root", required=True)
    parser.add_argument("--work-dir", required=True)
    parser.add_argument("--packer")
    return parser.parse_args()


def recreate_dir(path: Path):
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True)


def run_command(argv, cwd):
    result = subprocess.run(argv, cwd=str(cwd), capture_output=True, text=True, encoding="utf-8", errors="replace")
    if result.returncode != 0:
        if result.stdout:
            sys.stdout.write(result.stdout)
        if result.stderr:
            sys.stderr.write(result.stderr)
        raise RuntimeError(f"command failed with exit code {result.returncode}: {' '.join(argv)}")
    return result


def prepare_fixture(args, work_dir: Path):
    fixture_data = Path(args.fixture_root) / "data"
    if not fixture_data.is_dir():
        raise RuntimeError(f"missing fixture data directory: {fixture_data}")

    if args.mode == "folder":
        shutil.copytree(fixture_data, work_dir / "data")
        return

    if not args.packer:
        raise RuntimeError("legacy mode requires --packer")

    pack_src = work_dir / "pack_src"
    shutil.copytree(fixture_data, pack_src)
    archive_path = work_dir / "data.gsa"
    run_command([args.packer, "pack", "-f", str(pack_src), str(archive_path)], work_dir)


def main():
    args = parse_args()

    work_dir = Path(args.work_dir)
    recreate_dir(work_dir)
    prepare_fixture(args, work_dir)

    result = subprocess.run([args.launcher], cwd=str(work_dir), capture_output=True, text=True, encoding="utf-8", errors="replace")
    if result.stdout:
        sys.stdout.write(result.stdout)
    if result.stderr:
        sys.stderr.write(result.stderr)

    if result.returncode != 0:
        raise RuntimeError(f"launcher returned exit code {result.returncode}")

    expected_line = f"launcher-regression-ok language={args.language} source={args.expected_source}"
    if expected_line not in result.stdout:
        raise RuntimeError(f"missing expected output line: {expected_line}")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"launcher regression failed: {exc}", file=sys.stderr)
        sys.exit(1)
