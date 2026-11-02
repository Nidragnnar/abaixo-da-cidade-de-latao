# Etapa 2 — Masmorra procedural: Plano de implementação

> **Para agentes:** SUB-SKILL OBRIGATÓRIA: usar superpowers:subagent-driven-development (recomendado) ou superpowers:executing-plans para executar este plano tarefa por tarefa. Os passos usam caixas (`- [ ]`) para acompanhamento.

**Objetivo:** trocar a masmorra fixa por andares gerados a partir de uma semente, nas 3 zonas, com comportas que sobem, comporta trancada com válvula-chave, elevador de carga para o próximo andar e automapa (Tab).

**Arquitetura:** o gerador (`mundo/Gerador`) produz um `Andar` puro (grade, salas, início, elevador, chave). Uma nova classe `jogo::Partida` junta andar, jogador, comportas, chave, exploração e mensagens e é atualizada a 60 Hz; tudo isso é testado sem janela. O `render` ganha construtores puros de vértices para comportas, sprites e automapa (também testados) e o `main` só liga as peças.

**Tecnologia:** a mesma da Etapa 1 (C++20, MSVC 14.51, CMake 4.3, SDL2 2.32.10, glad 3.3 core, glm 1.0.1, stb_image, doctest 2.4.11).

**Spec:** `docs/superpowers/specs/2026-11-01-roguelike-design.md` (seção 3 inteira, seção 4 tecla E e Tab, seção 12, seção 13 etapa 2)

## Restrições globais

- Todas as da Etapa 1 continuam valendo (C++20, `/W4 /utf-8`, runtime `/MT`, `mundo/` e `jogo/` sem SDL/OpenGL, nomes e comentários em português, constantes de câmera/luz/neblina, assets achados pela pasta do exe).
- Grade de **64×64**; **8 a 15 salas**, lados de **4 a 10** células, **1 célula de margem**; corredores em "L" de **1 célula** formando árvore; **2 a 4** corredores extras; comportas em **~50%** das entradas; a partir do **andar 2**, **40%** de chance de comporta trancada.
- Zonas: andares **1–2 Esgoto**, **3–4 Fábrica**, **5+ Minas**. Luz ambiente: esgoto `(0.10, 0.09, 0.06)` (já usada), fábrica `(0.12, 0.07, 0.03)`, minas `(0.05, 0.07, 0.11)`.
- Comporta sobe em **0,5 s**; continua sólida até abrir por completo.
- Tecla **E** (usar) e **Tab** (automapa) conforme a seção 4 da spec.
- Comandos de evento (`usar`) valem só no **primeiro passo lógico** do quadro, como o giro do mouse.
- Não há HUD de texto nesta etapa (fica para a etapa 4): andar e mensagens curtas aparecem no **título da janela**, que só é trocado quando o texto muda.
- Constantes desta etapa (não fixadas pela spec; ajustar após o teste): alcance do usar **1,5**; raio para pegar a chave **0,6**; mensagens duram **2 s**; exploração marca células com linha de visão até **6** células (alcance da lanterna); automapa com **4 pixels por célula**.
- **Downloads:** nenhum novo previsto (os pacotes Tiny Texture Pack 1 e Zortch já estão em `arte_fonte/`). Qualquer pacote novo só com o ok do Nidragnnar.
- **Commit:** um único, no fim, depois do "sim" do Nidragnnar.

## Decisões de projeto (desvios pequenos da spec, registrados aqui)

- **Semente por andar:** cada andar usa `sementeDoAndar(sementePartida, numero)` em vez de consumir um único gerador em sequência. Continua determinístico pela semente da partida e permite `--andar N` sem gerar os andares anteriores.
- **Elevador:** célula `Elevador` (não sólida, com o chão da zona) + um sprite da gaiola em pé no centro, como a spec pede ("elevador é sprite").
- **Tranca:** quando o andar sorteia tranca, o gerador procura uma entrada única da sala do elevador; se a sala tiver mais de uma entrada, tenta de novo com variações da semente (até 10 vezes) e, se ainda falhar, o andar fica sem tranca. Por isso a taxa real fica um pouco abaixo de 40%.
- **Exploração do automapa:** "vista" = célula a até 6 de distância com linha de visão, em qualquer direção (o que a lanterna ilumina), não só o que está no campo de visão.

## Foco de revisão

1. **Sementes ruins** (salas que não cabem, corredores que atravessam salas, andares fundos como o 50): o gerador nunca pode travar nem produzir andar quebrado. → testes `invariantes em 1000 sementes` e `andar 50 gera` (Tarefa 3).
2. **Andar impossível** (chave atrás da comporta trancada, elevador inalcançável): → teste `chave alcançável sem passar pela tranca` (Tarefa 3).
3. **Segurar ou repetir E** (abrir comporta já abrindo, apertar E no elevador várias vezes): nada pode duplicar ou pular dois andares. → testes `usar duas vezes não desce dois andares` (Tarefa 4) e regra de evento de um passo (Tarefa 7).
4. **Troca de andar no meio da interpolação:** a câmera não pode "deslizar" pelo mapa do andar anterior até o novo início. → verificação na Tarefa 7 (zerar `anterior` quando `trocasDeAndar()` muda).
5. **Nascer em lugar ruim** (dentro de parede, de comporta ou em cima do elevador): → teste `início em célula de chão longe do elevador` (Tarefa 3).

---

## Estrutura de arquivos

