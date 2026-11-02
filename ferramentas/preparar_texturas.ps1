# Extrai dos pacotes em arte_fonte/ as texturas escolhidas e reduz para 64x64 em assets/texturas/
$ErrorActionPreference = "Stop"
Set-Location (Split-Path $PSScriptRoot)
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.IO.Compression.FileSystem

$tiny = "arte_fonte\sbs_tiny_texture_pack_1_128x128.zip"
$zortch = "arte_fonte\zortch_world_textures.zip"

# destino => (pacote, caminho dentro do zip)
$escolhas = [ordered]@{
    "esgoto_parede.png"     = @($tiny, "128x128/Bricks/Brick 5 - 128x128.png")
    "esgoto_chao.png"       = @($tiny, "128x128/Tile/Tile 24 - 128x128.png")
    "esgoto_teto.png"       = @($tiny, "128x128/Bricks/Brick 1 - 128x128.png")
    "fabrica_parede.png"    = @($zortch, "zortchskin/tile07.png")
    "fabrica_chao.png"      = @($zortch, "zortchskin/tile107.png")
    "fabrica_teto.png"      = @($zortch, "zortchskin/tile33.png")
    "minas_parede.png"      = @($tiny, "128x128/Bricks/Brick 25 - 128x128.png")
    "minas_chao.png"        = @($zortch, "zortchskin/tile05.png")
    "minas_teto.png"        = @($tiny, "128x128/Bricks/Brick 23 - 128x128.png")
    "comporta.png"          = @($zortch, "zortchskin/bigdoor.png")
    "comporta_trancada.png" = @($zortch, "zortchskin/bigdoor2.png")
}

foreach ($destino in $escolhas.Keys) {
    $pacote, $interno = $escolhas[$destino]
    $zip = [IO.Compression.ZipFile]::OpenRead((Resolve-Path $pacote))
    try {
        $entrada = $zip.Entries | Where-Object { $_.FullName -eq $interno }
        if (-not $entrada) { throw "não achei $interno em $pacote" }
        $fluxo = $entrada.Open()
        $original = [Drawing.Image]::FromStream($fluxo)
        $nova = New-Object Drawing.Bitmap 64, 64, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $g = [Drawing.Graphics]::FromImage($nova)
        $g.InterpolationMode = 'HighQualityBicubic'
        $g.PixelOffsetMode = 'HighQuality'
        $atributos = New-Object Drawing.Imaging.ImageAttributes
        $atributos.SetWrapMode('Tile')  # sem borda escura ao reduzir
        $g.DrawImage($original, (New-Object Drawing.Rectangle 0, 0, 64, 64), 0, 0, $original.Width, $original.Height, 'Pixel', $atributos)
        $nova.Save("$PWD\assets\texturas\$destino", [Drawing.Imaging.ImageFormat]::Png)
        $g.Dispose(); $nova.Dispose(); $original.Dispose(); $fluxo.Dispose()
        "$destino <- $interno"
    } finally { $zip.Dispose() }
}
