#Requires -Version 5.1
$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
Set-Location $root

$paths = @('tools', 'tests/luau_codegen')

& ruff format --check @paths
if ($LASTEXITCODE -ne 0) {
    Write-Host 'Run .\scripts\ci\format_python.ps1 to fix formatting.' -ForegroundColor Yellow
    exit $LASTEXITCODE
}

Write-Host "ruff format: OK ($($paths -join ', '))"
