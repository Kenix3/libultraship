[CmdletBinding()]
param(
    [switch]$Check
)

$ErrorActionPreference = 'Stop'

$llvmVersion = '14.0.6'
$llvmUrl = "https://github.com/llvm/llvm-project/releases/download/llvmorg-$llvmVersion/LLVM-$llvmVersion-win64.exe"
$localClangFormat = Join-Path $PSScriptRoot 'clang-format.exe'

function Get-ClangFormatMajor {
    param([string]$Exe)
    try {
        $versionText = & $Exe --version
    } catch {
        return $null
    }
    if ($versionText -match 'version\s+(\d+)\.') {
        return [int]$Matches[1]
    }
    return $null
}

function Get-7ZipPath {
    $cmd = Get-Command '7z.exe' -ErrorAction SilentlyContinue
    if ($null -ne $cmd) {
        return $cmd.Source
    }
    $defaultPath = 'C:\Program Files\7-Zip\7z.exe'
    if (Test-Path $defaultPath -PathType Leaf) {
        return $defaultPath
    }
    return $null
}

$clangFormat = $null

if (Test-Path $localClangFormat -PathType Leaf) {
    if ((Get-ClangFormatMajor $localClangFormat) -eq 14) {
        $clangFormat = $localClangFormat
    } else {
        Write-Host 'Local clang-format.exe is not version 14, replacing it.'
        Remove-Item $localClangFormat -Force
    }
}

if ($null -eq $clangFormat) {
    $onPath = Get-Command 'clang-format' -ErrorAction SilentlyContinue
    if ($null -ne $onPath) {
        if ((Get-ClangFormatMajor $onPath.Source) -eq 14) {
            $clangFormat = $onPath.Source
            Write-Host "Using clang-format from PATH: $clangFormat"
        }
    }
}

if ($null -eq $clangFormat) {
    $sevenZip = Get-7ZipPath
    if ($null -eq $sevenZip) {
        Write-Host 'clang-format 14 was not found, and 7-Zip is needed to extract it from the LLVM installer.'
        Write-Host 'Install 7-Zip, or put a clang-format 14 on PATH, then re-run.'
        exit 1
    }

    $installer = Join-Path $PSScriptRoot "LLVM-$llvmVersion-win64.exe"
    Write-Host "Downloading LLVM $llvmVersion to extract clang-format..."
    Invoke-WebRequest -Uri $llvmUrl -OutFile $installer

    Write-Host 'Extracting clang-format.exe...'
    & $sevenZip e $installer 'bin\clang-format.exe' "-o$PSScriptRoot" -aoa | Out-Null
    Remove-Item $installer -Force

    if (-not (Test-Path $localClangFormat -PathType Leaf)) {
        Write-Host 'Extraction failed: clang-format.exe was not produced.'
        exit 1
    }
    $clangFormat = $localClangFormat
}

Write-Host "Using $(& $clangFormat --version)"

$roots = @('src', 'include') | ForEach-Object { Join-Path $PSScriptRoot $_ } | Where-Object { Test-Path $_ }

$files = Get-ChildItem -Path $roots -Recurse -File |
    Where-Object { $_.Extension -in '.cpp', '.h' }

if ($files.Count -eq 0) {
    Write-Host 'No source files found. Run this from the repository root.'
    exit 1
}

$index = 0
foreach ($file in $files) {
    $index++
    $relativePath = $file.FullName.Substring($PSScriptRoot.Length + 1)
    Write-Host "Formatting [$index/$($files.Count)] $relativePath"
    & $clangFormat -i $file.FullName
}

Write-Host "Formatted $($files.Count) files."
