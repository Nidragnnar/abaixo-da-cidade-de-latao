# Desenha o sprite PROVISÓRIO do lampião de parede (64x64): arandela de latão com a chama dentro do vidro
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

$latao = Cor 200 150 56; $lataoEscuro = Cor 120 84 30; $vidro = Cor 255 214 140
$chama = Cor 255 170 50; $miolo = Cor 255 245 200

$b = New-Object Drawing.Bitmap 64, 64, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [Drawing.Graphics]::FromImage($b)
$g.SmoothingMode = 'None'  # bordas duras: o jogo descarta pixel com alfa < 0,5
Ret $g $lataoEscuro 22 2 20 6        # chapéu
Poligono $g $latao @(18, 8, 46, 8, 42, 14, 22, 14)
Ret $g $vidro 20 14 24 30            # vidro aceso
Ret $g $lataoEscuro 20 14 3 30; Ret $g $lataoEscuro 41 14 3 30; Ret $g $lataoEscuro 31 14 2 30  # armação
Elipse $g $chama 25 22 14 18         # chama
Elipse $g $miolo 29 28 6 10
Poligono $g $latao @(18, 44, 46, 44, 40, 52, 24, 52)  # base
Ret $g $lataoEscuro 30 52 4 10       # haste até a parede
$b.Save("$PWD\assets\sprites\lampiao.png", [Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $b.Dispose()
"assets\sprites\lampiao.png"

# Lustre a vapor pendurado no teto (64x64): corrente no alto, aro de latão com três chamas
$b = New-Object Drawing.Bitmap 64, 64, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [Drawing.Graphics]::FromImage($b)
$g.SmoothingMode = 'None'
Ret $g $lataoEscuro 30 0 4 22                 # corrente até o teto
Ret $g $latao 28 20 8 4
Poligono $g $latao @(6, 40, 58, 40, 50, 48, 14, 48)  # aro
Ret $g $lataoEscuro 6 38 52 3
Poligono $g $lataoEscuro @(30, 24, 34, 24, 34, 40, 30, 40)  # haste
Ret $g $lataoEscuro 14 22 36 2                # braços
Ret $g $lataoEscuro 14 22 2 18; Ret $g $lataoEscuro 48 22 2 18
foreach ($x in 9, 29, 49) {
    Ret $g $vidro ($x - 1) 28 8 10            # vela de vidro aceso
    Elipse $g $chama $x 24 6 10
    Elipse $g $miolo ($x + 2) 28 2 5
}
Poligono $g $latao @(26, 48, 38, 48, 34, 56, 30, 56)  # ponteira
$b.Save("$PWD\assets\sprites\lustre.png", [Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $b.Dispose()
"assets\sprites\lustre.png"
