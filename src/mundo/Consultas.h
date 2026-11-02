#pragma once
#include <optional>
#include <vector>

#include <glm/vec2.hpp>

#include "mundo/Grade.h"

namespace mundo {

// Percorre as células do segmento; falso se alguma sólida no caminho (a célula de destino não conta).
// Em empate exato dos eixos, confere as duas vizinhas: não enxerga entre paredes que se tocam pela quina.
bool temLinhaDeVisao(const Grade& g, glm::vec2 de, glm::vec2 para);

// Células que o jogador já viu (para o automapa)
class Exploracao {
public:
    Exploracao(int largura, int altura);
    bool vista(int x, int y) const;
    void marcar(int x, int y);
    void marcarTudo();

private:
    int largura_, altura_;
    std::vector<bool> vistas_;
};

constexpr float RAIO_EXPLORACAO = 6.0f;  // o que a lanterna ilumina

// Marca cada célula cujo centro está a até `raio` e com linha de visão
void explorarAoRedor(Exploracao& e, const Grade& g, glm::vec2 posicao, float raio);

// Primeira célula no raio de visão (incluindo a do jogador) que não é chão nem comporta aberta
std::optional<glm::ivec2> celulaAFrente(const Grade& g, glm::vec2 posicao, float angulo, float alcance);

// Busca em largura com 4 vizinhos; índice y * largura + x; -1 = inalcançável.
// Passa por tudo que não é parede (e não é comporta trancada, se atravessaTrancada for falso;
// nem o elevador, se atravessaElevador for falso — caminho de quem anda, já que a gaiola bloqueia a passagem).
std::vector<int> distanciasNaGrade(const Grade& g, glm::ivec2 origem, bool atravessaTrancada,
                                   bool atravessaElevador = true);

// Como temLinhaDeVisao, mas também barrado pelo que bloqueia a passagem (elevador)
bool temPassagemLivre(const Grade& g, glm::vec2 de, glm::vec2 para);

}
