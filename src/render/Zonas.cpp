#include "render/Zonas.h"

namespace render {

glm::vec3 corAmbiente(mundo::Zona z) {
    switch (z) {
        case mundo::Zona::Fabrica: return {0.12f, 0.07f, 0.03f};  // fornalhas, alaranjada
        case mundo::Zona::Minas: return {0.05f, 0.07f, 0.11f};    // lâmpadas elétricas, azulada
        default: return {0.10f, 0.09f, 0.06f};                    // lampiões a gás, amarelada
    }
}

}
