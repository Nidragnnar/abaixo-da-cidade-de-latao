#include <cstdint>
#include <exception>
#include <format>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include <SDL.h>
#include <glad/glad.h>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>

#include "jogo/Partida.h"
#include "jogo/PassoFixo.h"
#include "mundo/Gerador.h"
#include "plataforma/Caminhos.h"
#include "plataforma/Entrada.h"
#include "plataforma/Janela.h"
#include "plataforma/Log.h"
#include "render/Automapa.h"
#include "render/Camera.h"
#include "render/Comportas.h"
#include "render/Desenho2D.h"
#include "render/GeometriaMasmorra.h"
#include "render/MalhaGPU.h"
#include "render/Shader.h"
#include "render/Sprites.h"
#include "render/TelaBaixa.h"
#include "render/Texturas.h"
#include "render/Zonas.h"

namespace {

// Opções de linha de comando para testes
struct Opcoes {
    std::string captura;                 // --captura <arquivo.bmp>: salva o quadro 30 e fecha
    float anguloGraus = 0.0f;            // --angulo <graus>: direção inicial do jogador
    std::optional<uint32_t> semente;     // --semente <n>: sem ela, a partida é aleatória
    int andar = 1;                       // --andar <n>: andar inicial
    bool mapaCompleto = false;           // --mapa-completo: revela o mapa e abre o automapa
    bool descrever = false;              // --descrever: grava o andar no log e sai sem janela
    std::optional<glm::vec2> posicao;    // --posicao <x> <z>: começa nesse ponto (testes)
};

Opcoes lerOpcoes(int argc, char** argv) {
    Opcoes o;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const bool temValor = i + 1 < argc;
        if (arg == "--captura" && temValor) o.captura = argv[++i];
        else if (arg == "--angulo" && temValor) o.anguloGraus = std::stof(argv[++i]);
        else if (arg == "--semente" && temValor) o.semente = static_cast<uint32_t>(std::stoul(argv[++i]));
        else if (arg == "--andar" && temValor) o.andar = std::max(1, std::stoi(argv[++i]));
        else if (arg == "--mapa-completo") o.mapaCompleto = true;
        else if (arg == "--descrever") o.descrever = true;
        else if (arg == "--posicao" && i + 2 < argc) {
            const float x = std::stof(argv[++i]);
            o.posicao = glm::vec2(x, std::stof(argv[++i]));
        }
    }
    return o;
}

const char* nomeZona(mundo::Zona z) {
    switch (z) {
        case mundo::Zona::Fabrica: return "fábrica";
        case mundo::Zona::Minas: return "minas";
        default: return "esgoto";
    }
}

// Uma linha com o resumo do andar (usada para montar os cenários de teste)
void descreverAndar(uint32_t semente, int numero) {
    const mundo::Andar a = mundo::gerarAndar(semente, numero);
    std::string chave = "-";
    if (a.chave) chave = std::format("({:.1f}, {:.1f})", a.chave->x, a.chave->y);
    plataforma::logar(std::format("semente {} andar {}: zona {}, {} salas, tranca {}, chave {}, elevador ({}, {})",
                                  semente, numero, nomeZona(a.zona), a.salas.size(), a.chave ? "sim" : "não", chave,
                                  a.elevador.x, a.elevador.y));
}

// Lê a janela inteira e grava como BMP
void salvarCaptura(glm::ivec2 tamanho, const std::string& arquivo) {
    std::vector<unsigned char> px(static_cast<size_t>(tamanho.x) * tamanho.y * 4);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, tamanho.x, tamanho.y, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    // O OpenGL lê de baixo para cima; o BMP espera de cima para baixo
    std::vector<unsigned char> invertida(px.size());
    const size_t linha = static_cast<size_t>(tamanho.x) * 4;
    for (int y = 0; y < tamanho.y; ++y)
        std::copy_n(&px[(tamanho.y - 1 - y) * linha], linha, &invertida[y * linha]);
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormatFrom(invertida.data(), tamanho.x, tamanho.y, 32,
                                                        static_cast<int>(linha), SDL_PIXELFORMAT_RGBA32);
    if (s && SDL_SaveBMP(s, arquivo.c_str()) == 0) plataforma::logar("Captura salva: " + arquivo);
    else plataforma::logar(std::format("Falha ao salvar captura {}: {}", arquivo, SDL_GetError()));
    SDL_FreeSurface(s);
}

