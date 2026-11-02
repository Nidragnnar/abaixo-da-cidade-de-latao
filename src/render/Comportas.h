#pragma once
#include <functional>
#include <vector>

#include "mundo/Grade.h"
#include "render/GeometriaMasmorra.h"

namespace render {

// Placas das comportas fechadas ou subindo: duas faces no meio da célula, perpendiculares à passagem,
// erguidas pela abertura (0 fechada … 1 aberta). Comporta aberta não gera nada.
std::vector<Vertice> construirComportas(const mundo::Grade& g, const std::function<float(int, int)>& abertura);

}
