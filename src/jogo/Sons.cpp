#include "jogo/Sons.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

namespace jogo {

std::string_view nomeDoSom(Som s) {
    switch (s) {
        case Som::PassoPedra: return "passo_pedra";
        case Som::PassoMetal: return "passo_metal";
        case Som::GolpeAr: return "golpe_ar";
        case Som::Acerto: return "acerto";
        case Som::DisparoRevolver: return "disparo_revolver";
        case Som::DisparoBacamarte: return "disparo_bacamarte";
        case Som::DisparoTesla: return "disparo_tesla";
        case Som::Recarga: return "recarga";
        case Som::ArmaVazia: return "arma_vazia";
        case Som::Dano: return "dano";
        case Som::InimigoDestruido: return "inimigo_destruido";
        case Som::Comporta: return "comporta";
        case Som::Pegar: return "pegar";
        case Som::Nivel: return "nivel";
        case Som::Elevador: return "elevador";
        case Som::TiroInimigo: return "tiro_inimigo";
        case Som::MenuMover: return "menu_mover";
        case Som::MenuConfirmar: return "menu_confirmar";
        default: return "";
    }
}

Mixagem mixar(const EventoSom& e, glm::vec2 ouvinte, float angulo) {
    if (!e.origem) return {1.0f, 0.0f};
    const glm::vec2 ate = *e.origem - ouvinte;
    const float d = glm::length(ate);
    const float volume = std::clamp((DISTANCIA_SILENCIO - d) / (DISTANCIA_SILENCIO - DISTANCIA_VOLUME_CHEIO), 0.0f, 1.0f);
    if (d < 1e-4f) return {volume, 0.0f};
    const glm::vec2 direita{-std::sin(angulo), std::cos(angulo)};
    return {volume, std::clamp(glm::dot(ate / d, direita), -PAN_MAXIMO, PAN_MAXIMO)};
}

GanhosEstereo ganhosEstereo(float pan) {
    const auto ganho = [](float g) { return static_cast<uint8_t>(std::lround(255.0f * std::clamp(g, 0.0f, 1.0f))); };
    return {ganho(1.0f - pan), ganho(1.0f + pan)};
}

}
