# Desenha os sprites PROVISÓRIOS dos chefes (64x64), na ordem dos quadros de render\AnimacaoInimigos.h:
# parado, andar 1, andar 2, atacar 1 (preparo), atacar 2 (recuperação), morrer, destroços.
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
$olho = Cor 255 60 40; $fogo = Cor 255 150 40; $amarelo = Cor 255 230 120; $vapor = Cor 220 225 230
$pelo = Cor 96 90 84; $peloEscuro = Cor 60 56 52; $rosa = Cor 200 120 120

# --- Rainha dos Ratos: rato de ferro enorme com coroa de latão ---
function Rainha($g, $passo, $agachada, $tonta) {
    $y = if ($agachada) { 6 } else { 0 }
    Poligono $g $peloEscuro @(4, (44 + $y), 0, (30 + $y), 6, (34 + $y), 10, (46 + $y))  # rabo
    Elipse $g $pelo 8 (24 + $y) 44 30                   # corpo
    Elipse $g $peloEscuro 12 (34 + $y) 36 18
    Elipse $g $pelo 38 (18 + $y) 24 22                  # cabeça
    Poligono $g $pelo @(56, (26 + $y), 64, (32 + $y), 56, (36 + $y))  # focinho
    Elipse $g $rosa 40 (14 + $y) 8 8                    # orelha
    Elipse $g $olho 50 (24 + $y) 5 5
    Poligono $g $latao @(40, (16 + $y), 42, (8 + $y), 46, (14 + $y), 50, (6 + $y), 54, (14 + $y), 58, (8 + $y), 58, (18 + $y), 40, (18 + $y))  # coroa
    $p = if ($passo -eq 1) { 4 } elseif ($passo -eq 2) { -4 } else { 0 }
    Ret $g $ferro (14 + $p) 52 6 12; Ret $g $ferro (38 - $p) 52 6 12  # patas
    if ($tonta) { foreach ($x in 36, 46, 56) { Poligono $g $amarelo @($x, 2, ($x + 2), 6, ($x + 6), 6, ($x + 3), 9, ($x + 4), 13, $x, 10, ($x - 4), 13, ($x - 3), 9, ($x - 6), 6, ($x - 2), 6) } }
}
Desenhar "rainha_0.png" { param($g) Rainha $g 0 $false $false }
Desenhar "rainha_1.png" { param($g) Rainha $g 1 $false $false }
Desenhar "rainha_2.png" { param($g) Rainha $g 2 $false $false }
Desenhar "rainha_3.png" { param($g) Rainha $g 0 $true $false }
Desenhar "rainha_4.png" { param($g) Rainha $g 0 $false $true }
Desenhar "rainha_5.png" { param($g) Rainha $g 0 $false $false; Elipse $g $fogo 16 22 30 30; Elipse $g $amarelo 26 32 10 10 }
Desenhar "rainha_6.png" { param($g)
    Elipse $g $pelo 6 48 30 14; Elipse $g $peloEscuro 34 52 22 10; Poligono $g $latao @(22, 46, 26, 40, 30, 46)
    Ret $g $ferro 40 58 14 4
}

# --- Capataz a Vapor: autômato de ferro com caldeira na barriga e cartola ---
function Capataz($g, $passo, $bracosAcima, $vaporSaindo) {
    Ret $g $ferro 18 2 28 6; Ret $g $ferro 22 0 20 4  # cartola
    Elipse $g $ferroClaro 20 8 24 16                   # cabeça
    Ret $g $olho 26 14 4 3; Ret $g $olho 34 14 4 3
    Ret $g $ferro 12 24 40 24                          # tronco
    Elipse $g $lataoEscuro 20 28 24 18                 # caldeira
    Elipse $g $fogo 26 32 12 10
    Ret $g $latao 12 24 40 3
    if ($bracosAcima) { Ret $g $ferroClaro 4 6 8 22; Ret $g $ferroClaro 52 6 8 22 }
    else { Ret $g $ferroClaro 4 26 8 22; Ret $g $ferroClaro 52 26 8 22 }
    $p = if ($passo -eq 1) { 3 } elseif ($passo -eq 2) { -3 } else { 0 }
    Ret $g $ferro (18 + $p) 48 10 16; Ret $g $ferro (36 - $p) 48 10 16  # pernas
    if ($vaporSaindo) { Elipse $g $vapor 0 30 16 14; Elipse $g $vapor 48 30 16 14 }
}
Desenhar "capataz_0.png" { param($g) Capataz $g 0 $false $false }
Desenhar "capataz_1.png" { param($g) Capataz $g 1 $false $false }
Desenhar "capataz_2.png" { param($g) Capataz $g 2 $false $false }
Desenhar "capataz_3.png" { param($g) Capataz $g 0 $true $true }
Desenhar "capataz_4.png" { param($g) Capataz $g 0 $false $true }
Desenhar "capataz_5.png" { param($g) Capataz $g 0 $false $false; Elipse $g $fogo 14 20 36 34; Elipse $g $amarelo 26 30 12 12 }
Desenhar "capataz_6.png" { param($g)
    Ret $g $ferro 4 52 26 12; Elipse $g $lataoEscuro 30 46 22 18; Ret $g $ferro 18 56 4 8; Ret $g $ferroClaro 50 58 12 6
}

# --- Grande Máquina: núcleo de latão com olho vermelho, engrenagens e dois canhões ---
function Maquina($g, $giro, $canhoesAcesos) {
    Ret $g $ferro 2 40 60 24                           # base
    Ret $g $lataoEscuro 2 40 60 3
    foreach ($x in 6, 46) {                            # engrenagens
        Elipse $g $latao $x 44 14 14; Elipse $g $lataoEscuro ($x + 4) 48 6 6
        if ($giro) { Ret $g $latao ($x + 6) 42 2 18 } else { Ret $g $latao $x 50 18 2 }
    }
    Elipse $g $latao 14 6 36 38                        # núcleo
    Elipse $g $lataoEscuro 20 12 24 26
    Elipse $g $olho 25 17 14 14; Elipse $g $amarelo 30 22 4 4
    $boca = if ($canhoesAcesos) { $fogo } else { $ferro }
    Ret $g $ferroClaro 0 18 14 8; Ret $g $boca 0 18 3 8   # canhão esquerdo
    Ret $g $ferroClaro 50 18 14 8; Ret $g $boca 61 18 3 8 # canhão direito
}
Desenhar "maquina_0.png" { param($g) Maquina $g $false $false }
Desenhar "maquina_1.png" { param($g) Maquina $g $true $false }
Desenhar "maquina_2.png" { param($g) Maquina $g $false $false }
Desenhar "maquina_3.png" { param($g) Maquina $g $true $true }
Desenhar "maquina_4.png" { param($g) Maquina $g $false $true }
Desenhar "maquina_5.png" { param($g) Maquina $g $true $false; Elipse $g $fogo 10 2 44 44; Elipse $g $amarelo 22 14 20 20 }
Desenhar "maquina_6.png" { param($g)
    Ret $g $ferro 2 50 60 14; Elipse $g $latao 8 44 14 14; Elipse $g $lataoEscuro 28 46 22 14; Ret $g $ferroClaro 46 56 16 6
}
