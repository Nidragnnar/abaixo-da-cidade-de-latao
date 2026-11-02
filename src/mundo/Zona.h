#pragma once

namespace mundo {

// Conjunto de texturas e cor de luz de uma faixa de andares
enum class Zona { Esgoto, Fabrica, Minas };

// Andares 1–2 esgoto, 3–4 fábrica, 5 em diante minas
Zona zonaDoAndar(int andar);

}
