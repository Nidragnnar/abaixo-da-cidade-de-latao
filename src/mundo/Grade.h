#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>

namespace mundo {

// ComportaAberta: comporta que já subiu por completo (passa como chão)
enum class Celula : uint8_t { Parede, Chao, Comporta, ComportaTrancada, Elevador, ComportaAberta };

// Alturas em quartos de bloco (0,25); rampa sobe 4 quartos ao longo da célula, na direção indicada (Norte = −y)
enum class Rampa : uint8_t { Nenhuma, Norte, Sul, Leste, Oeste };
// Só para o desenho (textura da face)
enum class Detalhe : uint8_t { Nenhum, Caixa, Escada };
constexpr int TETO_PADRAO = 4;  // quartos acima do chão quando ninguém diz outro (o mundo da v1)

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

    // Alturas em quartos; fora da grade: chão 0, teto padrão
    int chao(int x, int y) const;
    int teto(int x, int y) const;
    Rampa rampa(int x, int y) const;
    Detalhe detalhe(int x, int y) const;
    void definirAltura(int x, int y, int chao, int teto);
    void definirRampa(int x, int y, Rampa r);
    void definirDetalhe(int x, int y, Detalhe d);

private:
    bool dentro(int x, int y) const { return x >= 0 && y >= 0 && x < largura_ && y < altura_; }

    int largura_;
    int altura_;
    std::vector<Celula> celulas_;
    std::vector<int8_t> chao_, teto_;
    std::vector<Rampa> rampas_;
    std::vector<Detalhe> detalhes_;
};

struct MapaCarregado {
    Grade grade;
    glm::vec2 inicio;                     // centro da célula '@'
    std::optional<glm::vec2> chave;       // centro da célula 'K'
    std::optional<glm::ivec2> elevador;   // célula 'E'
    std::vector<std::pair<char, glm::vec2>> marcas;  // r a g c v d: inimigos dos mapas de teste
};

// '#' parede, '.' chão, '@' chão + início, 'C' comporta, 'T' comporta trancada, 'E' elevador,
// 'K' chão + válvula-chave, r a g c chão + marca de inimigo; lança std::invalid_argument se o texto for inválido
// Camadas opcionais do mesmo tamanho: alturas (dígito ou a b c = 10 11 12 = chão em quartos; n s l o = rampa a partir de 0 subindo para
// norte, sul, leste, oeste; N S L O = a partir de 4) e tetos (quartos acima do chão: 1–9 e a–k = 10–20; 0 = padrão, 4)
MapaCarregado carregarDeTexto(const std::vector<std::string>& linhas, const std::vector<std::string>& alturas = {},
                              const std::vector<std::string>& tetos = {});

}
