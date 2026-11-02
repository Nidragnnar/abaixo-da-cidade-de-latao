#pragma once
#include <string>

#include <SDL.h>
#include <glm/vec2.hpp>

namespace plataforma {

// Janela SDL com contexto OpenGL 3.3 core e funções do glad carregadas
class Janela {
public:
    // Lança std::runtime_error com mensagem para o jogador se não der para abrir
    Janela(const char* titulo, int largura, int altura);
    ~Janela();
    Janela(const Janela&) = delete;
    Janela& operator=(const Janela&) = delete;

    SDL_Window* sdl() const { return janela_; }
    void trocarBuffers();
    // Tamanho real em pixels (difere do tamanho lógico com escala de tela)
    glm::ivec2 tamanhoDesenhavel() const;

private:
    SDL_Window* janela_ = nullptr;
    SDL_GLContext contexto_ = nullptr;
};

// Caixa de mensagem do Windows + linha no log
void mostrarErroFatal(const std::string& mensagem);

}
