# Harfang Python Wheel and PyPI Restoration Feasibility

Date: 2026-04-17

## Executive Summary

Restoring the HARFANG Python module as a modern PyPI-distributed wheel is
feasible, but it should be treated as a packaging and release engineering
project rather than as a small build-system fix.

The repository already contains the essential technical pieces:

- A CMake-controlled CPython binding target, `hg_python`.
- FABGen-based binding generation.
- A `languages/hg_python/pip` package directory with a `pyproject.toml`.
- Existing logic to install the extension module, runtime shared libraries,
  and command-line tools into a Python package.
- An existing PyPI project named `harfang`.

However, the current packaging path is stale relative to modern Python
packaging expectations:

- It mixes a PEP 517-style `pip` package path with an older CMake-driven
  `bdist_wheel` path.
- The last public PyPI release, `harfang 3.2.7`, was published on 2023-08-14
  and only exposes Windows wheels.
- The published Windows wheels are tagged `py3-none-win32` and
  `py3-none-win_amd64`, which is not the correct semantic tag for a CPython
  extension module using the Python C API. A Stable ABI build should use an
  `abi3` ABI tag, while a non-Stable-ABI build should use a CPython
  version-specific tag.
- The build script can clone FABGen from the network during the build, which
  is a reproducibility and downstream packaging problem.
- Documentation and package metadata still claim very old Python compatibility
  such as CPython 3.2+, 3.4+, or `python_requires = >3.6`.
- There is no CI release workflow in `.github` to build, repair, test, attest,
  and upload wheels for the supported platforms.

The recommended path is to modernize the Python package around a single PEP
517 build entry point, preferably using `scikit-build-core` or a similarly
current CMake-aware backend, and to publish a limited first platform matrix
before expanding. A minimal but credible PyPI restoration should target CPython
3.11+ with `abi3` wheels if the generated binding is verified to use only the
Limited API. If that verification fails, the fallback is version-specific
CPython wheels.

## Feasibility Verdict

Feasible with medium-to-high integration risk.

The highest risk is not the existence of the CPython binding. The binding
target exists. The risks are in proving ABI compatibility, producing correct
platform wheels, bundling or excluding runtime dependencies cleanly, removing
network access from the build, and creating a repeatable CI release pipeline.

Recommended first target:

- CPython 3.11+.
- Windows x86_64.
- Linux x86_64 with a compliant `manylinux_*` tag.
- Source distribution built without network access.
- `import harfang` smoke test from an installed wheel.
- `python -m harfang.bin assetc --help` or equivalent tool smoke test.
- TestPyPI publication before production PyPI.

Recommended second target:

- macOS x86_64 and arm64.
- Linux aarch64.
- Optional Windows ARM64 if there is demand and CI coverage.
- Broader runtime tests covering window creation, asset compilation, and a
  minimal rendering loop where the CI environment allows it.

Rough estimate:

- Minimal restoration for Windows x86_64 and Linux x86_64: 4 to 6
  engineer-weeks if the current CMake build is healthy on both platforms.
- Production-grade multi-platform publishing with macOS, dependency repair,
  CI hardening, TestPyPI validation, and documentation updates: 8 to 12
  engineer-weeks.
- Add 2 to 4 weeks if ABI3 validation fails and the project must ship
  per-Python-version wheels for every target platform.

## Current Repository State

### CMake Binding Target

The top-level `CMakeLists.txt` defines `HG_BUILD_HG_PYTHON` and uses
`find_package(Python3 COMPONENTS Interpreter Development REQUIRED)` when that
option is enabled. The actual CPython module is defined in
`languages/hg_python/CMakeLists.txt`.

Observed behavior:

- FABGen generates `bind_CPython.cpp`, `bind_CPython.h`, and `fabgen.h`.
- The shared library target is named `hg_python`.
- Windows output is named `harfang.pyd`.
- Non-Windows output is named `harfang` with no `lib` prefix; macOS explicitly
  uses the `.so` suffix.
- The target defines `Py_LIMITED_API=0x03020000`.
- Windows manually rewrites `Python3_LIBRARIES` from `python3X` to `python3`
  because the build is trying to use the Stable ABI.
- Packaging selects either `languages/hg_python/pip` or
  `languages/hg_python/bdist_wheel` depending on `HG_PYTHON_PIP`.

This is good prior work, but the Stable ABI wiring should be modernized. The
preferred CMake shape is to use the modern Python Stable ABI module component
where available, or a build backend that backports it, rather than mutating
library paths manually.

### `languages/hg_python/pip`

