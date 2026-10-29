# ==========================================
# ESP32 Backtrace Decoder for Arduino CLI
# ==========================================
# Recompiles the selected backend (so the ELF matches the running firmware) and
# decodes a "Backtrace:" line into file:line locations.
#
# Usage:
#   .\decode-backtrace.ps1                 # web backend (default)
#   .\decode-backtrace.ps1 -Backend hw     # hardware backend
#   .\decode-backtrace.ps1 -Fqbn <fqbn>    # override the FQBN

param (
    [ValidateSet("web", "hw", "p4")]
    [string]$Backend = "web",
    [string]$Fqbn = ""
)
$ErrorActionPreference = 'Stop'

$buildFolder = if ($Backend -eq 'p4') { 'p4' } else { "esp32-$Backend" }
$buildPath = Join-Path $PSScriptRoot "build\$buildFolder"

Write-Host "=== ESP32 Backtrace Decoder ($Backend, $Fqbn) ===`n"

# Step 1: Compile the selected backend into the shared build path.
Write-Host "Compiling project..."
& (Join-Path $PSScriptRoot 'tools\build-esp32.ps1') -Backend $Backend -Fqbn $Fqbn | Out-Null

if (-not (Test-Path $buildPath)) {
    Write-Error "Build path not found."
    exit 1
}

# Step 2: Locate ELF file
$elf = Get-ChildItem -Path $buildPath -Filter "*.elf" | Select-Object -First 1
if (-not $elf) {
    Write-Error "ELF file not found."
    exit 1
}

# Step 3: Locate the decoder matching the selected chip architecture.
$chip = if ($Fqbn) { $Fqbn } elseif ($Backend -eq 'p4') { 'esp32p4' } elseif ($Backend -eq 'web') { 'esp32s3' } else { 'esp32' }
$decoderName = if ($chip -match 'esp32(p4|c[0-9]|h[0-9])') {
    'riscv32-esp-elf-addr2line.exe'
} elseif ($chip -match 'esp32s3') {
    'xtensa-esp32s3-elf-addr2line.exe'
} elseif ($chip -match 'esp32s2') {
    'xtensa-esp32s2-elf-addr2line.exe'
} else { 'xtensa-esp32-elf-addr2line.exe' }
$userHome = [string]$env:USERPROFILE
$basePaths = @(
    "$userHome\AppData\Local\Arduino15\packages\esp32-bluepad32\tools",
    "$userHome\AppData\Local\Arduino15\packages\esp32\tools"
)

$addrTool = $null
foreach ($base in $basePaths) {
    if (Test-Path $base) {
        $addrTool = Get-ChildItem -Recurse -Path $base -Filter $decoderName -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($addrTool) { break }
    }
}

if (-not $addrTool) {
    Write-Error "Cannot find $decoderName under Arduino15."
    exit 1
}

Write-Host "`nELF file:`n $($elf.FullName)"
Write-Host "`nTool path:`n $($addrTool.FullName)"

# Step 4: Read backtrace from user
$bt = Read-Host "`nPaste your Backtrace line (e.g. Backtrace: 0x400e1bac:0x3ffe24d0 0x400dfecf:0x3ffe2510 ...)"
$addrs = [regex]::Matches($bt, "0x[0-9a-fA-F]+") | ForEach-Object { $_.Value }

if ($addrs.Count -eq 0) {
    Write-Error "No addresses found in input."
    exit 1
}

# Step 5: Decode
Write-Host "`nDecoding..."
& $addrTool.FullName -pfiaC -e $elf.FullName @addrs
