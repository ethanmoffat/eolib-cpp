<#
.SYNOPSIS
    Installs the dependencies for building eolib on Windows.

.DESCRIPTION
    Uses Chocolatey (installed if missing) to install CMake, git and vswhere, and optionally the Visual Studio Build
    Tools with the C++ workload. clang-format and clang-tidy (the versions used by CI) are downloaded as static
    binaries from https://github.com/cpp-linter/clang-tools-static-binaries into <repo>\.tools\bin, where CMake finds
    them for the format, format-check and tidy targets.

    With -Docs, Doxygen (the version used by CI) is downloaded from https://github.com/doxygen/doxygen/releases into
    <repo>\.tools\bin, where the docs target (EOLIB_BUILD_DOCS) finds it.

    pugixml, GoogleTest and nlohmann/json are downloaded by CMake with FetchContent at configure time.

    This script must be run as administrator.

.PARAMETER SkipCMake
    Skip installing CMake.
.PARAMETER SkipStyleTools
    Skip installing clang-format/clang-tidy into .tools.
.PARAMETER Docs
    Also install Doxygen into .tools, to build the docs.
.PARAMETER InstallBuildTools
    Install the Visual Studio 2022 Build Tools with the C++ workload. Not needed if Visual Studio with the "Desktop
    development with C++" workload is already installed.
.PARAMETER CMakeVersion
    CMake version to install.
.PARAMETER StyleToolsVersion
    clang-format/clang-tidy major version to install.
.PARAMETER StyleToolsRelease
    Release of cpp-linter/clang-tools-static-binaries to download the style tools from.
.PARAMETER DoxygenVersion
    Doxygen version to install with -Docs.
#>
param (
    [switch]$SkipCMake,
    [switch]$SkipStyleTools,
    [switch]$Docs,
    [switch]$InstallBuildTools,
    [string]$CMakeVersion = "3.31.6",
    [string]$StyleToolsVersion = "18",
    [string]$StyleToolsRelease = "2026.09.01-5fb8802d",
    [string]$DoxygenVersion = "1.18.0"
)

$MinCMakeVersion = [Version]"3.21.0"
$RepoRoot = Split-Path $PSScriptRoot -Parent

function Update-SessionPath {
    if (Get-Command refreshenv -ErrorAction SilentlyContinue) {
        refreshenv | Out-Null
    } else {
        $machinePath = [System.Environment]::GetEnvironmentVariable("PATH", [System.EnvironmentVariableTarget]::Machine)
        $userPath = [System.Environment]::GetEnvironmentVariable("PATH", [System.EnvironmentVariableTarget]::User)
        $env:PATH = "$machinePath;$userPath"
    }
}

function Install-ChocoPackage {
    param ([string]$Name, [string[]]$Arguments)

    Write-Output "Installing $Name..."
    choco install -y $Name @Arguments | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to install $Name (exit code $LASTEXITCODE)." -ErrorAction Continue
        exit $LASTEXITCODE
    }
    Update-SessionPath
}

function Get-CMakeVersion {
    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
    if (-not $cmake) {
        return $null
    }
    $versionLine = (& $cmake.Source --version | Select-Object -First 1)
    if ($versionLine -match "(\d+\.\d+\.\d+)") {
        return [Version]$Matches[1]
    }
    return $null
}

$CurrentIdentity = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $CurrentIdentity.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "This script must be run as administrator since it downloads/installs software dependencies." -ErrorAction Continue
    exit -1
}

# Ensure TLS 1.2 is used, older versions of powershell use TLS 1.0 as the default
#
[Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12

# Use chocolatey to install dependencies
#
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Output "Installing Chocolatey..."
    Set-ExecutionPolicy Bypass -Scope Process -Force
    Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    Update-SessionPath
}

if ($env:ChocolateyInstall -and (Test-Path "$env:ChocolateyInstall\helpers\chocolateyProfile.psm1")) {
    Import-Module "$env:ChocolateyInstall\helpers\chocolateyProfile.psm1"
}