```
src/mundo/Grade.h/.cpp        + Celula::ComportaAberta; carregarDeTexto aceita C, T, E, K
src/mundo/Zona.h/.cpp         Zona e zonaDoAndar
src/mundo/Consultas.h/.cpp    linha de visão, exploração, célula à frente, distâncias na grade
src/mundo/Gerador.h/.cpp      Sala, Andar, sementeDoAndar, gerarAndar, andarDeTexto
src/mundo/MapaFixo.*          removidos (e o teste "mapa fixo carrega")
src/jogo/Comandos.h           + usar
src/jogo/Partida.h/.cpp       estado da partida e regras de comporta, chave, elevador, mensagens
src/render/GeometriaMasmorra.*  camadas por zona; paredes só contra Parede
src/render/Comportas.h/.cpp   placas das comportas (puro)
src/render/Sprites.h/.cpp     sprites virados para a câmera (puro)
src/render/Automapa.h/.cpp    vértices coloridos do automapa (puro)
src/render/Desenho2D.h/.cpp   pipeline 2D colorido (GL) para o automapa
src/render/Zonas.h/.cpp       corAmbiente(Zona) (puro)
src/render/MalhaGPU.*         + enviar(..., dinamica)
src/plataforma/Entrada.*      E (evento) e Tab (alterna mapa)
src/main.cpp                  Partida, opções --semente --andar --mapa-completo --descrever
assets/shaders/cor2d.vert, cor2d.frag
assets/texturas/fabrica_*.png, minas_*.png, comporta.png, comporta_trancada.png
assets/sprites/valvula_chave.png, elevador.png
CMakeLists.txt                cópia de assets como alvo próprio (reaproveita a pendência da Etapa 1)
testes/teste_consultas.cpp, teste_gerador.cpp, teste_partida.cpp, teste_comportas.cpp,
testes/teste_sprites.cpp, teste_automapa.cpp (+ ajustes em teste_grade.cpp e teste_geometria.cpp)
```

`nucleo` recebe os novos `.cpp` de `mundo/`, `jogo/` e os puros de `render/` (`Comportas`, `Sprites`, `Automapa`, `Zonas`).

---

### Tarefa 1: Grade com comportas e elevador, zonas e geometria por zona

**Arquivos:**
- Modificar: `src/mundo/Grade.h/.cpp`, `src/render/GeometriaMasmorra.h/.cpp`, `src/render/Texturas.*` (nada além da lista maior), `CMakeLists.txt`
- Criar: `src/mundo/Zona.h/.cpp`
- Remover: `src/mundo/MapaFixo.h/.cpp` e o caso "mapa fixo da etapa 1 carrega"
- Testar: `testes/teste_grade.cpp`, `testes/teste_geometria.cpp`

**Interfaces:**
- Produz:
  - `enum class mundo::Celula : uint8_t { Parede, Chao, Comporta, ComportaTrancada, Elevador, ComportaAberta };` — `solida` continua verdadeiro só para `Parede`, `Comporta`, `ComportaTrancada`
  - `struct mundo::MapaCarregado { Grade grade; glm::vec2 inicio; std::optional<glm::vec2> chave; std::optional<glm::ivec2> elevador; };`
  - `carregarDeTexto` passa a aceitar `'C'` Comporta, `'T'` ComportaTrancada, `'E'` Elevador, `'K'` chão + chave (centro da célula)
  - `enum class mundo::Zona { Esgoto, Fabrica, Minas }; mundo::Zona mundo::zonaDoAndar(int andar);`
  - `enum render::CamadaTextura : int { ParedeEsgoto, ChaoEsgoto, TetoEsgoto, ParedeFabrica, ChaoFabrica, TetoFabrica, ParedeMinas, ChaoMinas, TetoMinas, CamadaComporta, CamadaComportaTrancada, TOTAL_CAMADAS };`
  - `int render::camadaParede(mundo::Zona z)`, `camadaChao(z)`, `camadaTeto(z)` (= `3 * z + 0/1/2`)
  - `std::vector<Vertice> render::construirGeometria(const mundo::Grade& g, mundo::Zona z)` — chão/teto em toda célula que não é `Parede` (comportas e elevador inclusos); faces de parede só contra vizinha `Parede`
  - CMake: alvo `copiar_assets` (ALL) que copia cada arquivo de `assets/` com `DEPENDS` nele (via `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`); `Roguelike` depende dele; tira a cópia de assets do `POST_BUILD`

- [ ] **Passo 1: Escrever/ajustar os testes**

