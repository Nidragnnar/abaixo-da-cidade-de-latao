#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>

#include "mundo/Grade.h"
#include "mundo/Zona.h"

namespace mundo {

constexpr int LADO_ANDAR = 64;

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
};

// Semente própria de cada andar, derivada da semente da partida (permite começar em qualquer andar)
uint32_t sementeDoAndar(uint32_t sementePartida, int numero);

// Salas e corredores conforme a seção 3 da spec; sempre devolve um andar válido
Andar gerarAndar(uint32_t sementePartida, int numero);

// Andar a partir de texto (testes); salas vazio, elevador {-1, -1} se não houver 'E'
Andar andarDeTexto(const std::vector<std::string>& linhas, int numero);

}
