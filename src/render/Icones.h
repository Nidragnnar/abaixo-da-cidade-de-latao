#pragma once
#include "jogo/Itens.h"
#include "render/AnimacaoInimigos.h"

namespace render {

// No array de sprites, depois dos inimigos: os 10 tipos de item, balas, cartuchos, células e engrenagens.
// Os mesmos desenhos servem de sprite no chão e de ícone no HUD e no inventário.
constexpr int PRIMEIRO_ITEM = PRIMEIRO_INIMIGO + 4 * QUADROS_POR_INIMIGO;
constexpr int ICONE_ENGRENAGENS = PRIMEIRO_ITEM + jogo::TOTAL_TIPOS_ITEM + 3;
constexpr int TOTAL_SPRITES = ICONE_ENGRENAGENS + 1;

constexpr int iconeDoItem(jogo::TipoItem t) { return PRIMEIRO_ITEM + static_cast<int>(t); }
constexpr int iconeDaMunicao(jogo::TipoMunicao t) { return PRIMEIRO_ITEM + jogo::TOTAL_TIPOS_ITEM + static_cast<int>(t); }

}
