#pragma once
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>

#include "jogo/Inventario.h"
#include "render/Letterbox.h"
#include "render/Primitivas2D.h"

namespace render {

struct LayoutInventario {
    Retangulo painel;
    std::array<Retangulo, jogo::ESPACOS_INVENTARIO> espacos;  // grade 4×4; a primeira linha são os itens rápidos 1–4
    std::array<Retangulo, 3> equipados;                     // arma branca, arma de fogo, armadura
    Retangulo largar;                                       // soltar um item arrastado aqui o larga no chão
};

// Grade 4×4 de 36 px com 4 px de folga e os equipados numa coluna à direita, centralizados na tela baixa
LayoutInventario layoutInventario(int telaL, int telaA);

// 0–15: espaços; 16–18: equipados; nada fora deles
std::optional<int> espacoSob(const LayoutInventario& l, glm::vec2 ponto);

// Soltar o item arrastado aqui o larga no chão: a caixa "Largar aqui" ou qualquer ponto fora do painel
bool sobLargar(const LayoutInventario& l, glm::vec2 ponto);

// "Dano 6–9   Intervalo 0,50 s", "Defesa 3", "Cura 15 de vida" (já com os bônus)
std::string descricaoItem(const jogo::Item& i);

struct ElementosInventario {
    std::vector<VerticeCor> cores;
    std::vector<VerticeTex> texto;   // fonte
    std::vector<VerticeTex> icones;  // array de sprites; o item arrastado vem por último
};

// Painel, espaços com borda na cor da qualidade, ícones, nomes dos equipados, detalhes do item sob o mouse e ajuda;
// a mensagem da partida ("Equipado: …", "Vida cheia") aparece acima do painel, já que o HUD fica escondido
ElementosInventario construirInventario(const jogo::Inventario& inv, const LayoutInventario& l,
                                        std::optional<int> sobMouse, std::optional<int> arrastando, glm::vec2 mouse,
                                        std::string_view mensagem = {},
                                        std::string_view titulo = "Inventário (I fecha)");

}
