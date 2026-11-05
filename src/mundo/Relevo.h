#pragma once
#include <cstdint>

namespace mundo {

struct Andar;

// Alturas, tetos, escadas e detalhes (plataformas, poços, caixas) de um andar já desenhado em 2D (Etapa 6A).
// Sorteios próprios: não mexe nas salas, corredores, comportas, elevador nem chave (só transforma em chão a
// comporta comum que ficaria torta). Garante o caminho a pé do início até a chave e o elevador e saída a pé de
// todo lugar; detalhe que quebraria isso é desfeito.
void aplicarRelevo(Andar& a, uint32_t semente);

}
