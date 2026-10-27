#!/usr/bin/env python3
"""Build/compile/run the opt-in M0 probe; preserve evidence under the build tree."""

import argparse
import hashlib
import json
import platform
from pathlib import Path
import shutil
import subprocess
import sys


def executable(directory, name, config):
    suffix = ".exe" if sys.platform == "win32" else ""
    candidates = [directory / (name + suffix), directory / config / (name + suffix)]
    for path in candidates:
        if path.is_file():
            return path
    raise RuntimeError(f"Missing {name}: checked {candidates}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True, help="Existing configured HARFANG CMake build with HG_BUILD_TESTS/ASSETC enabled")
    parser.add_argument("--renderer", choices=["metal", "dx11", "gl"], required=True)
    parser.add_argument("--config", default="Release")
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--compile-only", action="store_true", help="Compile assets only; does not qualify runtime support")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    build = args.build_dir.resolve()
    if not (build / "CMakeCache.txt").is_file():
        parser.error("--build-dir must contain a configured CMake build")
    output = build / "compute_probe" / args.renderer
    output.mkdir(parents=True, exist_ok=True)
    report = {"renderer_requested": args.renderer, "host": platform.platform(), "config": args.config,
              "compile_only": args.compile_only, "status": "failed", "steps": []}

    def run(label, command, expected=0, marker=None):
        print(f"Running {label}...", flush=True)
        try:
            result = subprocess.run([str(x) for x in command], cwd=repo, text=True, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, timeout=600 if label == "build" else 60)
        except subprocess.TimeoutExpired as error:
            partial = error.stdout or b""
            if isinstance(partial, bytes):
                partial = partial.decode("utf-8", errors="replace")
            (output / f"{label}.log").write_text(partial + "\nTIMEOUT: process terminated\n", encoding="utf-8")
            report["steps"].append({"name": label, "timeout": True, "log": f"{label}.log"})
            raise RuntimeError(f"{label} timed out; see {output / (label + '.log')}") from error
        (output / f"{label}.log").write_text(result.stdout, encoding="utf-8")
        report["steps"].append({"name": label, "command": [str(x) for x in command], "exit_code": result.returncode,
                                "expected_exit_code": expected, "log": f"{label}.log"})
        lines = result.stdout.splitlines()
        if len(lines) > 80:
            print(f"Full output: {output / (label + '.log')}", flush=True)
        print("\n".join(lines[-80:]), flush=True)
        if result.returncode != expected or (marker and marker not in result.stdout):
            raise RuntimeError(f"{label} failed (exit {result.returncode}, expected {expected}); see {output / (label + '.log')}")

    try:
        if not args.skip_build:
            run("build", ["cmake", "--build", build, "--config", args.config, "--target", "test_compute_probe", "assetc", "shaderc", "--parallel", "4"])
        assetc = executable(build / "tools/assetc", "assetc", args.config)
        shaderc = executable(build / "extern/bgfx", "shaderc", args.config)
        probe = executable(build / "harfang/tests", "test_compute_probe", args.config)
        source, compiled = output / "source", output / "compiled"
        source.mkdir(exist_ok=True)
        compiled.mkdir(exist_ok=True)
        for path in (repo / "harfang/tests/data/compute_probe").iterdir():
            if path.is_file():
                shutil.copyfile(path, source / path.name)
        for name in ["bgfx_shader.sh", "bgfx_compute.sh"]:
            shutil.copyfile(repo / "tutorials/resources/core/shader" / name, source / name)
        report["source_sha256"] = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(source.iterdir()) if p.is_file()}
        report["executable_sha256"] = hashlib.sha256(probe.read_bytes()).hexdigest()
        api = {"metal": "MTL", "dx11": "DX11", "gl": "GL"}[args.renderer]
        target = "osx" if args.renderer == "metal" else "windows" if args.renderer == "dx11" else "linux"
        run("assetc", [assetc, "-api", api, "-platform", target, "-toolchain", shaderc.parent, source, compiled])
        for name in ["producer.sc", "consumer.sc", "display.vsb", "display.fsb"]:
            path = compiled / name
            if not path.is_file() or path.stat().st_size == 0:
                raise RuntimeError(f"Missing compiled shader {path}; assetc may have skipped a compiler")
        if args.compile_only:
            report["status"] = "compiled_only_runtime_unverified"
        else:
            run("positive", [probe, args.renderer, compiled], marker="PASS compute probe")
            run("flip-before", [probe, args.renderer, compiled, "flip-before"], marker="PASS compute probe")
            # A stale previous dispatch must produce a pixel failure, not a setup error/crash.
            for control in ["skip-producer", "skip-consumer"]:
                run(control, [probe, args.renderer, compiled, control], expected=1, marker="FAIL pixel mismatch")
            report["status"] = "passed"
        return 0
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        report["error"] = str(error)
        print(str(error), file=sys.stderr)
        return 1
    finally:
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"Evidence: {output / 'report.json'}", flush=True)


if __name__ == "__main__":
    sys.exit(main())
