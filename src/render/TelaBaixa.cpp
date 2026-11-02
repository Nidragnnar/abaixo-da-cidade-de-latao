#include "render/TelaBaixa.h"

#include <stdexcept>

#include "plataforma/Caminhos.h"
#include "render/Letterbox.h"
#include "render/Shader.h"

namespace render {

TelaBaixa::TelaBaixa(int largura, int altura) : largura_(largura), altura_(altura) {
    glGenTextures(1, &cor_);
    glBindTexture(GL_TEXTURE_2D, cor_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, largura_, altura_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenRenderbuffers(1, &profundidade_);
    glBindRenderbuffer(GL_RENDERBUFFER, profundidade_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, largura_, altura_);

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cor_, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, profundidade_);
    const bool completo = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!completo) throw std::runtime_error("Não foi possível criar a imagem de baixa resolução (framebuffer).");

    programa_ = compilarPrograma(plataforma::caminhoAsset("shaders/tela.vert"),
                                 plataforma::caminhoAsset("shaders/tela.frag"));
    glUseProgram(programa_);
    glUniform1i(glGetUniformLocation(programa_, "uImagem"), 0);
    glGenVertexArrays(1, &vaoVazio_);  // o perfil core exige um VAO mesmo sem atributos
}

TelaBaixa::~TelaBaixa() {
    glDeleteVertexArrays(1, &vaoVazio_);
    glDeleteProgram(programa_);
    glDeleteFramebuffers(1, &fbo_);
    glDeleteRenderbuffers(1, &profundidade_);
    glDeleteTextures(1, &cor_);
}

void TelaBaixa::comecar() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, largura_, altura_);
}

void TelaBaixa::apresentar(glm::ivec2 tamanhoDesenhavel) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, tamanhoDesenhavel.x, tamanhoDesenhavel.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const Retangulo r = calcularLetterbox(tamanhoDesenhavel.x, tamanhoDesenhavel.y, largura_, altura_);
    if (r.largura == 0 || r.altura == 0) return;  // janela minimizada
    glViewport(r.x, r.y, r.largura, r.altura);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(programa_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cor_);
    glBindVertexArray(vaoVazio_);
    glDrawArrays(GL_TRIANGLES, 0, 3);  // um triângulo que cobre a tela toda
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

}
