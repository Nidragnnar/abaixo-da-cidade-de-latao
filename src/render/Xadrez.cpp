#include "render/Xadrez.h"

namespace render {

std::vector<uint8_t> gerarXadrez(int lado) {
    std::vector<uint8_t> px(static_cast<size_t>(lado) * lado * 4);
    for (int y = 0; y < lado; ++y)
        for (int x = 0; x < lado; ++x) {
            const bool rosa = ((x / 8) + (y / 8)) % 2 == 0;
            uint8_t* p = &px[(static_cast<size_t>(y) * lado + x) * 4];
            p[0] = rosa ? 255 : 0;
            p[1] = 0;
            p[2] = rosa ? 255 : 0;
            p[3] = 255;
        }
    return px;
}

}
