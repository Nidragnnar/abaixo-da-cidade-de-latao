# Desenha os sprites PROVISÓRIOS dos voadores (64x64), na ordem dos quadros de render\AnimacaoInimigos.h:
# parado, andar 1, andar 2, atacar 1, atacar 2, morrer, destroços. Também a bomba e a explosão.
$ErrorActionPreference = "Stop"
Set-Location (Split-Path $PSScriptRoot)
Add-Type -AssemblyName System.Drawing

function Cor($r, $g, $b) { [Drawing.Color]::FromArgb(255, $r, $g, $b) }
function Pincel($c) { New-Object Drawing.SolidBrush $c }
function Ret($g, $c, $x, $y, $l, $a) { $g.FillRectangle((Pincel $c), $x, $y, $l, $a) }
function Elipse($g, $c, $x, $y, $l, $a) { $g.FillEllipse((Pincel $c), $x, $y, $l, $a) }
function Poligono($g, $c, $pontos) {
    $p = for ($i = 0; $i -lt $pontos.Count; $i += 2) { New-Object Drawing.Point $pontos[$i], $pontos[$i + 1] }
    $g.FillPolygon((Pincel $c), [Drawing.Point[]]$p)
}
function Desenhar($arquivo, $desenho) {
    $b = New-Object Drawing.Bitmap 64, 64, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [Drawing.Graphics]::FromImage($b)
    $g.SmoothingMode = 'None'  # bordas duras: o jogo descarta pixel com alfa < 0,5
    & $desenho $g
    $b.Save("$PWD\assets\sprites\$arquivo", [Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $b.Dispose()
    "assets\sprites\$arquivo"
}

$latao = Cor 200 150 56; $lataoEscuro = Cor 120 84 30; $ferro = Cor 70 66 62; $ferroClaro = Cor 120 116 110
$asa = Cor 190 205 210; $asaEscura = Cor 140 155 165; $olho = Cor 255 70 40; $fogo = Cor 255 150 40
$amarelo = Cor 255 230 120; $lona = Cor 150 128 100; $lonaEscura = Cor 105 88 68

# --- Vespa a corda: abdômen listrado, cabeça com olhos vermelhos, asas para cima ou para baixo ---
function Vespa($g, $asasAltas, $ferrao, $quebrada) {
    if (-not $quebrada) {
        if ($asasAltas) { Poligono $g $asa @(26, 30, 10, 4, 22, 6, 32, 28); Poligono $g $asa @(38, 30, 54, 4, 42, 6, 32, 28) }
        else { Poligono $g $asaEscura @(26, 32, 4, 44, 10, 50, 30, 36); Poligono $g $asaEscura @(38, 32, 60, 44, 54, 50, 34, 36) }
    }
    Elipse $g $latao 20 28 24 22                       # tórax
    Elipse $g $latao 14 38 36 20                       # abdômen
    foreach ($x in 20, 30, 40) { Ret $g $ferro $x 39 4 18 }  # listras
    Elipse $g $lataoEscuro 22 18 20 16                 # cabeça
    Elipse $g $olho 24 22 6 6; Elipse $g $olho 34 22 6 6
    if ($ferrao) { Poligono $g $ferro @(28, 56, 36, 56, 32, 64) }
    Ret $g $ferroClaro 30 12 4 8                       # chave de corda
    Ret $g $ferroClaro 26 10 12 3
}
Desenhar "vespa_0.png" { param($g) Vespa $g $true $false $false }
Desenhar "vespa_1.png" { param($g) Vespa $g $true $false $false }
Desenhar "vespa_2.png" { param($g) Vespa $g $false $false $false }
Desenhar "vespa_3.png" { param($g) Vespa $g $false $true $false }
Desenhar "vespa_4.png" { param($g) Vespa $g $true $true $false }
Desenhar "vespa_5.png" { param($g) Vespa $g $false $false $true; Elipse $g $fogo 22 20 20 20; Elipse $g $amarelo 28 26 8 8 }
Desenhar "vespa_6.png" { param($g)
    Elipse $g $latao 6 50 22 12; Ret $g $ferro 12 52 4 10; Elipse $g $lataoEscuro 32 52 14 10
    Poligono $g $asaEscura @(46, 60, 62, 54, 60, 62); Ret $g $ferroClaro 26 58 10 3
}

# --- Dirigível a vapor: balão de lona com cintas de latão, gôndola com caldeira e hélice ---
function Dirigivel($g, $helice, $alcapao, $rasgado) {
    Elipse $g $lona 2 2 60 34                          # balão
    Elipse $g $lonaEscura 2 22 60 14
    foreach ($x in 14, 30, 46) { Ret $g $latao $x 4 3 30 }  # cintas
    if ($rasgado) { Poligono $g $fogo @(20, 10, 30, 2, 36, 12, 44, 4, 42, 20, 24, 22); Elipse $g $amarelo 28 10 10 8 }
    Ret $g $lataoEscuro 22 34 2 8; Ret $g $lataoEscuro 40 34 2 8  # cabos
    Ret $g $ferro 18 42 28 14                          # gôndola
    Ret $g $latao 18 42 28 3
    Elipse $g $lataoEscuro 22 45 10 10                 # caldeira
    Elipse $g $fogo 25 48 4 4
    if ($alcapao) { Ret $g $ferroClaro 34 56 10 6; Elipse $g $ferro 36 56 6 6 }  # bomba saindo
    else { Ret $g $ferroClaro 34 54 10 2 }
    Ret $g $ferroClaro 46 47 4 4                       # eixo da hélice
    if ($helice) { Ret $g $ferroClaro 50 38 3 22 } else { Ret $g $ferroClaro 44 48 18 3 }
}
Desenhar "dirigivel_0.png" { param($g) Dirigivel $g $true $false $false }
Desenhar "dirigivel_1.png" { param($g) Dirigivel $g $true $false $false }
Desenhar "dirigivel_2.png" { param($g) Dirigivel $g $false $false $false }
Desenhar "dirigivel_3.png" { param($g) Dirigivel $g $true $true $false }
Desenhar "dirigivel_4.png" { param($g) Dirigivel $g $false $true $false }
Desenhar "dirigivel_5.png" { param($g) Dirigivel $g $false $false $true }
Desenhar "dirigivel_6.png" { param($g)
    Poligono $g $lona @(2, 64, 10, 46, 24, 52, 36, 44, 52, 50, 62, 64)  # lona murcha no chão
    foreach ($x in 14, 30, 46) { Ret $g $latao $x 50 3 14 }
    Ret $g $ferro 20 54 22 10; Elipse $g $lataoEscuro 24 54 10 10
}

# --- Bomba (esfera de ferro com pavio aceso) e explosão (desenhada acesa) ---
Desenhar "bomba.png" { param($g)
    Elipse $g $ferro 12 16 40 40; Elipse $g $ferroClaro 20 22 10 8
    Ret $g $lataoEscuro 28 8 8 10; Elipse $g $fogo 26 0 12 12; Elipse $g $amarelo 29 3 6 6
}
Desenhar "explosao.png" { param($g)
    Poligono $g $fogo @(32, 0, 40, 18, 62, 10, 48, 28, 64, 40, 44, 42, 50, 64, 32, 48, 14, 64, 20, 42, 0, 40, 16, 28, 2, 10, 24, 18)
    Elipse $g $amarelo 18 18 28 28; Elipse $g (Cor 255 250 220) 26 26 12 12
}
