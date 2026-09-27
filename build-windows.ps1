<#
.SYNOPSIS
    Convenience script for building on Windows with Visual Studio.

.PARAMETER Clean
    Clean before building.
.PARAMETER Debug
    Build with debug symbols.
.PARAMETER Test
    Execute tests.
.PARAMETER NoInstall
    Build without local install.
.PARAMETER Offline
    Build without downloading dependencies. Reuses the dependencies downloaded by an earlier build in the same build
    directory.
.PARAMETER Package
    Create the release archives with CPack.
.PARAMETER Shared
    Build a shared library (DLL) instead of a static library.
.PARAMETER WarningsAsErrors
    Treat compiler warnings as errors.
.PARAMETER Platform
    Target platform: x64 or Win32.
.PARAMETER BuildDir
    Build directory [default: build/<mode>-<platform>].
.PARAMETER InstallDir
    Install directory [default: install].
#>
param (
    [switch]$Clean,
    [switch]$Debug,
    [switch]$Test,
    [switch]$NoInstall,
    [switch]$Offline,
    [switch]$Package,
    [switch]$Shared,
    [switch]$WarningsAsErrors,
    [ValidateSet("x64", "Win32")]
    [string]$Platform = "x64",
    [string]$BuildDir = "",
    [string]$InstallDir = (Join-Path $PSScriptRoot "install")
)

$ErrorActionPreference = "Stop"

function Invoke-Checked {
    param ([string]$Description, [scriptblock]$Command)

    & $Command
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Error during ${Description}!" -ErrorAction Continue
        exit $LASTEXITCODE
    }
}

function Find-VsWhere {
    $vswhere = Get-Command vswhere -ErrorAction SilentlyContinue
    if ($vswhere) {
        return $vswhere.Source
    }

    if (${env:ProgramFiles(x86)}) {
        $installerPath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $installerPath) {
            return $installerPath
        }
    }

    return $null
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error "CMake is not installed. Please install CMake 3.20 or later before running this script." -ErrorAction Continue
    exit -1
}

$vswhere = Find-VsWhere
if (-not $vswhere) {
    Write-Error "vswhere not found. Please install Visual Studio 2017 or later with the C++ workload." -ErrorAction Continue
    exit -1
}

$cmakeHelp = $(cmake --help)

# -requires param : ensure that the visual studio installs have the C++ workload
#
$generator = ""
$vsVersions = & $vswhere -property installationVersion -requires "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
foreach ($vsVersion in $vsVersions) {
    $versionMajor = [int]$vsVersion.Substring(0, $vsVersion.IndexOf("."))
    $candidate = switch ($versionMajor) {
        15 { "Visual Studio 15 2017" }
        16 { "Visual Studio 16 2019" }
        17 { "Visual Studio 17 2022" }
        18 { "Visual Studio 18 2026" }
        default { "" }
    }

    # Use the newest Visual Studio version that the installed CMake version supports
    #
    if ($candidate -and ($cmakeHelp -match $candidate)) {
        $generator = $candidate
    }
}

if (-not $generator) {
    Write-Error "Visual Studio not found, missing C++ toolchain, or not supported by the installed CMake version." -ErrorAction Continue
    exit -1
}

if ($Debug) {
    $buildMode = "Debug"
} else {
    $buildMode = "Release"
}

if (-not $BuildDir) {
    $BuildDir = Join-Path $PSScriptRoot "build/$($buildMode.ToLower())-$($Platform.ToLower())"
}

$sharedFlag = if ($Shared) { "ON" } else { "OFF" }
$warningsFlag = if ($WarningsAsErrors) { "ON" } else { "OFF" }
$offlineFlag = if ($Offline) { "ON" } else { "OFF" }

Write-Output ""
Write-Output "Using generator: $generator"
Write-Output "Build mode: $buildMode"
Write-Output "Platform: $Platform"
Write-Output "Build directory: $BuildDir"
if (-not $NoInstall) {
    Write-Output "Install directory: $InstallDir"
}
Write-Output "Shared library: $sharedFlag"
Write-Output "Warnings as errors: $warningsFlag"
Write-Output "Offline: $offlineFlag"

if ($Clean) {
    Write-Output ""
    Write-Output "Cleaning build and install directories"
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
    }
    if (-not $NoInstall -and (Test-Path $InstallDir)) {
        Remove-Item -Recurse -Force $InstallDir
    }
}

Write-Output ""
Invoke-Checked "cmake generation" {
    cmake -S $PSScriptRoot -B $BuildDir -G $generator -A $Platform `
        "-DCMAKE_INSTALL_PREFIX=$InstallDir" `
        "-DBUILD_SHARED_LIBS=$sharedFlag" `
        "-DEOLIB_BUILD_TESTS=ON" `
        "-DEOLIB_WARNINGS_AS_ERRORS=$warningsFlag" `
        "-DEOLIB_OFFLINE=$offlineFlag"
}

Invoke-Checked "cmake build" {
    cmake --build $BuildDir --config $buildMode --parallel
}

if ($Test) {
    Write-Output ""
    Invoke-Checked "tests" {
        ctest --test-dir $BuildDir --build-config $buildMode --output-on-failure --parallel ([Environment]::ProcessorCount)
    }
}

if (-not $NoInstall) {
    Write-Output ""
    Invoke-Checked "install" {
        cmake --install $BuildDir --config $buildMode
    }
}

if ($Package) {
    Write-Output ""
    Push-Location $BuildDir
    try {
        Invoke-Checked "packaging" {
            cpack -C $buildMode
        }
    } finally {
        Pop-Location
    }
}
