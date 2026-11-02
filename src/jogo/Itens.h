#pragma once
#include <cstdint>
#include <random>
#include <string>
#include <variant>
#include <vector>

#include <glm/vec2.hpp>

#include "jogo/Inimigos.h"
#include "mundo/Gerador.h"

namespace jogo {

enum class TipoItem { ChaveInglesa, Sabre, Martelo, Revolver, Bacamarte, Tesla, Casaco, Colete, ArmaduraLatao, Tonico };
constexpr int TOTAL_TIPOS_ITEM = 10;

enum class ClasseItem { ArmaBranca, ArmaDeFogo, Armadura, Consumivel };
ClasseItem classe(TipoItem t);

enum class TipoMunicao { Balas, Cartuchos, Celulas };

// Seção 4 da spec. carga 0 = sem recarga (gasta direto da reserva); alcance 0 nas armas de fogo = sem limite
struct DadosArma {
    const char* nome;
    bool feminino;
    int danoMin, danoMax;
    float intervalo;  // segundos entre ataques
    float alcance;
    int carga;
    float recarga;    // segundos
    TipoMunicao municao;
    int chumbos;      // traços por disparo
    float abertura;   // dispersão total, em graus
    float empurrao;   // armas brancas
    int andarMinimo;
};

struct DadosArmadura {
    const char* nome;
    bool feminino;
    int defesa;
    int andarMinimo;
};

const DadosArma& dadosArma(TipoItem t);          // só armas
const DadosArmadura& dadosArmadura(TipoItem t);  // só armaduras

enum class Qualidade { Comum, Modificada, ObraPrima };

// Velocidade e Recarga em porcentagem
enum class TipoBonus { Dano, Defesa, VidaMaxima, Velocidade, Carga, Recarga };

struct Bonus {
    TipoBonus tipo;
    int valor;
};

struct Item {
    TipoItem tipo;
    Qualidade qualidade = Qualidade::Comum;
    std::vector<Bonus> bonus;
    int quantidade = 1;  // tônicos empilhados
    int carga = 0;       // balas no tambor (armas de fogo)
};

int somaBonus(const Item& i, TipoBonus t);
// Carga da tabela + bônus de carga
int capacidade(const Item& arma);

// Comum; arma de fogo já carregada
Item criarItem(TipoItem t);

// "Revólver Calibrado +3", "Chave inglesa Veloz +15%", "Tônico restaurador ×3"
std::string nomeItem(const Item& i);

// Item sorteado para o andar (pesos, disponibilidade, qualidade e bônus da Etapa 4)
Item gerarItem(int andar, std::mt19937& rng);

int tamanhoCaixa(TipoMunicao t);  // balas 6, cartuchos 4, células 3

struct MunicaoNoChao {
    TipoMunicao tipo;
    int quantidade;
};

// Algo no chão: item (pede E), munição ou engrenagens (int; pegas ao passar por cima)
struct CoisaNoChao {
    glm::vec2 posicao;
    std::variant<Item, MunicaoNoChao, int> conteudo;
};

// Itens, caixas de munição e engrenagens do andar (nunca na sala inicial nem no elevador)
std::vector<CoisaNoChao> gerarChao(const mundo::Andar& andar, uint32_t semente);

// 25% de soltar item, 35% de soltar munição; Atiradores sempre soltam balas
std::vector<CoisaNoChao> soltarDoInimigo(TipoInimigo t, glm::vec2 posicao, int andar, std::mt19937& rng);

}