```cpp
// teste_grade.cpp
TEST_CASE("carregarDeTexto lê comporta, tranca, elevador e chave") {
    auto m = mundo::carregarDeTexto({"#######", "#@CTKE#", "#######"});
    CHECK(m.grade.em(2, 1) == mundo::Celula::Comporta);
    CHECK(m.grade.em(3, 1) == mundo::Celula::ComportaTrancada);
    CHECK(m.grade.em(4, 1) == mundo::Celula::Chao);
    CHECK(m.chave == glm::vec2(4.5f, 1.5f));
    CHECK(m.elevador == glm::ivec2(5, 1));
    CHECK(m.grade.solida(2, 1));
    CHECK_FALSE(m.grade.solida(5, 1));
}
TEST_CASE("comporta aberta não é sólida") {
    mundo::Grade g(1, 1);
    g.definir(0, 0, mundo::Celula::ComportaAberta);
    CHECK_FALSE(g.solida(0, 0));
}
TEST_CASE("zona do andar") {
    CHECK(mundo::zonaDoAndar(1) == mundo::Zona::Esgoto);
    CHECK(mundo::zonaDoAndar(2) == mundo::Zona::Esgoto);
    CHECK(mundo::zonaDoAndar(3) == mundo::Zona::Fabrica);
    CHECK(mundo::zonaDoAndar(4) == mundo::Zona::Fabrica);
    CHECK(mundo::zonaDoAndar(5) == mundo::Zona::Minas);
    CHECK(mundo::zonaDoAndar(50) == mundo::Zona::Minas);
}

// teste_geometria.cpp — os casos antigos passam a chamar construirGeometria(grade, mundo::Zona::Esgoto)
TEST_CASE("comporta no corredor não gera parede entre ela e o corredor") {
    auto m = mundo::carregarDeTexto({"#####", "#@C.#", "#####"});
    // 3 células com chão e teto = 6 quadrados; paredes: célula 1 (cima, baixo, esquerda),
    // comporta (cima, baixo), célula 3 (cima, baixo, direita) = 8
    CHECK(render::construirGeometria(m.grade, mundo::Zona::Esgoto).size() == 14 * 6);
}
TEST_CASE("zona da fábrica usa as camadas da fábrica") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    for (const auto& v : render::construirGeometria(m.grade, mundo::Zona::Fabrica))
        CHECK((v.camada == float(render::ParedeFabrica) || v.camada == float(render::ChaoFabrica) ||
               v.camada == float(render::TetoFabrica)));
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação (`Zona`, `ComportaAberta`, novo `construirGeometria`).
- [ ] **Passo 3: Implementar** `Celula::ComportaAberta`, os novos caracteres, `Zona`, as camadas e o novo `construirGeometria`; remover `MapaFixo`; trocar a cópia de assets no CMake. O `main` continua compilando com um mapa provisório de texto até a Tarefa 7.
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.
- [ ] **Passo 5: Conferir a cópia de assets:** editar só um `.frag` (um comentário), compilar e conferir com `Get-FileHash` que a cópia em `build\Debug\assets` mudou; desfazer o comentário.

---

### Tarefa 2: Consultas na grade (linha de visão, exploração, célula à frente, distâncias)

**Arquivos:**
- Criar: `src/mundo/Consultas.h/.cpp`
- Testar: `testes/teste_consultas.cpp`

**Interfaces:**
- Consome: `mundo::Grade`, `carregarDeTexto` (Tarefa 1).
- Produz:
  - `bool mundo::temLinhaDeVisao(const Grade& g, glm::vec2 de, glm::vec2 para)` — percorre as células do segmento (Amanatides–Woo); falso se alguma célula sólida no caminho, **sem contar a célula de destino**; em empate exato dos dois eixos, conta as duas vizinhas (não enxerga entre duas paredes que se tocam pela quina)
  - `class mundo::Exploracao { Exploracao(int largura, int altura); bool vista(int x, int y) const; void marcar(int x, int y); void marcarTudo(); }` (fora da grade: `vista` falso, `marcar` ignora)
  - `constexpr float mundo::RAIO_EXPLORACAO = 6.0f;`
  - `void mundo::explorarAoRedor(Exploracao& e, const Grade& g, glm::vec2 posicao, float raio)` — marca cada célula cujo centro está a até `raio` e com linha de visão
  - `std::optional<glm::ivec2> mundo::celulaAFrente(const Grade& g, glm::vec2 posicao, float angulo, float alcance)` — anda pelo raio em passos de 0,05 a partir da posição (incluindo a célula do jogador); devolve a primeira célula cujo tipo não é `Chao` nem `ComportaAberta`
  - `std::vector<int> mundo::distanciasNaGrade(const Grade& g, glm::ivec2 origem, bool atravessaTrancada)` — busca em largura com 4 vizinhos; índice `y * largura + x`; `-1` = inalcançável; passa por tudo que não é `Parede` (e não é `ComportaTrancada`, se `atravessaTrancada` for falso)

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("linha de visão numa sala aberta") {
    auto m = mundo::carregarDeTexto({"######", "#@...#", "#....#", "######"});
    CHECK(mundo::temLinhaDeVisao(m.grade, {1.5f, 1.5f}, {4.5f, 2.5f}));
}
TEST_CASE("parede bloqueia a linha de visão") {
    auto m = mundo::carregarDeTexto({"#####", "#@#.#", "#####"});
    CHECK_FALSE(mundo::temLinhaDeVisao(m.grade, {1.5f, 1.5f}, {3.5f, 1.5f}));
}
TEST_CASE("enxerga a própria parede de destino") {
    auto m = mundo::carregarDeTexto({"####", "#@.#", "####"});
    CHECK(mundo::temLinhaDeVisao(m.grade, {1.5f, 1.5f}, {3.5f, 1.5f}));
}
TEST_CASE("não vê através de quina diagonal") {
    auto m = mundo::carregarDeTexto({"####", "#@##", "##.#", "####"});
    CHECK_FALSE(mundo::temLinhaDeVisao(m.grade, {1.5f, 1.5f}, {2.5f, 2.5f}));
}
TEST_CASE("comporta fechada bloqueia a visão") {
    auto m = mundo::carregarDeTexto({"#####", "#@C.#", "#####"});
    CHECK_FALSE(mundo::temLinhaDeVisao(m.grade, {1.5f, 1.5f}, {3.5f, 1.5f}));
}
TEST_CASE("exploração marca perto e com visão, não longe nem atrás de parede") {
    auto m = mundo::carregarDeTexto({"####################", "#@...#.............#", "####################"});
    mundo::Exploracao e(m.grade.largura(), m.grade.altura());
    mundo::explorarAoRedor(e, m.grade, m.inicio, mundo::RAIO_EXPLORACAO);
    CHECK(e.vista(4, 1));
    CHECK(e.vista(5, 1));        // a parede que fecha o corredor
    CHECK_FALSE(e.vista(6, 1));  // atrás dela
    CHECK_FALSE(e.vista(18, 1)); // longe
}
TEST_CASE("célula à frente encontra a comporta") {
    auto m = mundo::carregarDeTexto({"#####", "#@C.#", "#####"});
    CHECK(mundo::celulaAFrente(m.grade, m.inicio, 0.0f, 1.5f) == glm::ivec2(2, 1));
}
TEST_CASE("célula à frente ignora o que está atrás e devolve nada se só há chão") {
    auto m = mundo::carregarDeTexto({"#######", "#C@...#", "#######"});
    CHECK_FALSE(mundo::celulaAFrente(m.grade, m.inicio, 0.0f, 1.5f).has_value());
}
TEST_CASE("célula à frente inclui a célula do jogador (em cima do elevador)") {
    auto m = mundo::carregarDeTexto({"####", "#@E#", "####"});
    CHECK(mundo::celulaAFrente(m.grade, {2.5f, 1.5f}, 3.14159f, 1.5f) == glm::ivec2(2, 1));
}
TEST_CASE("distâncias respeitam a comporta trancada") {
    auto m = mundo::carregarDeTexto({"######", "#@.T.#", "######"});
    auto semTranca = mundo::distanciasNaGrade(m.grade, {1, 1}, false);
    auto comTranca = mundo::distanciasNaGrade(m.grade, {1, 1}, true);
    CHECK(semTranca[1 * 6 + 4] == -1);
    CHECK(comTranca[1 * 6 + 4] == 3);
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `Consultas`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 3: Gerador procedural

**Arquivos:**
- Criar: `src/mundo/Gerador.h/.cpp`
- Testar: `testes/teste_gerador.cpp`

**Interfaces:**
- Consome: `Grade`, `Zona`, `zonaDoAndar`, `carregarDeTexto` (Tarefa 1); `distanciasNaGrade` (Tarefa 2).
- Produz:
  - `constexpr int mundo::LADO_ANDAR = 64;`
  - `struct mundo::Sala { int x, y, largura, altura; glm::ivec2 centro() const; bool contem(int cx, int cy) const; };`
  - `struct mundo::Andar { int numero; Zona zona; Grade grade; std::vector<Sala> salas; glm::vec2 inicio; glm::ivec2 elevador; std::optional<glm::vec2> chave; };`
  - `uint32_t mundo::sementeDoAndar(uint32_t sementePartida, int numero)` — mistura determinística (ex.: `sementePartida ^ (numero * 0x9E3779B9u)` seguido de um passo de "splitmix32")
  - `Andar mundo::gerarAndar(uint32_t sementePartida, int numero)`
  - `Andar mundo::andarDeTexto(const std::vector<std::string>& linhas, int numero)` — usa `carregarDeTexto`; `salas` vazio; `elevador` = o `'E'` (ou `{-1, -1}` se não houver)

**Algoritmo** (decisões que o executor não adivinha):

```
rng = std::mt19937(sementeDoAndar(semente, numero)), variação v = 0
querTranca = numero >= 2 && uniforme(rng) < 0.4          // sorteado antes de tudo
repetir com v = 0..9 (cada v usa mt19937(sementeDoAndar(semente, numero) + v * 7919)):
  1. salas: até 500 tentativas de sala com lados 4..10 em posição aleatória dentro de [1, 62];
     aceita se o retângulo expandido em 1 célula (margem) não encosta em nenhuma sala aceita; para em N salas,
     com N sorteado em 8..15. Se terminar com menos de 8, aceita o que tiver (mínimo 2; o teste exige 8).
  2. árvore: Prim pelos centros (distância de Manhattan), começando na sala 0; para cada aresta, cavar corredor em L
     de centro a centro (sorteia se vai primeiro em X ou em Y). Cavar = trocar Parede por Chao.
  3. extras: 2..4 vezes, sorteia uma sala e liga à sala mais próxima que ainda não é vizinha na árvore.
  4. entradas: célula de Chao no anel (retângulo expandido em 1) de uma sala que tem Parede nos dois lados
     perpendiculares à passagem e Chao nos dois lados da passagem. Cada entrada vira Comporta com chance 0,5.
  5. início: sala 0; inicio = centro + 0,5. elevador: centro da sala com maior distância (distanciasNaGrade a partir
     do centro da sala 0, atravessaTrancada = true); marca a célula como Elevador.
  6. se querTranca: entradas da sala do elevador (regra do passo 4). Se houver exatamente uma, vira ComportaTrancada
     e confere com distanciasNaGrade(atravessaTrancada = false) que o elevador ficou inalcançável; se não houver
     exatamente uma, ou a conferência falhar, desfaz e tenta a próxima variação v.
     Sem querTranca, aceita na primeira variação.
  7. chave (só com tranca): sorteia uma sala alcançável sem a tranca, diferente da sala 0 e da sala do elevador
     (se não houver, usa a sala 0); chave = centro + 0,5.
