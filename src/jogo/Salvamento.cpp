#include "jogo/Salvamento.h"

#include <charconv>
#include <fstream>
#include <sstream>
#include <format>
#include <vector>

#include "jogo/ArquivoTexto.h"
#include "mundo/Gerador.h"

namespace jogo {

namespace {

constexpr size_t TAMANHO_MAXIMO = 4u << 20;  // 4 MB: um salvamento de verdade tem poucas dezenas de KB
constexpr int MAXIMO_BONUS = 4;

// --- Escrita ---

std::string item(const Item& i) {
    std::string s = std::format("{} {} {} {} {}", static_cast<int>(i.tipo), static_cast<int>(i.qualidade), i.quantidade,
                                i.carga, i.bonus.size());
    for (const Bonus& b : i.bonus) s += std::format(" {} {}", static_cast<int>(b.tipo), b.valor);
    return s;
}

// --- Leitura: palavras de uma linha, lidas em ordem; qualquer erro marca a linha como inválida ---

class Linha {
public:
    explicit Linha(std::string_view texto) : resto_(texto) {}
    bool ok() const { return ok_; }
    bool acabou() {
        pularEspacos();
        return resto_.empty();
    }
    std::string_view palavra() {
        pularEspacos();
        const size_t fim = resto_.find(' ');
        const std::string_view p = resto_.substr(0, fim);
        resto_ = fim == std::string_view::npos ? std::string_view{} : resto_.substr(fim);
        if (p.empty()) ok_ = false;
        return p;
    }
    template <typename T>
    T numero(T minimo, T maximo) {
        const std::string_view p = palavra();
        T v{};
        const auto [fim, erro] = std::from_chars(p.data(), p.data() + p.size(), v);
        if (erro != std::errc{} || fim != p.data() + p.size() || !(v >= minimo && v <= maximo)) {
            ok_ = false;
            return minimo;
        }
        return v;
    }
    float real() { return numero<float>(-1e6f, 1e6f); }
    int inteiro(int minimo = -1000000000, int maximo = 1000000000) { return numero<int>(minimo, maximo); }
    bool logico() { return inteiro(0, 1) == 1; }

private:
    void pularEspacos() {
        while (!resto_.empty() && resto_.front() == ' ') resto_.remove_prefix(1);
    }
    std::string_view resto_;
    bool ok_ = true;
};

Item lerItem(Linha& l) {
    Item i{static_cast<TipoItem>(l.inteiro(0, TOTAL_TIPOS_ITEM - 1))};
    i.qualidade = static_cast<Qualidade>(l.inteiro(0, 2));
    i.quantidade = l.inteiro(0, 99);
    i.carga = l.inteiro(0, 99);
    const int bonus = l.inteiro(0, MAXIMO_BONUS);
    for (int k = 0; k < bonus && l.ok(); ++k) {
        const auto tipo = static_cast<TipoBonus>(l.inteiro(0, 5));
        i.bonus.push_back({tipo, l.inteiro(-1000, 1000)});
    }
    return i;
}

}

std::optional<std::string> Salvamento::escrever(const Partida& p) {
    if (p.morto_ || p.venceu_ || p.desdeQuedaFinal_) return std::nullopt;
    const mundo::Andar original = mundo::gerarAndar(p.semente_, p.andar_.numero);
    std::string s = std::format("versao {}\npartida {} {}\ndificuldade {}\n", VERSAO_SALVAMENTO, p.semente_,
                                p.andar_.numero, static_cast<int>(p.dificuldade_));
    const Jogador& j = p.jogador_;
    s += std::format("jogador {} {} {} {} {} {} {} {}\n", j.posicao.x, j.posicao.y, j.angulo, j.inclinacao, j.corpo.pes,
                     j.corpo.velocidade, j.corpo.noChao ? 1 : 0, j.corpo.maisAlto);
    s += std::format("contadores {} {} {} {} {} {} {} {} {}\n", p.vida_, p.nivel_, p.xp_, p.engrenagens_, p.destruidos_,
                     p.chefesDerrotados_, p.trocasDeAndar_, p.tempoDeJogo_, p.temChave_ ? 1 : 0);
    const Municao& m = p.armas_.municao;
    s += std::format("municao {} {} {}\narma {}\n", m.balas, m.cartuchos, m.celulas, static_cast<int>(p.armas_.ativa));
    const Inventario& inv = p.inventario_;
    s += std::format("equipado {}\nequipado {}\nequipado {}\n", item(inv.armaBranca_), item(inv.armaDeFogo_),
                     item(inv.armadura_));
    for (int k = 0; k < ESPACOS_INVENTARIO; ++k)
        if (const auto& i = inv.espacos_[static_cast<size_t>(k)]) s += std::format("espaco {} {}\n", k, item(*i));
    // Andar: só as células que mudaram desde a geração (comportas abertas, destrancadas, trancadas da arena)
    const mundo::Grade& g = p.andar_.grade;
    for (int y = 0; y < g.altura(); ++y)
        for (int x = 0; x < g.largura(); ++x)
            if (g.em(x, y) != original.grade.em(x, y)) s += std::format("celula {} {} {}\n", x, y, static_cast<int>(g.em(x, y)));
    s += p.andar_.chave ? std::format("chave 1 {} {}\n", p.andar_.chave->x, p.andar_.chave->y) : "chave 0\n";
    s += std::format("arena {} {}", p.lutando_ ? 1 : 0, p.comportasArena_.size());
    for (const glm::ivec2 c : p.comportasArena_) s += std::format(" {} {}", c.x, c.y);
    s += "\nexplorado ";
    for (int y = 0; y < g.altura(); ++y)
        for (int x = 0; x < g.largura(); x += 4) {
            int bits = 0;
            for (int b = 0; b < 4; ++b) bits |= (x + b < g.largura() && p.exploracao_.vista(x + b, y)) ? 1 << b : 0;
            s += "0123456789abcdef"[bits];
        }
    s += '\n';
    for (const Inimigo& i : p.inimigos_)
        s += std::format("inimigo {} {} {} {} {} {} {} {} {} {} {} {} {} {}\n", static_cast<int>(i.tipo), i.posicao.x,
                         i.posicao.y, i.corpo.pes, i.vida, i.vidaMaxima, i.danoMin, i.danoMax, static_cast<int>(i.estado),
                         i.recompensado ? 1 : 0, i.alertou ? 1 : 0, i.recarga, i.recarga2, i.chamados);
    for (const CoisaNoChao& c : p.chao_) {
        if (const auto* i = std::get_if<Item>(&c.conteudo))
            s += std::format("chao {} {} item {}\n", c.posicao.x, c.posicao.y, item(*i));
        else if (const auto* m2 = std::get_if<MunicaoNoChao>(&c.conteudo))
            s += std::format("chao {} {} municao {} {}\n", c.posicao.x, c.posicao.y, static_cast<int>(m2->tipo), m2->quantidade);
        else
            s += std::format("chao {} {} engrenagens {}\n", c.posicao.x, c.posicao.y, std::get<int>(c.conteudo));
    }
    s += "fim\n";
    return s;
}

std::optional<Partida> Salvamento::ler(std::string_view texto) {
    if (texto.size() > TAMANHO_MAXIMO) return std::nullopt;
    std::vector<std::string_view> linhas;
    for (size_t inicio = 0; inicio < texto.size();) {
        size_t fim = texto.find('\n', inicio);
        if (fim == std::string_view::npos) fim = texto.size();
        std::string_view linha = texto.substr(inicio, fim - inicio);
        if (!linha.empty() && linha.back() == '\r') linha.remove_suffix(1);
        if (!linha.empty()) linhas.push_back(linha);
        inicio = fim + 1;
    }
    // Versão 1 (v1.4) não tem a linha da dificuldade: vale Normal
    const bool versao1 = !linhas.empty() && linhas.front() == "versao 1";
    if (linhas.size() < 3 || (!versao1 && linhas.front() != std::format("versao {}", VERSAO_SALVAMENTO)) ||
        linhas.back() != "fim")
        return std::nullopt;

    Linha cabecalho(linhas[1]);
    if (cabecalho.palavra() != "partida") return std::nullopt;
    const auto semente = cabecalho.numero<uint32_t>(0, 0xFFFFFFFFu);
    const int numero = cabecalho.inteiro(1, 99);
    if (!cabecalho.ok() || !cabecalho.acabou()) return std::nullopt;
    Dificuldade dificuldade = Dificuldade::Normal;
    size_t primeira = 2;  // primeira linha do corpo
    if (!versao1) {
        if (linhas.size() < 4) return std::nullopt;
        Linha l(linhas[2]);
        if (l.palavra() != "dificuldade") return std::nullopt;
        dificuldade = static_cast<Dificuldade>(l.inteiro(0, TOTAL_DIFICULDADES - 1));
        if (!l.ok() || !l.acabou()) return std::nullopt;
        primeira = 3;
    }

    // O andar refeito pela semente; o resto é sobrescrito pelo arquivo
    Partida p(semente, numero, dificuldade);
    p.inimigos_.clear();
    p.chao_.clear();
    p.exploracao_ = mundo::Exploracao(p.andar_.grade.largura(), p.andar_.grade.altura());
    Inventario& inv = p.inventario_;
    inv.espacos_ = {};
    int equipados = 0;
    bool viuJogador = false, viuContadores = false;

    for (size_t n = primeira; n + 1 < linhas.size(); ++n) {
        Linha l(linhas[n]);
        const std::string_view tipo = l.palavra();
        if (tipo == "jogador") {
            Jogador& j = p.jogador_;
            j.posicao = {l.real(), l.real()};
            j.angulo = l.real();
            j.inclinacao = l.real();
            j.corpo.pes = l.real();
            j.corpo.velocidade = l.real();
            j.corpo.noChao = l.logico();
            j.corpo.maisAlto = l.real();
            viuJogador = true;
        } else if (tipo == "contadores") {
            p.vida_ = l.inteiro(-1000, 100000);
            p.nivel_ = l.inteiro(1, 1000);
            p.xp_ = l.inteiro(0, 100000000);
            p.engrenagens_ = l.inteiro(0, 100000000);
            p.destruidos_ = l.inteiro(0, 100000000);
            p.chefesDerrotados_ = l.inteiro(0, 3);
            p.trocasDeAndar_ = l.inteiro(0, 1000);
            p.tempoDeJogo_ = l.numero<float>(0.0f, 1e9f);
            p.temChave_ = l.logico();
            viuContadores = true;
        } else if (tipo == "municao") {
            p.armas_.municao.balas = l.inteiro(0, 999);
            p.armas_.municao.cartuchos = l.inteiro(0, 999);
            p.armas_.municao.celulas = l.inteiro(0, 999);
        } else if (tipo == "arma") {
            p.armas_.ativa = static_cast<ArmaAtiva>(l.inteiro(0, 1));
        } else if (tipo == "equipado") {
            const Item i = lerItem(l);
            if (equipados == 0) inv.armaBranca_ = i;
            else if (equipados == 1) inv.armaDeFogo_ = i;
            else if (equipados == 2) inv.armadura_ = i;
            else return std::nullopt;
            ++equipados;
        } else if (tipo == "espaco") {
            const int k = l.inteiro(0, ESPACOS_INVENTARIO - 1);
            inv.espacos_[static_cast<size_t>(k)] = lerItem(l);
        } else if (tipo == "celula") {
            const int x = l.inteiro(0, p.andar_.grade.largura() - 1), y = l.inteiro(0, p.andar_.grade.altura() - 1);
            p.andar_.grade.definir(x, y, static_cast<mundo::Celula>(l.inteiro(0, 5)));
        } else if (tipo == "chave") {
            if (l.logico()) p.andar_.chave = glm::vec2(l.real(), l.real());
            else p.andar_.chave.reset();  // já foi pega
        } else if (tipo == "arena") {
            p.lutando_ = l.logico();
            const int quantas = l.inteiro(0, 1024);
            p.comportasArena_.clear();
            for (int k = 0; k < quantas && l.ok(); ++k) p.comportasArena_.push_back({l.inteiro(0, 1023), l.inteiro(0, 1023)});
        } else if (tipo == "explorado") {
            const std::string_view hex = l.palavra();
            const mundo::Grade& g = p.andar_.grade;
            const int porLinha = (g.largura() + 3) / 4;
            if (static_cast<int>(hex.size()) != porLinha * g.altura()) return std::nullopt;
            for (int y = 0; y < g.altura(); ++y)
                for (int c = 0; c < porLinha; ++c) {
                    const char h = hex[static_cast<size_t>(y * porLinha + c)];
                    const int bits = h >= '0' && h <= '9' ? h - '0' : h >= 'a' && h <= 'f' ? h - 'a' + 10 : -1;
                    if (bits < 0) return std::nullopt;
                    for (int b = 0; b < 4; ++b)
                        if (bits & (1 << b) && c * 4 + b < g.largura()) p.exploracao_.marcar(c * 4 + b, y);
                }
        } else if (tipo == "inimigo") {
            Inimigo i = criarInimigo(static_cast<TipoInimigo>(l.inteiro(0, TOTAL_TIPOS_INIMIGO - 1)), {0, 0}, numero, dificuldade);
            i.posicao = {l.real(), l.real()};
            i.corpo.pes = l.real();
            i.vida = l.inteiro(-100000, 100000);
            i.vidaMaxima = l.inteiro(1, 100000);
            i.danoMin = l.inteiro(0, 10000);
            i.danoMax = l.inteiro(0, 10000);
            i.estado = static_cast<EstadoInimigo>(l.inteiro(0, 4));
            // Preparando e Recuperando voltam a perseguir (o preparo do ataque não é guardado)
            if (i.estado == EstadoInimigo::Preparando || i.estado == EstadoInimigo::Recuperando)
                i.estado = EstadoInimigo::Perseguindo;
            i.recompensado = l.logico();
            i.alertou = l.logico();
            i.recarga = l.real();
            i.recarga2 = l.real();
            i.chamados = l.inteiro(0, 100000);
            i.corpo.noChao = !dados(i.tipo).voa;
            if (i.estado == EstadoInimigo::Destruido) i.tempoNoEstado = 1.0f;  // destroços, sem repetir a explosão
            p.inimigos_.push_back(i);
        } else if (tipo == "chao") {
            const glm::vec2 onde{l.real(), l.real()};
            const std::string_view oQue = l.palavra();
            if (oQue == "item") p.chao_.push_back({onde, lerItem(l)});
            else if (oQue == "municao") {
                const auto t = static_cast<TipoMunicao>(l.inteiro(0, 2));
                p.chao_.push_back({onde, MunicaoNoChao{t, l.inteiro(1, 999)}});
            } else if (oQue == "engrenagens") p.chao_.push_back({onde, l.inteiro(1, 100000)});
            else return std::nullopt;
        } else {
            return std::nullopt;  // linha desconhecida
        }
        if (!l.ok() || !l.acabou()) return std::nullopt;
    }
    if (!viuJogador || !viuContadores || equipados != 3) return std::nullopt;
    p.recalcularAtributos();
    p.desdeCampo_ = 1e9f;  // os campos de caminho são refeitos no primeiro passo
    return p;
}

}

