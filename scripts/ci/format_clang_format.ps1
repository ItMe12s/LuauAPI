#Requires -Version 5.1
$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
Set-Location $root

$files = @(
    Get-ChildItem -Path src, include -Recurse -File -Include '*.cpp', '*.hpp'
    Get-ChildItem -Path tests -Recurse -File -Include '*.cpp', '*.hpp' |
    Where-Object { ($_.FullName -replace '\\', '/') -notmatch '/tests/host/' }
)

if ($files.Count -eq 0) {
    Write-Error 'No C++ files found for clang-format.'
}

$failed = 0
foreach ($file in $files) {
    & clang-format -i $file.FullName
    if ($LASTEXITCODE -ne 0) {
        $failed++
        Write-Warning "clang-format failed: $($file.FullName)"
    }
}

if ($failed -gt 0) {
    exit 1
}

Write-Host "clang-format: formatted $($files.Count) files"
