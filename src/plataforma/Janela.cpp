#include "plataforma/Janela.h"

#include <format>
#include <stdexcept>

#include <glad/glad.h>

#include "plataforma/Log.h"

namespace plataforma {

Janela::Janela(const char* titulo, int largura, int altura) {
    // Escala de tela (125%, 150%...): pixels reais em vez de imagem esticada e borrada pelo Windows
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
    // O clique que traz a janela de volta (depois de Alt+Tab) já recaptura o mouse
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        throw std::runtime_error(std::format("Não foi possível iniciar a SDL: {}", SDL_GetError()));

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    janela_ = SDL_CreateWindow(titulo, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, largura, altura,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!janela_)
        throw std::runtime_error(std::format("Não foi possível criar a janela: {}", SDL_GetError()));

    contexto_ = SDL_GL_CreateContext(janela_);
    if (!contexto_ || !gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
        throw std::runtime_error(
            "Sua placa de vídeo ou driver não suporta OpenGL 3.3, necessário para o jogo.\n"
            "Tente atualizar o driver de vídeo.");

    SDL_GL_SetSwapInterval(1);  // V-Sync
    logar(std::format("OpenGL {} | {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)),
                      reinterpret_cast<const char*>(glGetString(GL_RENDERER))));
}

Janela::~Janela() {
    if (contexto_) SDL_GL_DeleteContext(contexto_);
    if (janela_) SDL_DestroyWindow(janela_);
    SDL_Quit();
}

void Janela::trocarBuffers() { SDL_GL_SwapWindow(janela_); }

glm::ivec2 Janela::tamanhoDesenhavel() const {
    glm::ivec2 t;
    SDL_GL_GetDrawableSize(janela_, &t.x, &t.y);
    return t;
}

void mostrarErroFatal(const std::string& mensagem) {
    logar("ERRO FATAL: " + mensagem);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Roguelike", mensagem.c_str(), nullptr);
}

}
