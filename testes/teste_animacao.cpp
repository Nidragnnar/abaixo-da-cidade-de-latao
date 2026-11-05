#include <doctest/doctest.h>

#include "render/AnimacaoInimigos.h"

TEST_CASE("quadros do inimigo pelo estado e pelo tempo") {
    using T = jogo::TipoInimigo;
    using E = jogo::EstadoInimigo;
    const int base = render::PRIMEIRO_INIMIGO + 1 * render::QUADROS_POR_INIMIGO;  // autômato
    CHECK(render::camadaDoInimigo(T::Automato, E::Parado, 5.0f) == base + 0);
    CHECK(render::camadaDoInimigo(T::Automato, E::Perseguindo, 0.1f) == base + 1);
    CHECK(render::camadaDoInimigo(T::Automato, E::Perseguindo, 0.3f) == base + 2);
    CHECK(render::camadaDoInimigo(T::Automato, E::Preparando, 0.1f) == base + 3);
    CHECK(render::camadaDoInimigo(T::Automato, E::Recuperando, 0.1f) == base + 4);
    CHECK(render::camadaDoInimigo(T::Automato, E::Destruido, 0.1f) == base + 5);
    CHECK(render::camadaDoInimigo(T::Automato, E::Destruido, 2.0f) == base + 6);
}

TEST_CASE("cada tipo tem seu bloco de quadros e seu tamanho") {
    CHECK(render::camadaDoInimigo(jogo::TipoInimigo::Rato, jogo::EstadoInimigo::Parado, 0) == render::PRIMEIRO_INIMIGO);
    CHECK(render::camadaDoInimigo(jogo::TipoInimigo::Colosso, jogo::EstadoInimigo::Parado, 0) ==
          render::PRIMEIRO_INIMIGO + 3 * render::QUADROS_POR_INIMIGO);
    CHECK(render::tamanhoDoInimigo(jogo::TipoInimigo::Colosso).y == doctest::Approx(1.0f));
    CHECK(render::tamanhoDoInimigo(jogo::TipoInimigo::Rato).y < 0.5f);
}

TEST_CASE("hélice do dirigível gira enquanto recarrega a bomba") {
    const int base = render::PRIMEIRO_INIMIGO + 5 * render::QUADROS_POR_INIMIGO;
    CHECK(render::camadaDoInimigo(jogo::TipoInimigo::Dirigivel, jogo::EstadoInimigo::Recuperando, 0.1f) == base + 1);
    CHECK(render::camadaDoInimigo(jogo::TipoInimigo::Dirigivel, jogo::EstadoInimigo::Recuperando, 0.3f) == base + 2);
}
