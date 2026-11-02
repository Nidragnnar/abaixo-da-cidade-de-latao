# Desenha os sprites PROVISÓRIOS dos itens (64x64: no chão e como ícone) e das armas novas na mão (128x128)
$ErrorActionPreference = "Stop"
Set-Location (Split-Path $PSScriptRoot)
Add-Type -AssemblyName System.Drawing
New-Item -ItemType Directory -Force assets\sprites, assets\armas | Out-Null

function Cor($r, $g, $b) { [Drawing.Color]::FromArgb(255, $r, $g, $b) }
function Pincel($c) { New-Object Drawing.SolidBrush $c }
function Ret($g, $c, $x, $y, $l, $a) { $g.FillRectangle((Pincel $c), $x, $y, $l, $a) }
function Elipse($g, $c, $x, $y, $l, $a) { $g.FillEllipse((Pincel $c), $x, $y, $l, $a) }
function Poligono($g, $c, $pontos) {
    $p = for ($i = 0; $i -lt $pontos.Count; $i += 2) { New-Object Drawing.Point $pontos[$i], $pontos[$i + 1] }
    $g.FillPolygon((Pincel $c), [Drawing.Point[]]$p)
}
function Salvar($arquivo, $lado, $quadro, [scriptblock]$desenho) {
    $b = New-Object Drawing.Bitmap $lado, $lado, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [Drawing.Graphics]::FromImage($b)
    $g.SmoothingMode = 'None'  # bordas duras: o jogo descarta pixel com alfa < 0,5
    & $desenho $g $quadro
    $b.Save("$PWD\$arquivo", [Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $b.Dispose()
}

$ferro = Cor 78 74 70; $ferroClaro = Cor 150 144 134; $ferroEscuro = Cor 44 40 38
$latao = Cor 200 150 56; $lataoEscuro = Cor 120 84 30; $cobre = Cor 170 96 52
$couro = Cor 110 72 42; $couroEscuro = Cor 70 44 26; $madeira = Cor 120 80 44
$verde = Cor 70 170 90; $verdeClaro = Cor 150 230 160; $azul = Cor 90 170 255; $azulClaro = Cor 200 240 255
$vermelho = Cor 170 40 30; $amarelo = Cor 250 210 80

# --- Itens (vistos de frente, ocupando o meio da imagem) ---
$itens = [ordered]@{
    chave_inglesa = { param($g, $q) Poligono $g $ferroClaro @(28, 18, 36, 18, 36, 56, 28, 56); Ret $g $ferroClaro 20 8 24 12; Ret $g (Cor 0 0 0) 28 4 8 12 }
    sabre         = { param($g, $q) Poligono $g $ferroClaro @(30, 4, 36, 4, 36, 44, 30, 44); Ret $g $latao 22 44 22 4; Ret $g $couroEscuro 30 48 6 12; Elipse $g $latao 29 58 8 6 }
    martelo       = { param($g, $q) Ret $g $madeira 29 26 6 34; Ret $g $ferro 14 10 36 18; Ret $g $latao 14 16 36 4; Ret $g $cobre 46 12 6 14 }
    revolver      = { param($g, $q) Ret $g $ferroEscuro 8 22 36 8; Elipse $g $latao 30 20 16 16; Poligono $g $madeira @(40, 30, 52, 30, 56, 52, 46, 52) }
    bacamarte     = { param($g, $q) Ret $g $ferroEscuro 4 22 40 6; Ret $g $ferroEscuro 4 29 40 6; Ret $g $latao 40 20 8 16; Poligono $g $madeira @(46, 22, 60, 26, 60, 44, 46, 38) }
    tesla         = { param($g, $q) Ret $g $ferro 10 24 30 10; Elipse $g $azul 6 18 14 22; Elipse $g $azulClaro 9 22 8 14; Ret $g $cobre 22 22 4 14; Ret $g $cobre 30 22 4 14; Poligono $g $couroEscuro @(36, 30, 48, 30, 52, 52, 42, 52) }
    casaco        = { param($g, $q) Poligono $g $couro @(16, 10, 48, 10, 56, 58, 8, 58); Poligono $g $couroEscuro @(26, 10, 38, 10, 32, 30); Ret $g $latao 31 34 3 3; Ret $g $latao 31 44 3 3 }
    colete        = { param($g, $q) Poligono $g $ferro @(16, 12, 48, 12, 52, 54, 12, 54); Ret $g $ferroClaro 18 18 12 14; Ret $g $ferroClaro 34 18 12 14; Ret $g $ferroClaro 18 36 28 12 }
    armadura      = { param($g, $q) Poligono $g $latao @(14, 10, 50, 10, 54, 56, 10, 56); Elipse $g $lataoEscuro 18 16 28 18; Ret $g $lataoEscuro 14 40 40 4; Elipse $g (Cor 255 230 150) 22 18 8 6 }
    tonico        = { param($g, $q) Ret $g $couroEscuro 27 6 10 8; Ret $g $vidroTampa 26 12 12 6; Elipse $g $verde 14 18 36 40; Elipse $g $verdeClaro 20 24 10 12 }
    balas         = { param($g, $q) Ret $g $lataoEscuro 10 26 44 30; Ret $g $latao 10 26 44 6; for ($i = 0; $i -lt 5; $i++) { Ret $g $latao (14 + 8 * $i) 14 5 14; Elipse $g $cobre (14 + 8 * $i) 10 5 6 } }
    cartuchos     = { param($g, $q) Ret $g $vermelho 10 24 44 32; Ret $g (Cor 220 200 160) 10 24 44 6; for ($i = 0; $i -lt 4; $i++) { Ret $g (Cor 200 50 40) (14 + 10 * $i) 12 7 14; Ret $g $latao (14 + 10 * $i) 22 7 4 } }
    celulas       = { param($g, $q) for ($i = 0; $i -lt 3; $i++) { Ret $g $ferro (10 + 16 * $i) 16 12 40; Ret $g $azul (12 + 16 * $i) 20 8 30; Poligono $g $amarelo @((16 + 16 * $i), 22, (14 + 16 * $i), 34, (18 + 16 * $i), 34, (15 + 16 * $i), 46) } }
    engrenagens   = { param($g, $q) foreach ($e in @(@(8, 22, 28), @(30, 14, 24), @(26, 34, 26))) { $x, $y, $d = $e; Elipse $g $lataoEscuro ($x - 3) ($y - 3) ($d + 6) ($d + 6); Elipse $g $latao $x $y $d $d; Elipse $g $lataoEscuro ($x + $d / 3) ($y + $d / 3) ($d / 3) ($d / 3) } }
}
$vidroTampa = Cor 120 120 120
foreach ($nome in $itens.Keys) { Salvar "assets\sprites\item_$nome.png" 64 0 $itens[$nome] }

# --- Armas novas na mão (vistas de trás, 128x128) ---
function Mao($g, $x, $y) { Elipse $g $couro $x $y 44 40; Ret $g $couroEscuro ($x + 6) ($y + 30) 32 40 }
$sabre = {
    param($g, $q)
    if ($q -eq 1) { Poligono $g $ferroClaro @(20, 20, 30, 14, 84, 90, 74, 96); Ret $g $latao 64 84 26 6; Mao $g 62 92 }
    else { Poligono $g $ferroClaro @(62, 6, 70, 6, 70, 90, 62, 90); Ret $g $latao 50 86 32 6; Mao $g 44 90 }
}
$martelo = {
    param($g, $q)
    $dy = if ($q -eq 1) { -26 } else { 0 }
    Ret $g $madeira 60 (40 + $dy) 10 60; Ret $g $ferro 36 (14 + $dy) 58 30; Ret $g $latao 36 (24 + $dy) 58 6; Ret $g $cobre 90 (18 + $dy) 8 22
    Mao $g 42 (88 + $dy)
}
$bacamarte = {
    param($g, $q)
    $dy = if ($q -eq 2) { 30 } else { 0 }
    if ($q -eq 1) { Elipse $g $amarelo 36 0 56 40; Elipse $g (Cor 255 250 220) 50 8 28 22 }
    Ret $g $ferroEscuro 50 (24 + $dy) 12 52; Ret $g $ferroEscuro 66 (24 + $dy) 12 52
    Elipse $g (Cor 15 15 15) 52 (24 + $dy) 8 8; Elipse $g (Cor 15 15 15) 68 (24 + $dy) 8 8
    Ret $g $latao 46 (70 + $dy) 36 10; Poligono $g $madeira @(48, (80 + $dy), 80, (80 + $dy), 86, (120 + $dy), 42, (120 + $dy))
    if ($q -eq 2) { Elipse $g $vermelho 84 (40 + $dy) 12 18 }   # cartucho entrando
    Mao $g 40 (88 + $dy)
}
$tesla = {
    param($g, $q)
    if ($q -eq 1) { Poligono $g $azulClaro @(64, 0, 56, 22, 68, 22, 58, 44, 76, 18, 64, 18, 72, 0) }
    Ret $g $ferro 54 40 22 46; Elipse $g $azul 48 30 34 28; Elipse $g $azulClaro 56 36 18 14
    Ret $g $cobre 52 56 4 24; Ret $g $cobre 74 56 4 24
    Mao $g 42 88
}
for ($q = 0; $q -lt 2; $q++) { Salvar "assets\armas\sabre_$q.png" 128 $q $sabre }
for ($q = 0; $q -lt 2; $q++) { Salvar "assets\armas\martelo_$q.png" 128 $q $martelo }
for ($q = 0; $q -lt 3; $q++) { Salvar "assets\armas\bacamarte_$q.png" 128 $q $bacamarte }
for ($q = 0; $q -lt 2; $q++) { Salvar "assets\armas\tesla_$q.png" 128 $q $tesla }
"itens e armas gerados"
