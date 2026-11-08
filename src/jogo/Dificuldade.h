#pragma once
#include <cstdint>
#include <optional>
#include <string_view>

namespace jogo {

// Escolhida nas Opções, fica fixa na partida: muda vida e dano dos inimigos e multiplica a pontuação
enum class Dificuldade : uint8_t { Facil, Normal, Dificil, Pesadelo };
constexpr int TOTAL_DIFICULDADES = 4;

namespace detalhe {
struct DadosDificuldade {
    float inimigos, pontos;
    char letra;
    std::string_view nome, id;
};
constexpr DadosDificuldade DIFICULDADES[TOTAL_DIFICULDADES] = {
    {0.6f, 0.5f, 'F', "Fácil", "facil"},
    {1.0f, 1.0f, 'N', "Normal", "normal"},
    {1.4f, 1.5f, 'D', "Difícil", "dificil"},
    {1.8f, 2.0f, 'P', "Pesadelo", "pesadelo"},
};
constexpr const DadosDificuldade& de(Dificuldade d) { return DIFICULDADES[static_cast<int>(d)]; }
}

// Vida e dano dos inimigos (chefes inclusive)
constexpr float multiplicadorInimigos(Dificuldade d) { return detalhe::de(d).inimigos; }
constexpr float multiplicadorPontos(Dificuldade d) { return detalhe::de(d).pontos; }
// Marca no placar: F, N, D, P
constexpr char letraDa(Dificuldade d) { return detalhe::de(d).letra; }
constexpr std::string_view nomeDa(Dificuldade d) { return detalhe::de(d).nome; }
// No config: facil, normal, dificil, pesadelo
constexpr std::string_view idDa(Dificuldade d) { return detalhe::de(d).id; }
constexpr std::optional<Dificuldade> dificuldadeDoId(std::string_view id) {
    for (int i = 0; i < TOTAL_DIFICULDADES; ++i)
        if (detalhe::DIFICULDADES[i].id == id) return static_cast<Dificuldade>(i);
    return std::nullopt;
}

}
