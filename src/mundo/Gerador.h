#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>

#include "mundo/Grade.h"
#include "mundo/Luzes.h"
#include "mundo/Zona.h"

namespace mundo {

constexpr int LADO_ANDAR = 64;
constexpr int ULTIMO_ANDAR = 6;  // vencer o chefe dele termina o jogo (Etapa 7)

// Andares que terminam uma zona com um chefe: 2 (esgoto), 4 (fábrica) e 6 (minas)
constexpr bool andarDeChefe(int andar) { return andar == 2 || andar == 4 || andar == 6; }

struct Sala {
    int x, y, largura, altura;  // interior; o anel de parede fica em volta
    glm::ivec2 centro() const { return {x + largura / 2, y + altura / 2}; }
    bool contem(int cx, int cy) const { return cx >= x && cy >= y && cx < x + largura && cy < y + altura; }
};

struct Andar {
    int numero;
    Zona zona;
    Grade grade;
    std::vector<Sala> salas;
    glm::vec2 inicio;
    glm::ivec2 elevador;            // célula do elevador de carga
    std::optional<glm::vec2> chave;  // válvula-chave, só em andar com comporta trancada
    std::vector<std::pair<char, glm::vec2>> marcas;  // só em andares de texto (testes)
    std::vector<Luz> luzes;                          // lampiões (vazio nos andares de texto)
    std::vector<glm::vec2> pontosSecretos;           // topo de plataformas só alcançáveis pulando (com item)
    std::optional<size_t> arena;                     // sala do chefe (com o elevador), só nos andares de chefe
};

// Células de passagem logo fora do anel de parede da sala (onde ficam as comportas)
std::vector<glm::ivec2> entradasDaSala(const Grade& g, const Sala& s);

// Semente própria de cada andar, derivada da semente da partida (permite começar em qualquer andar)
uint32_t sementeDoAndar(uint32_t sementePartida, int numero);

// Salas e corredores conforme a seção 3 da spec; sempre devolve um andar válido
Andar gerarAndar(uint32_t sementePartida, int numero);

// Andar a partir de texto (testes); salas vazio, elevador {-1, -1} se não houver 'E'
Andar andarDeTexto(const std::vector<std::string>& linhas, int numero, const std::vector<std::string>& alturas = {},
                   const std::vector<std::string>& tetos = {});

}
