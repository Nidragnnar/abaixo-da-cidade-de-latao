#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>

#include "jogo/Placar.h"
#include "render/Hud.h"
#include "render/Letterbox.h"

namespace render {

struct VistaMenu {
    std::string_view titulo;
    std::vector<std::string> itens;
    int selecionado;
};

// Um retângulo por item, centralizados, de cima para baixo (para o mouse e para o destaque)
std::vector<Retangulo> layoutMenu(int quantidade, int telaL, int telaA);
std::optional<int> itemSob(const std::vector<Retangulo>& itens, glm::vec2 ponto);

// Painel escuro com o título (escala 2, latão) e os itens; o destaque do selecionado é o último retângulo
ElementosHud construirMenu(const VistaMenu& v, int telaL, int telaA);

// O item "Voltar" do placar, no rodapé
std::vector<Retangulo> layoutPlacar(int telaL, int telaA);
// Os 10 melhores (causa cortada com "..." para caber) ou "Nenhuma partida ainda"
ElementosHud construirPlacar(const std::vector<jogo::RegistroPlacar>& registros, int telaL, int telaA);

}
