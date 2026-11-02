#include <exception>
#include <format>
#include <string>
#include <vector>

#include <SDL.h>
#include <glad/glad.h>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>

#include "jogo/Jogador.h"
#include "jogo/PassoFixo.h"
#include "mundo/Grade.h"
#include "mundo/MapaFixo.h"
#include "plataforma/Caminhos.h"
#include "plataforma/Entrada.h"
#include "plataforma/Janela.h"
#include "plataforma/Log.h"
#include "render/Camera.h"
#include "render/GeometriaMasmorra.h"
#include "render/MalhaGPU.h"
#include "render/Shader.h"
#include "render/Texturas.h"
#include "render/TelaBaixa.h"

namespace {

// Opções de linha de comando para testes
struct Opcoes {
    std::string captura;  // --captura <arquivo.bmp>: salva o quadro 30 e fecha
    float anguloGraus = 0.0f;  // --angulo <graus>: direção inicial do jogador
};

Opcoes lerOpcoes(int argc, char** argv) {
    Opcoes o;
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--captura") o.captura = argv[++i];
        else if (arg == "--angulo") o.anguloGraus = std::stof(argv[++i]);
    }
    return o;
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

}

int main(int argc, char** argv) {
    plataforma::iniciarLog(plataforma::pastaDoExecutavel() / "log.txt");
    try {
        const Opcoes opcoes = lerOpcoes(argc, argv);
        plataforma::Janela janela("Roguelike", 1280, 720);
        plataforma::Entrada entrada(janela.sdl());

        auto mapa = mundo::carregarDeTexto(mundo::mapaFixoEtapa1());
        jogo::Jogador jogador{mapa.inicio, glm::radians(opcoes.anguloGraus)};
        jogo::Jogador anterior = jogador;  // estado do passo anterior, para interpolar
        jogo::PassoFixo passo;

        // Desenho da masmorra
        render::TelaBaixa telaBaixa;
        const GLuint programa = render::compilarPrograma(plataforma::caminhoAsset("shaders/masmorra.vert"),
                                                         plataforma::caminhoAsset("shaders/masmorra.frag"));
        const GLuint texturas = render::carregarArrayTexturas({plataforma::caminhoAsset("texturas/esgoto_parede.png"),
                                                               plataforma::caminhoAsset("texturas/esgoto_chao.png"),
                                                               plataforma::caminhoAsset("texturas/esgoto_teto.png")});
        render::MalhaGPU malha;
        malha.enviar(render::construirGeometria(mapa.grade));

        glUseProgram(programa);
        glUniform1i(glGetUniformLocation(programa, "uTexturas"), 0);
        glUniform3f(glGetUniformLocation(programa, "uCorLanterna"), 1.0f, 0.75f, 0.45f);
        glUniform1f(glGetUniformLocation(programa, "uAlcanceLanterna"), 6.0f);
        glUniform3f(glGetUniformLocation(programa, "uCorAmbiente"), 0.10f, 0.09f, 0.06f);  // esgoto
        glUniform1f(glGetUniformLocation(programa, "uFimNeblina"), 12.0f);
        const GLint locVisao = glGetUniformLocation(programa, "uVisao");
        const GLint locProjecao = glGetUniformLocation(programa, "uProjecao");
        const GLint locLanterna = glGetUniformLocation(programa, "uPosLanterna");
        // A projeção depende só da imagem pequena, que não muda de tamanho
        glUniformMatrix4fv(locProjecao, 1, GL_FALSE,
                           glm::value_ptr(render::matrizProjecao(static_cast<float>(telaBaixa.largura()) /
                                                                 telaBaixa.altura())));
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

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
                    anterior = jogador;
                    // O giro do mouse vai inteiro no primeiro passo; coletar() já o zera
                    jogo::atualizarJogador(jogador, entrada.coletar(), mapa.grade, static_cast<float>(passo.passo()));
                }
            } else {
                passo.zerar();
                entrada.descartarGiro();
                anterior = jogador;
            }

            const float alfa = static_cast<float>(passo.alfa());
            const glm::vec2 pos = glm::mix(anterior.posicao, jogador.posicao, alfa);
            const float ang = glm::mix(anterior.angulo, jogador.angulo, alfa);
            SDL_SetWindowTitle(janela.sdl(),
                               entrada.capturado() ? "Roguelike" : "Roguelike  (clique para jogar, Esc solta o mouse)");

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

            // Ampliada na janela
            const glm::ivec2 tela = janela.tamanhoDesenhavel();
            telaBaixa.apresentar(tela);
            if (!opcoes.captura.empty() && ++quadro == 30) {
                salvarCaptura(tela, opcoes.captura);
                break;
            }
            janela.trocarBuffers();
        }
        glDeleteTextures(1, &texturas);
        glDeleteProgram(programa);
    } catch (const std::exception& erro) {
        plataforma::mostrarErroFatal(erro.what());
        return 1;
    }
    return 0;
}
