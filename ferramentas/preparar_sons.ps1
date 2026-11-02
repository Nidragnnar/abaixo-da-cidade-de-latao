# Troca os sons provisórios pelos dos pacotes CC0 (originais em arte_fonte\sons, fora do git); registro em assets\LICENCAS.txt
# Uso: .\ferramentas\preparar_sons.ps1   (precisa de: cmake --build --preset release --target cortar_som)
$ErrorActionPreference = "Stop"
Set-Location (Split-Path $PSScriptRoot)
$origem = "arte_fonte\sons"
$destino = "assets\sons"
$cortar = "build\Release\cortar_som.exe"

# Kenney (.ogg, copiados como estão)
$kenney = [ordered]@{
    "passo_pedra"       = "kenney_impact\extraido\Audio\footstep_concrete_000.ogg"
    "passo_metal"       = "kenney_impact\extraido\Audio\impactPlate_light_000.ogg"
    "acerto"            = "kenney_impact\extraido\Audio\impactMetal_medium_000.ogg"
    "dano"              = "kenney_impact\extraido\Audio\impactPunch_heavy_000.ogg"
    "disparo_tesla"     = "kenney_scifi\extraido\Audio\laserLarge_000.ogg"
    "inimigo_destruido" = "kenney_scifi\extraido\Audio\explosionCrunch_000.ogg"
    "comporta"          = "kenney_scifi\extraido\Audio\doorOpen_000.ogg"
    "elevador"          = "kenney_scifi\extraido\Audio\spaceEngineLow_000.ogg"
    "arma_vazia"        = "kenney_rpg\extraido\Audio\metalClick.ogg"
    "pegar"             = "kenney_rpg\extraido\Audio\handleCoins.ogg"
    "nivel"             = "kenney_interface\extraido\Audio\confirmation_002.ogg"
    "menu_mover"        = "kenney_interface\extraido\Audio\tick_002.ogg"
    "menu_confirmar"    = "kenney_interface\extraido\Audio\select_002.ogg"
    "musica"            = "oga_factory_ambiance\Factory.ogg"
}
# OpenGameArt (.wav cortados, em mono e normalizados): arquivo, início, fim (s), pico
$cortes = [ordered]@{
    "disparo_revolver"  = @("oga_gunshot_sounds\extraido\sounds\cz.wav", 0.12, 1.30, 0.9)
    "disparo_bacamarte" = @("oga_gunshot_sounds\extraido\sounds\shotty.wav", 0.0, 0.70, 0.95)
    "tiro_inimigo"      = @("oga_gunshot_sounds\extraido\sounds\mosin.wav", 0.32, 1.40, 0.7)
    "recarga"           = @("oga_gun_reload\gunreload1.wav", 0.0, 1.58, 0.8)
    "golpe_ar"          = @("oga_swishes\extraido\swishes\swish-7.wav", 0.0, 0.19, 0.8)
}

foreach ($nome in $kenney.Keys) {
    Copy-Item (Join-Path $origem $kenney[$nome]) "$destino\$nome.ogg"
    Remove-Item "$destino\$nome.wav" -ErrorAction SilentlyContinue  # o provisório
    "$nome.ogg  <- $($kenney[$nome])"
}
foreach ($nome in $cortes.Keys) {
    $c = $cortes[$nome]
    $saida = & $cortar (Join-Path $origem $c[0]) "$destino\$nome.wav" $c[1] $c[2] $c[3]
    if ($LASTEXITCODE -ne 0) { throw "cortar_som falhou em $nome" }
    "$nome.wav  <- $($c[0]) [$($c[1])–$($c[2]) s]"
}
