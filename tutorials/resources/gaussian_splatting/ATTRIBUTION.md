# Houseplant

**Houseplant**, by **Marcel Padilla**, licensed under [Creative Commons Attribution 4.0 International (CC BY 4.0)](https://creativecommons.org/licenses/by/4.0/).

Source: https://github.com/marcelpadilla/splats/tree/c3e5ec0a8ddb3ab26f28a067a29d272fe1a23411/data/plant

`source/plant.splat` is the unmodified source asset: 3,636,736 bytes, 113,648 Gaussians, SH degree 0. SHA-256: `ce9d490b18f16d6730d832227bf541431b8b1a5d167c5a3fe89af4f324c0d8fd`. `source/meta.json` preserves the author's metadata and license declaration at that revision.

Native `gsplatc` centers the full source bounds, normalizes the largest extent to 2 units, converts attributes to canonical floats and stores them in quantized, compressed HGS2 blocks, and converts stored RGB into equivalent SH0 coefficients without a gamma transform. The default retains all 113,648 records; the optional preview profile naively selects 30,000 evenly spaced midpoint records. The Lua instance rotates 180 degrees around X (source up axis -Y), transforming positions and covariances together. These changes are made by the HARFANG tutorial; the source author does not endorse this project. Higher-order SH lighting is not reconstructed. All derived datasets remain attributed to Marcel Padilla under CC BY 4.0.
