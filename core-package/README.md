# HARFANG® Core Package

`core-package/core` is the reference implementation of HARFANG's rendering resources. Make shared shader and texture changes here first. `tutorials/resources/core` is a byte-identical copy retained so the tutorials remain self-contained.

Contents:

- `gaussian_splatting/`: native Gaussian projection, SH evaluation, global sorting, gather, clearing and drawing shaders.
- `shader/`: Forward/AAA rendering and post-processing shaders, plus the shared BGFX shader/compute headers.
- `noise/`: blue-noise textures used by the AAA pipeline.
- `pbr/`: HDR environments and BRDF maps, including the blue-sky environment referenced by the car-engine tutorial.

## Compile and use

From the repository root, using the matching installed toolchain:

```powershell
assetc -api DX11 -platform windows core-package resources_compiled
```

This preserves the `core/` prefix. Load Gaussian renderer programs from `resources_compiled/core/gaussian_splatting` with `CreateGaussianSplatRendererFromFile`, or from `core/gaussian_splatting` through the assets API. Forward/AAA creation continues to use the `core` resource root. The package contains renderer resources, not Gaussian model assets or tutorial-specific gizmo shaders.

D3D11 compilation and native Gaussian GPU probes are checked during this promotion. This does not establish new Metal or OpenGL runtime qualification.

## Keep the tutorial copy synchronized

From the repository root:

```powershell
cmake -P tools/sync_core_package.cmake
cmake -DMODE=UPDATE -P tools/sync_core_package.cmake
```

The first command checks every relative path and file hash. The second copies missing/changed reference resources to the tutorials. Both report tutorial-only files for review; they are never silently deleted. Recompile tutorial assets normally after synchronization.

The compute and Gaussian-sort regression runners now take their shared headers/shaders from this reference package. See [the resource audit](../specifications/CORE_PACKAGE_RESOURCE_AUDIT.md) for the differences promoted from the tutorial copy.

## License

Harfang is licensed under GPLv3, LGPLv3 and a commercial license. See [LICENSE](LICENSE) and https://www.harfang3d.com/license.
