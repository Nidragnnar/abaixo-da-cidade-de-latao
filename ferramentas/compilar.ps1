# Compila e roda os testes; uso: .\ferramentas\compilar.ps1 [debug|release]
param([string]$Config = "debug")
$env:PATH = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;" + $env:PATH
Set-Location (Split-Path $PSScriptRoot)
cmake --build --preset $Config *> build\compilar.log
$codigo = $LASTEXITCODE
# Só avisos e erros do nosso código (fora de _deps)
Select-String -Path build\compilar.log -Pattern "warning|error" |
    Where-Object { $_.Line -notmatch "_deps|SDL_error" } |
    Select-Object -First 30 | ForEach-Object { $_.Line }
if ($codigo -ne 0) { "COMPILAÇÃO FALHOU (código $codigo)"; exit $codigo }
"compilação ok"
ctest --preset $Config *> build\testes.log
$codigo = $LASTEXITCODE
Get-Content build\testes.log | Select-String -Pattern "tests passed|FAILED|ERROR|CHECK|REQUIRE|TEST CASE|error" | Select-Object -First 40 | ForEach-Object { $_.Line }
# Resumo do doctest (quantos casos e asserções)
& "build\$((Get-Culture).TextInfo.ToTitleCase($Config))\testes_roguelike.exe" 2>&1 | Select-String "test cases|assertions" | ForEach-Object { $_.Line }
exit $codigo
