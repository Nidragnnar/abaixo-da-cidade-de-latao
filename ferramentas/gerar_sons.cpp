// Gera os sons e a música provisórios do jogo (WAV 16 bits mono, 22.050 Hz) com semente fixa.
// Uso: gerar_sons <pasta>   (normalmente assets\sons)
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <numbers>
#include <random>
#include <string>
#include <vector>

namespace {

constexpr int TAXA = 22050;
constexpr float PI = std::numbers::pi_v<float>;

std::mt19937 rng(20261102u);
float ruido() { return std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng); }

using Amostras = std::vector<float>;

// Duração em segundos; f(t) devolve a amostra no tempo t
Amostras sintetizar(float duracao, const std::function<float(float)>& f) {
    Amostras a(static_cast<size_t>(duracao * TAXA));
    for (size_t i = 0; i < a.size(); ++i) a[i] = f(static_cast<float>(i) / TAXA);
    return a;
}

// Filtro passa-baixa de um polo (coeficiente 0–1: menor = mais abafado)
struct PassaBaixa {
    float y = 0;
    float operator()(float x, float k) { return y += k * (x - y); }
};

float seno(float freq, float t) { return std::sin(2.0f * PI * freq * t); }
float decai(float t, float rapidez) { return std::exp(-t * rapidez); }

// Sobe o pico até o alvo (sons gerados com volumes parecidos)
void normalizar(Amostras& a, float pico) {
    float maior = 0;
    for (float v : a) maior = std::max(maior, std::abs(v));
    if (maior > 0)
        for (float& v : a) v *= pico / maior;
}

