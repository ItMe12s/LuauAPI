#Requires -Version 5.1
$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
Set-Location $root

$paths = @('tools', 'tests/luau_codegen')

& ruff format @paths
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "ruff format: formatted $($paths -join ', ')"
