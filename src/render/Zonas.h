#pragma once
#include <glm/vec3.hpp>

#include "mundo/Zona.h"

namespace render {

// Luz ambiente fraca de cada zona (lampião, fornalha, lâmpada elétrica)
glm::vec3 corAmbiente(mundo::Zona z);

}
