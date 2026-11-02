# Desenha os sprites PROVISÓRIOS dos inimigos (7 quadros 64x64 cada), do projétil e das armas na mão (128x128).
# Quadros: 0 parado, 1 andar A, 2 andar B, 3 atacar (preparo), 4 atacar (golpe), 5 morrendo, 6 destroços
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

# Cria a imagem, chama o desenho e salva; o desenho recebe ($g, $quadro)
function Salvar($arquivo, $lado, $quadro, [scriptblock]$desenho) {
    $b = New-Object Drawing.Bitmap $lado, $lado, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [Drawing.Graphics]::FromImage($b)
    $g.SmoothingMode = 'None'  # bordas duras: o jogo descarta pixel com alfa < 0,5
    & $desenho $g $quadro
    $b.Save("$PWD\$arquivo", [Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $b.Dispose()
}

$ferro = Cor 78 74 70; $ferroClaro = Cor 128 120 110; $ferroEscuro = Cor 44 40 38
$latao = Cor 190 140 52; $lataoEscuro = Cor 120 84 30; $cobre = Cor 160 90 50
$vermelho = Cor 230 40 30; $amarelo = Cor 250 210 80; $laranja = Cor 240 130 30
$couro = Cor 98 66 40; $couroEscuro = Cor 62 40 24; $vidro = Cor 90 160 120; $fumaca = Cor 150 150 150

# --- Rato mecânico: corpo de lata visto de frente, orelhas, olhos vermelhos, patas ---
$rato = {
    param($g, $q)
    if ($q -eq 6) {  # destroços: peças no chão
        Ret $g $ferro 8 52 20 8; Elipse $g $latao 30 50 14 10; Ret $g $ferroEscuro 46 56 12 6; Elipse $g $vermelho 24 54 4 4
        return
    }
    $dy = if ($q -eq 5) { 10 } else { 0 }
    $inclina = if ($q -eq 5) { 8 } else { 0 }
    Elipse $g $ferro (12 + $inclina) (24 + $dy) 40 30              # corpo
    Elipse $g $ferroClaro (18 + $inclina) (28 + $dy) 28 12
    Elipse $g $latao (10 + $inclina) (14 + $dy) 14 14              # orelhas
    Elipse $g $latao (40 + $inclina) (14 + $dy) 14 14
    Elipse $g $vermelho (22 + $inclina) (32 + $dy) 6 6             # olhos
    Elipse $g $vermelho (36 + $inclina) (32 + $dy) 6 6
    if ($q -eq 3 -or $q -eq 4) { Ret $g $ferroEscuro (26 + $inclina) (42 + $dy) 12 (4 + 4 * ($q - 3)) }  # boca abrindo
    $pa = if ($q -eq 1) { 0 } elseif ($q -eq 2) { 4 } else { 2 }
    Ret $g $ferroEscuro 16 (52 + $pa) 6 (10 - $pa); Ret $g $ferroEscuro 42 (56 - $pa) 6 (6 + $pa)  # patas
}

# --- Autômato operário: caixa de ferro com olho único, braços de cano ---
$automato = {
    param($g, $q)
    if ($q -eq 6) {
        Ret $g $ferro 6 50 24 12; Ret $g $ferroEscuro 32 54 22 8; Elipse $g $amarelo 18 52 6 6; Ret $g $cobre 40 46 14 4
        return
    }
    $dy = if ($q -eq 5) { 12 } else { 0 }
    Ret $g $ferro 18 (4 + $dy) 28 18                                  # cabeça
    Elipse $g $amarelo 27 (8 + $dy) 10 10                             # olho
    Ret $g $ferroClaro 14 (22 + $dy) 36 24                            # tronco
    Ret $g $latao 14 (30 + $dy) 36 4                                  # faixa de latão
    Elipse $g $laranja 28 (36 + $dy) 8 6                              # caldeira
    $bracoE = if ($q -eq 3) { 2 } else { 24 }; $bracoD = if ($q -eq 4) { 40 } else { 24 }
    Ret $g $cobre 6 (20 + $dy + ($bracoE - 24) / 2) 8 $bracoE         # braço esquerdo (levanta no preparo)
    Ret $g $cobre 50 (24 + $dy) 8 ([math]::Min($bracoD, 40 - $dy))   # braço direito (desce no golpe)
    if ($q -ne 5) {
        $pe = if ($q -eq 1) { 4 } elseif ($q -eq 2) { -4 } else { 0 }
        Ret $g $ferroEscuro 18 46 10 (18 + [math]::Min(0, $pe)); Ret $g $ferroEscuro 36 46 10 (18 - [math]::Max(0, $pe))
    }
}

# --- Atirador da Guilda: casaco, chapéu, máscara de gás e rifle ---
$atirador = {
    param($g, $q)
    if ($q -eq 6) {
        Ret $g $couro 4 54 44 8; Elipse $g $vidro 46 52 8 8; Ret $g $ferroEscuro 10 50 30 3
        return
    }
    $dy = if ($q -eq 5) { 16 } else { 0 }
    Ret $g $couroEscuro 16 (0 + $dy) 32 5; Ret $g $couroEscuro 22 (-6 + $dy + 6) 20 6   # chapéu
    Elipse $g (Cor 200 170 140) 22 (6 + $dy) 20 16                   # rosto
    Elipse $g $vidro 23 (10 + $dy) 8 7; Elipse $g $vidro 33 (10 + $dy) 8 7   # óculos da máscara
    Elipse $g $ferro 28 (17 + $dy) 8 7                               # filtro
    Poligono $g $couro @(14, (22 + $dy), 50, (22 + $dy), 54, 64, 10, 64)  # casaco
    Ret $g $latao 30 (24 + $dy) 4 30                                 # botões / cinto
    if ($q -eq 3 -or $q -eq 4) {
        Ret $g $ferroEscuro 26 (24 + $dy) 12 12                      # rifle apontado (cano de frente)
        Elipse $g (Cor 20 20 20) 29 (27 + $dy) 6 6
        if ($q -eq 4) { Elipse $g $amarelo 24 (21 + $dy) 16 16; Elipse $g (Cor 255 250 220) 28 (25 + $dy) 8 8 }
    } else {
        Ret $g $ferroEscuro 46 (14 + $dy) 4 34                       # rifle de lado
    }
    if ($q -ne 5) {
        $pe = if ($q -eq 1) { 3 } elseif ($q -eq 2) { -3 } else { 0 }
        Ret $g $couroEscuro (20 + $pe) 58 8 6; Ret $g $couroEscuro (36 - $pe) 58 8 6
    }
}

# --- Colosso a vapor: corpo largo, chaminé com fumaça, fornalha e punhos grandes ---
$colosso = {
    param($g, $q)
    if ($q -eq 6) {
        Ret $g $ferro 2 46 30 18; Ret $g $ferroEscuro 34 50 28 14; Elipse $g $laranja 12 50 10 8; Ret $g $latao 40 44 6 8
        return
    }
    $dy = if ($q -eq 5) { 14 } else { 0 }
    Elipse $g $fumaca 20 (0 + $dy) 12 8                              # fumaça
    Ret $g $ferroEscuro 22 (4 + $dy) 8 10                            # chaminé
    Ret $g $ferro 10 (12 + $dy) 44 36                                # corpo
    Ret $g $ferroClaro 10 (12 + $dy) 44 4
    Elipse $g $laranja 22 (24 + $dy) 20 14                           # fornalha
    Elipse $g $amarelo 27 (27 + $dy) 10 8
    Elipse $g $vermelho 20 (15 + $dy) 6 5; Elipse $g $vermelho 38 (15 + $dy) 6 5   # olhos
    $sobe = if ($q -eq 3) { 18 } else { 0 }; $desce = if ($q -eq 4) { 8 } else { 0 }
    Ret $g $latao 0 (16 + $dy - $sobe) 10 22; Elipse $g $lataoEscuro 0 (36 + $dy - $sobe + $desce) 12 12   # braço/punho esq.
    Ret $g $latao 54 (16 + $dy) 10 22; Elipse $g $lataoEscuro 52 (36 + $dy + $desce) 12 12               # braço/punho dir.
    if ($q -ne 5) {
        $pe = if ($q -eq 1) { 3 } elseif ($q -eq 2) { -3 } else { 0 }
        Ret $g $ferroEscuro 14 (48 + [math]::Max(0, $pe)) 14 (16 - [math]::Max(0, $pe))
        Ret $g $ferroEscuro 36 (48 - [math]::Min(0, $pe)) 14 (16 + [math]::Min(0, $pe))
    }
}

$inimigos = [ordered]@{ rato = $rato; automato = $automato; atirador = $atirador; colosso = $colosso }
foreach ($nome in $inimigos.Keys) {
    for ($q = 0; $q -lt 7; $q++) { Salvar "assets\sprites\${nome}_$q.png" 64 $q $inimigos[$nome] }
}

# Projétil: bala incandescente
Salvar "assets\sprites\projetil.png" 64 0 {
    param($g, $q)
    Elipse $g $laranja 16 16 32 32; Elipse $g $amarelo 22 22 20 20; Elipse $g (Cor 255 250 220) 27 27 10 10
}

# --- Armas na mão (vistas de trás, 128x128): luva de couro segurando a arma ---
function Mao($g, $x, $y) { Elipse $g $couro $x $y 44 40; Ret $g $couroEscuro ($x + 6) ($y + 30) 32 40 }
$chave = {
    param($g, $q)
    $dx = if ($q -eq 1) { -26 } else { 0 }; $dy = if ($q -eq 1) { -22 } else { 0 }
    # cabo inclinado e boca da chave
    Poligono $g $ferroClaro @((70 + $dx), (40 + $dy), (82 + $dx), (44 + $dy), (70 + $dx), (100 + $dy), (58 + $dx), (96 + $dy))
    Ret $g $ferroClaro (62 + $dx) (22 + $dy) 30 22; Ret $g (Cor 0 0 0) (72 + $dx) (14 + $dy) 10 18
    Ret $g $ferro (64 + $dx) (36 + $dy) 26 4
    Mao $g (48 + $dx) (84 + $dy)
}
$revolver = {
    param($g, $q)
    $dy = if ($q -eq 4) { 26 } else { 0 }
    if ($q -eq 3) { Elipse $g $amarelo 44 0 40 36; Elipse $g (Cor 255 250 220) 54 8 20 18 }   # clarão
    Ret $g $ferroEscuro 56 (24 + $dy) 16 46                          # cano
    Elipse $g (Cor 15 15 15) 59 (24 + $dy) 10 8                      # boca do cano
    if ($q -eq 4) { Elipse $g $latao 70 (58 + $dy) 24 24; Elipse $g $ferroEscuro 76 (64 + $dy) 12 12 }   # tambor aberto
    else { Elipse $g $latao 50 (62 + $dy) 28 22 }                    # tambor
    Ret $g $lataoEscuro 52 (80 + $dy) 24 10
    Mao $g 42 (86 + $dy)
}
for ($q = 0; $q -lt 2; $q++) { Salvar "assets\armas\chave_$q.png" 128 $q $chave }
for ($q = 0; $q -lt 3; $q++) { Salvar "assets\armas\revolver_$q.png" 128 ($q + 2) $revolver }
"sprites gerados"
