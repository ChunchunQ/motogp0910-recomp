param([Parameter(Mandatory = $true)][string]$Destination)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Root = Split-Path -Parent $PSScriptRoot
$Folder = Join-Path $Root 'third_party/rexglue-runtime'
$Manifest = Get-Content -LiteralPath (Join-Path $Folder 'manifest.json') -Raw | ConvertFrom-Json
if ($Manifest.sdk_version -ne '0.10.0') { throw 'This project requires ReXGlue v0.10.0.' }
$Archive = Join-Path $Folder $Manifest.archive
if ((Get-FileHash -LiteralPath $Archive -Algorithm SHA256).Hash -ne $Manifest.sha256) {
    throw 'Spatial runtime archive SHA-256 mismatch.'
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$Zip = [System.IO.Compression.ZipFile]::OpenRead($Archive)
try {
    New-Item -ItemType Directory -Force $Destination | Out-Null
    $Contents = @{}
    foreach ($Property in $Manifest.dll_sha256.PSObject.Properties) {
        if ([IO.Path]::GetFileName($Property.Name) -ne $Property.Name) { throw 'Invalid DLL file name.' }
        $Entry = $Zip.GetEntry($Property.Name)
        if (!$Entry) { throw "Missing runtime DLL: $($Property.Name)" }
        $Stream = $Entry.Open()
        $Memory = New-Object IO.MemoryStream
        try { $Stream.CopyTo($Memory); $Data = $Memory.ToArray() }
        finally { $Stream.Dispose(); $Memory.Dispose() }
        $Hash = [Security.Cryptography.SHA256]::Create()
        try { $Digest = [BitConverter]::ToString($Hash.ComputeHash($Data)).Replace('-', '').ToLowerInvariant() }
        finally { $Hash.Dispose() }
        if ($Digest -ne $Property.Value) { throw "Runtime DLL checksum mismatch: $($Property.Name)" }
        $Contents[$Property.Name] = $Data
    }
    # Serialize simultaneous CMake post-build steps targeting the same folder.
    $PathHash = [Security.Cryptography.SHA256]::Create()
    try { $Key = [BitConverter]::ToString($PathHash.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.Path]::GetFullPath($Destination).ToUpperInvariant()))).Replace('-', '') }
    finally { $PathHash.Dispose() }
    $Mutex = New-Object Threading.Mutex($false, "Local\MotoGP0910RuntimeStage-$Key")
    $Acquired = $false
    try {
        try { $Acquired = $Mutex.WaitOne(60000) }
        catch [Threading.AbandonedMutexException] { $Acquired = $true }
        if (!$Acquired) { throw 'Timed out waiting for runtime staging.' }
        foreach ($Name in $Contents.Keys) {
            [IO.File]::WriteAllBytes((Join-Path $Destination $Name), $Contents[$Name])
        }
        $Licenses = Join-Path $Destination 'licenses'
        New-Item -ItemType Directory -Force $Licenses | Out-Null
        Get-ChildItem -LiteralPath (Join-Path $Folder 'licenses') -File | ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination $Licenses -Force
        }
    } finally { if ($Acquired) { $Mutex.ReleaseMutex() }; $Mutex.Dispose() }
    Write-Host "Staged ReXGlue v0.10.0 spatial FSR runtime: $Destination"
} finally { $Zip.Dispose() }
