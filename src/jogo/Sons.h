#pragma once
#include <cstdint>
#include <optional>
#include <string_view>

#include <glm/vec2.hpp>

namespace jogo {

// Efeitos sonoros (seção 9 da spec); o arquivo é assets/sons/<nomeDoSom>.ogg ou .wav
enum class Som {
    PassoPedra, PassoMetal, GolpeAr, Acerto, DisparoRevolver, DisparoBacamarte, DisparoTesla, Recarga, ArmaVazia,
    Dano, InimigoDestruido, Comporta, Pegar, Nivel, Elevador, TiroInimigo, MenuMover, MenuConfirmar,
    Zumbido, Motor, Explosao, TOTAL
};

std::string_view nomeDoSom(Som s);

struct EventoSom {
    Som som;
    std::optional<glm::vec2> origem;  // sem origem = no ouvido do jogador
};

struct Mixagem {
    float volume;  // 0–1
    float pan;     // −1 esquerda … +1 direita
};

constexpr float DISTANCIA_VOLUME_CHEIO = 2.0f, DISTANCIA_SILENCIO = 16.0f, PAN_MAXIMO = 0.8f;

// Volume cheio até 2 células, silêncio a 16; lado pela direção do olhar (ângulo 0 = +X, direita = +Z)
Mixagem mixar(const EventoSom& e, glm::vec2 ouvinte, float angulo);

struct GanhosEstereo {
    uint8_t esquerda, direita;
};

// esquerda = 255·min(1, 1 − pan), direita = 255·min(1, 1 + pan)
GanhosEstereo ganhosEstereo(float pan);

}
