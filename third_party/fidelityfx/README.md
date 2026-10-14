# AMD FidelityFX upscaling providers

Unmodified API headers and AMD's `PrebuiltSignedDLL/amd_fidelityfx_dx12.dll`
plus its import library come from the exact FidelityFX dependency pinned by
ReXGlue v0.10.0: [`eee08db`](https://github.com/rexglue/FidelityFX-SDK/tree/eee08db1688ac3d1275a70b728f4a8ba22914213).
Checksums and origin are recorded in manifest.json; the AMD MIT license is included.
The packaged DLL provides separate FSR 2.3.3 and FSR 3.1.4 upscalers. The bridge
selects the requested major version explicitly, rather than letting both labels
select the same default provider. No frame-generation provider is invoked.

The public [Downpour SDK fork](https://github.com/LittleBitUA/rexglue-sdk-dpour/tree/5497095131e8c6174ab698c5ea1ee4a60c0e395f)
was inspected as a compatibility-path reference. No Downpour executable or
provider binary was copied. See docs/experimental-rendering.md for limitations.
