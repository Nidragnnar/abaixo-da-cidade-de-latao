#include "mundo/Grade.h"

#include <format>
#include <stdexcept>

namespace mundo {

Grade::Grade(int largura, int altura)
    : largura_(largura),
      altura_(altura),
      celulas_(static_cast<size_t>(largura) * altura, Celula::Parede),
      chao_(celulas_.size(), 0),
      teto_(celulas_.size(), TETO_PADRAO),
      rampas_(celulas_.size(), Rampa::Nenhuma),
      detalhes_(celulas_.size(), Detalhe::Nenhum) {}

Celula Grade::em(int x, int y) const {
    if (!dentro(x, y)) return Celula::Parede;
    return celulas_[static_cast<size_t>(y) * largura_ + x];
}

void Grade::definir(int x, int y, Celula c) {
    if (dentro(x, y)) celulas_[static_cast<size_t>(y) * largura_ + x] = c;
}

bool Grade::solida(int x, int y) const {
    Celula c = em(x, y);
    return c == Celula::Parede || c == Celula::Comporta || c == Celula::ComportaTrancada;
}

int Grade::chao(int x, int y) const { return dentro(x, y) ? chao_[static_cast<size_t>(y) * largura_ + x] : 0; }
int Grade::teto(int x, int y) const { return dentro(x, y) ? teto_[static_cast<size_t>(y) * largura_ + x] : TETO_PADRAO; }
Rampa Grade::rampa(int x, int y) const { return dentro(x, y) ? rampas_[static_cast<size_t>(y) * largura_ + x] : Rampa::Nenhuma; }
Detalhe Grade::detalhe(int x, int y) const {
    return dentro(x, y) ? detalhes_[static_cast<size_t>(y) * largura_ + x] : Detalhe::Nenhum;
}

void Grade::definirAltura(int x, int y, int chao, int teto) {
    if (!dentro(x, y)) return;
    chao_[static_cast<size_t>(y) * largura_ + x] = static_cast<int8_t>(chao);
    teto_[static_cast<size_t>(y) * largura_ + x] = static_cast<int8_t>(teto);
}
void Grade::definirRampa(int x, int y, Rampa r) {
    if (dentro(x, y)) rampas_[static_cast<size_t>(y) * largura_ + x] = r;
}
void Grade::definirDetalhe(int x, int y, Detalhe d) {
    if (dentro(x, y)) detalhes_[static_cast<size_t>(y) * largura_ + x] = d;
}

namespace {

MapaCarregado lerSoMapa(const std::vector<std::string>& linhas);  // só os caracteres do mapa (abaixo)

// Camada opcional de mesmo tamanho do mapa; lança se estiver torta
void conferirCamada(const std::vector<std::string>& camada, const std::vector<std::string>& linhas, const char* nome) {
    if (camada.empty()) return;
    if (camada.size() != linhas.size()) throw std::invalid_argument(std::format("camada de {} com outra altura", nome));
    for (size_t y = 0; y < linhas.size(); ++y)
        if (camada[y].size() != linhas[y].size())
            throw std::invalid_argument(std::format("camada de {}: linha {} com tamanho diferente", nome, y));
}

}

MapaCarregado carregarDeTexto(const std::vector<std::string>& linhas, const std::vector<std::string>& alturas,
                              const std::vector<std::string>& tetos) {
    MapaCarregado mapa = lerSoMapa(linhas);
    conferirCamada(alturas, linhas, "alturas");
    conferirCamada(tetos, linhas, "tetos");
    for (size_t y = 0; y < alturas.size(); ++y)
        for (size_t x = 0; x < alturas[y].size(); ++x) {
            const char c = alturas[y][x];
            const int cx = static_cast<int>(x), cy = static_cast<int>(y);
            int chao = 0;
            Rampa r = Rampa::Nenhuma;
            if (c >= '0' && c <= '9') chao = c - '0';
            else if (c >= 'a' && c <= 'c') chao = 10 + (c - 'a');
            else {
                switch (c) {
                    case 'n': case 'N': r = Rampa::Norte; break;
                    case 's': case 'S': r = Rampa::Sul; break;
                    case 'l': case 'L': r = Rampa::Leste; break;
                    case 'o': case 'O': r = Rampa::Oeste; break;
                    default: throw std::invalid_argument(std::format("altura desconhecida em ({}, {})", x, y));
                }
                chao = (c >= 'A' && c <= 'Z') ? 4 : 0;
            }
            mapa.grade.definirAltura(cx, cy, chao, chao + TETO_PADRAO + (r != Rampa::Nenhuma ? 4 : 0));
            mapa.grade.definirRampa(cx, cy, r);
        }
    for (size_t y = 0; y < tetos.size(); ++y)
        for (size_t x = 0; x < tetos[y].size(); ++x) {
            const char c = tetos[y][x];
            int acima = 0;  // quartos acima do chão
            if (c >= '0' && c <= '9') acima = c - '0';
            else if (c >= 'a' && c <= 'k') acima = 10 + (c - 'a');
            else throw std::invalid_argument(std::format("teto desconhecido em ({}, {})", x, y));
            const int cx = static_cast<int>(x), cy = static_cast<int>(y);
            if (acima != 0) mapa.grade.definirAltura(cx, cy, mapa.grade.chao(cx, cy), mapa.grade.chao(cx, cy) + acima);
        }
    return mapa;
}

namespace {

MapaCarregado lerSoMapa(const std::vector<std::string>& linhas) {
    if (linhas.empty() || linhas[0].empty()) throw std::invalid_argument("mapa vazio");
    const int largura = static_cast<int>(linhas[0].size());
    const int altura = static_cast<int>(linhas.size());

    MapaCarregado mapa{Grade(largura, altura), {}, {}, {}, {}};
    int inicios = 0;
    for (int y = 0; y < altura; ++y) {
        if (static_cast<int>(linhas[y].size()) != largura)
            throw std::invalid_argument(std::format("linha {} com tamanho diferente", y));
        for (int x = 0; x < largura; ++x) {
            switch (linhas[y][x]) {
                case '#': mapa.grade.definir(x, y, Celula::Parede); break;
                case '.': mapa.grade.definir(x, y, Celula::Chao); break;
                case '@':
                    mapa.grade.definir(x, y, Celula::Chao);
                    mapa.inicio = {x + 0.5f, y + 0.5f};
                    ++inicios;
                    break;
                case 'C': mapa.grade.definir(x, y, Celula::Comporta); break;
                case 'T': mapa.grade.definir(x, y, Celula::ComportaTrancada); break;
                case 'E':
                    mapa.grade.definir(x, y, Celula::Elevador);
                    mapa.elevador = glm::ivec2(x, y);
                    break;
                case 'r':
                case 'a':
                case 'g':
                case 'c':
                case 'v':
                case 'd':
                case 'R':
                case 'P':
                case 'M':
                    mapa.grade.definir(x, y, Celula::Chao);
                    mapa.marcas.push_back({linhas[y][x], glm::vec2(x + 0.5f, y + 0.5f)});
                    break;
                case 'K':
                    mapa.grade.definir(x, y, Celula::Chao);
                    mapa.chave = glm::vec2(x + 0.5f, y + 0.5f);
                    break;
                default:
                    throw std::invalid_argument(std::format("caractere desconhecido em ({}, {})", x, y));
            }
        }
    }
    if (inicios != 1) throw std::invalid_argument("o mapa precisa de exatamente um '@'");
    return mapa;
}

}  // namespace

}
