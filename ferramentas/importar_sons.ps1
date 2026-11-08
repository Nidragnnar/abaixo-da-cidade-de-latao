# Importa os sons feitos por IA (pasta ou .zip com arquivos de mesmo nome que os do jogo, em WAV, OGG ou MP3)
#   efeitos: tira o silêncio das pontas, corta na duração máxima da tabela, mono, volume (pico) da tabela
#   músicas (musica, musica_chefe): copiadas como estão (WAV, OGG ou MP3), para tocar em loop
# Várias versões do mesmo som (zumbido.mp3, zumbido_2.mp3, "zumbido (3).mp3"): entra a sem número (ou a menor);
# todas ficam preparadas em arte_fonte\sons_ia\candidatos\ para ouvir. Para escolher outra, renomeie e rode de novo.
# Antes de mexer, copia os sons atuais para arte_fonte\sons_antes_<data>\ (fora do git).
# Uso: .\ferramentas\importar_sons.ps1 <pasta ou zip>   (precisa de: cmake --build --preset release --target cortar_som)
param([Parameter(Mandatory)][string]$Origem, [string]$Destino = "assets\sons")
$ErrorActionPreference = "Stop"
Set-Location (Split-Path $PSScriptRoot)
$cortar = "build\Release\cortar_som.exe"
if (-not (Test-Path $cortar)) { "falta $cortar (cmake --build --preset release --target cortar_som)"; exit 1 }
if (-not (Test-Path $Origem)) { "não achei $Origem"; exit 1 }

# som = duração máxima (s), pico (volume), saída suave (s); a duração é a do guia com folga
$efeitos = [ordered]@{
    "zumbido"           = @(0.9, 0.35, 0.15)
    "motor"             = @(1.4, 0.4, 0.3)
    "explosao"          = @(1.2, 0.7, 0.3)
    "disparo_revolver"  = @(0.5, 0.7, 0.1)
    "golpe_ar"          = @(0.3, 0.45, 0.04)
    "acerto"            = @(0.45, 0.55, 0.04)
    "disparo_bacamarte" = @(1.0, 0.75, 0.2)
    "disparo_tesla"     = @(1.0, 0.5, 0.1)
    "recarga"           = @(2.0, 0.45, 0.1)
    "arma_vazia"        = @(0.3, 0.45, 0.04)
    "passo_pedra"       = @(0.3, 0.22, 0.04)
    "passo_metal"       = @(0.45, 0.2, 0.15)
    "dano"              = @(0.7, 0.55, 0.15)
    "pegar"             = @(0.7, 0.4, 0.15)
    "nivel"             = @(0.9, 0.45, 0.1)
    "comporta"          = @(1.4, 0.45, 0.2)
    "elevador"          = @(2.0, 0.5, 0.7)
    "tiro_inimigo"      = @(0.7, 0.5, 0.2)
    "inimigo_destruido" = @(1.1, 0.55, 0.2)
    "menu_mover"        = @(0.15, 0.3, 0.02)
    "menu_confirmar"    = @(0.15, 0.35, 0.02)
}
$musicas = @("musica", "musica_chefe")
$extensoes = @(".wav", ".ogg", ".mp3")

# Zip: extrai numa pasta temporária
$pasta = $Origem
if ($Origem -like "*.zip") {
    $pasta = Join-Path $env:TEMP "sons_ia_$(Get-Random)"
    Expand-Archive -LiteralPath $Origem -DestinationPath $pasta
}

# Nome do som e número da versão: "zumbido" 0, "zumbido_2" 2, "Zumbido (3)" 3, "zumbido-v2" 2
function Identificar($arquivo) {
    $base = [IO.Path]::GetFileNameWithoutExtension($arquivo).ToLowerInvariant().Trim()
    if ($base -match '^(.+?)[ _-]*\(?v?(\d+)\)?$' -and ($efeitos.Contains($Matches[1]) -or $musicas -contains $Matches[1])) {
        return @($Matches[1], [int]$Matches[2])
    }
    return @($base, 0)
}

$achados = @{}  # nome -> lista de {Versao, Arquivo}
$ignorados = @()
foreach ($f in Get-ChildItem -LiteralPath $pasta -Recurse -File | Where-Object { $extensoes -contains $_.Extension.ToLowerInvariant() }) {
    $nome, $versao = Identificar $f.Name
    if (-not ($efeitos.Contains($nome) -or $musicas -contains $nome)) { $ignorados += $f.Name; continue }
    if (-not $achados.ContainsKey($nome)) { $achados[$nome] = @() }
    $achados[$nome] += [pscustomobject]@{ Versao = $versao; Arquivo = $f.FullName }
}
if ($achados.Count -eq 0) { "nenhum som com nome conhecido em $Origem"; exit 1 }

# Cópia dos sons atuais
$copia = "arte_fonte\sons_antes_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
New-Item -ItemType Directory -Force $copia | Out-Null
Copy-Item "$Destino\*" $copia
"cópia dos sons atuais em $copia"

$candidatos = "arte_fonte\sons_ia\candidatos"
New-Item -ItemType Directory -Force $candidatos | Out-Null

function TirarOutros($nome, $fica) {
    foreach ($e in $extensoes) {
        $p = "$Destino\$nome$e"
        if ($p -ne $fica -and (Test-Path $p)) { Remove-Item $p }
    }
}

foreach ($nome in $achados.Keys | Sort-Object) {
    $versoes = @($achados[$nome] | Sort-Object Versao)
    $escolhido = $versoes[0].Arquivo
    if ($musicas -contains $nome) {
        $ext = [IO.Path]::GetExtension($escolhido).ToLowerInvariant()
        $saida = "$Destino\$nome$ext"
        Copy-Item -LiteralPath $escolhido $saida
        TirarOutros $nome $saida
        "$nome$ext  <- $([IO.Path]::GetFileName($escolhido)) (música, sem alteração)"
    } else {
        $c = $efeitos[$nome]
        $saida = "$Destino\$nome.wav"
        $r = & $cortar $escolhido $saida auto $c[0] $c[1] $c[2]
        if ($LASTEXITCODE -ne 0) { "ERRO: não consegui ler $escolhido (o som atual ficou)"; continue }
        TirarOutros $nome $saida
        "$nome.wav  <- $([IO.Path]::GetFileName($escolhido))  [$($r -replace '^.*?: ', '')]"
    }
    # Todas as versões preparadas para ouvir (efeitos já cortados; músicas copiadas)
    foreach ($v in $versoes) {
        $rotulo = if ($v.Versao -gt 0) { "${nome}_$($v.Versao)" } else { $nome }
        if ($musicas -contains $nome) { Copy-Item -LiteralPath $v.Arquivo "$candidatos\$rotulo$([IO.Path]::GetExtension($v.Arquivo).ToLowerInvariant())" }
        else { $null = & $cortar $v.Arquivo "$candidatos\$rotulo.wav" auto $c[0] $c[1] $c[2] }
    }
    if ($versoes.Count -gt 1) { "   ($($versoes.Count) versões; todas em $candidatos)" }
}

$faltam = @($efeitos.Keys) + $musicas | Where-Object { -not $achados.ContainsKey($_) }
if ($faltam) { "mantidos os atuais: $($faltam -join ', ')" }
if ($ignorados) { "ignorados (nome desconhecido): $($ignorados -join ', ')" }
"Lembrete: anotar os sons novos em assets\LICENCAS.txt"
if ($pasta -ne $Origem) { Remove-Item -Recurse -Force $pasta }
