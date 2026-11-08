#include "jogo/Placar.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <optional>
#include <sstream>
#include <string_view>

#include "jogo/ArquivoTexto.h"

namespace jogo {

namespace {

template <typename T>
bool lerNumero(std::string_view texto, T& valor) {
    const auto [fim, erro] = std::from_chars(texto.data(), texto.data() + texto.size(), valor);
    return erro == std::errc() && fim == texto.data() + texto.size();
}

// pontuação;andar;nível;inimigos;engrenagens;semente;letra;causa
// (formato antigo, sem a letra da dificuldade: 6 separadores, vale Normal; a causa nunca tem ';')
std::optional<RegistroPlacar> lerLinha(std::string_view linha) {
    std::vector<std::string_view> campos;
    const auto separadores = std::count(linha.begin(), linha.end(), ';');
    const int numCampos = separadores == 7 ? 7 : 6;
    size_t inicio = 0;
    for (int i = 0; i < numCampos; ++i) {
        const size_t fim = linha.find(';', inicio);
        if (fim == std::string_view::npos) return std::nullopt;
        campos.push_back(linha.substr(inicio, fim - inicio));
        inicio = fim + 1;
    }
    RegistroPlacar r{};
    if (!lerNumero(campos[0], r.pontuacao) || !lerNumero(campos[1], r.andar) || !lerNumero(campos[2], r.nivel) ||
        !lerNumero(campos[3], r.inimigos) || !lerNumero(campos[4], r.engrenagens) || !lerNumero(campos[5], r.semente))
        return std::nullopt;
    if (numCampos == 7) {
        const auto letra = campos[6];
        int achada = -1;
        for (int d = 0; d < TOTAL_DIFICULDADES; ++d)
            if (letra.size() == 1 && letra[0] == letraDa(static_cast<Dificuldade>(d))) achada = d;
        if (achada < 0) return std::nullopt;
        r.dificuldade = static_cast<Dificuldade>(achada);
    }
    r.causa = std::string(linha.substr(inicio));
    return r;
}

// A causa não pode ter separador nem quebra de linha
std::string limpar(std::string texto) {
    std::replace_if(texto.begin(), texto.end(), [](char c) { return c == ';' || c == '\n' || c == '\r'; }, ' ');
    return texto;
}

}

int calcularPontuacao(int andar, int inimigos, int engrenagens, int nivel, int chefes) {
    return 100 * andar + 10 * inimigos + engrenagens + 50 * nivel + 500 * chefes;
}

std::vector<RegistroPlacar> inserirNoPlacar(std::vector<RegistroPlacar> placar, RegistroPlacar r, int* posicao) {
    // Entra depois de todos com pontuação maior ou igual (o mais antigo fica na frente)
    const auto onde = std::find_if(placar.begin(), placar.end(),
                                   [&](const RegistroPlacar& o) { return o.pontuacao < r.pontuacao; });
    const size_t indice = static_cast<size_t>(onde - placar.begin());
    placar.insert(onde, std::move(r));
    if (placar.size() > TAMANHO_PLACAR) placar.resize(TAMANHO_PLACAR);
    if (posicao) *posicao = indice < TAMANHO_PLACAR ? static_cast<int>(indice) + 1 : 0;
    return placar;
}

LeituraPlacar lerPlacar(const std::filesystem::path& arquivo) {
    LeituraPlacar leitura;
    std::error_code erro;
    if (!std::filesystem::exists(arquivo, erro)) return leitura;
    {
        std::ifstream f(arquivo, std::ios::binary);
        if (!f.is_open()) {
            // Existe mas não abriu (outro programa segurando, sem permissão): não é vazio, não pode ser sobrescrito
            leitura.erroDeLeitura = true;
            return leitura;
        }
        std::string linha;
        bool primeira = true;
        while (std::getline(f, linha)) {
            // Arquivo salvo pelo Bloco de Notas pode começar com o BOM do UTF-8
            if (primeira && linha.starts_with("\xEF\xBB\xBF")) linha.erase(0, 3);
            primeira = false;
            if (!linha.empty() && linha.back() == '\r') linha.pop_back();
            if (linha.empty()) continue;
            if (auto r = lerLinha(linha)) {
                leitura.registros.push_back(std::move(*r));
            } else {
                leitura.estavaCorrompido = true;
                break;
            }
        }
    }
    if (leitura.estavaCorrompido) {
        leitura.registros.clear();
        leitura.renomeado = renomearCorrompido(arquivo);
    }
    return leitura;
}

bool gravarPlacar(const std::filesystem::path& arquivo, const std::vector<RegistroPlacar>& registros) {
    std::ostringstream texto;
    for (const RegistroPlacar& r : registros)
        texto << r.pontuacao << ';' << r.andar << ';' << r.nivel << ';' << r.inimigos << ';' << r.engrenagens << ';'
              << r.semente << ';' << letraDa(r.dificuldade) << ';' << limpar(r.causa) << '\n';
    return gravarAtomico(arquivo, texto.str());
}

}
