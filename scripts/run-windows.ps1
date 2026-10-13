param([Parameter(Mandatory = $true)][string]$GameRoot)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$GameRoot = (Resolve-Path -LiteralPath $GameRoot).Path
$Executable = Join-Path $ProjectRoot 'out/build/win-amd64-release/motogp0910.exe'
if (!(Test-Path -LiteralPath $Executable -PathType Leaf)) { throw 'Run build-windows.ps1 first.' }
$Xex = Join-Path $GameRoot 'default.xex'
if ((Get-FileHash -LiteralPath $Xex -Algorithm SHA256).Hash -ne '54245417e996ed0aa561a5a803fd810e87684bd0b43082f316db711b3ee786ae') {
    throw 'Wrong stock XEX revision in GameRoot.'
}
$PatchSource = Join-Path $ProjectRoot 'assets/game-tu/default.xexp'
$PatchHash = '0419205ba10dc65eaa7b0bb62abc64cada46119ece5ed749e4ba20c06a6bc079'
if ((Get-FileHash -LiteralPath $PatchSource -Algorithm SHA256).Hash -ne $PatchHash) { throw 'Wrong title-update patch.' }
$PatchDestination = Join-Path $GameRoot 'default.xexp'
if (!(Test-Path -LiteralPath $PatchDestination)) {
    # Add the verified patch; leave the original XEX and all game data intact.
    Copy-Item -LiteralPath $PatchSource -Destination $PatchDestination
}
if ((Get-FileHash -LiteralPath $PatchDestination -Algorithm SHA256).Hash -ne $PatchHash) {
    throw 'A different default.xexp already exists; it was not overwritten.'
}
$UserData = Join-Path $ProjectRoot 'out/userdata'
New-Item -ItemType Directory -Force $UserData | Out-Null
Push-Location (Split-Path -Parent $Executable)
try {
    & $Executable "--game_data_root=$GameRoot" "--user_data_root=$UserData" "--log_file=$(Join-Path $ProjectRoot 'out/motogp-windows.log')"
    if ($LASTEXITCODE -ne 0) { throw "Game exited with code $LASTEXITCODE; see out/motogp-windows.log." }
} finally {
    Pop-Location
}
