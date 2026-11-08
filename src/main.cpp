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

#include "jogo/Config.h"
#include "jogo/Demo.h"
#include "jogo/IA.h"
#include "jogo/Partida.h"
#include "jogo/PassoFixo.h"
#include "jogo/Placar.h"
#include "jogo/Salvamento.h"
#include "jogo/Sons.h"
#include "mundo/Gerador.h"
#include "mundo/Terreno.h"
#include "plataforma/Audio.h"
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
#include "render/Icones.h"
#include "render/Luzes.h"
#include "render/MalhaGPU.h"
#include "render/Shader.h"
#include "render/Sprites.h"
#include "render/TelaBaixa.h"
#include "render/TelaInventario.h"
#include "render/TelasMenu.h"
#include "render/Texturas.h"
#include "render/Zonas.h"
#include "ui/Menus.h"

namespace {

constexpr const char* NOME_DO_JOGO = "Abaixo da Cidade de Latão";
constexpr uint32_t SEMENTE_DO_FUNDO = 1;      // andar mostrado atrás do menu
constexpr float GIRO_DO_FUNDO = 0.15f;        // rad/s
constexpr int PASSOS_ENTRE_CAPTURAS_DEMO = 120;  // 2 s de jogo
constexpr float LIMITE_DEMO = 180.0f;         // s de jogo

// Opções de linha de comando para testes
struct Opcoes {
    std::string captura;                 // --captura <arquivo.bmp>: salva um quadro e fecha
    int quadroCaptura = 30;              // --quadro <n>: qual quadro a captura salva (a lógica roda sozinha até lá)
    float anguloGraus = 0.0f;            // --angulo <graus>: direção inicial do jogador
    float inclinacaoGraus = 0.0f;        // --inclinacao <graus>: olhar vertical inicial (+ para cima)
    std::optional<uint32_t> semente;     // --semente <n>: sem ela, a partida é aleatória
    int andar = 1;                       // --andar <n>: andar inicial
    bool pediuAndar = false;
    bool mapaCompleto = false;           // --mapa-completo: revela o mapa e abre o automapa
    bool descrever = false;              // --descrever: grava o andar no log e sai sem janela
    std::optional<glm::vec2> posicao;    // --posicao <x> <z>: começa nesse ponto (testes)
    bool invencivel = false;             // --invencivel: o dano não reduz a vida (observar a IA)
    bool inventario = false;             // --inventario: começa com o inventário aberto (capturas)
    std::string dados;                   // --dados <pasta>: placar e opções fora do %APPDATA% (testes)
    std::optional<uint32_t> demo;        // --demo <semente>: anda sozinho até o elevador tirando capturas
    bool mudo = false;                   // --mudo: não abre o áudio
    std::string tela;                    // --tela principal|opcoes|placar|pausa: abre essa tela (capturas)
    // --inimigo <letra> <x> <z> (repetível): põe mais um inimigo no andar inicial (letras dos mapas de teste)
    std::vector<std::pair<char, glm::vec2>> inimigos;
    std::optional<int> vidaChefe;        // --vida-chefe <n>: o chefe do andar inicial começa com essa vida (testes)
    bool gravarPartida = false;          // --gravar-partida: monta a partida pedida, grava partida.sav e sai (cenários)

