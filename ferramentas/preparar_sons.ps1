# Troca os sons provisórios pelos dos pacotes CC0 (originais em arte_fonte\sons, fora do git); registro em assets\LICENCAS.txt
# Cada efeito é cortado, passado para mono e normalizado com o volume (pico) da tabela; a música é copiada como está.
# Também monta arte_fonte\sons\candidatos\ com alternativas em .wav para ouvir com dois cliques.
# Uso: .\ferramentas\preparar_sons.ps1   (precisa de: cmake --build --preset release --target cortar_som gerar_sons)
$ErrorActionPreference = "Stop"
Set-Location (Split-Path $PSScriptRoot)
$origem = "arte_fonte\sons"
$destino = "assets\sons"
$cortar = "build\Release\cortar_som.exe"
$ki = "kenney_impact\extraido\Audio"
$ks = "kenney_scifi\extraido\Audio"
$kr = "kenney_rpg\extraido\Audio"
$kf = "kenney_interface\extraido\Audio"
$tiros = "oga_gunshot_sounds\extraido\sounds"

# som = arquivo de origem, início, fim (s), pico (volume), saída suave (s)
$receita = [ordered]@{
    "passo_pedra"       = @("$ki\footstep_concrete_000.ogg", 0, 0.2, 0.22, 0.04)
    "passo_metal"       = @("$ki\impactPlate_light_000.ogg", 0, 0.3, 0.2, 0.15)
    "golpe_ar"          = @("oga_swishes\extraido\swishes\swish-7.wav", 0, 0.19, 0.45, 0.04)
    "acerto"            = @("$ki\impactMetal_medium_000.ogg", 0, 0.3, 0.55, 0.04)
    "dano"              = @("$ki\impactPunch_heavy_000.ogg", 0, 0.5, 0.55, 0.15)
    "disparo_revolver"  = @("sintetico:disparo_revolver.wav", 0, 0.35, 0.7, 0.1)  # escolhido de ouvido
    "disparo_bacamarte" = @("$tiros\shotty.wav", 0, 0.7, 0.75, 0.2)
    "disparo_tesla"     = @("$ks\laserLarge_000.ogg", 0, 0.7, 0.5, 0.1)
    "tiro_inimigo"      = @("$tiros\mosin.wav", 0.32, 0.8, 0.5, 0.2)
    "recarga"           = @("oga_gun_reload\gunreload1.wav", 0, 1.58, 0.45, 0.1)
    "arma_vazia"        = @("$kr\metalClick.ogg", 0, 0.2, 0.45, 0.04)
    "inimigo_destruido" = @("$ks\explosionCrunch_000.ogg", 0, 0.8, 0.55, 0.2)
    "comporta"          = @("$ks\forceField_000.ogg", 0, 1.0, 0.45, 0.2)  # escolhido de ouvido
    "elevador"          = @("$ks\spaceEngineLow_000.ogg", 0, 1.5, 0.5, 0.7)
    "pegar"             = @("$kr\handleCoins.ogg", 0.05, 0.6, 0.4, 0.15)
    "nivel"             = @("$kf\confirmation_002.ogg", 0, 0.6, 0.45, 0.1)
    "menu_mover"        = @("$kf\tick_004.ogg", 0, 0.1, 0.3, 0.02)
    "menu_confirmar"    = @("$kf\select_002.ogg", 0, 0.1, 0.35, 0.02)
    "zumbido"           = @("sintetico:zumbido.wav", 0, 0.6, 0.35, 0.15)
    "motor"             = @("sintetico:motor.wav", 0, 0.9, 0.4, 0.3)
    "explosao"          = @("sintetico:explosao.wav", 0, 0.8, 0.7, 0.3)
}
$musica = "oga_factory_ambiance\Factory.ogg"

# Os sons sintetizados por gerar_sons ("sintetico:<arquivo>") são gerados numa pasta temporária
$sinteticos = Join-Path $env:TEMP "sons_sinteticos_$(Get-Random)"
$null = & build\Release\gerar_sons.exe $sinteticos

function Preparar($entrada, $saida, $c) {
    $arquivo = if ($entrada -like "sintetico:*") { Join-Path $sinteticos $entrada.Substring(10) } else { Join-Path $origem $entrada }
    $null = & $cortar $arquivo $saida $c[1] $c[2] $c[3] $c[4]
    if ($LASTEXITCODE -ne 0) { throw "cortar_som falhou em $entrada" }
}

foreach ($nome in $receita.Keys) {
    $c = $receita[$nome]
    Preparar $c[0] "$destino\$nome.wav" $c
    Remove-Item "$destino\$nome.ogg" -ErrorAction SilentlyContinue  # versão anterior, sem ajuste de volume
    "$nome.wav  <- $($c[0]) [$($c[1])–$($c[2]) s, pico $($c[3])]"
}
Copy-Item (Join-Path $origem $musica) "$destino\musica.ogg"
"musica.ogg  <- $musica"

# Alternativas para ouvir e escolher (fora do jogo)
$candidatos = "$origem\candidatos"
if (Test-Path $candidatos) { Remove-Item $candidatos -Recurse -Force }
New-Item -ItemType Directory -Force $candidatos | Out-Null
$alternativas = [ordered]@{
    "tiro_A_pistola_curto"         = @("$tiros\cz.wav", 0.12, 0.55, 0.7, 0.15)
    "tiro_B_pistola_com_eco"       = @("$tiros\cz.wav", 0.12, 1.3, 0.7, 0.3)
    "tiro_C_rifle_sks"             = @("$tiros\sks.wav", 0.25, 0.7, 0.7, 0.15)
    "tiro_D_rifle_mosin"           = @("$tiros\mosin.wav", 0.32, 0.9, 0.7, 0.2)
    "porta_A_sopro_de_vapor"       = @("$ks\thrusterFire_000.ogg", 0, 0.8, 0.45, 0.35)
    "porta_B_porta_scifi_1"        = @("$ks\doorOpen_000.ogg", 0, 0.6, 0.45, 0.1)
    "porta_C_porta_scifi_2"        = @("$ks\doorOpen_001.ogg", 0, 0.6, 0.45, 0.1)
    "porta_D_porta_scifi_3"        = @("$ks\doorOpen_002.ogg", 0, 0.6, 0.45, 0.1)
    "porta_E_campo_de_forca (atual)" = @("$ks\forceField_000.ogg", 0, 1.0, 0.45, 0.2)
    "tiro_E_sintetizado (atual)"   = @("sintetico:disparo_revolver.wav", 0, 0.35, 0.7, 0.1)
    "porta_F_vapor_sintetizado"    = @("sintetico:comporta.wav", 0, 0.6, 0.45, 0.1)
}
foreach ($nome in $alternativas.Keys) { Preparar $alternativas[$nome][0] "$candidatos\$nome.wav" $alternativas[$nome] }
Remove-Item $sinteticos -Recurse -Force
"candidatos em $candidatos"
