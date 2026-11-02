# Desenha os sprites provisórios (64x64, fundo transparente) em assets/sprites/
$ErrorActionPreference = "Stop"
Set-Location (Split-Path $PSScriptRoot)
Add-Type -AssemblyName System.Drawing
New-Item -ItemType Directory -Force assets\sprites | Out-Null

function Novo-Sprite { $b = New-Object Drawing.Bitmap 64, 64, ([Drawing.Imaging.PixelFormat]::Format32bppArgb); $b }
function Cor($r, $g, $b) { [Drawing.Color]::FromArgb(255, $r, $g, $b) }

# Válvula-chave: roda de latão com 4 raios e cubo central
$b = Novo-Sprite
$g = [Drawing.Graphics]::FromImage($b)
$g.SmoothingMode = 'None'  # bordas duras: o jogo descarta pixel com alfa < 0,5
$latao = Cor 196 146 54; $sombra = Cor 112 78 24; $brilho = Cor 240 204 110
$g.DrawEllipse((New-Object Drawing.Pen $sombra, 9), 9, 9, 46, 46)
$g.DrawEllipse((New-Object Drawing.Pen $latao, 6), 9, 9, 46, 46)
$g.DrawArc((New-Object Drawing.Pen $brilho, 2), 11, 11, 42, 42, 200, 70)
$raio = New-Object Drawing.Pen $latao, 5
$g.DrawLine($raio, 32, 12, 32, 52); $g.DrawLine($raio, 12, 32, 52, 32)
$g.FillEllipse((New-Object Drawing.SolidBrush $sombra), 24, 24, 16, 16)
$g.FillEllipse((New-Object Drawing.SolidBrush $latao), 26, 26, 12, 12)
$g.FillRectangle((New-Object Drawing.SolidBrush $brilho), 29, 28, 3, 3)
$b.Save("$PWD\assets\sprites\valvula_chave.png", [Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $b.Dispose()

# Elevador de carga: gaiola de ferro com grade vertical e topo de latão
$b = Novo-Sprite
$g = [Drawing.Graphics]::FromImage($b)
$g.SmoothingMode = 'None'
$ferro = Cor 70 66 62; $ferroClaro = Cor 120 114 104; $latao = Cor 176 128 48
$pFerro = New-Object Drawing.SolidBrush $ferro
$g.FillRectangle($pFerro, 2, 6, 5, 58)            # coluna esquerda
$g.FillRectangle($pFerro, 57, 6, 5, 58)           # coluna direita
$g.FillRectangle((New-Object Drawing.SolidBrush $latao), 0, 0, 64, 7)  # viga do topo
$g.FillRectangle((New-Object Drawing.SolidBrush (Cor 110 80 30)), 0, 6, 64, 2)
$g.FillRectangle($pFerro, 2, 34, 60, 3)           # travessa do meio
$g.FillRectangle($pFerro, 2, 60, 60, 4)           # piso
for ($x = 11; $x -lt 57; $x += 7) {               # grades
    $g.FillRectangle($pFerro, $x, 8, 2, 52)
    $g.FillRectangle((New-Object Drawing.SolidBrush $ferroClaro), $x, 8, 1, 52)
}
$g.FillRectangle((New-Object Drawing.SolidBrush (Cor 230 190 90)), 30, 1, 4, 4)  # lâmpada de sinal
$b.Save("$PWD\assets\sprites\elevador.png", [Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $b.Dispose()
"sprites gerados"
