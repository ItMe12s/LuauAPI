#Requires -Version 5.1
param(
    [Parameter()]
    [ValidateSet('clang', 'python', 'all')]
    [string]$Target = 'all',

    [switch]$Check
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Set-Location $root

function Invoke-ClangFormat {
    param([switch]$CheckOnly)

    $files = @(
        Get-ChildItem -Path src, include, tests -Recurse -File -Include '*.cpp', '*.hpp'
    )

    if ($files.Count -eq 0) {
        Write-Error 'No C++ files found for clang-format.'
    }

    $failed = 0
    foreach ($file in $files) {
        if ($CheckOnly) {
            & clang-format --dry-run --Werror $file.FullName
        }
        else {
            & clang-format -i $file.FullName
        }
        if ($LASTEXITCODE -ne 0) {
            $failed++
            if (-not $CheckOnly) {
                Write-Warning "clang-format failed: $($file.FullName)"
            }
        }
    }

    if ($failed -gt 0) {
        return 1
    }

    if ($CheckOnly) {
        Write-Host "clang-format: $($files.Count) files OK"
    }
    else {
        Write-Host "clang-format: formatted $($files.Count) files"
    }
    return 0
}

function Invoke-PythonFormat {
    param([switch]$CheckOnly)

    $paths = @('tools', 'tests')

    if ($CheckOnly) {
        & ruff format --check @paths
        if ($LASTEXITCODE -ne 0) {
            Write-Host 'Run .\tools\format.ps1 -Target python to fix formatting.' -ForegroundColor Yellow
            return $LASTEXITCODE
        }
        Write-Host "ruff format: OK ($($paths -join ', '))"
        return 0
    }

    & ruff format @paths
    if ($LASTEXITCODE -ne 0) {
        return $LASTEXITCODE
    }

    Write-Host "ruff format: formatted $($paths -join ', ')"
    return 0
}

$exitCode = 0

if ($Target -eq 'clang' -or $Target -eq 'all') {
    $clangExit = Invoke-ClangFormat -CheckOnly:$Check
    if ($clangExit -ne 0) {
        $exitCode = $clangExit
    }
}

if ($Target -eq 'python' -or $Target -eq 'all') {
    $pythonExit = Invoke-PythonFormat -CheckOnly:$Check
    if ($pythonExit -ne 0) {
        $exitCode = $pythonExit
    }
}

if ($exitCode -ne 0) {
    exit $exitCode
}
