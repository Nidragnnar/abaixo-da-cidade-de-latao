#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace mundo {

// ComportaAberta: comporta que já subiu por completo (passa como chão)
enum class Celula : uint8_t { Parede, Chao, Comporta, ComportaTrancada, Elevador, ComportaAberta };

// Células de um andar; fora dos limites tudo é parede
class Grade {
public:
    Grade(int largura, int altura);  // começa toda em Parede

    int largura() const { return largura_; }
    int altura() const { return altura_; }
    Celula em(int x, int y) const;
    void definir(int x, int y, Celula c);
    // Bloqueia passagem e visão (paredes e comportas fechadas)
    bool solida(int x, int y) const;
    // Bloqueia só a passagem: sólida ou elevador (a gaiola é vazada, mas não dá para entrar nela)
    bool bloqueiaPassagem(int x, int y) const { return solida(x, y) || em(x, y) == Celula::Elevador; }

private:
    bool dentro(int x, int y) const { return x >= 0 && y >= 0 && x < largura_ && y < altura_; }

    int largura_;
    int altura_;
    std::vector<Celula> celulas_;
};

struct MapaCarregado {
    Grade grade;
    glm::vec2 inicio;                     // centro da célula '@'
    std::optional<glm::vec2> chave;       // centro da célula 'K'
    std::optional<glm::ivec2> elevador;   // célula 'E'
};

// '#' parede, '.' chão, '@' chão + início, 'C' comporta, 'T' comporta trancada, 'E' elevador,
// 'K' chão + válvula-chave; lança std::invalid_argument se o texto for inválido
MapaCarregado carregarDeTexto(const std::vector<std::string>& linhas);

}
