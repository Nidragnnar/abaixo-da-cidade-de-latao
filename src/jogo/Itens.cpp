#include "jogo/Itens.h"

#include <algorithm>
#include <format>

#include <glm/geometric.hpp>

#include "mundo/Aleatorio.h"

namespace jogo {

namespace {

//                         nome               fem   dano   interv alcance carga recarga munição            chumbos abert empurrão andar
constexpr DadosArma ARMAS[] = {
    {"Chave inglesa",      true,  4, 6,   0.35f, 1.1f,  0, 0.0f, TipoMunicao::Balas,     1, 0.0f,  0.3f, 1},
    {"Sabre de oficial",   false, 6, 9,   0.5f,  1.3f,  0, 0.0f, TipoMunicao::Balas,     1, 0.0f,  0.3f, 1},
    {"Martelo a vapor",    false, 11, 15, 0.9f,  1.1f,  0, 0.0f, TipoMunicao::Balas,     1, 0.0f,  0.6f, 1},
    {"Revólver",           false, 7, 10,  0.4f,  0.0f,  6, 1.5f, TipoMunicao::Balas,     1, 4.0f,  0.0f, 1},
    {"Bacamarte",          false, 2, 4,   0.6f,  0.0f,  2, 2.2f, TipoMunicao::Cartuchos, 7, 12.0f, 0.0f, 2},
    {"Pistola Tesla",      true,  14, 20, 0.7f,  15.0f, 0, 0.0f, TipoMunicao::Celulas,   1, 0.0f,  0.0f, 4},
};

constexpr DadosArmadura ARMADURAS[] = {
    {"Casaco de couro", false, 1, 1},
    {"Colete blindado", false, 3, 3},
    {"Armadura de latão", true, 5, 5},
};

constexpr float RAIO_ENTRE_COISAS = 0.5f;
constexpr int TENTATIVAS_POSICAO = 200;

// Sorteio com pesos (pesos 0 ficam de fora)
template <typename T, size_t N>
T sortearComPeso(std::mt19937& rng, const std::pair<T, int> (&opcoes)[N]) {
    int total = 0;
    for (const auto& o : opcoes) total += o.second;
    int r = mundo::sortear(rng, 0, total - 1);
    for (const auto& o : opcoes) {
        if (r < o.second) return o.first;
        r -= o.second;
    }
    return opcoes[0].first;
}

Qualidade sortearQualidade(int andar, std::mt19937& rng) {
    const float obra = std::min(5.0f + 0.5f * (andar - 1), 15.0f);
    const float modificada = std::min(25.0f + 1.5f * (andar - 1), 45.0f);
    const float p = mundo::sortear01(rng) * 100.0f;
    if (p < obra) return Qualidade::ObraPrima;
    if (p < obra + modificada) return Qualidade::Modificada;
    return Qualidade::Comum;
}

std::vector<TipoBonus> bonusPossiveis(TipoItem t) {
    switch (classe(t)) {
        case ClasseItem::ArmaBranca: return {TipoBonus::Dano, TipoBonus::Velocidade};
        case ClasseItem::Armadura: return {TipoBonus::Defesa, TipoBonus::VidaMaxima};
        case ClasseItem::ArmaDeFogo:
            if (t == TipoItem::Tesla) return {TipoBonus::Dano, TipoBonus::Velocidade};
            return {TipoBonus::Dano, TipoBonus::Velocidade, TipoBonus::Carga, TipoBonus::Recarga};
        default: return {};
    }
}

int valorBonus(TipoBonus b, TipoItem t, int andar, std::mt19937& rng) {
    switch (b) {
        case TipoBonus::Dano: return mundo::sortear(rng, 1, 3) + andar / 3;
        case TipoBonus::Defesa: return mundo::sortear(rng, 1, 2) + andar / 4;
        case TipoBonus::VidaMaxima: return mundo::sortear(rng, 5, 10) + andar;
        case TipoBonus::Velocidade: return mundo::sortear(rng, 10, 20);
        case TipoBonus::Carga: return t == TipoItem::Revolver ? mundo::sortear(rng, 1, 2) : 1;
        case TipoBonus::Recarga: return mundo::sortear(rng, 15, 30);
    }
    return 1;
}

const char* adjetivo(TipoBonus b, ClasseItem c, bool feminino) {
    switch (b) {
        case TipoBonus::Dano:
            if (c == ClasseItem::ArmaDeFogo) return feminino ? "Calibrada" : "Calibrado";
            return feminino ? "Afiada" : "Afiado";
        case TipoBonus::Defesa: return feminino ? "Reforçada" : "Reforçado";
        case TipoBonus::VidaMaxima: return "Vital";
        case TipoBonus::Velocidade: return "Veloz";
        case TipoBonus::Carga: return feminino ? "Ampliada" : "Ampliado";
        case TipoBonus::Recarga: return "Ágil";
    }
    return "";
}

MunicaoNoChao sortearCaixa(int andar, std::mt19937& rng) {
    const std::pair<TipoMunicao, int> opcoes[] = {{TipoMunicao::Balas, 60},
                                                  {TipoMunicao::Cartuchos, andar >= 2 ? 30 : 0},
                                                  {TipoMunicao::Celulas, andar >= 4 ? 10 : 0}};
    const TipoMunicao t = sortearComPeso(rng, opcoes);
    return {t, tamanhoCaixa(t)};
}

}

ClasseItem classe(TipoItem t) {
    switch (t) {
        case TipoItem::ChaveInglesa:
        case TipoItem::Sabre:
        case TipoItem::Martelo: return ClasseItem::ArmaBranca;
        case TipoItem::Revolver:
        case TipoItem::Bacamarte:
        case TipoItem::Tesla: return ClasseItem::ArmaDeFogo;
        case TipoItem::Casaco:
        case TipoItem::Colete:
        case TipoItem::ArmaduraLatao: return ClasseItem::Armadura;
        default: return ClasseItem::Consumivel;
    }
}

const DadosArma& dadosArma(TipoItem t) { return ARMAS[std::clamp(static_cast<int>(t), 0, 5)]; }

const DadosArmadura& dadosArmadura(TipoItem t) {
    return ARMADURAS[std::clamp(static_cast<int>(t) - static_cast<int>(TipoItem::Casaco), 0, 2)];
}

int somaBonus(const Item& i, TipoBonus t) {
    int s = 0;
    for (const Bonus& b : i.bonus)
        if (b.tipo == t) s += b.valor;
    return s;
}

int capacidade(const Item& arma) { return dadosArma(arma.tipo).carga + somaBonus(arma, TipoBonus::Carga); }

Item criarItem(TipoItem t) {
    Item i{t};
    if (classe(t) == ClasseItem::ArmaDeFogo) i.carga = capacidade(i);
    return i;
}

std::string nomeItem(const Item& i) {
    const ClasseItem c = classe(i.tipo);
    if (c == ClasseItem::Consumivel)
        return i.quantidade > 1 ? std::format("Tônico restaurador ×{}", i.quantidade) : "Tônico restaurador";
    const bool feminino = c == ClasseItem::Armadura ? dadosArmadura(i.tipo).feminino : dadosArma(i.tipo).feminino;
    std::string nome = c == ClasseItem::Armadura ? dadosArmadura(i.tipo).nome : dadosArma(i.tipo).nome;
    if (i.bonus.empty()) return nome;
    const Bonus& b = i.bonus.front();
    const bool porcento = b.tipo == TipoBonus::Velocidade || b.tipo == TipoBonus::Recarga;
    return std::format("{} {} +{}{}", nome, adjetivo(b.tipo, c, feminino), b.valor, porcento ? "%" : "");
}

Item gerarItem(int andar, std::mt19937& rng) {
    const std::pair<ClasseItem, int> classes[] = {{ClasseItem::Consumivel, 35},
                                                  {ClasseItem::ArmaBranca, 20},
                                                  {ClasseItem::ArmaDeFogo, 20},
                                                  {ClasseItem::Armadura, 25}};
    TipoItem tipo = TipoItem::Tonico;
    switch (sortearComPeso(rng, classes)) {
        case ClasseItem::ArmaBranca: {
            const std::pair<TipoItem, int> o[] = {{TipoItem::Sabre, 50}, {TipoItem::Martelo, 35}, {TipoItem::ChaveInglesa, 15}};
            tipo = sortearComPeso(rng, o);
            break;
        }
        case ClasseItem::ArmaDeFogo: {
            const std::pair<TipoItem, int> o[] = {{TipoItem::Revolver, 50},
                                                  {TipoItem::Bacamarte, andar >= 2 ? 35 : 0},
                                                  {TipoItem::Tesla, andar >= 4 ? 15 : 0}};
            tipo = sortearComPeso(rng, o);
            break;
        }
        case ClasseItem::Armadura: {
            const std::pair<TipoItem, int> o[] = {{TipoItem::Casaco, 50},
                                                  {TipoItem::Colete, andar >= 3 ? 30 : 0},
                                                  {TipoItem::ArmaduraLatao, andar >= 5 ? 20 : 0}};
            tipo = sortearComPeso(rng, o);
            break;
        }
        default: break;
    }

    Item item = criarItem(tipo);
    if (classe(tipo) == ClasseItem::Consumivel) return item;
    item.qualidade = sortearQualidade(andar, rng);
    const size_t quantos = item.qualidade == Qualidade::ObraPrima ? 2 : item.qualidade == Qualidade::Modificada ? 1 : 0;
    std::vector<TipoBonus> possiveis = bonusPossiveis(tipo);
    for (size_t k = 0; k < quantos && !possiveis.empty(); ++k) {
        // Sorteia sem repetir o tipo do bônus
        const size_t escolhido = static_cast<size_t>(mundo::sortear(rng, 0, static_cast<int>(possiveis.size()) - 1));
        item.bonus.push_back({possiveis[escolhido], valorBonus(possiveis[escolhido], tipo, andar, rng)});
        possiveis.erase(possiveis.begin() + static_cast<long long>(escolhido));
    }
    if (classe(tipo) == ClasseItem::ArmaDeFogo) item.carga = capacidade(item);  // vem carregada
    return item;
}

int tamanhoCaixa(TipoMunicao t) {
    switch (t) {
        case TipoMunicao::Cartuchos: return 4;
        case TipoMunicao::Celulas: return 3;
        default: return 6;
    }
}

std::vector<CoisaNoChao> gerarChao(const mundo::Andar& andar, uint32_t semente) {
    std::vector<CoisaNoChao> chao;
    if (andar.salas.size() < 2) return chao;
    std::mt19937 rng(mundo::sementeDoAndar(semente, andar.numero) ^ 0x17E5u);

    // Célula de chão numa sala que não é a inicial, longe das outras coisas
    auto colocar = [&](std::variant<Item, MunicaoNoChao, int> conteudo) {
        for (int t = 0; t < TENTATIVAS_POSICAO; ++t) {
            const mundo::Sala& s = andar.salas[mundo::sortear(rng, 1, static_cast<int>(andar.salas.size()) - 1)];
            const int x = mundo::sortear(rng, s.x, s.x + s.largura - 1);
            const int y = mundo::sortear(rng, s.y, s.y + s.altura - 1);
            if (andar.grade.em(x, y) != mundo::Celula::Chao) continue;
            const glm::vec2 p{x + 0.5f, y + 0.5f};
            const bool perto = std::any_of(chao.begin(), chao.end(), [&](const CoisaNoChao& c) {
                return glm::distance(c.posicao, p) < RAIO_ENTRE_COISAS;
            });
            if (perto) continue;
            chao.push_back({p, std::move(conteudo)});
            return;
        }
    };

    const int itens = mundo::sortear(rng, 4, 7);
    const int caixas = mundo::sortear(rng, 3, 5);
    const int pilhas = mundo::sortear(rng, 3, 6);
    for (int i = 0; i < itens; ++i) colocar(gerarItem(andar.numero, rng));
    for (int i = 0; i < caixas; ++i) colocar(sortearCaixa(andar.numero, rng));
    for (int i = 0; i < pilhas; ++i) colocar(mundo::sortear(rng, 5, 20) * andar.numero);
    return chao;
}

std::vector<CoisaNoChao> soltarDoInimigo(TipoInimigo t, glm::vec2 posicao, int andar, std::mt19937& rng) {
    std::vector<CoisaNoChao> v;
    if (mundo::sortear01(rng) < 0.25f) v.push_back({posicao, gerarItem(andar, rng)});
    // Um pouco ao lado, para não ficar um em cima do outro
    const glm::vec2 lado = posicao + glm::vec2(0.2f, 0.15f);
    if (t == TipoInimigo::Atirador) v.push_back({lado, MunicaoNoChao{TipoMunicao::Balas, tamanhoCaixa(TipoMunicao::Balas)}});
    else if (mundo::sortear01(rng) < 0.35f) v.push_back({lado, sortearCaixa(andar, rng)});
    return v;
}

}
