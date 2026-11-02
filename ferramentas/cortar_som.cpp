// Prepara um som baixado de um pacote: corta um trecho, passa para mono, suaviza o começo e o fim e normaliza o pico.
// Uso: cortar_som analisar <entrada>
//      cortar_som <entrada> <saida.wav> <inicio_s> <fim_s> [pico 0–1, padrão 0,9] [saída suave em s, padrão 0,04]
// Entrada: WAV PCM de 8, 16 ou 24 bits, ou OGG (decodificado pelo SDL_mixer em 22.050 Hz mono).
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

struct Wav {
    int taxa = 0;
    std::vector<float> amostras;  // mono, −1…1
};

uint32_t u32(const std::vector<unsigned char>& b, size_t p) { return b[p] | b[p + 1] << 8 | b[p + 2] << 16 | uint32_t(b[p + 3]) << 24; }
uint16_t u16(const std::vector<unsigned char>& b, size_t p) { return static_cast<uint16_t>(b[p] | b[p + 1] << 8); }

// OGG pelo SDL_mixer (áudio de mentira: nada toca); o pedaço já vem em 22.050 Hz, 16 bits, mono
bool lerOgg(const char* arquivo, Wav& w) {
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_AUDIO) != 0 || Mix_OpenAudio(22050, AUDIO_S16SYS, 1, 1024) != 0) return false;
    Mix_Chunk* c = Mix_LoadWAV(arquivo);
    if (c) {
        w.taxa = 22050;
        const auto* s = reinterpret_cast<const int16_t*>(c->abuf);
        for (Uint32 i = 0; i < c->alen / 2; ++i) w.amostras.push_back(s[i] / 32768.0f);
        Mix_FreeChunk(c);
    }
    Mix_CloseAudio();
    SDL_Quit();
    return c != nullptr;
}

bool ler(const char* arquivo, Wav& w) {
    const std::string nome = arquivo;
    if (nome.size() > 4 && nome.substr(nome.size() - 4) == ".ogg") return lerOgg(arquivo, w);
    std::ifstream f(arquivo, std::ios::binary);
    const std::vector<unsigned char> b((std::istreambuf_iterator<char>(f)), {});
    if (b.size() < 44 || std::memcmp(b.data(), "RIFF", 4) || std::memcmp(b.data() + 8, "WAVE", 4)) return false;
    size_t p = 12, fmt = 0, dados = 0, tamanho = 0;
    while (p + 8 <= b.size()) {
        const uint32_t t = u32(b, p + 4);
        if (!std::memcmp(b.data() + p, "fmt ", 4)) fmt = p + 8;
        if (!std::memcmp(b.data() + p, "data", 4)) {
            dados = p + 8;
            tamanho = std::min<size_t>(t, b.size() - dados);
            break;
        }
        p += 8 + t + (t & 1);
    }
    if (!fmt || !dados || u16(b, fmt) != 1) return false;  // só PCM
    const int canais = u16(b, fmt + 2), bits = u16(b, fmt + 14);
    w.taxa = static_cast<int>(u32(b, fmt + 4));
    const size_t passo = static_cast<size_t>(canais) * bits / 8;
    for (size_t i = 0; i + passo <= tamanho; i += passo) {
        float soma = 0;
        for (int c = 0; c < canais; ++c) {
            const size_t o = dados + i + static_cast<size_t>(c) * bits / 8;
            if (bits == 16) soma += static_cast<int16_t>(u16(b, o)) / 32768.0f;
            else if (bits == 24) soma += static_cast<float>(static_cast<int32_t>(b[o] << 8 | b[o + 1] << 16 | uint32_t(b[o + 2]) << 24) >> 8) / 8388608.0f;
            else soma += (b[o] - 128) / 128.0f;
        }
        w.amostras.push_back(soma / static_cast<float>(canais));
    }
    return true;
}

