# Python 3.11+ must be installed. Arguments are forwarded as separate arguments.
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    if (Get-Command py -ErrorAction SilentlyContinue) {
        & py -3 (Join-Path $PSScriptRoot 'community_server.py') @args
    } else {
        & python (Join-Path $PSScriptRoot 'community_server.py') @args
    }
    if ($LASTEXITCODE -ne 0) { throw "Community server exited with code $LASTEXITCODE" }
} finally { Pop-Location }