void gravarWav(const std::filesystem::path& arquivo, const Amostras& a) {
    std::ofstream f(arquivo, std::ios::binary);
    const auto u32 = [&](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
    const auto u16 = [&](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };
    const uint32_t bytes = static_cast<uint32_t>(a.size() * 2);
    f.write("RIFF", 4);
    u32(36 + bytes);
    f.write("WAVEfmt ", 8);
    u32(16);
    u16(1);  // PCM
    u16(1);  // mono
    u32(TAXA);
    u32(TAXA * 2);
    u16(2);
    u16(16);
    f.write("data", 4);
    u32(bytes);
    for (float v : a) {
        const auto s = static_cast<int16_t>(std::lround(std::clamp(v, -1.0f, 1.0f) * 32767.0f));
        f.write(reinterpret_cast<const char*>(&s), 2);
    }
}

// Estalo curto de ruído (cliques de arma e de menu)
float clique(float t, float inicio, float freqTom) {
    if (t < inicio) return 0;
    const float u = t - inicio;
    return ruido() * decai(u, 300) * 0.6f + seno(freqTom, u) * decai(u, 120) * 0.5f;
}

// Disparo: estouro de ruído abafado + baque grave descendo
Amostras disparo(float duracao, float abafado, float rapidez, float grave, float peso) {
    PassaBaixa lp;
    return sintetizar(duracao, [=](float t) mutable {
        const float estouro = lp(ruido(), abafado) * decai(t, rapidez);
        const float baque = seno(grave * (1.0f - 0.4f * t / duracao), t) * decai(t, rapidez * 0.7f) * peso;
        return estouro + baque;
    });
}

struct Efeito {
    const char* nome;
    std::function<Amostras()> gerar;
};

}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "uso: gerar_sons <pasta>\n");
        return 1;
    }
    const std::filesystem::path pasta = argv[1];
    std::filesystem::create_directories(pasta);

    const std::vector<Efeito> efeitos = {
        {"passo_pedra", [] {
             PassaBaixa lp;
             return sintetizar(0.12f, [=](float t) mutable {
                 return lp(ruido(), 0.15f) * decai(t, 40) * 2.0f + seno(90, t) * decai(t, 30) * 0.5f;
             });
         }},
        {"passo_metal", [] {
             PassaBaixa lp;
             return sintetizar(0.2f, [=](float t) mutable {
                 return lp(ruido(), 0.3f) * decai(t, 35) + (seno(1250, t) + seno(1870, t)) * decai(t, 18) * 0.25f;
             });
         }},
        {"golpe_ar", [] {
             PassaBaixa lp;
             return sintetizar(0.22f, [=](float t) mutable {
                 const float u = t / 0.22f;
                 return lp(ruido(), 0.05f + 0.35f * u) * std::pow(std::sin(PI * u), 2.0f);
             });
         }},
        {"acerto", [] {
             return sintetizar(0.25f, [](float t) {
                 return (seno(320, t) + seno(845, t) * 0.6f + seno(1530, t) * 0.4f) * decai(t, 20) +
                        ruido() * decai(t, 80);
             });
         }},
        {"disparo_revolver", [] { return disparo(0.35f, 0.5f, 18, 70, 0.8f); }},
        {"disparo_bacamarte", [] { return disparo(0.65f, 0.3f, 8, 48, 1.2f); }},
        {"disparo_tesla", [] {
             return sintetizar(0.45f, [](float t) {
                 const float zumbido = std::sin(2 * PI * 110 * t + 3 * std::sin(2 * PI * 7 * t)) > 0 ? 0.4f : -0.4f;
                 const float estalo = ruido() > 0.93f ? ruido() : 0.0f;
                 return (zumbido + estalo) * decai(t, 6);
             });
         }},
        {"recarga", [] { return sintetizar(0.35f, [](float t) { return clique(t, 0.0f, 2200) + clique(t, 0.2f, 1700); }); }},
        {"arma_vazia", [] { return sintetizar(0.08f, [](float t) { return clique(t, 0.0f, 1800); }); }},
        {"dano", [] {
             PassaBaixa lp;
             return sintetizar(0.25f, [=](float t) mutable {
                 return seno(140 - 70 * t / 0.25f, t) * decai(t, 12) + lp(ruido(), 0.1f) * decai(t, 15);
             });
         }},
        {"inimigo_destruido", [] {
             PassaBaixa lp;
             return sintetizar(0.7f, [=](float t) mutable {
                 const float x = ruido();
                 const float chiado = (x - lp(x, 0.2f)) * decai(t, 4) * 0.6f;
                 const float tilintar = (seno(2400, t) * decai(t, 10) + seno(3100, std::max(0.0f, t - 0.15f)) *
                                                                         decai(std::max(0.0f, t - 0.15f), 10) *
                                                                         (t > 0.15f ? 1.0f : 0.0f)) * 0.3f;
                 return chiado + tilintar;
             });
         }},
        {"comporta", [] {
             PassaBaixa lp;
             return sintetizar(0.6f, [=](float t) mutable {
                 const float x = ruido();
                 return (x - lp(x, 0.25f)) * std::sin(PI * t / 0.6f);
             });
         }},
        {"pegar", [] {
             return sintetizar(0.18f, [](float t) {
                 return t < 0.09f ? seno(660, t) * decai(t, 15) : seno(990, t - 0.09f) * decai(t - 0.09f, 15);
             });
         }},
        {"nivel", [] {
             return sintetizar(0.55f, [](float t) {
                 const float notas[] = {523.25f, 659.25f, 783.99f};
                 const int n = std::min(2, static_cast<int>(t / 0.15f));
                 const float u = t - n * 0.15f;
                 return (seno(notas[n], u) + seno(notas[n] * 2, u) * 0.3f) * decai(u, n == 2 ? 5.0f : 10.0f);
             });
         }},
        {"elevador", [] {
             PassaBaixa lp;
             return sintetizar(1.0f, [=](float t) mutable {
                 const float freq = 55 + 6 * std::sin(2 * PI * 3 * t);
                 const float serra = 2.0f * std::fmod(freq * t, 1.0f) - 1.0f;
                 return (lp(serra, 0.2f) + ruido() * 0.1f) * std::sin(PI * t);
             });
         }},
        {"tiro_inimigo", [] { return disparo(0.3f, 0.2f, 20, 90, 0.6f); }},
        {"menu_mover", [] { return sintetizar(0.04f, [](float t) { return seno(1200, t) * decai(t, 150); }); }},
        {"menu_confirmar", [] {
             return sintetizar(0.15f, [](float t) {
                 return t < 0.07f ? seno(880, t) * decai(t, 30) : seno(1320, t - 0.07f) * decai(t - 0.07f, 25);
             });
         }},
    };
    // Não sobrescreve sons já trocados por pacotes (ferramentas\preparar_sons.ps1); apague o arquivo para gerar de novo
    const auto existe = [&](const std::string& nome) {
        return std::filesystem::exists(pasta / (nome + ".wav")) || std::filesystem::exists(pasta / (nome + ".ogg"));
    };
    for (const Efeito& e : efeitos) {
        if (existe(e.nome)) {
            std::printf("%s: já existe, mantido\n", e.nome);
            continue;
        }
        Amostras a = e.gerar();
        normalizar(a, 0.9f);
        gravarWav(pasta / (std::string(e.nome) + ".wav"), a);
        std::printf("%s.wav  %.2f s\n", e.nome, static_cast<double>(a.size()) / TAXA);
    }

    // Música ambiente industrial de 32 s: frequências inteiras fecham ciclos exatos, então o loop não tem emenda
    if (existe("musica")) {
        std::printf("musica: já existe, mantida\n");
    } else {
        PassaBaixa lpVapor;
        Amostras m = sintetizar(32.0f, [&](float t) {
            const float onda = 0.6f + 0.4f * std::sin(2 * PI * t / 16.0f);  // respira a cada 16 s
            float s = (seno(55, t) * 0.5f + seno(110, t) * 0.25f + seno(165, t) * 0.12f) * onda;
            const float desdeBatida = std::fmod(t, 2.0f);  // batida metálica a cada 2 s
            s += (seno(220, desdeBatida) + seno(587, desdeBatida) * 0.5f + seno(1043, desdeBatida) * 0.3f) *
                 decai(desdeBatida, 6) * 0.35f;
            const float desdeVapor = std::fmod(t + 3.0f, 8.0f);  // chiado de vapor a cada 8 s
            if (desdeVapor < 1.5f) {
                const float x = ruido();
                s += (x - lpVapor(x, 0.3f)) * std::sin(PI * desdeVapor / 1.5f) * 0.25f;
            }
            return s;
        });
        normalizar(m, 0.7f);
        gravarWav(pasta / "musica.wav", m);
        std::printf("musica.wav  %.2f s\n", static_cast<double>(m.size()) / TAXA);
    }
    return 0;
}
