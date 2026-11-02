#pragma once
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "render/Primitivas2D.h"

namespace render {

constexpr int LADO_GLIFO = 8;
constexpr int COLUNAS_FONTE = 16;  // glifos por linha na imagem da fonte

// ASCII 32–126 seguido dos acentos do português; a camada do glifo é o índice nesta lista
const std::u32string& caracteresFonte();

// Desconhecido → índice de '?'
int indiceGlifo(char32_t c);

// Um quadrado de 8 × escala por caractere (UTF-8; sequência inválida vira '?')
std::vector<VerticeTex> construirTexto(std::string_view utf8, glm::vec2 posicao, float escala, glm::vec4 cor);
float larguraTexto(std::string_view utf8, float escala);

}
