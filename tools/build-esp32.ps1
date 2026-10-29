param(
    [ValidateSet('p4', 'web', 'hw')][string]$Backend = 'p4',
    [switch]$Upload,
    [switch]$Profiling,
    [string]$Port = 'COM3',
    [string]$Fqbn = ''
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$buildRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build'))
$stagingPath = [IO.Path]::GetFullPath((Join-Path $buildRoot "arduino-stage\$Backend\sdlpal"))
$outputPath = Join-Path $buildRoot $(if ($Backend -eq 'p4') { 'p4' } else { "esp32-$Backend" })

# Discard only our generated sketch so moved sources cannot compile twice.
if (-not $stagingPath.StartsWith($buildRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Arduino staging directory escaped the build directory'
}
if (Test-Path -LiteralPath $stagingPath) {
    $resolvedStage = (Resolve-Path -LiteralPath $stagingPath).Path
    if (-not $resolvedStage.StartsWith($buildRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Resolved Arduino staging directory escaped the build directory'
    }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}
New-Item -ItemType Directory -Force $stagingPath, $outputPath | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'src') -Destination $stagingPath -Recurse -Force
$stagedDrivers = Join-Path $stagingPath 'src\driver'
New-Item -ItemType Directory -Force $stagedDrivers | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'driver\esp32') -Destination $stagedDrivers -Recurse -Force
# Account for the generated sketch's extra src/ level. Keep explicit relative
# headers so Arduino's library discovery also resolves the portable core.
Get-ChildItem -LiteralPath $stagedDrivers -Recurse -File | Where-Object {
    $_.Extension -in '.c', '.cpp', '.h'
} | ForEach-Object {
    $original = [IO.File]::ReadAllText($_.FullName)
    $staged = $original.Replace('#include "../../../src/', '#include "../../../')
    if ($staged -ne $original) {
        [IO.File]::WriteAllText($_.FullName, $staged, [Text.UTF8Encoding]::new($false))
    }
}
# Core files that reach the platform layer reference the external driver in the source checkout.
Get-ChildItem -LiteralPath (Join-Path $stagingPath 'src') -Filter '*.c' -File | ForEach-Object {
    $original = [IO.File]::ReadAllText($_.FullName)
    $staged = $original.Replace('"../driver/esp32/', '"driver/esp32/')
    if ($staged -ne $original) {
        [IO.File]::WriteAllText($_.FullName, $staged, [Text.UTF8Encoding]::new($false))
    }
}
$sketch = [IO.File]::ReadAllText((Join-Path $projectRoot 'sdlpal.ino'))
$sketch = $sketch.Replace('"driver/esp32/', '"src/driver/esp32/')
[IO.File]::WriteAllText((Join-Path $stagingPath 'sdlpal.ino'), $sketch, [Text.UTF8Encoding]::new($false))

if (-not $Fqbn) {
    $Fqbn = switch ($Backend) {
        'p4' { 'esp32:esp32:esp32p4:ChipVariant=prev3,PSRAM=enabled,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,CDCOnBoot=default' }
        'web' { 'esp32:esp32:esp32s3' }
        'hw' { 'esp32-bluepad32:esp32:esp32wrover' }
    }
}
$webFlag = [int]($Backend -eq 'web')
$hwFlag = [int]($Backend -eq 'hw')
$p4Flag = [int]($Backend -eq 'p4')
$flags = "-DPAL_ESP32_BACKEND_WEB=$webFlag -DPAL_ESP32_BACKEND_HW=$hwFlag -DPAL_ESP32_BACKEND_P4=$p4Flag -DPAL_NO_MAIN=1"
if ($Backend -ne 'web') { $flags += ' -DOPL3_IRAM_ATTR=' }
# PALPROF window reports on the UART every 2 s (see src/profile.h).
if ($Profiling) { $flags += ' -DPAL_PROFILE=1' }
$cliArgs = @('compile', '--fqbn', $Fqbn, '--build-path', $outputPath,
    '--build-property', "compiler.c.extra_flags=$flags",
    '--build-property', "compiler.cpp.extra_flags=$flags")
if ($Backend -eq 'p4') {
    # Resolve the sketchbook from arduino-cli: Documents may be redirected away from USERPROFILE.
    $sketchbook = (& arduino-cli config get directories.user | Out-String).Trim()
    $gfxLibrary = Join-Path $sketchbook 'libraries\GFX_Library_for_Arduino'
    if (-not (Test-Path -LiteralPath $gfxLibrary)) {
        throw "GFX Library for Arduino not found at $gfxLibrary; run: arduino-cli lib install `"GFX Library for Arduino`""
    }
    $cliArgs += @('--library', $gfxLibrary)
}
if ($Upload) { $cliArgs += @('--upload', '--port', $Port) }
$ErrorActionPreference = 'Continue'
& arduino-cli @cliArgs $stagingPath
if ($LASTEXITCODE -ne 0) { throw "Arduino $Backend build failed: $LASTEXITCODE" }
