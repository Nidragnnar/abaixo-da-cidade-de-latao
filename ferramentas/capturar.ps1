# Tira uma captura do jogo e converte para PNG; uso:
#   .\ferramentas\capturar.ps1 <nome> [-Release] [argumentos do jogo...]   (gera capturas\<nome>.png)
param(
    [Parameter(Mandatory, Position = 0)][string]$Nome,
    [switch]$Release,
    [Parameter(ValueFromRemainingArguments)][string[]]$Argumentos
)
$ErrorActionPreference = "Stop"
Set-Location (Split-Path $PSScriptRoot)
New-Item -ItemType Directory -Force capturas | Out-Null
$config = if ($Release) { "Release" } else { "Debug" }
$bmp = "$PWD\capturas\$Nome.bmp"
# Áudio de mentira: o caminho inteiro do som roda, mas nada sai nos alto-falantes
$env:SDL_AUDIODRIVER = "dummy"
Start-Process "$PWD\build\$config\Roguelike.exe" -ArgumentList (@("--captura", $bmp) + $Argumentos) -Wait
Add-Type -AssemblyName System.Drawing
$img = [Drawing.Image]::FromFile($bmp)
$img.Save("$PWD\capturas\$Nome.png", [Drawing.Imaging.ImageFormat]::Png)
$img.Dispose()
Remove-Item $bmp
"capturas\$Nome.png"
