# Importa a arte feita por IA (pasta com sprites\, texturas\ e armas\ em tamanho grande) para assets\
#   sprites: recorta pela área desenhada (a mesma nos 7 quadros de cada inimigo), apoia no chão, quadrado de 128
#   texturas: 128x128 sem transparência; armas: 128x128 inteiras (a mão já vem enquadrada)
# Alfa vira 0 ou 255 (o jogo descarta pixel com alfa < 0,5). Escreve no fim a fração da altura ocupada pelo
# desenho de cada sprite (o jogo usa para o desenho ficar na altura certa).
# Uso: .\ferramentas\importar_arte.ps1 <pasta da arte>
param([Parameter(Mandatory)][string]$Origem)
$ErrorActionPreference = "Stop"
Set-Location (Split-Path $PSScriptRoot)
Add-Type -AssemblyName System.Drawing
$LADO = 128

# Retângulo com tudo que tem alfa >= 128 (amostra a cada `passo` pixels, depois ajusta 1 pixel de folga)
function AreaDesenhada($bmp) {
    $passo = [Math]::Max(1, [int]($bmp.Width / 400))
    $x0 = $bmp.Width; $y0 = $bmp.Height; $x1 = -1; $y1 = -1
    $dados = $bmp.LockBits((New-Object Drawing.Rectangle 0, 0, $bmp.Width, $bmp.Height), 'ReadOnly', 'Format32bppArgb')
    $bytes = New-Object byte[] ($dados.Stride * $bmp.Height)
    [Runtime.InteropServices.Marshal]::Copy($dados.Scan0, $bytes, 0, $bytes.Length)
    $bmp.UnlockBits($dados)
    for ($y = 0; $y -lt $bmp.Height; $y += $passo) {
        $linha = $y * $dados.Stride
        for ($x = 0; $x -lt $bmp.Width; $x += $passo) {
            if ($bytes[$linha + $x * 4 + 3] -ge 128) {
                if ($x -lt $x0) { $x0 = $x }; if ($x -gt $x1) { $x1 = $x }
                if ($y -lt $y0) { $y0 = $y }; if ($y -gt $y1) { $y1 = $y }
            }
        }
    }
    if ($x1 -lt 0) { return New-Object Drawing.Rectangle 0, 0, $bmp.Width, $bmp.Height }
    $x1 = [Math]::Min($bmp.Width - 1, $x1 + $passo); $y1 = [Math]::Min($bmp.Height - 1, $y1 + $passo)
    New-Object Drawing.Rectangle $x0, $y0, ($x1 - $x0 + 1), ($y1 - $y0 + 1)
}

function Uniao($a, $b) { [Drawing.Rectangle]::Union($a, $b) }

# Desenha `area` da origem num quadrado LADO x LADO: centrado na horizontal, encostado embaixo. `ladoOrigem` é
# quantos pixels da origem cabem no quadrado (a mesma escala para todos os quadros de um inimigo); sem ele, o maior
# lado da área. (No PowerShell $lado e $LADO seriam a mesma variável.)
function Quadrado($bmp, $area, $ladoOrigem = 0) {
    if ($ladoOrigem -le 0) { $ladoOrigem = [Math]::Max($area.Width, $area.Height) }
    $saida = New-Object Drawing.Bitmap $LADO, $LADO, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [Drawing.Graphics]::FromImage($saida)
    $g.InterpolationMode = 'HighQualityBicubic'; $g.PixelOffsetMode = 'HighQuality'; $g.CompositingMode = 'SourceCopy'
    $escala = $LADO / $ladoOrigem
    $l = [int][Math]::Round($area.Width * $escala); $a = [int][Math]::Round($area.Height * $escala)
    $destino = New-Object Drawing.Rectangle ([int](($LADO - $l) / 2)), ($LADO - $a), $l, $a
    $g.DrawImage($bmp, $destino, $area, 'Pixel')
    $g.Dispose()
    @{ Imagem = $saida; Fracao = $a / $LADO }
}

