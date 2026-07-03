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
    [ValidateSet("web", "hw")]
    [string]$Backend = "web",
    [string]$Fqbn = ""
)

# Resolve FQBN + backend selector flags.
if ($Backend -eq "hw") {
    if ($Fqbn -eq "") { $Fqbn = "esp32-bluepad32:esp32:esp32wrover" }
    $webFlag = 0; $hwFlag = 1
} else {
    # TODO: set your actual ESP32-S3-CAM board FQBN for the web backend.
    if ($Fqbn -eq "") { $Fqbn = "esp32-bluepad32:esp32:esp32wrover" }
    $webFlag = 1; $hwFlag = 0
}

# Classic ESP32 (hw backend + Bluepad32/BTstack) has tight IRAM; keep OPL3 in flash
# so .iram0.text fits (the web/S3 build has headroom and keeps OPL3 in IRAM).
$iram = if ($Backend -eq "hw") { " -DOPL3_IRAM_ATTR=" } else { "" }

$defC   = "compiler.c.extra_flags=-DPAL_ESP32_BACKEND_WEB=$webFlag -DPAL_ESP32_BACKEND_HW=$hwFlag$iram"
$defCpp = "compiler.cpp.extra_flags=-DPAL_ESP32_BACKEND_WEB=$webFlag -DPAL_ESP32_BACKEND_HW=$hwFlag$iram"
$buildPath = "$PSScriptRoot\build_arduino"

Write-Host "=== ESP32 Backtrace Decoder ($Backend, $Fqbn) ===`n"

# Step 1: Compile the selected backend into the shared build path.
Write-Host "Compiling project..."
arduino-cli compile --fqbn $Fqbn `
    --build-property $defC --build-property $defCpp `
    --build-path $buildPath $PSScriptRoot | Out-Null

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

# Step 3: Locate xtensa-esp32-elf-addr2line
$userHome = [string]$env:USERPROFILE
$basePaths = @(
    "$userHome\AppData\Local\Arduino15\packages\esp32-bluepad32\tools",
    "$userHome\AppData\Local\Arduino15\packages\esp32\tools"
)

$addrTool = $null
foreach ($base in $basePaths) {
    if (Test-Path $base) {
        $addrTool = Get-ChildItem -Recurse -Path $base -Filter "xtensa-esp32-elf-addr2line.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($addrTool) { break }
    }
}

if (-not $addrTool) {
    Write-Error "Cannot find xtensa-esp32-elf-addr2line tool under Arduino15."
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
