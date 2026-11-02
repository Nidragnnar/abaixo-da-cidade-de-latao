#pragma once
#include <glad/glad.h>
#include <glm/vec2.hpp>

namespace render {

// Imagem pequena onde a cena é desenhada, depois ampliada com pixels nítidos e faixas pretas
class TelaBaixa {
public:
    // Precisa de um contexto OpenGL ativo; lança std::runtime_error se o framebuffer falhar
    explicit TelaBaixa(int largura = 480, int altura = 270);
    ~TelaBaixa();
    TelaBaixa(const TelaBaixa&) = delete;
    TelaBaixa& operator=(const TelaBaixa&) = delete;

    int largura() const { return largura_; }
    int altura() const { return altura_; }

    // Passa a desenhar na imagem pequena
    void comecar();
    // Desenha a imagem pequena ampliada na janela
    void apresentar(glm::ivec2 tamanhoDesenhavel);

private:
    int largura_, altura_;
    GLuint fbo_ = 0, cor_ = 0, profundidade_ = 0;
    GLuint programa_ = 0, vaoVazio_ = 0;
};

}
