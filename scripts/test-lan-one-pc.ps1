param([string]$ProgramDirectory = $PSScriptRoot)
# Run beside the packaged EXEs, or pass -ProgramDirectory when using repo sources.
$ProgramDirectory = (Resolve-Path -LiteralPath $ProgramDirectory -ErrorAction Stop).Path
$Monitor = Join-Path $ProgramDirectory 'motogp0910-crash-monitor.exe'
if (-not (Test-Path -LiteralPath $Monitor -PathType Leaf)) {
    throw 'Extract the complete Windows ZIP and run this script from its EXE folder.'
}
if (Get-Process -Name 'motogp0910' -ErrorAction SilentlyContinue) {
    throw 'Close all MotoGP game windows before starting the two-instance test.'
}
foreach ($TestFolder in @('logs','lan-test/host/cache','lan-test/host/metadata','lan-test/client/cache','lan-test/client/metadata')) {
    New-Item -ItemType Directory -Force -Path (Join-Path $ProgramDirectory $TestFolder) -ErrorAction Stop | Out-Null
}
$SharedFlags = '--lan_enabled=true --lan_bind_ip=127.0.0.1 --lan_host_ip=127.0.0.1 --fullscreen=false --display_mode=windowed --window_width=960 --window_height=540 --aspect_ratio=16:9 --resolution_scale=1 --draw_resolution_scale_x=1 --draw_resolution_scale_y=1 --present_effect=bilinear --native_2x_msaa=false'
Start-Process -FilePath $Monitor -WorkingDirectory $ProgramDirectory -ArgumentList "$SharedFlags --lan_player_name=Host --user_data_root=lan-test/host --cache_root=lan-test/host/cache --metadata_root=lan-test/host/metadata --log_file=logs/lan-host.log" -ErrorAction Stop
Start-Process -FilePath $Monitor -WorkingDirectory $ProgramDirectory -ArgumentList "$SharedFlags --lan_player_name=Client --user_data_root=lan-test/client --cache_root=lan-test/client/cache --metadata_root=lan-test/client/metadata --log_file=logs/lan-client.log" -ErrorAction Stop
Write-Host 'Two game windows started. Create a Player Match in the first, search/join in the second.'
Write-Host 'Use Alt+Tab to switch. Select Ready on both, then try starting the race.'
Write-Host 'Test profiles/cache are isolated under lan-test. Logs: logs/lan-host.log and logs/lan-client.log.'
Write-Host 'Loopback testing checks local connection; real two-PC Radmin routing is a separate test.'