namespace jogo {

bool gravarPartida(const std::filesystem::path& arquivo, const Partida& p) {
    const auto texto = escreverPartida(p);
    return texto && gravarAtomico(arquivo, *texto);
}

bool podeGravarAgora(const Partida& p) { return escreverPartida(p).has_value(); }

bool existePartidaSalva(const std::filesystem::path& arquivo) {
    std::error_code e;
    return std::filesystem::is_regular_file(arquivo, e);
}

std::pair<ResultadoCarga, std::optional<Partida>> continuarPartida(const std::filesystem::path& arquivo) {
    if (!existePartidaSalva(arquivo)) return {ResultadoCarga::Nenhum, std::nullopt};
    std::optional<Partida> p;
    std::error_code e;
    const auto tamanho = std::filesystem::file_size(arquivo, e);
    if (!e && tamanho <= TAMANHO_MAXIMO) {  // grande demais nem é lido
        std::ifstream f(arquivo, std::ios::binary);
        std::ostringstream conteudo;
        conteudo << f.rdbuf();
        p = lerPartida(conteudo.str());
    }
    if (!p) {
        renomearCorrompido(arquivo);
        return {ResultadoCarga::Estragado, std::nullopt};
    }
    // Um salvamento só: continuar o consome. Se não der para apagar, não continua (senão daria para voltar depois
    // de morrer)
    std::filesystem::remove(arquivo, e);
    if (e) return {ResultadoCarga::Preso, std::nullopt};
    return {ResultadoCarga::Carregou, std::move(p)};
}

void apagarPartidaSalva(const std::filesystem::path& arquivo) {
    std::error_code e;
    std::filesystem::remove(arquivo, e);
}

}