std::vector<std::filesystem::path> arquivosDeTextura() {
    // Mesma ordem de render::CamadaTextura
    std::vector<std::filesystem::path> a;
    for (const char* zona : {"esgoto", "fabrica", "minas"})
        for (const char* parte : {"parede", "chao", "teto"})
            a.push_back(plataforma::caminhoAsset(std::format("texturas/{}_{}.png", zona, parte)));
    a.push_back(plataforma::caminhoAsset("texturas/comporta.png"));
    a.push_back(plataforma::caminhoAsset("texturas/comporta_trancada.png"));
    return a;
}

}

int main(int argc, char** argv) {
    plataforma::iniciarLog(plataforma::pastaDoExecutavel() / "log.txt");
    try {
        const Opcoes opcoes = lerOpcoes(argc, argv);
        const uint32_t semente = opcoes.semente.value_or(std::random_device{}());
        if (opcoes.descrever) {
            descreverAndar(semente, opcoes.andar);
            return 0;
        }
        plataforma::logar(std::format("Partida com semente {} (andar inicial {})", semente, opcoes.andar));

        plataforma::Janela janela("Roguelike", 1280, 720);
        plataforma::Entrada entrada(janela.sdl());

        jogo::Partida partida(semente, opcoes.andar);
        partida.definirAngulo(glm::radians(opcoes.anguloGraus));
        if (opcoes.posicao) partida.definirPosicao(*opcoes.posicao);
        if (opcoes.mapaCompleto) partida.revelarMapa();
        jogo::Jogador anterior = partida.jogador();  // estado do passo anterior, para interpolar
        jogo::PassoFixo passo;

        // Desenho
        render::TelaBaixa telaBaixa;
        render::Desenho2D desenho2D;
        const GLuint programa = render::compilarPrograma(plataforma::caminhoAsset("shaders/masmorra.vert"),
                                                         plataforma::caminhoAsset("shaders/masmorra.frag"));
        const GLuint texturas = render::carregarArrayTexturas(arquivosDeTextura());
        const GLuint texturasSprites = render::carregarArrayTexturas(
            {plataforma::caminhoAsset("sprites/valvula_chave.png"), plataforma::caminhoAsset("sprites/elevador.png")});
        render::MalhaGPU malha, malhaComportas, malhaSprites;

        glUseProgram(programa);
        glUniform1i(glGetUniformLocation(programa, "uTexturas"), 0);
        glUniform3f(glGetUniformLocation(programa, "uCorLanterna"), 1.0f, 0.75f, 0.45f);
        glUniform1f(glGetUniformLocation(programa, "uAlcanceLanterna"), 6.0f);
        glUniform1f(glGetUniformLocation(programa, "uFimNeblina"), 12.0f);
        const GLint locVisao = glGetUniformLocation(programa, "uVisao");
        const GLint locProjecao = glGetUniformLocation(programa, "uProjecao");
        const GLint locLanterna = glGetUniformLocation(programa, "uPosLanterna");
        const GLint locAmbiente = glGetUniformLocation(programa, "uCorAmbiente");
        // A projeção depende só da imagem pequena, que não muda de tamanho
        glUniformMatrix4fv(locProjecao, 1, GL_FALSE,
                           glm::value_ptr(render::matrizProjecao(static_cast<float>(telaBaixa.largura()) /
                                                                 telaBaixa.altura())));
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        // Remonta o que depende do andar (malha fixa e cor da zona)
        auto prepararAndar = [&] {
            malha.enviar(render::construirGeometria(partida.andar().grade, partida.andar().zona));
            const glm::vec3 ambiente = render::corAmbiente(partida.andar().zona);
            glUseProgram(programa);
            glUniform3f(locAmbiente, ambiente.r, ambiente.g, ambiente.b);
        };
        prepararAndar();
        int andarPreparado = partida.trocasDeAndar();

        std::string tituloAtual;
        int quadro = 0;
        Uint64 ultimo = SDL_GetPerformanceCounter();
        while (!entrada.pediuSair()) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) entrada.processarEvento(e);

            // Minimizado: alguns drivers não seguram o V-Sync e o laço giraria a 100% de CPU
            if (SDL_GetWindowFlags(janela.sdl()) & SDL_WINDOW_MINIMIZED) {
                if (SDL_WaitEventTimeout(&e, 100)) entrada.processarEvento(e);
                ultimo = SDL_GetPerformanceCounter();
                continue;
            }

            const Uint64 agora = SDL_GetPerformanceCounter();
            const double dt = static_cast<double>(agora - ultimo) / SDL_GetPerformanceFrequency();
            ultimo = agora;

            if (entrada.capturado()) {
                const int passos = passo.adicionar(dt);
                for (int i = 0; i < passos; ++i) {
                    anterior = partida.jogador();
                    // Giro e E vão no primeiro passo; coletar() já os zera para os seguintes
                    partida.atualizar(entrada.coletar(), static_cast<float>(passo.passo()));
                }
            } else {
                passo.zerar();
                entrada.descartarGiro();
                anterior = partida.jogador();
            }

            // Andar novo: remonta a malha e não interpola a partir do andar anterior
            if (partida.trocasDeAndar() != andarPreparado) {
                andarPreparado = partida.trocasDeAndar();
                prepararAndar();
                anterior = partida.jogador();
            }

            const float alfa = static_cast<float>(passo.alfa());
            const glm::vec2 pos = glm::mix(anterior.posicao, partida.jogador().posicao, alfa);
            const float ang = glm::mix(anterior.angulo, partida.jogador().angulo, alfa);

            // Título só muda quando o texto muda
            std::string titulo = std::format("Roguelike — Andar {}", partida.andar().numero);
            if (!partida.mensagem().empty()) titulo += std::format("  |  {}", partida.mensagem());
            if (!entrada.capturado()) titulo += "  (clique para jogar, Esc solta o mouse)";
            if (titulo != tituloAtual) {
                SDL_SetWindowTitle(janela.sdl(), titulo.c_str());
                tituloAtual = std::move(titulo);
            }

            // Cena na imagem pequena
            telaBaixa.comecar();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(programa);
            glUniformMatrix4fv(locVisao, 1, GL_FALSE, glm::value_ptr(render::matrizVisao(pos, ang)));
            glUniform3f(locLanterna, pos.x, render::ALTURA_OLHOS, pos.y);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_ARRAY, texturas);
            malha.desenhar();
            malhaComportas.enviar(render::construirComportas(partida.andar().grade, [&](int x, int y) {
                return partida.aberturaComporta(x, y);
            }), true);
            malhaComportas.desenhar();

            std::vector<render::Sprite> sprites;
            const glm::ivec2 elevador = partida.andar().elevador;
            sprites.push_back({glm::vec2(elevador) + 0.5f, render::SpriteElevador, 0.9f, 1.0f});
            if (partida.andar().chave) sprites.push_back({*partida.andar().chave, render::SpriteValvulaChave, 0.4f, 0.4f});
            glBindTexture(GL_TEXTURE_2D_ARRAY, texturasSprites);
            malhaSprites.enviar(render::construirSprites(sprites, pos), true);
            malhaSprites.desenhar();

            if (entrada.mapaVisivel() || opcoes.mapaCompleto)
                desenho2D.desenhar(render::construirAutomapa(partida.andar().grade, partida.exploracao(), pos, ang,
                                                             partida.andar().chave, telaBaixa.largura(),
                                                             telaBaixa.altura()),
                                   telaBaixa.largura(), telaBaixa.altura());

            // Ampliada na janela
            const glm::ivec2 tela = janela.tamanhoDesenhavel();
            telaBaixa.apresentar(tela);
            if (!opcoes.captura.empty() && ++quadro == 30) {
                salvarCaptura(tela, opcoes.captura);
                break;
            }
            janela.trocarBuffers();
        }
        glDeleteTextures(1, &texturasSprites);
        glDeleteTextures(1, &texturas);
        glDeleteProgram(programa);
    } catch (const std::exception& erro) {
        plataforma::mostrarErroFatal(erro.what());
        return 1;
    }
    return 0;
}
