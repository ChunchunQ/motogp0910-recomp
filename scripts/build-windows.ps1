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

# Initialize installed VS 2022 or newer x64 tools from ordinary PowerShell.
$VsInstall = $env:VSINSTALLDIR
if (!$env:VCToolsInstallDir -or !$env:WindowsSdkDir -or $env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
    $VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (!(Test-Path -LiteralPath $VsWhere -PathType Leaf)) {
        throw 'Install Visual Studio 2022 or newer with Desktop development with C++ and Windows SDK. See docs/windows-build.md.'
    }
    $VsInstall = & $VsWhere -latest -products '*' -version '[17.0,)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or !$VsInstall) {
        throw 'Visual Studio 2022 or newer x64 C++ tools were not found. See docs/windows-build.md.'
    }
    & (Join-Path $VsInstall 'Common7/Tools/Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
}
if (!$env:VCToolsInstallDir -or !$env:WindowsSdkDir) {
    throw 'Visual Studio x64 C++ tools and Windows SDK must be installed.'
}

# VS 2026 can provide Clang 22, CMake and Ninja without separate installers.
$ClangCandidates = @()
if ($VsInstall) {
    $ClangCandidates += Join-Path $VsInstall 'VC/Tools/Llvm/x64/bin/clang++.exe'
    $ClangCandidates += Join-Path $VsInstall 'VC/Tools/Llvm/bin/clang++.exe'
}
$ClangCandidates += Join-Path $env:ProgramFiles 'LLVM/bin/clang++.exe'
$PathClang = Get-Command clang++ -ErrorAction SilentlyContinue
if ($PathClang) { $ClangCandidates += $PathClang.Source }
$Clang = $null
foreach ($Candidate in $ClangCandidates | Select-Object -Unique) {
    if (!(Test-Path -LiteralPath $Candidate -PathType Leaf)) { continue }
    $VersionText = (& $Candidate --version) -join "`n"
    if ($LASTEXITCODE -eq 0 -and $VersionText -match 'clang version (\d+)' -and [int]$Matches[1] -ge 20 -and $VersionText -match 'Target:\s*x86_64-.*windows-msvc') {
        $Clang = $Candidate
        break
    }
}
if (!$Clang) {
    throw 'Install C++ Clang Compiler for Windows (20+) in Visual Studio, or LLVM for Windows x64 with an MSVC target. See docs/windows-build.md.'
}
function Find-BuildTool {
    param([string]$Name, [string]$VsRelativePath)
    if ($VsInstall) {
        $Bundled = Join-Path $VsInstall $VsRelativePath
        if (Test-Path -LiteralPath $Bundled -PathType Leaf) { return $Bundled }
    }
    $Tool = Get-Command $Name -ErrorAction SilentlyContinue
    if ($Tool) { return $Tool.Source }
    throw "$Name was not found. Install C++ CMake tools for Windows in Visual Studio, or install $Name separately."
}
$CMake = Find-BuildTool -Name 'cmake' -VsRelativePath 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$Ninja = Find-BuildTool -Name 'ninja' -VsRelativePath 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'
$CMakeVersion = (& $CMake --version) -join "`n"
if ($LASTEXITCODE -ne 0 -or $CMakeVersion -notmatch 'cmake version (\d+\.\d+\.\d+)' -or [version]$Matches[1] -lt [version]'3.25.0') {
    throw 'CMake 3.25+ is required. Update the VS CMake tools or install a newer CMake.'
}
Write-Host "Clang: $Clang"
Write-Host "CMake: $CMake"
Write-Host "Ninja: $Ninja"
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
    Invoke-Native -Command $CMake -Arguments @('-S', 'tools', '-B', $ToolsBuild, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_CXX_COMPILER=$Clang", "-DCMAKE_MAKE_PROGRAM=$Ninja", "-DCMAKE_PREFIX_PATH=$Sdk")
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
    Invoke-Native -Command $CMake -Arguments @('--preset', 'win-amd64-release', "-DCMAKE_CXX_COMPILER=$Clang", "-DCMAKE_MAKE_PROGRAM=$Ninja", "-DCMAKE_PREFIX_PATH=$Sdk")
    Invoke-Native -Command $CMake -Arguments @('--build', '--preset', 'win-amd64-release', '--parallel', "$Jobs")
    $ExeDirectory = Join-Path $ProjectRoot 'out/build/win-amd64-release'
    $ControllerDb = Join-Path $ProjectRoot 'third_party/SDL_GameControllerDB'
    Copy-Item -LiteralPath (Join-Path $ControllerDb 'gamecontrollerdb.txt') -Destination $ExeDirectory -Force
    $LicensesDirectory = Join-Path $ExeDirectory 'licenses'
    New-Item -ItemType Directory -Force $LicensesDirectory | Out-Null
    Copy-Item -LiteralPath (Join-Path $ControllerDb 'LICENSE.txt') -Destination (Join-Path $LicensesDirectory 'SDL_GameControllerDB-LICENSE.txt') -Force
    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'third_party/tomlplusplus-LICENSE.txt') -Destination (Join-Path $LicensesDirectory 'tomlplusplus-LICENSE.txt') -Force
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
