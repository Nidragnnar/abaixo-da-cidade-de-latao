#include "mundo/Zona.h"

namespace mundo {

Zona zonaDoAndar(int andar) {
    if (andar <= 2) return Zona::Esgoto;
    if (andar <= 4) return Zona::Fabrica;
    return Zona::Minas;
}

}
