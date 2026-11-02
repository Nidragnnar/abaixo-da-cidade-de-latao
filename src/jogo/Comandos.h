#pragma once

namespace jogo {

// O que o jogador quer fazer neste passo, já traduzido da entrada
struct Comandos {
    float frente = 0;  // [-1, 1]
    float lado = 0;    // [-1, 1], positivo = direita
    float giro = 0;    // radianos, já com a sensibilidade aplicada
    bool usar = false;        // tecla E apertada (evento: vale só num passo)
    bool atacar = false;      // botão esquerdo segurado (estado)
    bool trocarArma = false;  // Q ou roda do mouse (evento)
    bool recarregar = false;  // R (evento)
    float inclinar = 0;       // radianos (mouse vertical), positivo = olhar para cima
};

}