This is the most relevant current packaging path.

It contains:

- `pyproject.toml` using `setuptools.build_meta`.
- `setup.py` using `cmake-build-extension`.
- `setup.cfg` with `python_requires = >3.6` and console script entry points.
- `CMakeLists.txt` that installs the `hg_python` target and C++ SDK
  dependencies under the package prefix.

Important behavior in `setup.py`:

- It supports `python -m build --sdist --outdir dist languages/hg_python/pip`.
- It can build a binary package through `setup.py bdist_wheel`.
- It writes `commit_id` into the source tree.
- It initializes git submodules from the local repository metadata.
- It clones `https://github.com/ejulien/FABGen.git` into `extern/fabgen` if
  the directory is missing.
- It packages `assetc`, `assimp_converter`, `gltf_exporter`, and
  `gltf_importer` as exposed binaries.

This path is close enough to be a useful migration base, but it still has too
much custom build-time behavior for a reliable PyPI release.

### `languages/hg_python/bdist_wheel`

This is the older package construction path.

It copies a small package skeleton into the CMake binary directory, installs
the extension and selected dependencies, runs `setup.py bdist_wheel`, then
copies the resulting wheel into the install tree.

Observed issues:

- On non-Windows it passes `--py-limited-api=cp32`.
- On Windows it disables that flag and renames the wheel to
  `harfang-<version>-py3-none-<platform>.whl`.
- The rename makes the wheel install broadly, but it also hides the fact that
  the wheel contains a CPython extension.
- The static `bin/__main__.py` package has command-line module behavior, but
  the `pip/setup.cfg` console scripts point to `harfang.bin.__main__:main`,
  while the generated module does not define a `main` function. The documented
  `python -m harfang.bin assetc ...` path is safer than the console script path
  today.

This path should not be the basis for new PyPI publication. Keep it only as
historical reference while the modern package path is rebuilt.

### Documentation and Metadata

Current documentation still says that Harfang for Python is compatible with
official CPython 3.4 or newer, and the README still references CPython 3.2+.
Those claims are no longer appropriate for a 2026 PyPI package.

As of 2026-04-17:

- Python 3.9 is end-of-life.
- Python 3.10 is in security-only support and reaches end-of-life in 2026-10.
- Python 3.11 and 3.12 are security-only.
- Python 3.13 and 3.14 are in bugfix support.

The new package metadata should make an explicit support decision. A pragmatic
choice is `requires-python = ">=3.11"` for new releases. If 3.10 customers must
be supported, use `>=3.10` as a transitional policy and plan to drop it soon.

### Existing PyPI Project

The `harfang` project already exists on PyPI and is maintained by the
`harfang3d` account.

Observed public state on 2026-04-17:

- Latest release: `3.2.7`.
- Release date: 2023-08-14.
- Source distribution: `harfang-3.2.7.tar.gz`, 6.6 MB.
- Built distributions:
  - `harfang-3.2.7-py3-none-win_amd64.whl`, 14.2 MB.
  - `harfang-3.2.7-py3-none-win32.whl`, 12.3 MB.
- No Linux or macOS wheels are published for `3.2.7`.
- Uploads were made with `twine/4.0.1`, not Trusted Publishing.

Because PyPI does not allow reusing an existing release file, restoration must
publish a new version, for example `3.2.8` or a deliberate `3.3.0`.

## Current Python Packaging Standards To Target

### Project Metadata

Use `pyproject.toml` as the canonical build and metadata entry point.

The modern package should define:

- `[build-system]` with a PEP 517 build backend.
- `[project]` metadata from PEP 621.
- `requires-python`.
- `readme` with a valid content type.
- `license` and `license-files`, updated for current metadata expectations.
- `classifiers` matching the actual supported OS and Python versions.
- `project.urls`.
- `project.scripts` only if the script target actually exposes a callable.

The existing `setup.py` can remain temporarily as an implementation detail only
if needed, but the release interface should be `python -m build`, not
`python setup.py bdist_wheel`.

### Wheel Tags

Wheel filenames include three compatibility tags:

- Python tag.
- ABI tag.
- Platform tag.

For HARFANG, `py3-none-*` should not be used for wheels that contain the
CPython extension module.

Valid choices are:

- Stable ABI path: `cp311-abi3-win_amd64`,
  `cp311-abi3-manylinux_2_28_x86_64`, and equivalent platform tags.
- Version-specific path: `cp311-cp311-*`, `cp312-cp312-*`,
  `cp313-cp313-*`, etc.

