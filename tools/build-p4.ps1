param([switch]$Upload, [string]$Port = 'COM3')
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'build-esp32.ps1') -Backend p4 -Upload:$Upload -Port $Port
