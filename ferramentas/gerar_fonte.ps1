# Rasteriza a fonte Press Start 2P (arte_fonte/) numa grade 16 × N de glifos 8x8 em assets/fonte/fonte.png,
# na mesma ordem de render::caracteresFonte() (ASCII 32–126 + acentos do português)
$ErrorActionPreference = "Stop"
Set-Location (Split-Path $PSScriptRoot)
Add-Type -AssemblyName System.Drawing

$caracteres = (-join (32..126 | ForEach-Object { [char]$_ })) + "ÁÂÃÀÇÉÊÍÓÔÕÚáâãàçéêíóôõú—º–×"
$colunas = 16
$linhas = [math]::Ceiling($caracteres.Length / $colunas)

$colecao = New-Object Drawing.Text.PrivateFontCollection
$colecao.AddFontFile("$PWD\arte_fonte\PressStart2P-Regular.ttf")
$fonte = New-Object Drawing.Font $colecao.Families[0], 8, ([Drawing.GraphicsUnit]::Pixel)

$imagem = New-Object Drawing.Bitmap ($colunas * 8), ($linhas * 8), ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [Drawing.Graphics]::FromImage($imagem)
$g.Clear([Drawing.Color]::Transparent)
$g.TextRenderingHint = [Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit  # sem suavização
$formato = [Drawing.StringFormat]::GenericTypographic
$branco = [Drawing.Brushes]::White
for ($i = 0; $i -lt $caracteres.Length; $i++) {
    $x = ($i % $colunas) * 8
    $y = [math]::Floor($i / $colunas) * 8
    $g.SetClip((New-Object Drawing.Rectangle $x, $y, 8, 8))
    $g.DrawString([string]$caracteres[$i], $fonte, $branco, [float]$x, [float]($y + $args[0]), $formato)
    $g.ResetClip()
}
New-Item -ItemType Directory -Force assets\fonte | Out-Null
$imagem.Save("$PWD\assets\fonte\fonte.png", [Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $imagem.Dispose(); $fonte.Dispose()
"fonte.png: $($caracteres.Length) glifos"
