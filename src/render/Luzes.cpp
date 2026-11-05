#include "render/Luzes.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

namespace render {

std::vector<mundo::Luz> luzesMaisProximas(const std::vector<mundo::Luz>& luzes, glm::vec3 ponto) {
    std::vector<mundo::Luz> v = luzes;
    const auto quantas = std::min(v.size(), static_cast<size_t>(MAXIMO_LUZES));
    std::partial_sort(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(quantas), v.end(),
                      [&](const mundo::Luz& a, const mundo::Luz& b) {
                          return glm::distance(a.posicao, ponto) < glm::distance(b.posicao, ponto);
                      });
    v.resize(quantas);
    return v;
}

float tremulacao(float tempo, float fase) {
    // Duas ondas fora de compasso: 0,06 + 0,04 = 0,1 de variação no máximo
    return 1.0f + 0.06f * std::sin(7.3f * tempo + fase) + 0.04f * std::sin(13.1f * tempo + 2.0f * fase);
}

}
