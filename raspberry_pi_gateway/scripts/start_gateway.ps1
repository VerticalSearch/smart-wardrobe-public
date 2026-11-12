$env:PYTHONPATH = Join-Path $PSScriptRoot "..\src"
python (Join-Path $PSScriptRoot "..\run.py")

