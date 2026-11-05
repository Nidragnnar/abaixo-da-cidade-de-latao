#pragma once
#include "jogo/Itens.h"
#include "render/AnimacaoInimigos.h"

namespace render {

// No array de sprites, depois dos inimigos: os 10 tipos de item, balas, cartuchos, células e engrenagens.
// Os mesmos desenhos servem de sprite no chão e de ícone no HUD e no inventário.
constexpr int PRIMEIRO_ITEM = PRIMEIRO_INIMIGO + 4 * QUADROS_POR_INIMIGO;
constexpr int ICONE_ENGRENAGENS = PRIMEIRO_ITEM + jogo::TOTAL_TIPOS_ITEM + 3;
constexpr int SPRITE_LAMPIAO = ICONE_ENGRENAGENS + 1;  // lampião de parede (Etapa 6A)
constexpr int SPRITE_LUSTRE = SPRITE_LAMPIAO + 1;   // lustre de teto (Etapa 6A)
constexpr int TOTAL_SPRITES = SPRITE_LUSTRE + 1;

constexpr int iconeDoItem(jogo::TipoItem t) { return PRIMEIRO_ITEM + static_cast<int>(t); }
constexpr int iconeDaMunicao(jogo::TipoMunicao t) { return PRIMEIRO_ITEM + jogo::TOTAL_TIPOS_ITEM + static_cast<int>(t); }

}