if (-not $SkipCMake) {
    $currentVersion = Get-CMakeVersion
    if ($currentVersion -and $currentVersion -ge $MinCMakeVersion) {
        Write-Output "Found CMake $currentVersion"
    } else {
        Install-ChocoPackage "cmake" @("--version=$CMakeVersion", "--installargs", '"ADD_CMAKE_TO_PATH=System"')
        if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
            Write-Warning "Could not detect cmake.exe after install. Shell may need to be restarted."
        }
    }
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Install-ChocoPackage "git"
}

if (-not (Get-Command vswhere -ErrorAction SilentlyContinue)) {
    Install-ChocoPackage "vswhere"
    if (-not (Get-Command vswhere -ErrorAction SilentlyContinue)) {
        Write-Warning "Could not detect vswhere after install. Shell may need to be restarted."
    }
}

if ($InstallBuildTools) {
    Install-ChocoPackage "visualstudio2022buildtools"
    Install-ChocoPackage "visualstudio2022-workload-vctools" @("--package-parameters", "--includeRecommended")
}

if (Get-Command vswhere -ErrorAction SilentlyContinue) {
    $vsInstalls = vswhere -products * -property installationPath -requires "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
    if (-not $vsInstalls) {
        Write-Warning "No Visual Studio installation with the C++ toolchain was found. Re-run with -InstallBuildTools, or install the ""Desktop development with C++"" workload."
    }
}

if (-not $SkipStyleTools) {
    $arch = if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") { "arm64" } else { "amd64" }
    $toolsDir = Join-Path $RepoRoot ".tools\bin"
    Write-Output "Installing clang-format and clang-tidy $StyleToolsVersion to $toolsDir..."
    New-Item -ItemType Directory -Force -Path $toolsDir | Out-Null
    foreach ($tool in @("clang-format", "clang-tidy")) {
        $url = "https://github.com/cpp-linter/clang-tools-static-binaries/releases/download/$StyleToolsRelease/$tool-${StyleToolsVersion}_windows-$arch.exe"
        try {
            Invoke-WebRequest -Uri $url -OutFile (Join-Path $toolsDir "$tool-$StyleToolsVersion.exe") -UseBasicParsing
        } catch {
            Write-Error "Failed to download ${tool}: $_" -ErrorAction Continue
            exit 1
        }
    }
}

if ($Docs) {
    $toolsDir = Join-Path $RepoRoot ".tools\bin"
    $archive = "doxygen-$DoxygenVersion.windows.x64.bin.zip"
    $downloadDir = Join-Path ([System.IO.Path]::GetTempPath()) "eolib-doxygen-$PID"
    Write-Output "Installing Doxygen $DoxygenVersion to $toolsDir..."
    New-Item -ItemType Directory -Force -Path $toolsDir, $downloadDir | Out-Null
    try {
        $release = "Release_" + $DoxygenVersion.Replace(".", "_")
        Invoke-WebRequest -Uri "https://github.com/doxygen/doxygen/releases/download/$release/$archive" -OutFile (Join-Path $downloadDir $archive) -UseBasicParsing
        Expand-Archive -Path (Join-Path $downloadDir $archive) -DestinationPath $downloadDir -Force
        # doxygen.exe loads libclang.dll from its own directory.
        Copy-Item -Path (Join-Path $downloadDir "doxygen.exe"), (Join-Path $downloadDir "libclang.dll") -Destination $toolsDir -Force
    } catch {
        Write-Error "Failed to download Doxygen: $_" -ErrorAction Continue
        exit 1
    } finally {
        Remove-Item -Recurse -Force $downloadDir -ErrorAction SilentlyContinue
    }
}

Write-Output ""
Write-Output "Done. Initialize the submodules if you haven't already: git submodule update --init --recursive"
