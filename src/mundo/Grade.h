#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace mundo {

enum class Celula : uint8_t { Parede, Chao, Comporta, ComportaTrancada, Elevador };

// Células de um andar; fora dos limites tudo é parede
class Grade {
public:
    Grade(int largura, int altura);  // começa toda em Parede

    int largura() const { return largura_; }
    int altura() const { return altura_; }
    Celula em(int x, int y) const;
    void definir(int x, int y, Celula c);
    // Bloqueia passagem (paredes e comportas fechadas)
    bool solida(int x, int y) const;

private:
    bool dentro(int x, int y) const { return x >= 0 && y >= 0 && x < largura_ && y < altura_; }

    int largura_;
    int altura_;
    std::vector<Celula> celulas_;
};

struct MapaCarregado {
    Grade grade;
    glm::vec2 inicio;  // centro da célula '@'
};

// '#' parede, '.' chão, '@' chão + início; lança std::invalid_argument se o texto for inválido
MapaCarregado carregarDeTexto(const std::vector<std::string>& linhas);

}