void gravar(const char* arquivo, const Wav& w) {
    std::ofstream f(arquivo, std::ios::binary);
    const auto e32 = [&](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
    const auto e16 = [&](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };
    const uint32_t bytes = static_cast<uint32_t>(w.amostras.size() * 2);
    f.write("RIFF", 4);
    e32(36 + bytes);
    f.write("WAVEfmt ", 8);
    e32(16);
    e16(1);
    e16(1);
    e32(static_cast<uint32_t>(w.taxa));
    e32(static_cast<uint32_t>(w.taxa) * 2);
    e16(2);
    e16(16);
    f.write("data", 4);
    e32(bytes);
    for (float v : w.amostras) {
        const auto s = static_cast<int16_t>(std::lround(std::clamp(v, -1.0f, 1.0f) * 32767.0f));
        f.write(reinterpret_cast<const char*>(&s), 2);
    }
}

// Duração e onde começam os golpes (trechos de 50 ms acima de 30% do pico)
void analisar(const char* arquivo, const Wav& w) {
    const size_t janela = static_cast<size_t>(w.taxa / 20);
    std::vector<float> env;
    for (size_t i = 0; i < w.amostras.size(); i += janela) {
        float m = 0;
        for (size_t k = i; k < std::min(w.amostras.size(), i + janela); ++k) m = std::max(m, std::abs(w.amostras[k]));
        env.push_back(m);
    }
    const float pico = env.empty() ? 0 : *std::max_element(env.begin(), env.end());
    std::printf("%s: %d Hz, %.2f s, pico %.2f, golpes em", arquivo, w.taxa,
                static_cast<double>(w.amostras.size()) / w.taxa, static_cast<double>(pico));
    bool antes = false;
    for (size_t j = 0; j < env.size(); ++j) {
        const bool alto = env[j] > 0.3f * pico;
        if (alto && !antes) std::printf(" %.2f", j * 0.05);
        antes = alto;
    }
    std::printf("\n");
}

}

int main(int argc, char** argv) {
    Wav w;
    if (argc == 3 && std::string(argv[1]) == "analisar") {
        if (!ler(argv[2], w)) return std::fprintf(stderr, "não li %s\n", argv[2]), 1;
        analisar(argv[2], w);
        return 0;
    }
    if (argc < 5) return std::fprintf(stderr, "uso: cortar_som analisar <wav> | <entrada> <saida> <inicio_s> <fim_s> [pico]\n"), 1;
    if (!ler(argv[1], w)) return std::fprintf(stderr, "não li %s\n", argv[1]), 1;
    const size_t inicio = std::min(w.amostras.size(), static_cast<size_t>(std::atof(argv[3]) * w.taxa));
    const size_t fim = std::clamp(static_cast<size_t>(std::atof(argv[4]) * w.taxa), inicio, w.amostras.size());
    Wav saida{w.taxa, std::vector<float>(w.amostras.begin() + static_cast<std::ptrdiff_t>(inicio),
                                         w.amostras.begin() + static_cast<std::ptrdiff_t>(fim))};
    // 5 ms de entrada e (por padrão) 40 ms de saída evitam estalos no corte; saída longa = som que some aos poucos
    const double segundosSaida = argc > 6 ? std::atof(argv[6]) : 0.04;
    const size_t entrada = std::min(saida.amostras.size(), static_cast<size_t>(w.taxa / 200));
    const size_t saidaSuave = std::min(saida.amostras.size(), std::max<size_t>(1, static_cast<size_t>(segundosSaida * w.taxa)));
    for (size_t i = 0; i < entrada; ++i) saida.amostras[i] *= static_cast<float>(i) / entrada;
    for (size_t i = 0; i < saidaSuave; ++i) saida.amostras[saida.amostras.size() - 1 - i] *= static_cast<float>(i) / saidaSuave;
    float pico = 0;
    for (float v : saida.amostras) pico = std::max(pico, std::abs(v));
    const float alvo = argc > 5 ? static_cast<float>(std::atof(argv[5])) : 0.9f;
    if (pico > 0)
        for (float& v : saida.amostras) v *= alvo / pico;
    gravar(argv[2], saida);
    analisar(argv[2], saida);
    return 0;
}
