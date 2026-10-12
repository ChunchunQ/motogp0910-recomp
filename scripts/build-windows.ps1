param(
    [Parameter(Mandatory = $true)][string]$GameRoot,
    [Parameter(Mandatory = $true)][string]$TitleUpdatePath,
    [ValidateRange(1, 16)][int]$Jobs = 2
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$ProjectRoot = Split-Path -Parent $PSScriptRoot

function Invoke-Native {
    param([string]$Command, [string[]]$Arguments)
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Command failed with exit code $LASTEXITCODE" }
}
function Assert-Hash {
    param([string]$Path, [string]$Expected)
    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { throw "File missing: $Path" }
    if ((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -ne $Expected) {
        throw "SHA-256 mismatch: $Path"
    }
}
function Stage-File {
    param([string]$Source, [string]$Destination, [string]$Expected)
    Assert-Hash -Path $Source -Expected $Expected
    if (!(Test-Path -LiteralPath $Destination)) {
        Copy-Item -LiteralPath $Source -Destination $Destination
    }
    Assert-Hash -Path $Destination -Expected $Expected
}

# Run inside an x64 VS 2022 developer PowerShell so MSVC headers/libs are visible.
$Clang = (Get-Command clang++ -ErrorAction Stop).Source
$CMake = (Get-Command cmake -ErrorAction Stop).Source
$null = Get-Command ninja -ErrorAction Stop
if (!$env:VCToolsInstallDir -or !$env:WindowsSdkDir) {
    throw 'Use VS 2022 Developer PowerShell for x64 with Desktop C++ and Clang 20+ installed.'
}
$VersionText = (& $Clang --version) -join "`n"
if ($VersionText -notmatch 'clang version (\d+)' -or [int]$Matches[1] -lt 20) {
    throw 'Windows builds require Clang 20+.'
}
$GameRoot = (Resolve-Path -LiteralPath $GameRoot).Path
$TitleUpdatePath = (Resolve-Path -LiteralPath $TitleUpdatePath).Path
$Xex = Join-Path $GameRoot 'default.xex'
$XexHash = '54245417e996ed0aa561a5a803fd810e87684bd0b43082f316db711b3ee786ae'
$TuHash = '441df6cc2e20ac8aeeed475138c1714d477f965423d8c5c9b0202e3a18d8141f'
$PatchHash = '0419205ba10dc65eaa7b0bb62abc64cada46119ece5ed749e4ba20c06a6bc079'
Assert-Hash -Path $Xex -Expected $XexHash
Assert-Hash -Path $TitleUpdatePath -Expected $TuHash

$SdkRoot = Join-Path $ProjectRoot 'out/sdk-0.10.0'
$Archive = Join-Path $ProjectRoot 'out/rexglue-sdk-0.10.0-win-amd64.zip'
New-Item -ItemType Directory -Force (Join-Path $ProjectRoot 'out') | Out-Null
if (!(Test-Path -LiteralPath $Archive)) {
    Invoke-WebRequest -Uri 'https://github.com/rexglue/rexglue-sdk/releases/download/v0.10.0/rexglue-sdk-0.10.0-win-amd64.zip' -OutFile "$Archive.part"
    Move-Item -LiteralPath "$Archive.part" -Destination $Archive
}
Assert-Hash -Path $Archive -Expected 'ad4485da853fbe932e9f5dbb643f9b5fe85dc73775ebbad4df6879fe6df0c11b'
if (!(Test-Path -LiteralPath $SdkRoot)) {
    Expand-Archive -LiteralPath $Archive -DestinationPath $SdkRoot
}
$Sdk = Join-Path $SdkRoot 'win-amd64'
$RexGlue = Join-Path $Sdk 'bin/rexglue.exe'
if ((& $RexGlue --version) -ne '0.10.0' -or $LASTEXITCODE -ne 0) {
    throw 'The installed SDK must be exactly 0.10.0.'
}

Push-Location $ProjectRoot
try {
    $ToolsBuild = Join-Path $ProjectRoot 'out/tools-win'
    Invoke-Native -Command $CMake -Arguments @('-S', 'tools', '-B', $ToolsBuild, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_CXX_COMPILER=$Clang", "-DCMAKE_PREFIX_PATH=$Sdk")
    Invoke-Native -Command $CMake -Arguments @('--build', $ToolsBuild, '--parallel', "$Jobs")
    $Extracted = Join-Path $ProjectRoot 'assets/updates/extracted'
    if (!(Test-Path -LiteralPath $Extracted)) {
        Invoke-Native -Command (Join-Path $ToolsBuild 'motogp_inspect.exe') -Arguments @('stfs-extract', $TitleUpdatePath, $Extracted)
    }
    $Staged = Join-Path $ProjectRoot 'assets/game-tu'
    New-Item -ItemType Directory -Force $Staged | Out-Null
    Stage-File -Source $Xex -Destination (Join-Path $Staged 'default.xex') -Expected $XexHash
    Stage-File -Source (Join-Path $Extracted 'default.xexp') -Destination (Join-Path $Staged 'default.xexp') -Expected $PatchHash
    Invoke-Native -Command $RexGlue -Arguments @('codegen', '--ignore-stamp', 'motogp0910_manifest.toml')
    Invoke-Native -Command $CMake -Arguments @('--preset', 'win-amd64-release', "-DCMAKE_CXX_COMPILER=$Clang", "-DCMAKE_PREFIX_PATH=$Sdk")
    Invoke-Native -Command $CMake -Arguments @('--build', '--preset', 'win-amd64-release', '--parallel', "$Jobs")
    $ExeDirectory = Join-Path $ProjectRoot 'out/build/win-amd64-release'
    # Ship the SDK's auxiliary runtime DLLs as well as the plugin CMake stages.
    Get-ChildItem -LiteralPath (Join-Path $Sdk 'bin') -Filter '*.dll' -File | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $ExeDirectory -Force
    }
    Invoke-Native -Command $CMake -Arguments @('--build', '--preset', 'win-amd64-release', '--target', 'motogp_headless_probe', '--parallel', "$Jobs")
    Invoke-Native -Command (Join-Path $ExeDirectory 'motogp_headless_probe.exe') -Arguments @($Staged)
    Write-Host 'Windows build complete. Executable: out/build/win-amd64-release/motogp0910.exe'
    Write-Host 'Gameplay has not been validated. See README.md for full game data and patch requirements.'
} finally {
    Pop-Location
}