The Stable ABI path is attractive because it reduces the number of wheels. It
is only acceptable if the generated binding and all Python-facing C API usage
are validated against the Limited API. Defining `Py_LIMITED_API` is necessary,
but it is not sufficient proof by itself.

### Source Distribution

The source distribution should be self-contained enough to build the package
from source in a normal PEP 517 environment.

For HARFANG this means:

- Do not clone FABGen during the build.
- Pin FABGen as a submodule, vendored source directory, release archive, or
  externally provisioned dependency with a clear error if missing.
- Include the source files needed by the CMake build.
- Include license files and dependency manifests.
- Avoid generating `commit_id` by mutating the source tree during a build.
- Validate by building wheels from the generated sdist, not only from the git
  checkout.

### Linux Wheels

Linux wheels for PyPI should be `manylinux_*` or `musllinux_*` wheels, not
plain `linux_x86_64` wheels.

The immediate recommendation is:

- Build inside a manylinux container through `cibuildwheel`.
- Run the repair step through `auditwheel`.
- Treat audit failures as release blockers, not warnings.
- Decide whether dependencies such as OpenAL, GLFW, Lua, OpenVR, GTK/X11
  transitive dependencies, and tool binaries are bundled, statically linked,
  or intentionally excluded.

The existing setup notes mention Linux development packages such as
`uuid-dev`, `libreadline-dev`, `libxml2-dev`, and `libgtk-3-dev`. Those are
build-time clues, not a release strategy. The release strategy must be proven
by `auditwheel show` and `auditwheel repair`.

### Windows Wheels

Windows should be the first restoration target because historical wheels
already exist.

Required changes:

- Stop renaming binary wheels to `py3-none-win*`.
- Use an `abi3` tag only if the Stable ABI path is validated.
- Bundle non-system DLLs or repair the wheel with `delvewheel`.
- Decide whether to keep 32-bit Windows. Dropping `win32` is reasonable unless
  there is a known customer requirement.
- Verify that the Microsoft Visual C++ runtime expectations are documented or
  bundled according to current practice.

### macOS Wheels

macOS support is feasible, but it should be second wave unless there is an
urgent need.

Required work:

- Build x86_64 and arm64 wheels, or a universal2 wheel if the size tradeoff is
  acceptable.
- Set an explicit `MACOSX_DEPLOYMENT_TARGET`.
- Use `delocate` to bundle non-system shared libraries.
- Smoke-test on both architectures, because cross-built arm64 wheels cannot
  always be tested on x86_64 CI.

### Publishing

The recommended PyPI upload path is:

- Build wheels and sdist in CI.
- Upload artifacts from build jobs.
- Run `twine check dist/*`.
- Publish to TestPyPI first.
- Publish to PyPI through Trusted Publishing with a dedicated GitHub Actions
  environment, not a long-lived API token.

PyPI's default limits are currently 100 MiB per file and 10 GiB per project.
The historical HARFANG wheel sizes are well under the per-file limit, but macOS
universal2 wheels and bundled toolchains could change that quickly.

## Recommended Technical Direction

### Preferred Path: Single Modern CMake-Aware Backend

Use a single modern package path based on `scikit-build-core`.

Why this is preferred:

- It is designed for CMake projects.
- It supports PEP 517 builds directly.
- It avoids relying on `setuptools`, `distutils`, and `wheel` behavior for the
  CMake integration.
- It supports selecting CMake install components.
- It has direct configuration for Limited API / Stable ABI wheel tags.
- It is compatible with `cibuildwheel`.
- It reduces the amount of custom `setup.py` logic that the project owns.

Illustrative shape, assuming the package files are installed by CMake's
`python` component:

```toml
[build-system]
requires = ["scikit-build-core>=0.12"]
build-backend = "scikit_build_core.build"

[project]
name = "harfang"
dynamic = ["version"]
requires-python = ">=3.11"

[tool.scikit-build]
minimum-version = "build-system.requires"
cmake.source-dir = "../../.."
install.components = ["python"]
wheel.packages = []
wheel.py-api = "cp311"
```

The exact paths and version source need to be adapted to the final package
layout. The important design point is that CMake installs the extension,
package Python files, and runtime dependencies into the wheel layout, while the
Python build backend owns metadata and wheel tagging.

### Acceptable Fallback: Modernize Existing `cmake-build-extension`

If the team wants the smallest short-term diff, the existing
`languages/hg_python/pip` path can be modernized instead.

Minimum changes:

- Move metadata from `setup.py`/`setup.cfg` into `[project]` in
  `pyproject.toml`.
