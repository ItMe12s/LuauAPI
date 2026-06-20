#Requires -Version 5.1
param(
    [ValidateSet('clang', 'python', 'all')]
    [string]$Target = 'all',
    [switch]$Check
)

$ErrorActionPreference = 'Stop'
Set-Location (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path

if ($Target -in 'clang', 'all') {
    $files = @(
        Get-ChildItem src, include -Recurse -File -Include '*.cpp', '*.hpp'
        Get-ChildItem tests -Recurse -File -Include '*.cpp', '*.hpp' |
            Where-Object { ($_.FullName -replace '\\', '/') -notmatch '/tests/host/' }
    )
    $clangArgs = if ($Check) { @('--dry-run', '--Werror') } else { @('-i') }
    & clang-format @clangArgs @($files.FullName)
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
}

if ($Target -in 'python', 'all') {
    $pyArgs = if ($Check) { @('format', '--check') } else { @('format') }
    & ruff @pyArgs tools tests/luau_codegen
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
}