    // Começa direto na partida, sem o menu (cenários de teste e roteiros)
    bool direto() const {
        return semente || pediuAndar || posicao || !captura.empty() || invencivel || inventario || demo;
    }
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
        else if (arg == "--andar" && temValor) {
            o.andar = std::max(1, std::stoi(argv[++i]));
            o.pediuAndar = true;
        }
        else if (arg == "--mapa-completo") o.mapaCompleto = true;
        else if (arg == "--descrever") o.descrever = true;
        else if (arg == "--invencivel") o.invencivel = true;
        else if (arg == "--inventario") o.inventario = true;
        else if (arg == "--dados" && temValor) o.dados = argv[++i];
        else if (arg == "--demo" && temValor) o.demo = static_cast<uint32_t>(std::stoul(argv[++i]));
        else if (arg == "--mudo") o.mudo = true;
        else if (arg == "--tela" && temValor) o.tela = argv[++i];
        else if (arg == "--vida-chefe" && temValor) o.vidaChefe = std::stoi(argv[++i]);
        else if (arg == "--gravar-partida") o.gravarPartida = true;
        else if (arg == "--inimigo" && i + 3 < argc) {
            const char letra = argv[++i][0];
            const float x = std::stof(argv[++i]);
            o.inimigos.push_back({letra, glm::vec2(x, std::stof(argv[++i]))});
        }
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
    // Relevo (Etapa 6A): caixas, rampas, chão mais alto e mais baixo, segredos
    int caixas = 0, rampas = 0, maisAlto = 0, maisBaixo = 99;
    for (int y = 0; y < a.grade.altura(); ++y)
        for (int x = 0; x < a.grade.largura(); ++x) {
            if (a.grade.em(x, y) == mundo::Celula::Parede) continue;
            caixas += a.grade.detalhe(x, y) == mundo::Detalhe::Caixa;
            rampas += a.grade.rampa(x, y) != mundo::Rampa::Nenhuma;
            maisAlto = std::max(maisAlto, a.grade.chao(x, y));
            maisBaixo = std::min(maisBaixo, a.grade.chao(x, y));
        }
    plataforma::logar(std::format("  relevo: chão de {} a {} quartos, {} caixas, {} rampas, {} segredos", maisBaixo, maisAlto,
                                  caixas, rampas, a.pontosSecretos.size()));
    for (const glm::vec2 p : a.pontosSecretos) plataforma::logar(std::format("  segredo em ({:.1f}, {:.1f})", p.x, p.y));
    for (int y = 0; y < a.grade.altura(); ++y)
        for (int x = 0; x < a.grade.largura(); ++x)
            if (a.grade.rampa(x, y) != mundo::Rampa::Nenhuma)
                plataforma::logar(std::format("  rampa em ({}, {}), chão {} quartos", x, y, a.grade.chao(x, y)));
    for (size_t i = 0; i < a.salas.size(); ++i) {
        const mundo::Sala& s = a.salas[i];
        plataforma::logar(std::format("  sala {}: ({}, {}) {}×{}, chão {} no centro", i, s.x, s.y, s.largura, s.altura,
                                      a.grade.chao(s.centro().x, s.centro().y)));
    }
    if (a.arena) {
        const mundo::Sala& s = a.salas[*a.arena];
        for (const glm::ivec2 e : mundo::entradasDaSala(a.grade, s))
            plataforma::logar(std::format("  arena: sala {}, entrada em ({}, {})", *a.arena, e.x, e.y));
    }
    for (const jogo::Inimigo& i : jogo::gerarInimigos(a, semente))
        plataforma::logar(std::format("  {} em ({:.1f}, {:.1f})", jogo::dados(i.tipo).nome, i.posicao.x, i.posicao.y));
    for (const jogo::CoisaNoChao& c : jogo::gerarChao(a, semente)) {
        std::string o = "engrenagens";
        if (const auto* item = std::get_if<jogo::Item>(&c.conteudo)) o = jogo::nomeItem(*item);
        else if (const auto* m = std::get_if<jogo::MunicaoNoChao>(&c.conteudo)) o = std::format("caixa de munição ({})", m->quantidade);
        plataforma::logar(std::format("  chão: {} em ({:.1f}, {:.1f})", o, c.posicao.x, c.posicao.y));
    }
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
    a.push_back(plataforma::caminhoAsset("texturas/caixa.png"));
    a.push_back(plataforma::caminhoAsset("texturas/degrau.png"));
    return a;
}

std::vector<std::filesystem::path> arquivosDeSprites() {
    // Mesma ordem de render::CamadaSprite + quadros dos inimigos (AnimacaoInimigos.h)
    std::vector<std::filesystem::path> a{plataforma::caminhoAsset("sprites/valvula_chave.png"),
                                         plataforma::caminhoAsset("sprites/elevador.png"),
                                         plataforma::caminhoAsset("sprites/projetil.png")};
    for (const char* inimigo : {"rato", "automato", "atirador", "colosso", "vespa", "dirigivel", "rainha", "capataz", "maquina"})
        for (int q = 0; q < render::QUADROS_POR_INIMIGO; ++q)
            a.push_back(plataforma::caminhoAsset(std::format("sprites/{}_{}.png", inimigo, q)));
    // Itens, munição e engrenagens (render/Icones.h)
    for (const char* item : {"chave_inglesa", "sabre", "martelo", "revolver", "bacamarte", "tesla", "casaco", "colete",
                             "armadura", "tonico", "balas", "cartuchos", "celulas", "engrenagens"})
        a.push_back(plataforma::caminhoAsset(std::format("sprites/item_{}.png", item)));
    a.push_back(plataforma::caminhoAsset("sprites/lampiao.png"));  // render::SPRITE_LAMPIAO
    a.push_back(plataforma::caminhoAsset("sprites/lustre.png"));   // render::SPRITE_LUSTRE
    a.push_back(plataforma::caminhoAsset("sprites/bomba.png"));    // render::SPRITE_BOMBA
    a.push_back(plataforma::caminhoAsset("sprites/explosao.png")); // render::SPRITE_EXPLOSAO
    return a;
}

std::vector<std::filesystem::path> arquivosDeArmas() {
    // Mesma ordem de render::quadroArma
    std::vector<std::filesystem::path> a;
    for (const auto& [arma, quadros] : {std::pair{"chave", 2}, std::pair{"revolver", 3}, std::pair{"sabre", 2},
                                        std::pair{"martelo", 2}, std::pair{"bacamarte", 3}, std::pair{"tesla", 2}})
        for (int q = 0; q < quadros; ++q) a.push_back(plataforma::caminhoAsset(std::format("armas/{}_{}.png", arma, q)));
    return a;
}

// Camada e tamanho do sprite de algo no chão
render::Sprite spriteDoChao(const jogo::CoisaNoChao& c, const mundo::Grade& g) {
    int camada = render::ICONE_ENGRENAGENS;
    if (const auto* item = std::get_if<jogo::Item>(&c.conteudo)) camada = render::iconeDoItem(item->tipo);
    else if (const auto* m = std::get_if<jogo::MunicaoNoChao>(&c.conteudo)) camada = render::iconeDaMunicao(m->tipo);
    return {c.posicao, camada, 0.35f, 0.35f, mundo::alturaDoChao(g, c.posicao)};
}

// O que o HUD mostra, tirado da partida
render::EstadoHud estadoHud(const jogo::Partida& p) {
    const jogo::Armas& armas = p.armas();
    const jogo::Inventario& inv = p.inventario();
    const jogo::DadosArma& fogo = jogo::dadosArma(inv.armaDeFogo().tipo);
    render::EstadoHud e;
    e.vida = p.vida();
    e.vidaMaxima = p.vidaMaxima();
    e.nivel = p.nivel();
    e.xp = p.xp();
    e.xpProximo = p.xpParaProximo();
    e.engrenagens = p.engrenagens();
    e.andar = p.andar().numero;
    e.armaDeFogoAtiva = armas.ativa == jogo::ArmaAtiva::Fogo;
    e.arma = e.armaDeFogoAtiva ? inv.armaDeFogo().tipo : inv.armaBranca().tipo;
    e.carga = inv.armaDeFogo().carga;
    e.reserva = armas.municao.de(fogo.municao);
    e.municao = fogo.municao;
    e.semRecarga = fogo.carga == 0;
    e.recarregando = armas.recarga > 0;
    e.animacao = armas.animacao;
    e.tempoDesdeDano = p.tempoDesdeDano();
    e.mensagem = p.mensagem();
    for (int k = 0; k < 4; ++k)
        if (const auto& item = inv.espaco(k))
            e.rapidos[static_cast<size_t>(k)] = {render::iconeDoItem(item->tipo), item->quantidade};
    if (p.lutaComChefe())
        if (const jogo::Inimigo* c = p.chefe()) {
            e.nomeChefe = jogo::dados(c->tipo).nome;
            e.vidaChefe = c->vida;
            e.vidaMaximaChefe = c->vidaMaxima;
        }
    return e;
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
    if (leitura.estavaCorrompido)
        plataforma::logar(leitura.renomeado ? "placar.txt corrompido: renomeado para placar.txt.corrompido"
                                            : "placar.txt corrompido e não foi possível renomeá-lo; será substituído");
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


// Opções do jogador; config.txt corrompido é recriado com o padrão (seção 11 da spec)
struct OpcoesLidas {
    jogo::Config config;
    bool podeGravar = true;  // falso se o arquivo existe mas não abriu (não sobrescrever)
};

OpcoesLidas lerOpcoesDoJogador(const std::filesystem::path& arquivo) {
    const jogo::LeituraConfig leitura = jogo::lerConfig(arquivo);
    if (leitura.erroDeLeitura) {
        plataforma::logar("Não foi possível ler config.txt (arquivo em uso?); usando o padrão sem gravar: " +
                          arquivo.string());
        return {leitura.config, false};
    }
    if (leitura.estavaCorrompido) {
        plataforma::logar(leitura.renomeado ? "config.txt corrompido: renomeado para config.txt.corrompido"
                                            : "config.txt corrompido e não foi possível renomeá-lo; será substituído");
        if (!jogo::gravarConfig(arquivo, leitura.config))
            plataforma::logar("Não foi possível recriar " + arquivo.string());
    }
    return {leitura.config, true};
}

ui::Tela telaDaOpcao(const std::string& nome) {
    if (nome == "principal") return ui::Tela::Principal;
    if (nome == "opcoes") return ui::Tela::Opcoes;
    if (nome == "placar") return ui::Tela::Placar;
    if (nome == "pausa") return ui::Tela::Pausa;
    return ui::Tela::Jogo;
}

}

int main(int argc, char** argv) {
    plataforma::iniciarLog(plataforma::pastaDoExecutavel() / "log.txt");
    int codigoSaida = 0;
    try {
        const Opcoes opcoes = lerOpcoes(argc, argv);
        if (!opcoes.dados.empty()) plataforma::definirPastaDeDados(opcoes.dados);
        const uint32_t sementeInicial = opcoes.demo.value_or(opcoes.semente.value_or(std::random_device{}()));
        if (opcoes.descrever) {
            descreverAndar(sementeInicial, opcoes.andar);
            return 0;
        }
        const bool modoCaptura = !opcoes.captura.empty();
        const bool modoDemo = opcoes.demo.has_value();
        const bool automatico = modoCaptura || modoDemo;  // roda sozinho, sem teclado
        // Roteiros automáticos só mexem em placar e opções numa pasta indicada com --dados (nunca no %APPDATA% do jogador)
        const bool usaDadosDoJogador = !automatico || !opcoes.dados.empty();

        // Opções do jogador
        const std::filesystem::path arquivoConfig = plataforma::pastaDeDados() / "config.txt";
        OpcoesLidas lidas;
        if (usaDadosDoJogador) lidas = lerOpcoesDoJogador(arquivoConfig);
        jogo::Config config = lidas.config;
        const bool podeGravarConfig = usaDadosDoJogador && lidas.podeGravar;
        // "Salvar e sair": nunca nas partidas de teste (demo, captura, invencível)
        const std::filesystem::path arquivoSalvamento = plataforma::pastaDeDados() / "partida.sav";
        const bool podeSalvar = usaDadosDoJogador &&
                                jogo::podeUsarSalvamento(automatico, opcoes.invencivel, opcoes.direto(), !opcoes.dados.empty());

        plataforma::Janela janela(NOME_DO_JOGO, 1280, 720);
        bool telaCheia = config.telaCheia && !automatico;
        if (telaCheia) janela.definirTelaCheia(true);
        plataforma::Entrada entrada(janela.sdl());
        entrada.definirSensibilidade(jogo::radianosPorPixel(config.sensibilidade));
        // Depois da janela: é destruído antes dela (que fecha a SDL)
        std::optional<plataforma::Audio> audio;
        if (!opcoes.mudo) {
            audio.emplace();
            audio->carregar();
            audio->definirVolumes(config.volumeEfeitos, config.volumeMusica);
            audio->tocarMusica();
        }
        auto tocar = [&](jogo::Som s, float volume = 1.0f, float pan = 0.0f) {
            if (audio) audio->tocar(s, volume, pan);
        };
        // Opções que valem na hora (menu de opções)
        auto aplicarConfig = [&] {
            entrada.definirSensibilidade(jogo::radianosPorPixel(config.sensibilidade));
            if (audio) audio->definirVolumes(config.volumeEfeitos, config.volumeMusica);
            if (config.telaCheia != telaCheia && !automatico) {
                telaCheia = config.telaCheia;
                janela.definirTelaCheia(telaCheia);
            }
        };

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
        render::MalhaGPU malha, malhaComportas, malhaSprites, malhaLampioes;

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
        const GLint locLuzPos = glGetUniformLocation(programa, "uLuzPos");
        const GLint locLuzCor = glGetUniformLocation(programa, "uLuzCor");
        const GLint locNumLuzes = glGetUniformLocation(programa, "uNumLuzes");
        const GLint locEmissivo = glGetUniformLocation(programa, "uEmissivo");
        const GLint locLuzAlcance = glGetUniformLocation(programa, "uLuzAlcance");
        // A projeção depende só da imagem pequena, que não muda de tamanho
        glUniformMatrix4fv(locProjecao, 1, GL_FALSE,
                           glm::value_ptr(render::matrizProjecao(static_cast<float>(telaL) / telaA)));
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        // Partida em andamento; no menu, uma partida "de fundo" só para mostrar a masmorra
        ui::Menus menus;
        std::unique_ptr<jogo::Partida> partida;
        bool partidaDeFundo = false;
        jogo::Jogador anterior{};  // estado do passo anterior, para interpolar
        jogo::PassoFixo passo;
        int andarPreparado = 0;
        bool placarGravado = false;
        int posicaoPlacar = 0;
        float tempoMorto = 0.0f;  // a tela de fim só aceita teclas um pouco depois da morte
        std::optional<int> arrastando;  // espaço do inventário sendo arrastado
        std::vector<jogo::RegistroPlacar> placarLido;
        jogo::PilotoDemo piloto;
        int passosDemo = 0, capturasDemo = 0;
        const Uint64 inicioDemo = SDL_GetPerformanceCounter();  // para medir quadros por segundo de verdade

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
            partidaDeFundo = false;
            if (opcoes.invencivel || modoDemo) partida->tornarInvencivel();
            if (opcoes.mapaCompleto) partida->revelarMapa();
            placarGravado = false;
            posicaoPlacar = 0;
            tempoMorto = 0.0f;
            passo.zerar();
            prepararAndar();
        };
        auto mostrarFundo = [&] {
            partida = std::make_unique<jogo::Partida>(SEMENTE_DO_FUNDO, 1);
            partidaDeFundo = true;
            passo.zerar();
            prepararAndar();
        };
        auto lerPlacarParaTela = [&] {
            placarLido.clear();
            if (!usaDadosDoJogador) return;
            const jogo::LeituraPlacar l = jogo::lerPlacar(plataforma::pastaDeDados() / "placar.txt");
            if (l.estavaCorrompido) plataforma::logar("placar.txt corrompido ao abrir a tela de placar");
            placarLido = l.registros;
        };

        if (opcoes.direto()) {
            novaPartida(sementeInicial, opcoes.andar);
            partida->definirAngulo(glm::radians(opcoes.anguloGraus));
            partida->definirInclinacao(glm::radians(opcoes.inclinacaoGraus));
            if (opcoes.posicao) partida->definirPosicao(*opcoes.posicao);
            for (const auto& [letra, onde] : opcoes.inimigos) partida->acrescentarInimigo(jogo::tipoDaMarca(letra), onde);
            if (opcoes.vidaChefe) partida->definirVidaDoChefe(*opcoes.vidaChefe);
            anterior = partida->jogador();
            menus.abrir(ui::Tela::Jogo);
            if (opcoes.inventario) entrada.abrirInventario(true);
        } else {
            mostrarFundo();
            entrada.modoMenu(true);
        }
        // --tela: abre uma tela pronta para a captura
        if (const ui::Tela t = telaDaOpcao(opcoes.tela); t != ui::Tela::Jogo) {
            if (t != ui::Tela::Pausa && !partidaDeFundo) mostrarFundo();
            if (t == ui::Tela::Placar) lerPlacarParaTela();
            menus.abrir(t);
            entrada.modoMenu(true);
        }

        SDL_SetWindowTitle(janela.sdl(), NOME_DO_JOGO);
        int quadro = 0;
        Uint64 ultimo = SDL_GetPerformanceCounter();
        menus.definirPartidaSalva(podeSalvar && jogo::existePartidaSalva(arquivoSalvamento));
        bool sair = false;
        if (opcoes.gravarPartida && partida && !partidaDeFundo) {  // cenário de teste: só grava e sai
            plataforma::logar(jogo::gravarPartida(arquivoSalvamento, *partida) ? "--gravar-partida: gravada" : "--gravar-partida: falhou");
            sair = true;
        }
        bool demoTerminou = false;
        while (!entrada.pediuSair() && !sair) {
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
            if (automatico) dt = passo.passo();  // um passo por quadro, sem depender do relógio

            // Esc ou foco perdido no jogo: pausa (o inventário aberto fecha)
            if (entrada.pediuPausa() && menus.tela() == ui::Tela::Jogo && !automatico) {
                menus.focoPerdido();
                entrada.modoMenu(true);
                arrastando.reset();
            }

            // Lógica: só no jogo; o inventário aberto pausa; a captura e a demo rodam sem teclado
            bool capturarQuadroDemo = false;
            if (menus.tela() == ui::Tela::Jogo && (entrada.capturado() || automatico) && !entrada.inventarioAberto()) {
                const int passos = passo.adicionar(dt);
                for (int i = 0; i < passos; ++i) {
                    anterior = partida->jogador();
                    const float dtPasso = static_cast<float>(passo.passo());
                    // Giro e eventos vão no primeiro passo; coletar() já os zera para os seguintes
                    const jogo::Comandos c = modoDemo      ? piloto.comandar(*partida, dtPasso)
                                             : modoCaptura ? jogo::Comandos{}
                                                           : entrada.coletar();
                    partida->atualizar(c, dtPasso);
                    for (const jogo::EventoSom& s : partida->sons()) {
                        const jogo::Mixagem m = jogo::mixar(s, partida->jogador().posicao, partida->jogador().angulo);
                        tocar(s.som, m.volume, m.pan);
                    }
                    if (modoDemo && ++passosDemo % PASSOS_ENTRE_CAPTURAS_DEMO == 0) capturarQuadroDemo = true;
                }
            } else {
                passo.zerar();
                entrada.descartarGiro();
                anterior = partida->jogador();
            }
            if (partidaDeFundo) {  // a câmera gira devagar atrás do menu
                partida->definirAngulo(partida->jogador().angulo + GIRO_DO_FUNDO * static_cast<float>(dt));
                anterior = partida->jogador();
            }

            // Luta com chefe: música própria enquanto dura
            if (audio) audio->tocarMusica(partida->lutaComChefe() ? "musica_chefe" : "musica");

            // Fim (morte ou vitória): grava o placar uma vez, solta o mouse e mostra a tela de fim
            if (partida->terminou() && !placarGravado) {
                if (usaDadosDoJogador) posicaoPlacar = registrarNoPlacar(*partida);
                placarGravado = true;
                menus.abrir(ui::Tela::Fim);
                entrada.modoMenu(true);
                arrastando.reset();
            }
            if (partida->terminou()) tempoMorto += static_cast<float>(dt);

            // Demo: chegou ao elevador ou desistiu
            bool fimDaDemo = false;
            if (modoDemo && partida->trocasDeAndar() > 0) {
                plataforma::logar(std::format("demo: chegou ao elevador em {:.1f} s", passosDemo * passo.passo()));
                const double real = static_cast<double>(SDL_GetPerformanceCounter() - inicioDemo) / SDL_GetPerformanceFrequency();
                plataforma::logar(std::format("demo: {} quadros em {:.1f} s ({:.0f} por segundo)", passosDemo, real,
                                              real > 0 ? passosDemo / real : 0.0));
                capturarQuadroDemo = fimDaDemo = true;
            } else if (modoDemo && passosDemo * passo.passo() > LIMITE_DEMO) {
                plataforma::logar(std::format("demo: não chegou ao elevador em {:.0f} s", LIMITE_DEMO));
                capturarQuadroDemo = fimDaDemo = true;
                codigoSaida = 2;
            }

            // Andar novo: remonta a malha e não interpola a partir do andar anterior
            if (partida->trocasDeAndar() != andarPreparado) prepararAndar();

            // Menus (pausa, opções, placar, fim)
            const ui::Tela telaAtual = menus.tela();
            std::vector<render::Retangulo> itensNaTela;
            if (telaAtual == ui::Tela::Placar) itensNaTela = render::layoutPlacar(telaL, telaA);
            else if (telaAtual != ui::Tela::Jogo && telaAtual != ui::Tela::Fim)
                itensNaTela = render::layoutMenu(static_cast<int>(menus.itens(config).size()), telaL, telaA);
            if (telaAtual != ui::Tela::Jogo) {
                int janelaL = 0, janelaA = 0;
                SDL_GetWindowSize(janela.sdl(), &janelaL, &janelaA);
                const glm::vec2 mouse = render::janelaParaTelaBaixa(entrada.posicaoMouse(), {janelaL, janelaA},
                                                                    janela.tamanhoDesenhavel(), telaL, telaA);
                ui::EntradaMenu em = entrada.coletarMenu();
                em.itemSobMouse = render::itemSob(itensNaTela, mouse);
                // Na captura o teclado é ignorado; na tela de fim, só depois de meio segundo
                if (automatico || (telaAtual == ui::Tela::Fim && tempoMorto <= 0.5f)) em = {};
                const int selecionadoAntes = menus.selecionado();
                const std::vector<std::string> itensAntes = menus.itens(config);
                const ui::Acao acao = menus.atualizar(em, config);
                const bool mudouDeTela = menus.tela() != telaAtual;
                if (!mudouDeTela && menus.selecionado() != selecionadoAntes) tocar(jogo::Som::MenuMover);
                // Confirmar só soa quando algo aconteceu (ação, outra tela ou o texto do item mudou, como no abandonar)
                if (acao != ui::Acao::Nenhuma || mudouDeTela || (!mudouDeTela && menus.itens(config) != itensAntes))
                    tocar(jogo::Som::MenuConfirmar);
                switch (acao) {
                    case ui::Acao::NovoJogo:
                        // Com partida salva, o menu já pediu confirmação: começar outra a apaga
                        if (podeSalvar) jogo::apagarPartidaSalva(arquivoSalvamento);
                        menus.definirPartidaSalva(false);
                        novaPartida(config.semente.value_or(std::random_device{}()), 1);
                        entrada.entrarNoJogo();
                        break;
                    case ui::Acao::ContinuarSalva: {
                        auto [resultado, lida] = jogo::continuarPartida(arquivoSalvamento);
                        // Continuar consome o salvamento (ou ele estava estragado); preso, ele fica para outra tentativa
                        menus.definirPartidaSalva(resultado == jogo::ResultadoCarga::Preso);
                        if (resultado != jogo::ResultadoCarga::Carregou) {
                            menus.abrir(ui::Tela::Principal);
                            if (resultado == jogo::ResultadoCarga::Estragado) {
                                menus.definirAviso("Partida salva não pôde ser lida");
                                plataforma::logar("partida.sav não pôde ser lida (renomeada para .corrompido)");
                            } else if (resultado == jogo::ResultadoCarga::Preso) {
                                menus.definirAviso("Partida salva em uso: feche outros programas");
                                plataforma::logar("partida.sav não pôde ser apagada; a partida não continua");
                            }
                            break;
                        }
                        plataforma::logar(std::format("Partida salva continuada (semente {}, andar {})", lida->semente(),
                                                      lida->andar().numero));
                        partida = std::make_unique<jogo::Partida>(std::move(*lida));
                        partidaDeFundo = false;
                        placarGravado = false;
                        posicaoPlacar = 0;
                        tempoMorto = 0.0f;
                        passo.zerar();
                        prepararAndar();
                        entrada.entrarNoJogo();
                        break;
                    }
                    case ui::Acao::SalvarESair:
                        if (!podeSalvar || !jogo::podeGravarAgora(*partida)) {
                            // Modo de teste sem pasta de dados, ou a vitória contando: não é falha do disco
                            menus.definirAviso("Não dá para salvar agora");  // fica na pausa
                        } else if (jogo::gravarPartida(arquivoSalvamento, *partida)) {
                            plataforma::logar("Partida salva; saindo");
                            sair = true;
                        } else {
                            menus.definirAviso("Não foi possível salvar");
                            plataforma::logar("Não foi possível salvar " + arquivoSalvamento.string());
                        }
                        break;
                    case ui::Acao::Continuar: entrada.entrarNoJogo(); break;
                    case ui::Acao::Abandonar: mostrarFundo(); break;
                    case ui::Acao::Sair: sair = true; break;
                    case ui::Acao::OpcoesMudaram: aplicarConfig(); break;
                    case ui::Acao::SalvarOpcoes:
                        aplicarConfig();
                        if (podeGravarConfig && !jogo::gravarConfig(arquivoConfig, config))
                            plataforma::logar("Não foi possível gravar " + arquivoConfig.string());
                        break;
                    case ui::Acao::AbrirPlacar: lerPlacarParaTela(); break;
                    default: break;
                }
            }

            const float alfa = static_cast<float>(passo.alfa());
            const glm::vec2 pos = glm::mix(anterior.posicao, partida->jogador().posicao, alfa);
            const float ang = glm::mix(anterior.angulo, partida->jogador().angulo, alfa);
            const float incl = glm::mix(anterior.inclinacao, partida->jogador().inclinacao, alfa);
            // Olhos acima dos pés, interpolados como a posição (pulo, queda, rampa)
            const float olhos = glm::mix(anterior.corpo.pes, partida->jogador().corpo.pes, alfa) + render::ALTURA_OLHOS;
            const ui::Tela tela = menus.tela();

            // Cena na imagem pequena
            telaBaixa.comecar();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(programa);
            glUniformMatrix4fv(locVisao, 1, GL_FALSE, glm::value_ptr(render::matrizVisao(pos, ang, incl, olhos)));
            glUniform3f(locLanterna, pos.x, olhos, pos.y);
            glUniform1f(locClarao, partida->clarao() > 0.0f ? 1.0f : 0.0f);
            // Lampiões: os 16 mais perto do olho, com a chama tremendo
            {
                const glm::vec3 olho{pos.x, olhos, pos.y};
                const float segundos = static_cast<float>(SDL_GetTicks()) / 1000.0f;
                std::vector<glm::vec3> posicoes, cores;
                std::vector<float> alcances;
                for (const mundo::Luz& l : render::luzesMaisProximas(partida->andar().luzes, olho)) {
                    posicoes.push_back(l.posicao);
                    cores.push_back(l.cor * render::INTENSIDADE_LAMPIAO * render::tremulacao(segundos, l.fase));
                    alcances.push_back(l.alcance);
                }
                const auto quantas = static_cast<GLsizei>(posicoes.size());
                glUniform1i(locNumLuzes, quantas);
                if (quantas > 0) {
                    glUniform3fv(locLuzPos, quantas, glm::value_ptr(posicoes[0]));
                    glUniform3fv(locLuzCor, quantas, glm::value_ptr(cores[0]));
                    glUniform1fv(locLuzAlcance, quantas, alcances.data());
                }
                glUniform1i(locEmissivo, 0);
            }
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_ARRAY, texturas);
            malha.desenhar();
            malhaComportas.enviar(render::construirComportas(partida->andar().grade, [&](int x, int y) {
                return partida->aberturaComporta(x, y);
            }), true);
            malhaComportas.desenhar();

            // Sprites: elevador, válvula, coisas no chão, inimigos e projéteis
            std::vector<render::Sprite> sprites;
            for (const jogo::CoisaNoChao& c : partida->chao()) sprites.push_back(spriteDoChao(c, partida->andar().grade));
            const glm::vec2 elevador = glm::vec2(partida->andar().elevador) + 0.5f;
            sprites.push_back({elevador, render::SpriteElevador, 1.0f, 1.0f, mundo::alturaDoChao(partida->andar().grade, elevador)});
            if (partida->andar().chave)
                sprites.push_back({*partida->andar().chave, render::SpriteValvulaChave, 0.4f, 0.4f,
                                   mundo::alturaDoChao(partida->andar().grade, *partida->andar().chave)});
            for (const jogo::Inimigo& i : partida->inimigos()) {
                const glm::vec2 tamanho = render::tamanhoDoInimigo(i.tipo);
                sprites.push_back({i.posicao, render::camadaDoInimigo(i.tipo, i.estado, i.tempoNoEstado), tamanho.x,
                                   tamanho.y, i.corpo.pes});
            }
            for (const jogo::Projetil& t : partida->projeteis()) {
                if (t.raioExplosao == 0) {
                    sprites.push_back({{t.posicao.x, t.posicao.z}, render::SpriteProjetil, 0.15f, 0.15f, t.posicao.y - 0.075f});
                } else if (std::fmod(SDL_GetTicks() / 1000.0f, 0.2f) < 0.1f) {  // a bomba pisca enquanto cai
                    sprites.push_back({{t.posicao.x, t.posicao.z}, render::SPRITE_BOMBA, 0.25f, 0.25f, t.posicao.y - 0.125f});
                }
            }
            glBindTexture(GL_TEXTURE_2D_ARRAY, texturasSprites);
            malhaSprites.enviar(render::construirSprites(sprites, pos), true);
            malhaSprites.desenhar();
            // Lampiões sempre acesos (emissivos)
            std::vector<render::Sprite> lampioes;
            for (const mundo::Luz& l : partida->andar().luzes)
                lampioes.push_back(l.tipo == mundo::TipoLuz::Lustre
                                       ? render::Sprite{{l.posicao.x, l.posicao.z}, render::SPRITE_LUSTRE, 0.5f, 0.5f, l.posicao.y - 0.15f}
                                       : render::Sprite{{l.posicao.x, l.posicao.z}, render::SPRITE_LAMPIAO, 0.3f, 0.3f, l.posicao.y - 0.15f});
            for (const jogo::Explosao& x : partida->explosoes()) {  // explosões das bombas, acesas como os lampiões
                const glm::vec2 onde{x.posicao.x, x.posicao.z};
                const float base = std::max(x.posicao.y - 0.5f, mundo::alturaDoChao(partida->andar().grade, onde));
                lampioes.push_back({onde, render::SPRITE_EXPLOSAO, 1.0f, 1.0f, base});  // nunca enterrada no chão
            }
            malhaLampioes.enviar(render::construirSprites(lampioes, pos), true);
            glUniform1i(locEmissivo, 1);
            malhaLampioes.desenhar();
            glUniform1i(locEmissivo, 0);

            // HUD (arma na mão por baixo, depois barras/flash e texto); só no jogo e na tela de fim
            const bool mostrarHud = !partidaDeFundo && (tela == ui::Tela::Jogo || tela == ui::Tela::Fim);
            if (mostrarHud && !entrada.inventarioAberto()) {  // o inventário e os menus cobrem o HUD
                const render::ElementosHud hud = render::construirHud(estadoHud(*partida), telaL, telaA);
                if (!partida->terminou()) desenhoTextura.desenhar(hud.arma, texturasArmas, telaL, telaA);
                desenho2D.desenhar(hud.cores, telaL, telaA);
                desenhoTextura.desenhar(hud.icones, texturasSprites, telaL, telaA);
                desenhoTextura.desenhar(hud.texto, fonte, telaL, telaA);
            }

            if (tela == ui::Tela::Jogo && (entrada.mapaVisivel() || opcoes.mapaCompleto))
                desenho2D.desenhar(render::construirAutomapa(partida->andar().grade, partida->exploracao(), pos, ang,
                                                             partida->andar().chave, telaL, telaA),
                                   telaL, telaA);

            // Inventário: clique usa/equipa, arrastar troca, botão direito larga no chão
            if (tela == ui::Tela::Jogo && entrada.inventarioAberto()) {
                int janelaL = 0, janelaA = 0;
                SDL_GetWindowSize(janela.sdl(), &janelaL, &janelaA);
                const glm::vec2 mouse = render::janelaParaTelaBaixa(entrada.posicaoMouse(), {janelaL, janelaA},
                                                                    janela.tamanhoDesenhavel(), telaL, telaA);
                const render::LayoutInventario layout = render::layoutInventario(telaL, telaA);
                const std::optional<int> sob = render::espacoSob(layout, mouse);
                const bool sobEspaco = sob && *sob < jogo::ESPACOS_INVENTARIO;
                const plataforma::CliqueInventario cliques = entrada.coletarCliques();
                if (cliques.pressionou && sobEspaco && partida->inventario().espaco(*sob)) arrastando = *sob;
                if (cliques.soltou && arrastando) {
                    if (sobEspaco && *sob == *arrastando) partida->usarEspaco(*sob);
                    else if (sobEspaco) partida->trocarEspacos(*arrastando, *sob);
                    else if (sob) {  // num espaço de equipamento: equipa se for o lugar dele
                        const auto& item = partida->inventario().espaco(*arrastando);
                        if (item && jogo::espacoDeEquipamento(item->tipo) == *sob - jogo::ESPACOS_INVENTARIO)
                            partida->usarEspaco(*arrastando);
                    }
                    else if (render::sobLargar(layout, mouse)) partida->largarEspaco(*arrastando);
                    arrastando.reset();
                }
                if (cliques.direito && sobEspaco) partida->largarEspaco(*sob);
                const render::ElementosInventario telaInv =
                    render::construirInventario(partida->inventario(), layout, sob, arrastando, mouse, partida->mensagem());
                desenho2D.desenhar(telaInv.cores, telaL, telaA);
                desenhoTextura.desenhar(telaInv.icones, texturasSprites, telaL, telaA);
                desenhoTextura.desenhar(telaInv.texto, fonte, telaL, telaA);
            } else {
                arrastando.reset();
            }

            if (tela == ui::Tela::Fim) {
                const render::ElementosHud fim = render::construirTelaFim(
                    {partida->andar().numero, partida->nivel(), partida->inimigosDestruidos(), partida->engrenagens(),
                     partida->pontuacao(), partida->semente(), partida->causaDaMorte(), posicaoPlacar,
                     partida->venceu(), static_cast<int>(partida->tempoDeJogo())},
                    telaL, telaA);
                desenho2D.desenhar(fim.cores, telaL, telaA);
                desenhoTextura.desenhar(fim.texto, fonte, telaL, telaA);
            } else if (tela != ui::Tela::Jogo) {
                const render::ElementosHud m =
                    tela == ui::Tela::Placar
                        ? render::construirPlacar(placarLido, telaL, telaA)
                        : render::construirMenu({menus.titulo(), menus.itens(config), menus.selecionado(), menus.aviso()}, telaL, telaA);
                desenho2D.desenhar(m.cores, telaL, telaA);
                desenhoTextura.desenhar(m.texto, fonte, telaL, telaA);
            }

            // Ampliada na janela
            const glm::ivec2 tamanho = janela.tamanhoDesenhavel();
            telaBaixa.apresentar(tamanho);
            if (modoCaptura && ++quadro == opcoes.quadroCaptura) {
                salvarCaptura(tamanho, opcoes.captura);
                break;
            }
            if (capturarQuadroDemo) {
                const std::filesystem::path pasta = plataforma::pastaDoExecutavel() / "capturas";
                std::error_code erro;
                std::filesystem::create_directories(pasta, erro);
                salvarCaptura(tamanho, (pasta / std::format("demo_{}_{:02}.bmp", *opcoes.demo, ++capturasDemo)).string());
            }
            if (fimDaDemo) {
                demoTerminou = true;
                break;
            }
            janela.trocarBuffers();
        }
        // Fechou a janela no meio de uma partida: grava como "Salvar e sair" (morto ou vencido não grava)
        if (!sair && podeSalvar && partida && !partidaDeFundo && !partida->terminou() &&
            (menus.tela() == ui::Tela::Jogo || menus.tela() == ui::Tela::Pausa ||
             (menus.tela() == ui::Tela::Opcoes))) {
            if (jogo::gravarPartida(arquivoSalvamento, *partida)) plataforma::logar("Janela fechada: partida salva");
            else plataforma::logar("Janela fechada: não foi possível salvar " + arquivoSalvamento.string());
        }
        // Fechou a janela (X, Alt+F4) com a tela de opções aberta: não perde o que mudou
        if (menus.tela() == ui::Tela::Opcoes && podeGravarConfig && !jogo::gravarConfig(arquivoConfig, config))
            plataforma::logar("Não foi possível gravar " + arquivoConfig.string());
        // Demo fechada por fora: não conta como sucesso (o teste do pacote depende disso)
        if (modoDemo && !demoTerminou) {
            plataforma::logar("demo: interrompida antes de chegar ao elevador");
            codigoSaida = 3;
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
    return codigoSaida;
}