- Remove build-time network cloning.
- Replace direct `setup.py bdist_wheel` instructions with `python -m build`.
- Fix wheel tags for Windows and non-Windows.
- Remove or repair broken console script entry points.
- Add `cibuildwheel` configuration.
- Build and test from the sdist.

This fallback is viable for a first restoration, but it keeps more custom
packaging code in the repository.

## Migration Plan

### Phase 0: Release Policy Decisions

Decide before implementation:

- Minimum Python version: recommended `>=3.11`, optional transitional
  `>=3.10`.
- Stable ABI or per-version CPython wheels.
- Supported platforms for the first release.
- Whether `assetc`, `assimp_converter`, `gltf_importer`, and `gltf_exporter`
  remain bundled in `harfang`, or move to a separate package later.
- License metadata to publish on PyPI.
- Whether 32-bit Windows remains supported.

### Phase 1: Build Reproducibility

Tasks:

- Make FABGen available without network access during builds.
- Replace source tree mutation for `commit_id` with a generated build file in
  the build directory.
- Ensure a clean checkout plus initialized dependencies can build the CPython
  target.
- Ensure the generated sdist contains everything required for a wheel build.
- Add a local command that builds from sdist and not from the checkout.

Exit criteria:

- `python -m build --sdist` succeeds.
- The wheel build from the generated sdist succeeds without internet access
  other than normal Python build dependency provisioning.

### Phase 2: Package Layout and Metadata

Tasks:

- Create a single canonical package skeleton for `harfang`.
- Generate or maintain `harfang/__init__.py` without relying on backend side
  effects.
- Package `harfang/bin` scripts and helper modules explicitly.
- Replace stale Python support classifiers.
- Add `license-files`.
- Add `project.urls`.
- Fix or remove console script entry points.

Exit criteria:

- `twine check dist/*` passes.
- The PyPI long description renders locally.
- Installing the wheel into a fresh virtual environment gives the expected
  package layout.

### Phase 3: ABI and Wheel Tag Correctness

Tasks:

- Decide the Stable ABI target, for example `cp311-abi3`.
- Use modern CMake/Python Stable ABI linkage instead of manual library path
  rewriting.
- Build with the lowest supported Python version.
- Test the installed wheel on every supported Python minor version.
- If any Limited API violation is found, switch to per-version wheels rather
  than forcing `abi3`.

Exit criteria:

- Wheel tags match actual binary compatibility.
- `import harfang` succeeds on every declared Python version.
- The package does not advertise support for Python versions that are not
  tested.

### Phase 4: Platform Wheels

Tasks:

- Add `cibuildwheel` configuration.
- Build Windows x86_64.
- Build Linux x86_64 in manylinux.
- Run `auditwheel` and `delvewheel` repair steps as needed.
- Add macOS after Windows/Linux are stable.
- Store built wheels as CI artifacts.

Exit criteria:

- All wheels install in fresh virtual environments.
- All wheels pass import and tool smoke tests.
- Linux wheels are accepted by `auditwheel`.
- Windows wheels include or document every required DLL.
- Wheel sizes remain under PyPI's per-file limit.

### Phase 5: Publication Workflow

Tasks:

- Add a release workflow triggered by a tag or GitHub release.
- Use PyPI Trusted Publishing with a protected `pypi` environment.
- Publish to TestPyPI first.
- Install from TestPyPI in a clean environment.
- Promote the same artifacts to PyPI.

Exit criteria:

- TestPyPI installation works with no local artifacts.
- Production PyPI upload is performed by CI using Trusted Publishing.
- Release documentation includes supported Python and platform tags.

## Risk Register

### Stable ABI May Be Incorrect

The build defines `Py_LIMITED_API=0x03020000`, but that alone does not prove
that the generated FABGen binding only uses the Limited API correctly. Python's
own documentation warns that Limited API builds should still be tested on all
supported Python minor versions.

Mitigation:

- Build with the lowest supported Python version.
- Test all supported minor versions.
- Add an ABI audit step if practical.
- Fall back to version-specific wheels if needed.

### Linux Dependency Repair May Be Hard

HARFANG brings graphics, audio, VR, and toolchain dependencies. Some may be
easy to bundle, while others may be unsuitable for manylinux wheels or may
pull desktop system dependencies.

Mitigation:

- Start with Linux x86_64 only.
- Let `auditwheel` define the truth.
- Disable optional APIs such as OpenVR in the first Linux wheel if they block a
  compliant package.
- Consider feature-split wheels only if the dependency closure becomes too
  large or fragile.