# Alfa 0 ou 255; `opaco` força tudo a 255 (texturas)
function Endurecer($bmp, [bool]$opaco) {
    $dados = $bmp.LockBits((New-Object Drawing.Rectangle 0, 0, $bmp.Width, $bmp.Height), 'ReadWrite', 'Format32bppArgb')
    $bytes = New-Object byte[] ($dados.Stride * $bmp.Height)
    [Runtime.InteropServices.Marshal]::Copy($dados.Scan0, $bytes, 0, $bytes.Length)
    for ($i = 3; $i -lt $bytes.Length; $i += 4) { $bytes[$i] = if ($opaco -or $bytes[$i] -ge 128) { 255 } else { 0 } }
    [Runtime.InteropServices.Marshal]::Copy($bytes, 0, $dados.Scan0, $bytes.Length)
    $bmp.UnlockBits($dados)
}

function Redimensionar($bmp) {
    $saida = New-Object Drawing.Bitmap $LADO, $LADO, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [Drawing.Graphics]::FromImage($saida)
    $g.InterpolationMode = 'HighQualityBicubic'; $g.PixelOffsetMode = 'HighQuality'; $g.CompositingMode = 'SourceCopy'
    $g.DrawImage($bmp, 0, 0, $LADO, $LADO)
    $g.Dispose()
    $saida
}

function Salvar($bmp, $arquivo) { $bmp.Save("$PWD\$arquivo", [Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose() }

$fracoes = [ordered]@{}

# Inimigos e chefes: os quadros 0–4 (parado, andando, atacando) usam a mesma área, para o corpo não pular de
# tamanho nem de lugar; a explosão (5) e os destroços (6) usam a mesma escala, cada um apoiado no chão
foreach ($nome in "rato", "automato", "atirador", "colosso", "vespa", "dirigivel", "rainha", "capataz", "maquina") {
    $quadros = 0..6 | ForEach-Object { [Drawing.Bitmap]::FromFile((Resolve-Path "$Origem\sprites\${nome}_$_.png")) }
    $area = AreaDesenhada $quadros[0]
    foreach ($q in $quadros[1..4]) { $area = Uniao $area (AreaDesenhada $q) }
    $ladoOrigem = [Math]::Max($area.Width, $area.Height)
    for ($k = 0; $k -le 6; $k++) {
        $deste = if ($k -le 4) { $area } else { AreaDesenhada $quadros[$k] }
        $r = Quadrado $quadros[$k] $deste $ladoOrigem
        Endurecer $r.Imagem $false
        Salvar $r.Imagem "assets\sprites\${nome}_$k.png"
        if ($k -eq 0) { $fracoes[$nome] = $r.Fracao }
        $quadros[$k].Dispose()
    }
}

# Objetos e itens: cada um pela sua área
$soltos = Get-ChildItem "$Origem\sprites" -Filter *.png | Where-Object { $_.BaseName -notmatch '_\d$' }
foreach ($f in $soltos) {
    $bmp = [Drawing.Bitmap]::FromFile($f.FullName)
    $r = Quadrado $bmp (AreaDesenhada $bmp)
    Endurecer $r.Imagem $false
    Salvar $r.Imagem "assets\sprites\$($f.Name)"
    $fracoes[$f.BaseName] = $r.Fracao
    $bmp.Dispose()
}

# Texturas (opacas) e armas (inteiras)
foreach ($f in Get-ChildItem "$Origem\texturas" -Filter *.png) {
    $bmp = [Drawing.Bitmap]::FromFile($f.FullName); $r = Redimensionar $bmp; $bmp.Dispose()
    Endurecer $r $true; Salvar $r "assets\texturas\$($f.Name)"
}
foreach ($f in Get-ChildItem "$Origem\armas" -Filter *.png) {
    $bmp = [Drawing.Bitmap]::FromFile($f.FullName); $r = Redimensionar $bmp; $bmp.Dispose()
    Endurecer $r $false; Salvar $r "assets\armas\$($f.Name)"
}

"Fração da altura ocupada pelo desenho (128 = 1,0):"
foreach ($k in $fracoes.Keys) { "  {0,-20} {1:N3}" -f $k, $fracoes[$k] }
