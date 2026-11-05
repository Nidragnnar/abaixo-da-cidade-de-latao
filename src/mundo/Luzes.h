#pragma once
#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

namespace mundo {

struct Andar;

enum class TipoLuz { Lampiao, Lustre };

// Lampião preso na parede ou lustre pendurado no teto (x, altura, z)
struct Luz {
    glm::vec3 posicao;
    glm::vec3 cor;
    float alcance;
    float fase;  // ritmo próprio da tremulação
    TipoLuz tipo = TipoLuz::Lampiao;
};

constexpr float ALCANCE_LUZ = 5.0f;
constexpr float ALTURA_LUZ = 0.8f;       // lampião: acima do chão local
constexpr float ALCANCE_LUSTRE = 7.0f;   // lustre: ilumina o teto alto e o chão da sala
constexpr float DESCIDA_LUSTRE = 0.35f;  // abaixo do teto

// Salas: lustre no teto, no centro (1 nas pequenas, 2 nas grandes), e lampiões na parede (1 nas pequenas, 2–3 nas
// grandes); corredores: nenhuma célula a mais de 4,5 de uma luz. Lampião sempre numa célula de chão encostada numa
// parede. Cor da zona. Sorteio próprio: não mexe no resto do andar.
std::vector<Luz> gerarLuzes(const Andar& a, uint32_t semente);

}
