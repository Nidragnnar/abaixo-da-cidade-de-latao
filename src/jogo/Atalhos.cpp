#include "jogo/Atalhos.h"

#include <algorithm>
#include <vector>

namespace jogo {

namespace {

struct NomeTecla {
    int tecla;
    std::string id, nome;
};

// Tabela de nomes: scancodes do SDL (tabela USB) e botões do mouse. Os símbolos seguem o teclado ABNT2
// (o Ç fica no lugar do ";" americano); os ids no config seguem o nome da tecla americana.
const std::vector<NomeTecla>& tabela() {
    static const std::vector<NomeTecla> t = [] {
        std::vector<NomeTecla> v;
        for (int i = 0; i < 26; ++i) {
            const char letra = static_cast<char>('a' + i);
            v.push_back({4 + i, std::string(1, letra), std::string(1, static_cast<char>('A' + i))});
        }
        for (int i = 0; i < 10; ++i) {  // 1…9 e 0
            const std::string d(1, static_cast<char>(i < 9 ? '1' + i : '0'));
            v.push_back({30 + i, d, d});
        }
        const std::vector<NomeTecla> outras = {
            {40, "enter", "Enter"},
            {42, "backspace", "Backspace"},
            {43, "tab", "Tab"},
            {44, "espaco", "Espaço"},
            {45, "menos", "-"},
            {46, "igual", "="},
            {47, "abre_colchete", "Acento agudo"},
            {48, "fecha_colchete", "["},
            {49, "barra_inv", "]"},
            {51, "ponto_virgula", "Ç"},
            {52, "aspas", "~"},
            {53, "crase", "'"},
            {54, "virgula", ","},
            {55, "ponto", "."},
            {56, "barra", ";"},
            {57, "caps", "Caps Lock"},
            {73, "insert", "Insert"},
            {74, "home", "Home"},
            {75, "pgup", "Page Up"},
            {76, "delete", "Delete"},
            {77, "end", "End"},
            {78, "pgdn", "Page Down"},
            {79, "seta_direita", "Seta direita"},
            {80, "seta_esquerda", "Seta esquerda"},
            {81, "seta_baixo", "Seta para baixo"},
            {82, "seta_cima", "Seta para cima"},
            {84, "num_barra", "Num /"},
            {85, "num_asterisco", "Num *"},
            {86, "num_menos", "Num -"},
            {87, "num_mais", "Num +"},
            {88, "num_enter", "Num Enter"},
            {99, "num_ponto", "Num ,"},
            {100, "contrabarra", "\\"},
            {135, "barra_abnt", "/"},
            {224, "ctrl_esq", "Ctrl esq."},
            {225, "shift_esq", "Shift esq."},
            {226, "alt_esq", "Alt esq."},
            {228, "ctrl_dir", "Ctrl dir."},
            {229, "shift_dir", "Shift dir."},
            {230, "alt_dir", "Alt Gr"},
            {BOTAO_MOUSE + 1, "mouse1", "Botão esquerdo"},
            {BOTAO_MOUSE + 2, "mouse2", "Botão do meio"},
            {BOTAO_MOUSE + 3, "mouse3", "Botão direito"},
            {BOTAO_MOUSE + 4, "mouse4", "Botão lateral 1"},
            {BOTAO_MOUSE + 5, "mouse5", "Botão lateral 2"},
        };
        v.insert(v.end(), outras.begin(), outras.end());
        for (int i = 0; i < 12; ++i) {
            const std::string n = std::to_string(i + 1);
            v.push_back({58 + i, "f" + n, "F" + n});
        }
        for (int i = 0; i < 10; ++i) {  // teclado numérico 1…9 e 0
            const std::string d(1, static_cast<char>(i < 9 ? '1' + i : '0'));
            v.push_back({89 + i, "num_" + d, "Num " + d});
        }
        return v;
    }();
    return t;
}

const NomeTecla* procurar(int t) {
    for (const NomeTecla& n : tabela())
        if (n.tecla == t) return &n;
    return nullptr;
}

struct DadosAcao {
    std::string_view id, nome;
};
constexpr DadosAcao ACOES[TOTAL_ACOES] = {
    {"frente", "Andar para frente"}, {"tras", "Andar para trás"},  {"esquerda", "Andar à esquerda"},
    {"direita", "Andar à direita"},  {"pular", "Pular"},           {"usar", "Usar"},
    {"trocar_arma", "Trocar arma"},  {"recarregar", "Recarregar"}, {"inventario", "Inventário"},
    {"mapa", "Mapa"},                {"item1", "Item rápido 1"},   {"item2", "Item rápido 2"},
    {"item3", "Item rápido 3"},      {"item4", "Item rápido 4"},   {"atacar", "Atacar"},
};

}

Atalhos Atalhos::padrao() {
    // W S A D, Espaço, E, Q, R, I, Tab, 1 2 3 4, botão esquerdo
    return {{26, 22, 4, 7, 44, 8, 20, 21, 12, 43, 30, 31, 32, 33, BOTAO_MOUSE + 1}};
}

void Atalhos::definir(AcaoJogo a, int t) {
    const size_t i = static_cast<size_t>(a);
    if (const auto outra = acaoDa(t); outra && *outra != a) tecla[static_cast<size_t>(*outra)] = tecla[i];
    tecla[i] = t;
}

std::optional<AcaoJogo> Atalhos::acaoDa(int t) const {
    for (int i = 0; i < TOTAL_ACOES; ++i)
        if (tecla[static_cast<size_t>(i)] == t) return static_cast<AcaoJogo>(i);
    return std::nullopt;
}

void Atalhos::corrigirRepetidas(const std::array<bool, TOTAL_ACOES>& fixas) {
    // Primeiro ficam as teclas boas (as fixas antes, depois as outras na ordem das ações); o resto é corrigido
    std::vector<size_t> ordem;
    for (size_t i = 0; i < TOTAL_ACOES; ++i)
        if (fixas[i]) ordem.push_back(i);
    for (size_t i = 0; i < TOTAL_ACOES; ++i)
        if (!fixas[i]) ordem.push_back(i);
    std::vector<int> usadas;
    std::vector<size_t> corrigir;
    const auto usada = [&](int t) { return std::find(usadas.begin(), usadas.end(), t) != usadas.end(); };
    for (size_t i : ordem) {
        if (teclaAceita(tecla[i]) && !usada(tecla[i])) usadas.push_back(tecla[i]);
        else corrigir.push_back(i);
    }
    const Atalhos fabrica = padrao();
    for (size_t i : corrigir) {
        int nova = fabrica.tecla[i];
        // Padrão ocupado: a primeira tecla de fábrica livre (sempre há uma: são 15 para 14 outras ações)
        for (size_t k = 0; usada(nova) && k < TOTAL_ACOES; ++k) nova = fabrica.tecla[k];
        tecla[i] = nova;
        usadas.push_back(nova);
    }
}

bool teclaAceita(int t) { return procurar(t) != nullptr; }

std::string_view nomeDaTecla(int t) {
    const NomeTecla* n = procurar(t);
    return n ? std::string_view(n->nome) : std::string_view();
}

std::string_view idDaTecla(int t) {
    const NomeTecla* n = procurar(t);
    return n ? std::string_view(n->id) : std::string_view();
}

std::optional<int> teclaDoId(std::string_view id) {
    for (const NomeTecla& n : tabela())
        if (n.id == id) return n.tecla;
    return std::nullopt;
}

std::string_view idDaAcao(AcaoJogo a) { return ACOES[static_cast<size_t>(a)].id; }

std::optional<AcaoJogo> acaoDoId(std::string_view id) {
    for (int i = 0; i < TOTAL_ACOES; ++i)
        if (ACOES[i].id == id) return static_cast<AcaoJogo>(i);
    return std::nullopt;
}

std::string_view nomeDaAcao(AcaoJogo a) { return ACOES[static_cast<size_t>(a)].nome; }

bool apertada(int t, const uint8_t* teclado, uint32_t botoesMouse) {
    if (t > BOTAO_MOUSE && t <= BOTAO_MOUSE + 5) return (botoesMouse >> (t - BOTAO_MOUSE - 1)) & 1u;
    return teclado && t > 0 && t < 512 && teclado[t];
}

std::string comTeclas(std::string_view texto, const std::function<std::string(AcaoJogo)>& nome) {
    std::string s(texto);
    for (int i = 0; i < TOTAL_ACOES; ++i) {
        const auto acao = static_cast<AcaoJogo>(i);
        const std::string marca = "{" + std::string(idDaAcao(acao)) + "}";
        for (size_t p = s.find(marca); p != std::string::npos; p = s.find(marca, p)) {
            const std::string n = nome(acao);
            s.replace(p, marca.size(), n);
            p += n.size();
        }
    }
    return s;
}

}