### Tool Bundling Increases Scope

`assetc` is important to the user experience, and release notes show that it
was intentionally packaged into the Python wheel. The additional tools also add
binary dependencies, size, and test surface.

Mitigation:

- Keep `assetc` in the first release.
- Treat `assimp_converter`, `gltf_importer`, and `gltf_exporter` as explicit
  release-scope choices.
- Consider a later `harfang-tools` split if wheel size or dependency repair
  becomes a blocker.

### Build-Time Network Access Breaks Reproducibility

The current `setup.py` clones FABGen if missing. That can fail in offline,
firewalled, mirrored, or audited build environments, and it makes old releases
harder to rebuild.

Mitigation:

- Pin FABGen.
- Vendor it, add it as a proper submodule, or require it as a pre-fetched
  source dependency.
- Fail with a clear message if it is missing.

### Package Metadata Needs Legal Review

The existing PyPI metadata says `Other/Proprietary License`, while the
repository contains multiple license files including GPL and commercial terms.
Modern metadata supports explicit license files and SPDX-style license
expressions, but the correct expression is a legal/product decision.

Mitigation:

- Have maintainers decide the exact PyPI license metadata.
- Include the relevant license files in both sdist and wheel.
- Avoid publishing ambiguous or misleading classifiers.

### GPU and Windowing Tests Are CI-Sensitive

Basic import tests are easy. Meaningful render tests may require a headless GL,
EGL, software renderer, virtual display, or a real GPU runner.

Mitigation:

- Gate the first release on import and non-window tool tests.
- Add optional render smoke tests separately.
- Do not block package restoration on full graphics coverage unless the module
  cannot import without those runtime dependencies.

## Definition of Done

A PyPI-ready restoration is complete when all of the following are true:

- A clean checkout can build an sdist and wheels through `python -m build` or
  `cibuildwheel`.
- The sdist can be used as the source for wheel builds.
- The build does not clone FABGen or other non-Python resources from the
  internet.
- Wheel tags accurately describe the binary compatibility.
- `twine check dist/*` passes.
- Linux wheels are repaired and tagged as `manylinux_*` or `musllinux_*`.
- Windows wheels are repaired or otherwise proven to find required DLLs.
- The package installs in a fresh virtual environment on every supported
  Python and platform combination.
- `import harfang` succeeds.
- At least one bundled tool smoke test succeeds.
- The PyPI long description renders correctly.
- TestPyPI publication and installation have been validated.
- Production PyPI publication uses a new version number and Trusted
  Publishing or an explicitly approved secure equivalent.

## Open Questions

- Should the first restored release be `3.2.8`, or should packaging
  modernization trigger a `3.3.0` release?
- Is the first release allowed to drop CPython 3.10, or does it need a
  transitional `>=3.10` support policy?
- Is `abi3` a hard requirement, or are per-version wheels acceptable if the
  binding generator uses non-Limited API features?
- Should Linux wheels include OpenVR support in the first release?
- Should macOS be part of the first public restoration or a second release?
- Should `assetc` remain bundled in `harfang`, or should tooling eventually
  move to a separate package?
- What exact license expression and license files should be published?

## References

- PyPA Packaging Flow: https://packaging.python.org/en/latest/flow/
- PyPA `pyproject.toml` specification:
  https://packaging.python.org/en/latest/specifications/pyproject-toml/
- PyPA source distribution format:
  https://packaging.python.org/en/latest/specifications/source-distribution-format/
- PyPA platform compatibility tags:
  https://packaging.python.org/en/latest/specifications/platform-compatibility-tags/
- Python Stable ABI and Limited API:
  https://docs.python.org/3.13/c-api/stable.html
- Python version status:
  https://devguide.python.org/versions/
- PyPI `harfang` project:
  https://pypi.org/project/harfang/
- PyPI help, storage limits, description checks, and file deletion behavior:
  https://pypi.org/help/
- PyPI Trusted Publishing:
  https://docs.pypi.org/trusted-publishers/using-a-publisher/
- Adding a Trusted Publisher to an existing PyPI project:
  https://docs.pypi.org/trusted-publishers/adding-a-publisher/
- `cibuildwheel` documentation:
  https://cibuildwheel.pypa.io/en/stable/
- `cibuildwheel` platform notes:
  https://cibuildwheel.pypa.io/en/latest/platforms/
- `scikit-build-core` project:
  https://pypi.org/project/scikit-build-core/
- `scikit-build-core` configuration:
  https://scikit-build-core.readthedocs.io/en/latest/configuration/
