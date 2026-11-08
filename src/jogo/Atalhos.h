#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace jogo {

// Ações do jogo que trocam de tecla nas Opções (Esc, teclas dos menus e roda do mouse ficam fixas)
enum class AcaoJogo : uint8_t {
    Frente, Tras, Esquerda, Direita, Pular, Usar, TrocarArma, Recarregar, Inventario, Mapa,
    Item1, Item2, Item3, Item4, Atacar
};
constexpr int TOTAL_ACOES = 15;

// Tecla física: scancode do SDL (números da tabela USB, 1–511) ou BOTAO_MOUSE + botão do SDL
// (1 esquerdo, 2 meio, 3 direito, 4 e 5 laterais)
constexpr int BOTAO_MOUSE = 1000;

struct Atalhos {
    std::array<int, TOTAL_ACOES> tecla{};

    // W S A D, Espaço, E, Q, R, I, Tab, 1 2 3 4 e o botão esquerdo
    static Atalhos padrao();
    int de(AcaoJogo a) const { return tecla[static_cast<size_t>(a)]; }
    // Tecla que já é de outra ação: as duas trocam (nenhuma fica sem tecla)
    void definir(AcaoJogo a, int t);
    std::optional<AcaoJogo> acaoDa(int t) const;
    // Tecla não aceita ou repetida (de um config editado à mão) volta ao padrão; se o padrão estiver ocupado,
    // ganha a primeira tecla de fábrica livre. As marcadas em fixas (vindas do arquivo) têm preferência.
    void corrigirRepetidas(const std::array<bool, TOTAL_ACOES>& fixas = {});
};

// Só as teclas da tabela de nomes (e não o Esc) podem virar atalho
bool teclaAceita(int t);
// Para mostrar ("Espaço", "Shift esq.", "Botão esquerdo"); vazio se não está na tabela
std::string_view nomeDaTecla(int t);
// No config, sem acento ("espaco", "shift_esq", "mouse1")
std::string_view idDaTecla(int t);
std::optional<int> teclaDoId(std::string_view id);
// No config: tecla_<id> ("pular", "trocar_arma", "item1"...)
std::string_view idDaAcao(AcaoJogo a);
std::optional<AcaoJogo> acaoDoId(std::string_view id);
// Na tela de teclas ("Andar para frente", "Atacar"...)
std::string_view nomeDaAcao(AcaoJogo a);

// Segurada agora? teclado = SDL_GetKeyboardState (pode ser nulo); botoesMouse = SDL_GetMouseState (bit b − 1)
bool apertada(int t, const uint8_t* teclado, uint32_t botoesMouse);

// Troca "{recarregar}", "{inventario}"... (o id da ação entre chaves) pelo nome dado
std::string comTeclas(std::string_view texto, const std::function<std::string(AcaoJogo)>& nome);

}
