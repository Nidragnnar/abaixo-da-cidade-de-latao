#pragma once

namespace jogo {

// O que o jogador quer fazer neste passo, já traduzido da entrada
struct Comandos {
    float frente = 0;  // [-1, 1]
    float lado = 0;    // [-1, 1], positivo = direita
    float giro = 0;    // radianos, já com a sensibilidade aplicada
};

}