se nenhuma variação conseguiu a tranca: devolve a variação 0 sem tranca.
```

- [ ] **Passo 1: Escrever os testes**

```cpp
static bool tudoAlcancavel(const mundo::Andar& a) {
    auto d = mundo::distanciasNaGrade(a.grade, glm::ivec2(a.inicio), true);
    for (int y = 0; y < a.grade.altura(); ++y)
        for (int x = 0; x < a.grade.largura(); ++x)
            if (a.grade.em(x, y) != mundo::Celula::Parede && d[y * a.grade.largura() + x] < 0) return false;
    return true;
}
static int contarTrancas(const mundo::Andar& a) { /* conta ComportaTrancada */ }

TEST_CASE("mesma semente gera o mesmo andar") {
    auto a = mundo::gerarAndar(1234, 2), b = mundo::gerarAndar(1234, 2);
    for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) REQUIRE(a.grade.em(x, y) == b.grade.em(x, y));
    CHECK(a.inicio == b.inicio);
}
TEST_CASE("sementes ou andares diferentes geram andares diferentes") {
    auto a = mundo::gerarAndar(1, 1), b = mundo::gerarAndar(2, 1), c = mundo::gerarAndar(1, 2);
    bool difAB = false, difAC = false;
    for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
        difAB |= a.grade.em(x, y) != b.grade.em(x, y);
        difAC |= a.grade.em(x, y) != c.grade.em(x, y);
    }
    CHECK(difAB); CHECK(difAC);
}
TEST_CASE("invariantes em 1000 sementes") {
    for (uint32_t s = 1; s <= 1000; ++s)
        for (int andar = 1; andar <= 3; ++andar) {
            auto a = mundo::gerarAndar(s, andar);
            INFO("semente " << s << " andar " << andar);
            REQUIRE(a.grade.largura() == 64);
            REQUIRE(a.salas.size() >= 8); REQUIRE(a.salas.size() <= 15);
            REQUIRE(tudoAlcancavel(a));
            REQUIRE(a.grade.em(a.elevador.x, a.elevador.y) == mundo::Celula::Elevador);
            REQUIRE(contarTrancas(a) <= 1);
            REQUIRE(a.chave.has_value() == (contarTrancas(a) == 1));
        }
}
TEST_CASE("início em célula de chão longe do elevador") {
    for (uint32_t s = 1; s <= 200; ++s) {
        auto a = mundo::gerarAndar(s, 1);
        REQUIRE(a.grade.em(int(a.inicio.x), int(a.inicio.y)) == mundo::Celula::Chao);
        REQUIRE(glm::distance(a.inicio, glm::vec2(a.elevador) + 0.5f) > 8.0f);
    }
}
TEST_CASE("chave alcançável sem passar pela tranca, elevador só com ela") {
    int comTranca = 0;
    for (uint32_t s = 1; s <= 500; ++s) {
        auto a = mundo::gerarAndar(s, 2);
        if (!a.chave) continue;
        ++comTranca;
        auto d = mundo::distanciasNaGrade(a.grade, glm::ivec2(a.inicio), false);
        auto idx = [&](glm::ivec2 c) { return c.y * 64 + c.x; };
        REQUIRE(d[idx(glm::ivec2(*a.chave))] >= 0);
        REQUIRE(d[idx(a.elevador)] < 0);
    }
    CHECK(comTranca > 500 * 0.25);   // alvo da spec 40%; algumas variações desistem
    CHECK(comTranca < 500 * 0.50);
}
TEST_CASE("andar 1 nunca tem tranca") {
    for (uint32_t s = 1; s <= 300; ++s) REQUIRE_FALSE(mundo::gerarAndar(s, 1).chave.has_value());
}
TEST_CASE("comportas ficam entre duas paredes") {
    for (uint32_t s = 1; s <= 200; ++s) {
        auto a = mundo::gerarAndar(s, 2);
        for (int y = 1; y < 63; ++y) for (int x = 1; x < 63; ++x) {
            auto c = a.grade.em(x, y);
            if (c != mundo::Celula::Comporta && c != mundo::Celula::ComportaTrancada) continue;
            using mundo::Celula;
            bool horizontal = a.grade.em(x, y - 1) == Celula::Parede && a.grade.em(x, y + 1) == Celula::Parede;
            bool vertical = a.grade.em(x - 1, y) == Celula::Parede && a.grade.em(x + 1, y) == Celula::Parede;
            REQUIRE((horizontal || vertical));
        }
    }
}
TEST_CASE("andar 50 gera na zona das minas") {
    auto a = mundo::gerarAndar(77, 50);
    CHECK(a.zona == mundo::Zona::Minas);
    CHECK(a.numero == 50);
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `Gerador`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa; anotar o tempo de `testes_roguelike.exe` (deve ficar abaixo de ~5 s em Debug; se passar, rodar as 1000 sementes só em Release com `#ifdef NDEBUG` e 100 em Debug, registrando a decisão).

---

### Tarefa 4: Partida (comportas, chave, elevador, exploração, mensagens)

**Arquivos:**
- Modificar: `src/jogo/Comandos.h` (+ `bool usar = false;`)
- Criar: `src/jogo/Partida.h/.cpp`
- Testar: `testes/teste_partida.cpp`

**Interfaces:**
- Consome: `Jogador`, `atualizarJogador` (Etapa 1); `Andar`, `gerarAndar`, `andarDeTexto` (Tarefa 3); `Exploracao`, `explorarAoRedor`, `celulaAFrente` (Tarefa 2).
- Produz:
  - `constexpr float jogo::TEMPO_ABRIR_COMPORTA = 0.5f, jogo::ALCANCE_USAR = 1.5f, jogo::RAIO_PEGAR = 0.6f, jogo::DURACAO_MENSAGEM = 2.0f;`
  - `class jogo::Partida`
    - `Partida(uint32_t semente, int andarInicial = 1);` (gera com `gerarAndar`)
    - `Partida(mundo::Andar andar, uint32_t semente);` (para testes; o próximo andar sai de `gerarAndar(semente, andar.numero + 1)`)
    - `void atualizar(const Comandos& c, float dt);`
    - `const mundo::Andar& andar() const; const Jogador& jogador() const; bool temChave() const;`
    - `float aberturaComporta(int x, int y) const;` (0 fechada … 1 aberta; `ComportaAberta` = 1)
    - `const mundo::Exploracao& exploracao() const; void revelarMapa();`
    - `std::string_view mensagem() const;` (vazia quando não há)
    - `int trocasDeAndar() const; uint32_t semente() const;`
  - Ordem dentro de `atualizar`: mover jogador → se `usar`, agir sobre `celulaAFrente(…, ALCANCE_USAR)` → avançar comportas → pegar chave → explorar ao redor → contar tempo da mensagem.
  - Ações do usar: `Comporta` começa a abrir (se já abrindo, nada); `ComportaTrancada` com chave vira `Comporta`, começa a abrir, gasta a chave, mensagem **"Comporta destrancada"**; sem chave, mensagem **"Comporta trancada: encontre a válvula-chave"**; `Elevador` desce (novo andar, jogador no início com o mesmo ângulo, sem chave, comportas e exploração zeradas, `trocasDeAndar` + 1, mensagem **"Andar N"**).
  - Pegar a chave: mensagem **"Você pegou a válvula-chave"**.

- [ ] **Passo 1: Escrever os testes**

```cpp
static void passos(jogo::Partida& p, int n, jogo::Comandos c = {}) {
    for (int i = 0; i < n; ++i) { p.atualizar(c, 1.0f / 60); c.usar = false; c.giro = 0; }
}

TEST_CASE("E abre a comporta à frente em 0,5 s") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@C.#", "#####"}, 1), 1);
    jogo::Comandos usar; usar.usar = true;
    passos(p, 15, usar);                                     // 0,25 s
    CHECK(p.aberturaComporta(2, 1) == doctest::Approx(0.5f).epsilon(0.05));
    CHECK(p.andar().grade.solida(2, 1));                     // ainda fechada para passar
    passos(p, 16);
    CHECK(p.andar().grade.em(2, 1) == mundo::Celula::ComportaAberta);
    passos(p, 60, {1, 0, 0});
    CHECK(p.jogador().posicao.x > 3.0f);                     // atravessou
}
TEST_CASE("comporta trancada sem chave não abre e avisa") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@T.#", "#####"}, 2), 1);
    jogo::Comandos usar; usar.usar = true;
    passos(p, 60, usar);
    CHECK(p.andar().grade.em(2, 1) == mundo::Celula::ComportaTrancada);
    CHECK(p.mensagem() == "Comporta trancada: encontre a válvula-chave");
}
TEST_CASE("passar pela chave pega a válvula-chave") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#@.K..#", "#######"}, 2), 1);
    passos(p, 60, {1, 0, 0});
    CHECK(p.temChave());
    CHECK_FALSE(p.andar().chave.has_value());
    CHECK(p.mensagem() == "Você pegou a válvula-chave");
}
TEST_CASE("com a chave, E destranca e abre") {
    jogo::Partida p(mundo::andarDeTexto({"#######", "#@K.T.#", "#######"}, 2), 1);
    passos(p, 40, {1, 0, 0});                                // anda até encostar
    jogo::Comandos usar; usar.usar = true;
    passos(p, 1, usar);
    CHECK_FALSE(p.temChave());
    CHECK(p.mensagem() == "Comporta destrancada");
    passos(p, 40);
    CHECK(p.andar().grade.em(4, 1) == mundo::Celula::ComportaAberta);
}
TEST_CASE("E no elevador desce para o próximo andar") {
    jogo::Partida p(mundo::andarDeTexto({"####", "#@E#", "####"}, 1), 99);
    jogo::Comandos usar; usar.usar = true;
    passos(p, 1, usar);
    CHECK(p.andar().numero == 2);
    CHECK(p.trocasDeAndar() == 1);
    CHECK(p.jogador().posicao == p.andar().inicio);
    CHECK(p.andar().grade.largura() == 64);                  // veio do gerador
}
TEST_CASE("usar duas vezes no mesmo passo de troca não desce dois andares") {
    jogo::Partida p(mundo::andarDeTexto({"####", "#@E#", "####"}, 1), 99);
    jogo::Comandos usar; usar.usar = true;
    passos(p, 1, usar);
    passos(p, 1, usar);                                       // novo andar: o início fica longe do elevador
    CHECK(p.andar().numero == 2);
}
TEST_CASE("exploração marca as células ao redor depois de um passo") {
    jogo::Partida p(mundo::andarDeTexto({"####################", "#@.................#", "####################"}, 1), 1);
    passos(p, 1);
    CHECK(p.exploracao().vista(4, 1));
    CHECK_FALSE(p.exploracao().vista(15, 1));
}
TEST_CASE("mensagem some depois de 2 s") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@T.#", "#####"}, 2), 1);
    jogo::Comandos usar; usar.usar = true;
    passos(p, 1, usar);
    passos(p, 121);
    CHECK(p.mensagem().empty());
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `Partida`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 5: Desenho das comportas, sprites e cor das zonas

**Arquivos:**
- Criar: `src/render/Comportas.h/.cpp`, `src/render/Sprites.h/.cpp`, `src/render/Zonas.h/.cpp`
- Modificar: `src/render/MalhaGPU.h/.cpp` (+ `dinamica`)
- Testar: `testes/teste_comportas.cpp`, `testes/teste_sprites.cpp`

**Interfaces:**
- Consome: `Vertice`, `CamadaTextura` (Tarefa 1); `mundo::Grade`, `mundo::Zona`.
- Produz:
  - `std::vector<Vertice> render::construirComportas(const mundo::Grade& g, const std::function<float(int, int)>& abertura)` — para cada `Comporta`/`ComportaTrancada`, duas faces (uma para cada lado) no plano do meio da célula, perpendicular à passagem; base em `Y = abertura`, topo em `Y = 1 + abertura`; camada `CamadaComporta` ou `CamadaComportaTrancada`. `ComportaAberta` não gera nada.
  - `enum render::CamadaSprite : int { SpriteValvulaChave = 0, SpriteElevador = 1, TOTAL_SPRITES };`
  - `struct render::Sprite { glm::vec2 posicao; int camada; float largura; float altura; };`
  - `std::vector<Vertice> render::construirSprites(const std::vector<Sprite>& s, glm::vec2 camera)` — um quadrado por sprite, de pé no chão (`Y` de 0 a `altura`), centrado na posição, perpendicular à direção sprite→câmera no plano XZ; normal apontando para a câmera
  - `glm::vec3 render::corAmbiente(mundo::Zona z)` (valores das Restrições globais)
  - `void MalhaGPU::enviar(const std::vector<Vertice>& v, bool dinamica = false)` (`GL_DYNAMIC_DRAW` quando verdadeiro)
  - Tamanhos dos sprites: válvula-chave 0,4 × 0,4; elevador 0,9 × 1,0.

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("comporta fechada gera duas faces no meio da célula") {
    auto m = mundo::carregarDeTexto({"#####", "#@C.#", "#####"});
    auto v = render::construirComportas(m.grade, [](int, int) { return 0.0f; });
    REQUIRE(v.size() == 12);
    for (const auto& p : v) CHECK(p.posicao.x == doctest::Approx(2.5f));   // passagem em X → plano X = 2,5
}
TEST_CASE("comporta em passagem vertical fica no plano Z") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "#C#", "#.#", "###"});
    for (const auto& p : render::construirComportas(m.grade, [](int, int) { return 0.0f; }))
        CHECK(p.posicao.z == doctest::Approx(2.5f));
}
TEST_CASE("comporta meio aberta sobe metade") {
    auto m = mundo::carregarDeTexto({"#####", "#@C.#", "#####"});
    float menorY = 10;
    for (const auto& p : render::construirComportas(m.grade, [](int, int) { return 0.5f; })) menorY = std::min(menorY, p.posicao.y);
    CHECK(menorY == doctest::Approx(0.5f));
}
TEST_CASE("comporta aberta não aparece") {
    auto m = mundo::carregarDeTexto({"#####", "#@C.#", "#####"});
    m.grade.definir(2, 1, mundo::Celula::ComportaAberta);
    CHECK(render::construirComportas(m.grade, [](int, int) { return 1.0f; }).empty());
}
TEST_CASE("sprite fica de frente para a câmera") {
    auto v = render::construirSprites({{{2.0f, 0.0f}, render::SpriteElevador, 0.9f, 1.0f}}, {0.0f, 0.0f});
    REQUIRE(v.size() == 6);
    for (const auto& p : v) {
        CHECK(p.posicao.x == doctest::Approx(2.0f));
        CHECK(p.normal.x == doctest::Approx(-1.0f));
        CHECK(std::abs(p.posicao.z) <= 0.45f + 1e-4f);
        CHECK(p.posicao.y >= 0.0f);
        CHECK(p.posicao.y <= 1.0f + 1e-4f);
    }
}
TEST_CASE("cor ambiente por zona") {
    CHECK(render::corAmbiente(mundo::Zona::Esgoto) == glm::vec3(0.10f, 0.09f, 0.06f));
    CHECK(render::corAmbiente(mundo::Zona::Fabrica) == glm::vec3(0.12f, 0.07f, 0.03f));
    CHECK(render::corAmbiente(mundo::Zona::Minas) == glm::vec3(0.05f, 0.07f, 0.11f));
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `Comportas`, `Sprites`, `Zonas` e o parâmetro `dinamica`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 6: Automapa

**Arquivos:**
- Criar: `src/render/Automapa.h/.cpp` (puro), `src/render/Desenho2D.h/.cpp` (GL), `assets/shaders/cor2d.vert`, `assets/shaders/cor2d.frag`
- Testar: `testes/teste_automapa.cpp`

**Interfaces:**
- Consome: `Grade`, `Exploracao` (Tarefa 2).
- Produz:
  - `struct render::VerticeCor { glm::vec2 posicao; glm::vec3 cor; };` (pixels da tela baixa, origem no canto de cima à esquerda)
  - `constexpr int render::PIXELS_POR_CELULA = 4;`
  - `std::vector<VerticeCor> render::construirAutomapa(const mundo::Grade& g, const mundo::Exploracao& e, glm::vec2 posJogador, float angulo, std::optional<glm::vec2> chave, int telaL, int telaA)`
    - painel preto do tamanho da grade (64 × 4 = 256 px) centralizado: origem `((telaL − 256) / 2, (telaA − 256) / 2)` = `(112, 7)` em 480×270;
    - um quadrado por célula vista que não é `Parede`: chão `(0.35, 0.33, 0.30)`, comporta/aberta `(0.80, 0.60, 0.20)`, trancada `(0.80, 0.20, 0.20)`, elevador `(0.30, 0.80, 0.40)`;
    - chave (se existir e a célula dela estiver vista): quadrado `(0.30, 0.80, 0.90)`;
    - jogador por último: triângulo branco apontando na direção do ângulo, centrado na posição (lado ~6 px).
  - `class render::Desenho2D { Desenho2D(); void desenhar(const std::vector<VerticeCor>& v, int telaL, int telaA); }` — projeção ortográfica com Y para baixo, sem profundidade

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("nada explorado: só o painel e o jogador") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    mundo::Exploracao e(3, 3);
    CHECK(render::construirAutomapa(m.grade, e, m.inicio, 0.0f, {}, 480, 270).size() == 6 + 3);
}
TEST_CASE("célula de chão vista vira um quadrado") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    mundo::Exploracao e(3, 3);
    e.marcarTudo();                                    // paredes vistas não desenham nada
    CHECK(render::construirAutomapa(m.grade, e, m.inicio, 0.0f, {}, 480, 270).size() == 6 + 6 + 3);
}
TEST_CASE("jogador aparece na posição certa do mapa") {
    auto m = mundo::carregarDeTexto({"####", "#.@#", "####"});
    mundo::Exploracao e(4, 3);
    auto v = render::construirAutomapa(m.grade, e, m.inicio, 0.0f, {}, 480, 270);
    glm::vec2 centro = (v[v.size() - 1].posicao + v[v.size() - 2].posicao + v[v.size() - 3].posicao) / 3.0f;
    CHECK(centro.x == doctest::Approx(112 + 2.5f * 4).epsilon(0.05));
    CHECK(centro.y == doctest::Approx(7 + 1.5f * 4).epsilon(0.05));
}
TEST_CASE("chave só aparece se a célula dela foi vista") {
    auto m = mundo::carregarDeTexto({"#####", "#@.K#", "#####"});
    mundo::Exploracao e(5, 3);
    auto semVer = render::construirAutomapa(m.grade, e, m.inicio, 0.0f, m.chave, 480, 270).size();
    e.marcar(3, 1);
    auto vendo = render::construirAutomapa(m.grade, e, m.inicio, 0.0f, m.chave, 480, 270).size();
    CHECK(vendo == semVer + 6 + 6);                    // chão da célula + marca da chave
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `construirAutomapa`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.
- [ ] **Passo 5: Implementar `Desenho2D` e os shaders `cor2d`.**

---

### Tarefa 7: Entrada, opções e laço principal com a Partida

**Arquivos:**
- Modificar: `src/plataforma/Entrada.h/.cpp`, `src/main.cpp`

**Interfaces:**
- Consome: `Partida` (Tarefa 4), `construirGeometria`, `construirComportas`, `construirSprites`, `corAmbiente`, `MalhaGPU` (Tarefas 1 e 5), `construirAutomapa`, `Desenho2D` (Tarefa 6).
- Produz:
  - `Entrada`: `E` (`SDL_KEYDOWN` sem repetição, só com o mouse capturado) liga `usar` no próximo `coletar()`, que o zera; `Tab` alterna `bool mapaVisivel() const`
  - Opções: `--semente <n>` (sem ela: `std::random_device`), `--andar <n>` (andar inicial), `--mapa-completo` (revela o mapa e já abre o automapa; para capturas), `--descrever` (gera o andar, grava no `log.txt` uma linha `semente S andar N: zona Z, K salas, tranca sim/não, chave (x, z), elevador (x, z)` e sai sem abrir janela). `--captura` e `--angulo` continuam.
  - O `log.txt` registra a semente da partida ao começar.
  - Título: `"Roguelike — Andar N"` + `"  |  " + mensagem` quando houver + o aviso de clique quando o mouse está solto; `SDL_SetWindowTitle` só quando o texto muda.
  - Quando `trocasDeAndar()` muda: remontar a malha da masmorra, atualizar `uCorAmbiente` e zerar a interpolação (`anterior = jogador`).
  - Por quadro: comportas e sprites (válvula-chave se existir; elevador) reconstruídos e enviados como malha dinâmica; sprites com o array de sprites ligado; automapa desenhado na tela baixa por cima da cena quando `mapaVisivel()`.

- [ ] **Passo 1: Implementar a `Entrada` e o novo `main`.**
- [ ] **Passo 2: Compilar e rodar `Roguelike.exe --descrever --semente 1 --andar 2`.** Esperado: uma linha no `log.txt` e saída sem janela.
- [ ] **Passo 3: Capturas** `--semente 1 --captura capturas\e2_inicio.bmp` e `--semente 1 --mapa-completo --captura capturas\e2_mapa.bmp`. Esperado: andar gerado com o xadrez rosa nas texturas que ainda faltam; automapa com salas, corredores, comportas em amarelo e elevador em verde.

---

### Tarefa 8: Arte das zonas, comportas e sprites

**Arquivos:**
- Criar: `assets/texturas/fabrica_parede.png`, `fabrica_chao.png`, `fabrica_teto.png`, `minas_parede.png`, `minas_chao.png`, `minas_teto.png`, `comporta.png`, `comporta_trancada.png`, `assets/sprites/valvula_chave.png`, `assets/sprites/elevador.png`
- Modificar: `assets/LICENCAS.txt`

- [ ] **Passo 1: Fábrica e comportas:** escolher no Zortch (`arte_fonte/zortch_world_textures.zip`) chapa rebitada/canos (parede), piso de ferro (chão), teto metálico e duas portas (`bigdoor*.png`) bem diferentes para comporta e comporta trancada (a trancada mais avermelhada ou com faixa de aviso). Reduzir para 64×64.
- [ ] **Passo 2: Minas:** escolher no Tiny Texture Pack 1 rocha/pedra escura (Brick 23 ou 25 para parede, Tile 24 ou 25 para chão, Brick 23 para teto). Reduzir para 64×64.
- [ ] **Passo 3: Sprites:** os pacotes já baixados não têm válvula nem gaiola de elevador. Gerar sprites provisórios 64×64 com fundo transparente por um script `ferramentas\gerar_sprites.ps1` (System.Drawing): válvula de latão (roda com raios) e gaiola de elevador (moldura de ferro com grade). Marcar como **provisórios** no `LICENCAS.txt`. Se o Nidragnnar quiser arte de terceiros, procurar e pedir ok antes de baixar.
- [ ] **Passo 4: Capturas** com `--andar 1`, `--andar 3` e `--andar 5` (mesma semente) e uma olhando para o elevador e a chave. Esperado: cada zona com textura e cor de luz próprias, sem xadrez rosa.

---

### Tarefa 9: Verificação, cenários de teste, revisão e entrega

- [ ] **Passo 1: Compilar em Release e rodar os testes.** Esperado: tudo passa, sem avisos novos.
- [ ] **Passo 2: Catálogo de sementes:** rodar `--descrever` para as sementes 1–100 nos andares 2 e 3 e escolher: (a) uma semente com comporta trancada no andar 2; (b) uma sem tranca; (c) uma para ver cada zona (`--andar 1/3/5`).
- [ ] **Passo 3: Robustez:** apertar E repetidamente no elevador (desce um andar por aperto); trocar de andar e conferir que a câmera não desliza; abrir com a pasta de trabalho em `C:\`; capturas do automapa.
- [ ] **Passo 4: Revisão independente** do conjunto (Foco de revisão + Decisões de projeto) e correção dos achados importantes com teste.
- [ ] **Passo 5: Deixar o jogo aberto para o Nidragnnar** com a tabela de cenários (comandos prontos com `--semente`/`--andar`) e o que testar em cada um: abrir comporta, comporta trancada sem e com chave, descer de elevador, zonas, automapa.
- [ ] **Passo 6: Perguntar "Depois de testar, faço o commit e envio?"** e, com o "sim", commitar (autor Nidragnnar, mensagem em português, sem atribuição ao Claude).

---

## Próximas etapas

Etapa 3 (inimigos e combate básico) reaproveita `temLinhaDeVisao`, `distanciasNaGrade`, `Sprite`/`construirSprites` e a `Partida`; o plano dela sai depois do teste desta.
