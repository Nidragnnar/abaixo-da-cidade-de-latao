#include "jogo/Config.h"

#include <array>
#include <charconv>
#include <cmath>
#include <fstream>
#include <string>
#include <string_view>

#include "jogo/ArquivoTexto.h"

namespace jogo {

namespace {

template <typename T>
bool lerNumero(std::string_view texto, T& valor) {
    if (texto.empty() || texto.front() == '-' || texto.front() == '+') return false;
    const auto [fim, erro] = std::from_chars(texto.data(), texto.data() + texto.size(), valor);
    return erro == std::errc() && fim == texto.data() + texto.size();
}

bool lerFaixa(std::string_view texto, int minimo, int maximo, int& valor) {
    int v = 0;
    if (!lerNumero(texto, v) || v < minimo || v > maximo) return false;
    valor = v;
    return true;
}

// Aplica uma linha "chave=valor"; falso se a linha estraga o arquivo
bool aplicarLinha(std::string_view linha, Config& c, std::array<bool, TOTAL_ACOES>& fixas) {
    const size_t igual = linha.find('=');
    if (igual == std::string_view::npos) return false;
    // Quem edita à mão costuma pôr espaços: "sensibilidade = 7"
    const auto aparar = [](std::string_view s) {
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.remove_suffix(1);
        return s;
    };
    const std::string_view chave = aparar(linha.substr(0, igual)), valor = aparar(linha.substr(igual + 1));
    if (chave == "sensibilidade") return lerFaixa(valor, SENSIBILIDADE_MIN, SENSIBILIDADE_MAX, c.sensibilidade);
    if (chave == "volume_efeitos") return lerFaixa(valor, 0, VOLUME_MAX, c.volumeEfeitos);
    if (chave == "volume_musica") return lerFaixa(valor, 0, VOLUME_MAX, c.volumeMusica);
    if (chave == "tela_cheia") {
        if (valor != "sim" && valor != "nao") return false;
        c.telaCheia = valor == "sim";
        return true;
    }
    if (chave == "dificuldade") {
        const auto d = dificuldadeDoId(valor);
        if (!d) return false;
        c.dificuldade = *d;
        return true;
    }
    if (chave == "sensibilidade_controle")
        return lerFaixa(valor, SENSIBILIDADE_MIN, SENSIBILIDADE_MAX, c.sensibilidadeControle);
    if (chave == "inverter_vertical") {
        if (valor != "0" && valor != "1") return false;
        c.inverterVertical = valor == "1";
        return true;
    }
    if (chave == "semente") {
        if (valor.empty()) {
            c.semente.reset();
            return true;
        }
        uint32_t s = 0;
        if (!lerNumero(valor, s)) return false;
        c.semente = s;
        return true;
    }
    // tecla_<ação>: valor inválido ou não aceito deixa o padrão só daquela ação (não estraga o arquivo)
    if (chave.starts_with("tecla_")) {
        const auto acao = acaoDoId(chave.substr(6));
        const auto tecla = teclaDoId(valor);
        if (acao && tecla && teclaAceita(*tecla)) {
            c.teclas.tecla[static_cast<size_t>(*acao)] = *tecla;
            fixas[static_cast<size_t>(*acao)] = true;
        }
        return true;
    }
    return true;  // chave desconhecida: ignorada
}

}

LeituraConfig lerConfig(const std::filesystem::path& arquivo) {
    LeituraConfig leitura;
    std::error_code erro;
    if (!std::filesystem::exists(arquivo, erro)) return leitura;
    {
        std::ifstream f(arquivo, std::ios::binary);
        if (!f.is_open()) {
            leitura.erroDeLeitura = true;
            return leitura;
        }
        std::string linha;
        bool primeira = true;
        std::array<bool, TOTAL_ACOES> fixas{};  // ações com tecla vinda do arquivo (têm preferência nas repetidas)
        while (std::getline(f, linha)) {
            // Arquivo salvo pelo Bloco de Notas pode começar com o BOM do UTF-8
            if (primeira && linha.starts_with("\xEF\xBB\xBF")) linha.erase(0, 3);
            primeira = false;
            if (!linha.empty() && linha.back() == '\r') linha.pop_back();
            if (linha.empty() || linha.front() == '#') continue;
            if (!aplicarLinha(linha, leitura.config, fixas)) {
                leitura.estavaCorrompido = true;
                break;
            }
        }
        leitura.config.teclas.corrigirRepetidas(fixas);
    }
    if (leitura.estavaCorrompido) {
        leitura.config = Config{};
        leitura.renomeado = renomearCorrompido(arquivo);
    }
    return leitura;
}

bool gravarConfig(const std::filesystem::path& arquivo, const Config& c) {
    std::string texto = "# Abaixo da Cidade de Latão — opções (apague este arquivo para voltar ao padrão)\n";
    texto += "sensibilidade=" + std::to_string(c.sensibilidade) + "\n";
    texto += "volume_efeitos=" + std::to_string(c.volumeEfeitos) + "\n";
    texto += "volume_musica=" + std::to_string(c.volumeMusica) + "\n";
    texto += std::string("tela_cheia=") + (c.telaCheia ? "sim" : "nao") + "\n";
    texto += "semente=" + (c.semente ? std::to_string(*c.semente) : std::string()) + "\n";
    texto += "dificuldade=" + std::string(idDa(c.dificuldade)) + "\n";
    texto += "sensibilidade_controle=" + std::to_string(c.sensibilidadeControle) + "\n";
    texto += std::string("inverter_vertical=") + (c.inverterVertical ? "1" : "0") + "\n";
    for (int i = 0; i < TOTAL_ACOES; ++i) {
        const auto acao = static_cast<AcaoJogo>(i);
        texto += "tecla_" + std::string(idDaAcao(acao)) + "=" + std::string(idDaTecla(c.teclas.de(acao))) + "\n";
    }
    return gravarAtomico(arquivo, texto);
}

float radianosPorPixel(int sensibilidade) {
    return 0.0025f * std::pow(1.25f, static_cast<float>(sensibilidade - 5));
}

}
