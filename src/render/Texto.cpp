#include "render/Texto.h"

namespace render {

namespace {

// Decodifica UTF-8; sequência inválida ou cortada vira '?'
std::u32string decodificar(std::string_view s) {
    std::u32string r;
    for (size_t i = 0; i < s.size();) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        int extras;  // bytes de continuação
        char32_t cp;
        if (c < 0x80) extras = 0, cp = c;
        else if ((c >> 5) == 0x6) extras = 1, cp = c & 0x1F;
        else if ((c >> 4) == 0xE) extras = 2, cp = c & 0x0F;
        else if ((c >> 3) == 0x1E) extras = 3, cp = c & 0x07;
        else extras = -1, cp = 0;
        bool valido = extras >= 0 && i + extras < s.size();
        for (int k = 1; valido && k <= extras; ++k) {
            const unsigned char cont = static_cast<unsigned char>(s[i + k]);
            if ((cont >> 6) != 0x2) valido = false;
            else cp = (cp << 6) | (cont & 0x3F);
        }
        if (!valido) {
            r.push_back(U'?');
            ++i;
            continue;
        }
        r.push_back(cp);
        i += extras + 1;
    }
    return r;
}

}

const std::u32string& caracteresFonte() {
    static const std::u32string lista = [] {
        std::u32string s;
        for (char32_t c = 32; c <= 126; ++c) s.push_back(c);
        s += U"ÁÂÃÀÇÉÊÍÓÔÕÚáâãàçéêíóôõú—º–×";
        return s;
    }();
    return lista;
}

int indiceGlifo(char32_t c) {
    const auto& lista = caracteresFonte();
    const size_t i = lista.find(c);
    return static_cast<int>(i != std::u32string::npos ? i : lista.find(U'?'));
}

std::vector<VerticeTex> construirTexto(std::string_view utf8, glm::vec2 posicao, float escala, glm::vec4 cor) {
    std::vector<VerticeTex> v;
    const float lado = LADO_GLIFO * escala;
    glm::vec2 p = posicao;
    for (char32_t c : decodificar(utf8)) {
        adicionarQuadro(v, p, p + lado, static_cast<float>(indiceGlifo(c)), cor);
        p.x += lado;
    }
    return v;
}

float larguraTexto(std::string_view utf8, float escala) {
    return static_cast<float>(decodificar(utf8).size()) * LADO_GLIFO * escala;
}

}
