param(
  [string]$EmpExe = "..\..\out\build\make-x64\Debug\emp.exe"
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $here
try {
  if (-not (Test-Path $EmpExe)) {
    throw "emp.exe not found at: $EmpExe"
  }

  $outExe = Join-Path $here "hashmap_bench.exe"

  Write-Host "Building: $outExe" -ForegroundColor Cyan
  & $EmpExe "main.em" --out $outExe

  Write-Host "Running..." -ForegroundColor Cyan
  & $outExe
} finally {
  Pop-Location
}
