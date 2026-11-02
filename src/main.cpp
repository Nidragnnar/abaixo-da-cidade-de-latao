#include <cstdint>
#include <exception>
#include <format>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include <SDL.h>
#include <glad/glad.h>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>

#include "jogo/IA.h"
#include "jogo/Partida.h"
#include "jogo/PassoFixo.h"
#include "jogo/Placar.h"
#include "mundo/Gerador.h"
#include "plataforma/Caminhos.h"
#include "plataforma/Entrada.h"
#include "plataforma/Janela.h"
#include "plataforma/Log.h"
#include "render/AnimacaoInimigos.h"
#include "render/Automapa.h"
#include "render/Camera.h"
#include "render/Comportas.h"
#include "render/Desenho2D.h"
#include "render/GeometriaMasmorra.h"
#include "render/Hud.h"
#include "render/MalhaGPU.h"
#include "render/Shader.h"
#include "render/Sprites.h"
#include "render/TelaBaixa.h"
#include "render/Texturas.h"
#include "render/Zonas.h"

namespace {

// Opções de linha de comando para testes
struct Opcoes {
    std::string captura;                 // --captura <arquivo.bmp>: salva um quadro e fecha
    int quadroCaptura = 30;              // --quadro <n>: qual quadro a captura salva (a lógica roda sozinha até lá)
    float anguloGraus = 0.0f;            // --angulo <graus>: direção inicial do jogador
    float inclinacaoGraus = 0.0f;        // --inclinacao <graus>: olhar vertical inicial (+ para cima)
    std::optional<uint32_t> semente;     // --semente <n>: sem ela, a partida é aleatória
    int andar = 1;                       // --andar <n>: andar inicial
    bool mapaCompleto = false;           // --mapa-completo: revela o mapa e abre o automapa
    bool descrever = false;              // --descrever: grava o andar no log e sai sem janela
    std::optional<glm::vec2> posicao;    // --posicao <x> <z>: começa nesse ponto (testes)
    bool invencivel = false;             // --invencivel: o dano não reduz a vida (observar a IA)
    std::string dados;                   // --dados <pasta>: placar fora do %APPDATA% (testes)
};

Opcoes lerOpcoes(int argc, char** argv) {
    Opcoes o;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const bool temValor = i + 1 < argc;
        if (arg == "--captura" && temValor) o.captura = argv[++i];
        else if (arg == "--quadro" && temValor) o.quadroCaptura = std::max(1, std::stoi(argv[++i]));
        else if (arg == "--angulo" && temValor) o.anguloGraus = std::stof(argv[++i]);
        else if (arg == "--inclinacao" && temValor) o.inclinacaoGraus = std::stof(argv[++i]);
        else if (arg == "--semente" && temValor) o.semente = static_cast<uint32_t>(std::stoul(argv[++i]));
        else if (arg == "--andar" && temValor) o.andar = std::max(1, std::stoi(argv[++i]));
        else if (arg == "--mapa-completo") o.mapaCompleto = true;
        else if (arg == "--descrever") o.descrever = true;
        else if (arg == "--invencivel") o.invencivel = true;
        else if (arg == "--dados" && temValor) o.dados = argv[++i];
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
    for (const jogo::Inimigo& i : jogo::gerarInimigos(a, semente))
        plataforma::logar(std::format("  {} em ({:.1f}, {:.1f})", jogo::dados(i.tipo).nome, i.posicao.x, i.posicao.y));
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

std::vector<std::filesystem::path> arquivosDeSprites() {
    // Mesma ordem de render::CamadaSprite + quadros dos inimigos (AnimacaoInimigos.h)
    std::vector<std::filesystem::path> a{plataforma::caminhoAsset("sprites/valvula_chave.png"),
                                         plataforma::caminhoAsset("sprites/elevador.png"),
                                         plataforma::caminhoAsset("sprites/projetil.png")};
    for (const char* inimigo : {"rato", "automato", "atirador", "colosso"})
        for (int q = 0; q < render::QUADROS_POR_INIMIGO; ++q)
            a.push_back(plataforma::caminhoAsset(std::format("sprites/{}_{}.png", inimigo, q)));
    return a;
}

std::vector<std::filesystem::path> arquivosDeArmas() {
    // Mesma ordem de render::QuadroArma
    return {plataforma::caminhoAsset("armas/chave_0.png"), plataforma::caminhoAsset("armas/chave_1.png"),
            plataforma::caminhoAsset("armas/revolver_0.png"), plataforma::caminhoAsset("armas/revolver_1.png"),
            plataforma::caminhoAsset("armas/revolver_2.png")};
}

// Grava o resultado no placar; devolve a colocação (0 se ficou de fora ou não deu para gravar)
int registrarNoPlacar(const jogo::Partida& p) {
    const std::filesystem::path arquivo = plataforma::pastaDeDados() / "placar.txt";
    const jogo::LeituraPlacar leitura = jogo::lerPlacar(arquivo);
    if (leitura.erroDeLeitura) {
        plataforma::logar("Não foi possível ler o placar (arquivo em uso?); o resultado não foi gravado: " +
                          arquivo.string());
        return 0;
    }
    if (leitura.estavaCorrompido) plataforma::logar("placar.txt corrompido: renomeado para placar.txt.corrompido");
    int posicao = 0;
    const auto novo = jogo::inserirNoPlacar(
        leitura.registros,
        {p.pontuacao(), p.andar().numero, p.nivel(), p.inimigosDestruidos(), p.engrenagens(), p.semente(),
         std::string(p.causaDaMorte())},
        &posicao);
    if (!jogo::gravarPlacar(arquivo, novo)) {
        plataforma::logar("Não foi possível gravar o placar em " + arquivo.string());
        return 0;
    }
    return posicao;
}

}

int main(int argc, char** argv) {
    plataforma::iniciarLog(plataforma::pastaDoExecutavel() / "log.txt");
    try {
        const Opcoes opcoes = lerOpcoes(argc, argv);
        if (!opcoes.dados.empty()) plataforma::definirPastaDeDados(opcoes.dados);
        const uint32_t sementeInicial = opcoes.semente.value_or(std::random_device{}());
        if (opcoes.descrever) {
            descreverAndar(sementeInicial, opcoes.andar);
            return 0;
        }

        plataforma::Janela janela("Roguelike", 1280, 720);
        plataforma::Entrada entrada(janela.sdl());
        const bool modoCaptura = !opcoes.captura.empty();

        // Desenho
        render::TelaBaixa telaBaixa;
        render::Desenho2D desenho2D;
        render::Desenho2DTextura desenhoTextura;
        const int telaL = telaBaixa.largura(), telaA = telaBaixa.altura();
        const GLuint programa = render::compilarPrograma(plataforma::caminhoAsset("shaders/masmorra.vert"),
                                                         plataforma::caminhoAsset("shaders/masmorra.frag"));
        const GLuint texturas = render::carregarArrayTexturas(arquivosDeTextura());
        const GLuint texturasSprites = render::carregarArrayTexturas(arquivosDeSprites());
        const GLuint texturasArmas = render::carregarArrayTexturas(arquivosDeArmas(), 128);
        const GLuint fonte = render::carregarFonte(plataforma::caminhoAsset("fonte/fonte.png"));
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
        const GLint locClarao = glGetUniformLocation(programa, "uClarao");
        // A projeção depende só da imagem pequena, que não muda de tamanho
        glUniformMatrix4fv(locProjecao, 1, GL_FALSE,
                           glm::value_ptr(render::matrizProjecao(static_cast<float>(telaL) / telaA)));
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        // Estado da partida em andamento (recriado com Enter na tela de fim)
        std::unique_ptr<jogo::Partida> partida;
        jogo::Jogador anterior{};  // estado do passo anterior, para interpolar
        jogo::PassoFixo passo;
        int andarPreparado = 0;
        bool placarGravado = false;
        int posicaoPlacar = 0;

        // Remonta o que depende do andar (malha fixa e cor da zona)
        auto prepararAndar = [&] {
            malha.enviar(render::construirGeometria(partida->andar().grade, partida->andar().zona));
            const glm::vec3 ambiente = render::corAmbiente(partida->andar().zona);
            glUseProgram(programa);
            glUniform3f(locAmbiente, ambiente.r, ambiente.g, ambiente.b);
            andarPreparado = partida->trocasDeAndar();
            anterior = partida->jogador();
        };
        auto novaPartida = [&](uint32_t semente, int andar) {
            plataforma::logar(std::format("Partida com semente {} (andar inicial {})", semente, andar));
            partida = std::make_unique<jogo::Partida>(semente, andar);
            if (opcoes.invencivel) partida->tornarInvencivel();
            if (opcoes.mapaCompleto) partida->revelarMapa();
            placarGravado = false;
            posicaoPlacar = 0;
            passo.zerar();
            prepararAndar();
        };
        novaPartida(sementeInicial, opcoes.andar);
        partida->definirAngulo(glm::radians(opcoes.anguloGraus));
        partida->definirInclinacao(glm::radians(opcoes.inclinacaoGraus));
        if (opcoes.posicao) partida->definirPosicao(*opcoes.posicao);
        anterior = partida->jogador();

        std::string tituloAtual;
        int quadro = 0;
        float relogio = 0.0f;
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
            double dt = static_cast<double>(agora - ultimo) / SDL_GetPerformanceFrequency();
            ultimo = agora;
            if (modoCaptura) dt = passo.passo();  // captura: um passo por quadro, sem depender do relógio
            relogio += static_cast<float>(dt);

            // Na captura a lógica roda sozinha (sem comandos) para os inimigos agirem
            if (entrada.capturado() || modoCaptura) {
                const int passos = passo.adicionar(dt);
                for (int i = 0; i < passos; ++i) {
                    anterior = partida->jogador();
                    // Giro e eventos vão no primeiro passo; coletar() já os zera para os seguintes
                    partida->atualizar(modoCaptura ? jogo::Comandos{} : entrada.coletar(),
                                       static_cast<float>(passo.passo()));
                }
            } else {
                passo.zerar();
                entrada.descartarGiro();
                anterior = partida->jogador();
            }

            // Morte: grava o placar uma vez e solta o mouse; Enter começa outra partida
            if (partida->morto() && !placarGravado) {
                // Capturas de teste só gravam placar numa pasta indicada com --dados (nunca no %APPDATA% do jogador)
                if (!modoCaptura || !opcoes.dados.empty()) posicaoPlacar = registrarNoPlacar(*partida);
                placarGravado = true;
                entrada.soltarMouse();
            }
            // Na captura o teclado é ignorado (a janela rouba o foco e pode receber teclas digitadas em outro lugar)
            if (entrada.pediuNovaPartida() && partida->morto() && !modoCaptura) novaPartida(std::random_device{}(), 1);

            // Andar novo: remonta a malha e não interpola a partir do andar anterior
            if (partida->trocasDeAndar() != andarPreparado) prepararAndar();

            const float alfa = static_cast<float>(passo.alfa());
            const glm::vec2 pos = glm::mix(anterior.posicao, partida->jogador().posicao, alfa);
            const float ang = glm::mix(anterior.angulo, partida->jogador().angulo, alfa);
            const float incl = glm::mix(anterior.inclinacao, partida->jogador().inclinacao, alfa);

            // Título só muda quando o texto muda
            const std::string titulo =
                entrada.capturado() || partida->morto() ? "Roguelike" : "Roguelike  (clique para jogar, Esc solta o mouse)";
            if (titulo != tituloAtual) {
                SDL_SetWindowTitle(janela.sdl(), titulo.c_str());
                tituloAtual = titulo;
            }

            // Cena na imagem pequena
            telaBaixa.comecar();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(programa);
            glUniformMatrix4fv(locVisao, 1, GL_FALSE, glm::value_ptr(render::matrizVisao(pos, ang, incl)));
            glUniform3f(locLanterna, pos.x, render::ALTURA_OLHOS, pos.y);
            glUniform1f(locClarao, partida->clarao() > 0.0f ? 1.0f : 0.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_ARRAY, texturas);
            malha.desenhar();
            malhaComportas.enviar(render::construirComportas(partida->andar().grade, [&](int x, int y) {
                return partida->aberturaComporta(x, y);
            }), true);
            malhaComportas.desenhar();

            // Sprites: elevador, válvula, inimigos e projéteis
            std::vector<render::Sprite> sprites;
            sprites.push_back({glm::vec2(partida->andar().elevador) + 0.5f, render::SpriteElevador, 0.9f, 1.0f});
            if (partida->andar().chave)
                sprites.push_back({*partida->andar().chave, render::SpriteValvulaChave, 0.4f, 0.4f});
            for (const jogo::Inimigo& i : partida->inimigos()) {
                const glm::vec2 tamanho = render::tamanhoDoInimigo(i.tipo);
                sprites.push_back({i.posicao, render::camadaDoInimigo(i.tipo, i.estado, i.tempoNoEstado), tamanho.x,
                                   tamanho.y});
            }
            for (const jogo::Projetil& t : partida->projeteis())
                sprites.push_back({t.posicao, render::SpriteProjetil, 0.15f, 0.15f, 0.42f});
            glBindTexture(GL_TEXTURE_2D_ARRAY, texturasSprites);
            malhaSprites.enviar(render::construirSprites(sprites, pos), true);
            malhaSprites.desenhar();

            // HUD (arma na mão por baixo, depois barras/flash e texto)
            const jogo::Armas& armas = partida->armas();
            const render::ElementosHud hud = render::construirHud(
                {partida->vida(), partida->vidaMaxima(), armas.ativa, armas.carga, armas.reserva, armas.recarga > 0,
                 armas.animacao, partida->tempoDesdeDano(), partida->mensagem(), partida->andar().numero, relogio},
                telaL, telaA);
            if (!partida->morto()) desenhoTextura.desenhar(hud.arma, texturasArmas, telaL, telaA);
            desenho2D.desenhar(hud.cores, telaL, telaA);
            desenhoTextura.desenhar(hud.texto, fonte, telaL, telaA);

            if (entrada.mapaVisivel() || opcoes.mapaCompleto)
                desenho2D.desenhar(render::construirAutomapa(partida->andar().grade, partida->exploracao(), pos, ang,
                                                             partida->andar().chave, telaL, telaA),
                                   telaL, telaA);

            if (partida->morto()) {
                const render::ElementosHud fim = render::construirTelaFim(
                    {partida->andar().numero, partida->nivel(), partida->inimigosDestruidos(), partida->engrenagens(),
                     partida->pontuacao(), partida->semente(), partida->causaDaMorte(), posicaoPlacar},
                    telaL, telaA);
                desenho2D.desenhar(fim.cores, telaL, telaA);
                desenhoTextura.desenhar(fim.texto, fonte, telaL, telaA);
            }

            // Ampliada na janela
            const glm::ivec2 tela = janela.tamanhoDesenhavel();
            telaBaixa.apresentar(tela);
            if (modoCaptura && ++quadro == opcoes.quadroCaptura) {
                salvarCaptura(tela, opcoes.captura);
                break;
            }
            janela.trocarBuffers();
        }
        glDeleteTextures(1, &fonte);
        glDeleteTextures(1, &texturasArmas);
        glDeleteTextures(1, &texturasSprites);
        glDeleteTextures(1, &texturas);
        glDeleteProgram(programa);
    } catch (const std::exception& erro) {
        plataforma::mostrarErroFatal(erro.what());
        return 1;
    }
    return 0;
}
