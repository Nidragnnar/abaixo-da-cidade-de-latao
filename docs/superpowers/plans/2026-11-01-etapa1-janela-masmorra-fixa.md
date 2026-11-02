# Etapa 1 — Janela e masmorra fixa: Plano de implementação

> **Para agentes:** SUB-SKILL OBRIGATÓRIA: usar superpowers:subagent-driven-development (recomendado) ou superpowers:executing-plans para executar este plano tarefa por tarefa. Os passos usam caixas (`- [ ]`) para acompanhamento.

**Objetivo:** abrir uma janela com uma masmorra escrita à mão em 3D (texturas de esgoto, lanterna, neblina, visual 480×270 ampliado) pela qual o jogador anda com WASD + mouse, com colisão nas paredes.

**Arquitetura:** a lógica pura (`src/mundo`, `src/jogo` e as partes de `src/render` que só fazem contas) vira a biblioteca estática `nucleo`, testada com doctest sem abrir janela. O executável junta `nucleo` com `src/plataforma` (SDL2) e os arquivos de `src/render` que chamam OpenGL. A lógica roda em passo fixo de 60 Hz e o desenho interpola a posição do jogador.

**Tecnologia:** C++20, MSVC 14.51 (Visual Studio Community 2026), CMake 4.3 (o que vem com o VS) com o gerador "Visual Studio 18 2026", SDL2 2.32.x, glad (OpenGL 3.3 core), glm 1.0.1, stb_image, doctest 2.4.11.

**Spec:** `docs/superpowers/specs/2026-11-01-roguelike-design.md` (seções 2, 3 "Desenho", 4 "Controles", 11 e 13, etapa 1)

## Restrições globais

- C++20; MSVC com `/W4 /utf-8`; runtime estático: `CMAKE_MSVC_RUNTIME_LIBRARY = "MultiThreaded$<$<CONFIG:Debug>:Debug>"`.
- Versões fixas: SDL2 = tag `release-2.32.x` mais recente no GitHub (conferir antes; registrar a tag escolhida); glm `1.0.1`; doctest `v2.4.11`; stb_image do commit atual de `nothings/stb` (registrar o hash em `assets/LICENCAS.txt`); glad gerado para OpenGL 3.3 core, sem extensões.
- `src/mundo` e `src/jogo` não incluem SDL nem OpenGL (glm pode).
- Nomes de tipos, funções e arquivos em português, no estilo da spec (`Grade`, `Celula`, `Jogador`); comentários curtos em português.
- Constantes da spec: framebuffer **480×270**; FOV **horizontal 75°**; altura dos olhos **0,5**; parede com **1** de altura; raio do jogador **0,25**; alcance da lanterna **≈ 6 células**; neblina chega ao preto em **≈ 12 células**; lógica a **60 Hz**; V-Sync ligado.
- Constantes desta etapa (não fixadas pela spec; ajustar após o teste do Nidragnnar): velocidade de andar **3,0 células/s**; sensibilidade do mouse **0,0025 rad/pixel**; janela inicial **1280×720**.
- Coordenadas: a célula `(x, y)` da grade ocupa o mundo `X ∈ [x, x+1]`, `Z ∈ [y, y+1]`; `Y` é para cima; chão em `Y = 0` e teto em `Y = 1`. Ângulo `0` olha para `+X`; ângulo crescente gira para a direita (rumo a `+Z`). Frente = `(cos a, sin a)`, direita = `(−sin a, cos a)` no plano `(X, Z)`.
- `log.txt` fica ao lado do `.exe`; arquivos de arte são procurados a partir da pasta do `.exe` (`SDL_GetBasePath`), nunca da pasta de trabalho.
- **Downloads** (bibliotecas, glad, stb, arte): só depois do ok do Nidragnnar. **Commit:** só depois do "sim" do Nidragnnar; por isso esta etapa termina com **um único commit** (Tarefa 9), no lugar de commits por tarefa.

## Foco de revisão

1. **Janela minimizada, redimensionada ou com escala de tela de 125%/150%:** o jogo não pode quebrar nem dividir por zero, e a imagem precisa ocupar a área real em pixels (`SDL_GL_GetDrawableSize`), com faixas pretas. → teste `letterbox com janela zerada` (Tarefa 7) + verificação manual (Tarefa 9).
2. **Alt+Tab ou perda de foco:** o mouse é solto, o jogo pausa e não "gira sozinho" na volta; um clique na janela recaptura o mouse. → verificação manual (Tarefa 5).
3. **Travada longa** (arrastar a janela, PC lento): o passo fixo não pode entrar em espiral, e o jogador não pode atravessar parede com um passo grande. → testes `passo fixo limita travadas` (Tarefa 2) e `deslocamento grande não atravessa parede` (Tarefa 4).
4. **Abrir o `.exe` por atalho ou de outra pasta:** texturas e shaders ainda precisam ser encontrados. → verificação na Tarefa 9 (rodar com a pasta de trabalho em `C:\`).
5. **Textura faltando ou com tamanho errado; placa sem OpenGL 3.3; shader com erro:** textura xadrez rosa + linha no `log.txt`; nos casos fatais, caixa de mensagem clara em vez de fechar em silêncio. → teste `xadrez de reserva` (Tarefa 6) + verificação na Tarefa 9 (renomear uma textura).

---

## Estrutura de arquivos

```
CMakeLists.txt                 alvos nucleo, Roguelike, testes_roguelike; copia SDL2.dll e assets ao lado do exe
CMakePresets.json              preset "vs" (gerador Visual Studio 18 2026, pasta build/), build/test debug e release
cmake/Dependencias.cmake       FetchContent de SDL2, glm, doctest
.gitignore                     build/, capturas/, log.txt
externo/glad/                  glad.c, include/glad/glad.h, include/KHR/khrplatform.h (OpenGL 3.3 core)
externo/stb/stb_image.h
src/main.cpp                   inicialização, laço principal, --captura
src/plataforma/Log.h/.cpp      log.txt
src/plataforma/Caminhos.h/.cpp pasta do exe e caminho de assets
src/plataforma/Janela.h/.cpp   SDL + contexto OpenGL 3.3 + glad + caixa de mensagem de erro
src/plataforma/Entrada.h/.cpp  eventos SDL → jogo::Comandos; mouse relativo; foco
src/jogo/PassoFixo.h/.cpp      acumulador de 60 Hz
src/jogo/Comandos.h            intenção do jogador num passo
src/jogo/Jogador.h/.cpp        estado e movimento do jogador
src/mundo/Grade.h/.cpp         células do andar
src/mundo/Colisao.h/.cpp       movimento com deslize nas paredes
src/mundo/MapaFixo.h/.cpp      masmorra escrita à mão desta etapa
src/render/Camera.h/.cpp       matrizes de visão e projeção (puro)
src/render/GeometriaMasmorra.h/.cpp  grade → lista de vértices (puro)
src/render/Letterbox.h/.cpp    área da imagem ampliada dentro da janela (puro)
src/render/Xadrez.h/.cpp       textura de reserva (puro)
src/render/Shader.h/.cpp       compilar programa GLSL
src/render/Texturas.h/.cpp     carregar PNGs num GL_TEXTURE_2D_ARRAY
src/render/MalhaGPU.h/.cpp     VAO/VBO da masmorra
src/render/TelaBaixa.h/.cpp    framebuffer 480×270 e ampliação
assets/shaders/masmorra.vert, masmorra.frag, tela.vert, tela.frag
assets/texturas/esgoto_parede.png, esgoto_chao.png, esgoto_teto.png
assets/LICENCAS.txt
testes/main.cpp, teste_passo_fixo.cpp, teste_grade.cpp, teste_colisao.cpp, teste_jogador.cpp,
testes/teste_camera.cpp, teste_geometria.cpp, teste_letterbox.cpp, teste_xadrez.cpp
```

Biblioteca `nucleo` = `src/mundo/*`, `src/jogo/*`, `src/render/{Camera,GeometriaMasmorra,Letterbox,Xadrez}.cpp` (depende só de glm).

Comandos usados em todas as tarefas (pasta `Z:\Roguelike`; `cmake`/`ctest` = os do Visual Studio, em `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\`):

- Configurar: `cmake --preset vs`
- Compilar: `cmake --build --preset debug`
- Testar: `ctest --preset debug --output-on-failure`

---

### Tarefa 1: Projeto, dependências e janela vazia

**Arquivos:**
- Criar: `CMakeLists.txt`, `CMakePresets.json`, `cmake/Dependencias.cmake`, `.gitignore`, `externo/glad/*`, `externo/stb/stb_image.h`, `assets/LICENCAS.txt`
- Criar: `src/main.cpp`, `src/plataforma/Log.h/.cpp`, `src/plataforma/Caminhos.h/.cpp`, `src/plataforma/Janela.h/.cpp`
- Testar: `testes/main.cpp` (só `#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` + include)

**Interfaces:**
- Produz:
  - `void plataforma::iniciarLog(const std::filesystem::path& arquivo)`; `void plataforma::logar(std::string_view mensagem)` (uma linha, com hora, `flush` a cada linha)
  - `std::filesystem::path plataforma::pastaDoExecutavel()` (via `SDL_GetBasePath`); `std::filesystem::path plataforma::caminhoAsset(std::string_view relativo)` = pasta do exe / `"assets"` / relativo
  - `class plataforma::Janela { Janela(const char* titulo, int largura, int altura); ~Janela(); SDL_Window* sdl() const; void trocarBuffers(); glm::ivec2 tamanhoDesenhavel() const; }` — cria o contexto OpenGL 3.3 core, carrega o glad, liga o V-Sync (`SDL_GL_SetSwapInterval(1)`); se falhar, lança `std::runtime_error` com a mensagem para o jogador
  - `void plataforma::mostrarErroFatal(const std::string& mensagem)` (`SDL_ShowSimpleMessageBox` + `logar`)

- [ ] **Passo 1: Pedir ok ao Nidragnnar para os downloads** — SDL2 (GitHub libsdl-org, via FetchContent), glm, doctest, `stb_image.h` e o glad (gerado em `https://gen.glad.dav1d.de/`: linguagem C/C++, especificação gl, API gl 3.3, perfil core, sem extensões, com "generate loader"). Sem o ok, parar aqui.
- [ ] **Passo 2: Rodar `git init` e criar o `.gitignore`.** Nenhum commit.
- [ ] **Passo 3: Escrever `CMakeLists.txt`, `CMakePresets.json` e `cmake/Dependencias.cmake`.** Detalhes que o executor não adivinha:
  - definir `CMAKE_MSVC_RUNTIME_LIBRARY` **antes** do `FetchContent_MakeAvailable`;
  - SDL2 como DLL (`SDL_SHARED ON`, `SDL_STATIC OFF`, testes do SDL desligados); se o CMake 4 recusar o `cmake_minimum_required` antigo de alguma dependência, definir `CMAKE_POLICY_VERSION_MINIMUM 3.5`;
  - `Roguelike` como `WIN32_EXECUTABLE` ligado a `SDL2::SDL2main`;
  - pós-compilação: copiar `$<TARGET_FILE:SDL2::SDL2>` e a pasta `assets/` para o lado do exe;
  - `enable_testing()` + `add_test(NAME testes COMMAND testes_roguelike)`;
  - presets: configure `vs` (gerador `Visual Studio 18 2026`, arquitetura `x64`, `binaryDir` `build`); build e test `debug` e `release`.
- [ ] **Passo 4: Escrever `Log`, `Caminhos`, `Janela` e um `main.cpp` provisório** que abre a janela 1280×720 "Roguelike", limpa a tela com `(0.05, 0.04, 0.03)` e fecha no X da janela. `main` envolve tudo em `try/catch` → `mostrarErroFatal`.
- [ ] **Passo 5: Configurar, compilar e testar.** Esperado: o configure baixa as dependências; a compilação sai sem erros; `ctest` informa `100% tests passed` (zero casos ainda); o `log.txt` mostra a versão do OpenGL.
- [ ] **Passo 6: Abrir `build\Debug\Roguelike.exe`.** Esperado: janela marrom escura; fecha no X.

---

### Tarefa 2: Passo fixo

**Arquivos:**
- Criar: `src/jogo/PassoFixo.h/.cpp`
- Testar: `testes/teste_passo_fixo.cpp`

**Interfaces:**
- Produz: `class jogo::PassoFixo { explicit PassoFixo(double passo = 1.0 / 60.0, double maxAcumulado = 0.25); int adicionar(double dtReal); double alfa() const; double passo() const; void zerar(); }` — `adicionar` soma o tempo real (limitado a `maxAcumulado`), devolve quantos passos lógicos rodar e guarda a sobra; `alfa()` = sobra / passo, em `[0, 1)`, para interpolar o desenho; `zerar()` descarta a sobra (usado na pausa).

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("passo fixo conta passos de 1/60") {
    jogo::PassoFixo p;
    CHECK(p.adicionar(0.051) == 3);
    CHECK(p.alfa() == doctest::Approx(0.06).epsilon(0.01));
}
TEST_CASE("passo fixo acumula sobra entre quadros") {
    jogo::PassoFixo p;
    CHECK(p.adicionar(0.010) == 0);
    CHECK(p.adicionar(0.010) == 1);
}
TEST_CASE("passo fixo limita travadas") {
    jogo::PassoFixo p;
    CHECK(p.adicionar(10.0) == 15);   // 0,25 s no máximo
}
TEST_CASE("zerar descarta a sobra") {
    jogo::PassoFixo p;
    p.adicionar(0.010);
    p.zerar();
    CHECK(p.adicionar(0.010) == 0);
}
```

- [ ] **Passo 2: Rodar `ctest`.** Esperado: falha de compilação (`PassoFixo` não existe).
- [ ] **Passo 3: Implementar `PassoFixo`.**
- [ ] **Passo 4: Rodar `ctest`.** Esperado: tudo passa.

---

### Tarefa 3: Grade e mapa fixo

**Arquivos:**
- Criar: `src/mundo/Grade.h/.cpp`, `src/mundo/MapaFixo.h/.cpp`
- Testar: `testes/teste_grade.cpp`

**Interfaces:**
- Produz:
  - `enum class mundo::Celula : uint8_t { Parede, Chao, Comporta, ComportaTrancada, Elevador };`
  - `class mundo::Grade { Grade(int largura, int altura); int largura() const; int altura() const; Celula em(int x, int y) const; void definir(int x, int y, Celula c); bool solida(int x, int y) const; }` — o construtor preenche tudo com `Parede`; `em` fora da grade devolve `Parede`; `solida` é verdadeiro para `Parede`, `Comporta` e `ComportaTrancada`
  - `struct mundo::MapaCarregado { Grade grade; glm::vec2 inicio; };`
  - `MapaCarregado mundo::carregarDeTexto(const std::vector<std::string>& linhas)` — `'#'` parede, `'.'` chão, `'@'` chão + início (centro da célula); linhas de tamanhos diferentes, caractere desconhecido ou nenhum/mais de um `'@'` lançam `std::invalid_argument`
  - `std::vector<std::string> mundo::mapaFixoEtapa1()`

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("carregarDeTexto lê paredes, chão e início") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "#.#", "###"});
    CHECK(m.grade.largura() == 3);
    CHECK(m.grade.altura() == 4);
    CHECK(m.grade.em(1, 2) == mundo::Celula::Chao);
    CHECK(m.grade.solida(0, 0));
    CHECK(m.inicio == glm::vec2(1.5f, 1.5f));
}
TEST_CASE("fora da grade é parede") {
    mundo::Grade g(2, 2);
    CHECK(g.em(-1, 0) == mundo::Celula::Parede);
    CHECK(g.solida(5, 5));
}
TEST_CASE("texto inválido é recusado") {
    CHECK_THROWS_AS(mundo::carregarDeTexto({"###", "#@"}), std::invalid_argument);
    CHECK_THROWS_AS(mundo::carregarDeTexto({"#x#"}), std::invalid_argument);
    CHECK_THROWS_AS(mundo::carregarDeTexto({"#.#"}), std::invalid_argument);
}
TEST_CASE("mapa fixo da etapa 1 carrega") {
    CHECK_NOTHROW(mundo::carregarDeTexto(mundo::mapaFixoEtapa1()));
}
```

- [ ] **Passo 2: Rodar `ctest`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `Grade`, `carregarDeTexto` e `mapaFixoEtapa1`** com este mapa (24×16: sala inicial, sala de pilares, corredores com ciclo, beco sem saída):

```
########################
#......####............#
#......####.##.##.##...#
#...@..####............#
#......####.##.##.##...#
#......####............#
###.#######.############
###.#######.############
###.........######...###
#########.#.######.#.###
#########.#........#.###
#########.##########.###
#.......#.##########...#
#....................#.#
#.......#############..#
########################
```

- [ ] **Passo 4: Rodar `ctest`.** Esperado: tudo passa.

---

### Tarefa 4: Colisão e movimento do jogador

**Arquivos:**
- Criar: `src/mundo/Colisao.h/.cpp`, `src/jogo/Comandos.h`, `src/jogo/Jogador.h/.cpp`
- Testar: `testes/teste_colisao.cpp`, `testes/teste_jogador.cpp`

**Interfaces:**
- Consome: `mundo::Grade`, `mundo::carregarDeTexto` (Tarefa 3).
- Produz:
  - `glm::vec2 mundo::moverComColisao(const Grade& grade, glm::vec2 posicao, glm::vec2 deslocamento, float raio)`
  - `struct jogo::Comandos { float frente = 0; float lado = 0; float giro = 0; };` — `frente` e `lado` em `[-1, 1]` (lado positivo = direita); `giro` em radianos, já multiplicado pela sensibilidade
  - `struct jogo::Jogador { glm::vec2 posicao; float angulo = 0; };`
  - `constexpr float jogo::VELOCIDADE_ANDAR = 3.0f; constexpr float jogo::RAIO_JOGADOR = 0.25f;`
  - `void jogo::atualizarJogador(Jogador& j, const Comandos& c, const mundo::Grade& g, float dt)` — aplica o giro, normaliza o vetor (frente, lado) se o comprimento passar de 1 e move com `moverComColisao`

- [ ] **Passo 1: Escrever os testes de colisão**

```cpp
// Sala 4×1 aberta entre paredes: colunas 1..4 livres, parede na coluna 5.
static const std::vector<std::string> CORREDOR = {"#######", "#@...##", "#######"};

TEST_CASE("para encostado na parede") {
    auto m = mundo::carregarDeTexto(CORREDOR);
    auto p = mundo::moverComColisao(m.grade, {4.0f, 1.5f}, {1.0f, 0.0f}, 0.25f);
    CHECK(p.x == doctest::Approx(4.75f).epsilon(0.002));
    CHECK(p.y == doctest::Approx(1.5f));
}
TEST_CASE("deslocamento grande não atravessa parede") {
    auto m = mundo::carregarDeTexto(CORREDOR);
    auto p = mundo::moverComColisao(m.grade, {1.5f, 1.5f}, {20.0f, 0.0f}, 0.25f);
    CHECK(p.x < 5.0f);
}
TEST_CASE("desliza ao longo da parede") {
    auto m = mundo::carregarDeTexto({"#####", "#@..#", "#...#", "#####"});
    auto p = mundo::moverComColisao(m.grade, {1.5f, 1.5f}, {0.5f, -1.0f}, 0.25f);
    CHECK(p.x == doctest::Approx(2.0f));          // andou em X
    CHECK(p.y == doctest::Approx(1.25f).epsilon(0.002)); // parou em Y na parede de cima
}
TEST_CASE("passa por corredor de uma célula sem enroscar na quina") {
    auto m = mundo::carregarDeTexto({"#####", "#@..#", "##.##", "##.##", "#####"});
    glm::vec2 p{2.4f, 1.5f};
    for (int i = 0; i < 60; ++i) p = mundo::moverComColisao(m.grade, p, {0.0f, 0.05f}, 0.25f);
    CHECK(p.y > 3.0f);
}
```

- [ ] **Passo 2: Escrever os testes do jogador**

```cpp
TEST_CASE("anda para a frente na direção do ângulo") {
    auto m = mundo::carregarDeTexto({"##########", "#@.......#", "##########"});
    jogo::Jogador j{m.inicio, 0.0f};
    for (int i = 0; i < 60; ++i) jogo::atualizarJogador(j, {1, 0, 0}, m.grade, 1.0f / 60);
    CHECK(j.posicao.x == doctest::Approx(4.5f).epsilon(0.01));   // 3 células em 1 s
}
TEST_CASE("lado positivo anda para a direita (+Z com ângulo 0)") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "#.#", "#.#", "###"});
    jogo::Jogador j{m.inicio, 0.0f};
    jogo::atualizarJogador(j, {0, 1, 0}, m.grade, 0.1f);
    CHECK(j.posicao.y > 1.5f);
}
TEST_CASE("diagonal não é mais rápida") {
    auto m = mundo::carregarDeTexto({"########", "#@.....#", "#......#", "#......#", "########"});
    jogo::Jogador j{m.inicio, 0.0f};
    jogo::atualizarJogador(j, {1, 1, 0}, m.grade, 0.1f);
    CHECK(glm::length(j.posicao - m.inicio) == doctest::Approx(0.3f).epsilon(0.01));
}
TEST_CASE("giro altera o ângulo") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    jogo::Jogador j{m.inicio, 0.0f};
    jogo::atualizarJogador(j, {0, 0, 0.5f}, m.grade, 1.0f / 60);
    CHECK(j.angulo == doctest::Approx(0.5f));
}
```

- [ ] **Passo 3: Rodar `ctest`.** Esperado: falha de compilação.
- [ ] **Passo 4: Implementar `moverComColisao`** com este algoritmo (caixa do tamanho do círculo, como Doom):

```
dividir o deslocamento em N = ceil(|d| / raio) subpassos iguais
para cada subpasso:
    eixo X: x' = x + dx; se a caixa [x'−r, x'+r] × [y−r, y+r] tocar célula sólida:
            dx > 0 → x' = (coluna da célula sólida) − r − 0,001
            dx < 0 → x' = (coluna da célula sólida + 1) + r + 0,001
    eixo Y: o mesmo com a posição já corrigida em X
```

- [ ] **Passo 5: Implementar `atualizarJogador`.**
- [ ] **Passo 6: Rodar `ctest`.** Esperado: tudo passa.

---

### Tarefa 5: Entrada e laço principal

**Arquivos:**
- Criar: `src/plataforma/Entrada.h/.cpp`
- Modificar: `src/main.cpp`

**Interfaces:**
- Consome: `jogo::Comandos`, `jogo::Jogador`, `jogo::atualizarJogador`, `jogo::PassoFixo`, `mundo::carregarDeTexto`, `mundo::mapaFixoEtapa1`.
- Produz: `class plataforma::Entrada { explicit Entrada(SDL_Window* janela); void processarEvento(const SDL_Event& e); jogo::Comandos coletar(); bool capturado() const; bool pediuSair() const; }`
  - WASD lidos pelo estado do teclado; o giro acumula o `xrel` do mouse × 0,0025 desde o último `coletar()`, que zera o acumulado;
  - clique na janela → `SDL_SetRelativeMouseMode(SDL_TRUE)`; `Esc` ou `SDL_WINDOWEVENT_FOCUS_LOST` → solta o mouse; `capturado()` diz se o mouse está preso;
  - `SDL_QUIT` → `pediuSair()`.

- [ ] **Passo 1: Implementar `Entrada`.**
- [ ] **Passo 2: Reescrever o laço do `main`:** eventos → `Entrada`; se `capturado()`: `n = passo.adicionar(dt)` e, para cada passo, `atualizarJogador(jogador, entrada.coletar(), grade, passo.passo())` (o giro do mouse vai inteiro no primeiro passo e os demais recebem giro 0); senão `passo.zerar()` e descartar o giro acumulado. Guardar a posição e o ângulo anteriores para interpolar com `alfa()`. Até a Tarefa 6, escrever no título da janela a posição e o ângulo.
- [ ] **Passo 3: Compilar e rodar.** Esperado: clicar prende o mouse; WASD e o mouse mudam a posição e o ângulo no título; a posição não atravessa as paredes do mapa; com Alt+Tab o mouse é solto, os números param e, na volta, o ângulo não pula.

---

### Tarefa 6: Desenho 3D da masmorra (câmera, geometria, texturas, shader)

**Arquivos:**
- Criar: `src/render/Camera.h/.cpp`, `GeometriaMasmorra.h/.cpp`, `Xadrez.h/.cpp`, `Shader.h/.cpp`, `Texturas.h/.cpp`, `MalhaGPU.h/.cpp`, `assets/shaders/masmorra.vert`, `assets/shaders/masmorra.frag`
- Modificar: `src/main.cpp`
- Testar: `testes/teste_camera.cpp`, `testes/teste_geometria.cpp`, `testes/teste_xadrez.cpp`

**Interfaces:**
- Consome: `mundo::Grade`, `jogo::Jogador`, `plataforma::caminhoAsset`, `plataforma::logar`.
- Produz:
  - `constexpr float render::ALTURA_OLHOS = 0.5f; constexpr float render::FOV_HORIZONTAL_GRAUS = 75.0f;`
  - `glm::mat4 render::matrizVisao(glm::vec2 posicao, float angulo)` — olho `(x, 0.5, z)`, olhando para `(cos a, 0, sin a)`, cima `(0, 1, 0)`
  - `glm::mat4 render::matrizProjecao(float aspecto)` — FOV vertical = `2·atan(tan(37,5°) / aspecto)`, perto 0,05, longe 64
  - `enum render::CamadaTextura : int { ParedeEsgoto = 0, ChaoEsgoto = 1, TetoEsgoto = 2 };`
  - `struct render::Vertice { glm::vec3 posicao; glm::vec2 uv; float camada; glm::vec3 normal; };`
  - `std::vector<Vertice> render::construirGeometria(const mundo::Grade& g)` — para cada célula não sólida: um quadrado de chão (`Y = 0`, normal `+Y`), um de teto (`Y = 1`, normal `−Y`) e uma face de parede para cada vizinha sólida (normal apontando para a célula aberta); 2 triângulos = 6 vértices por quadrado; UV de 0 a 1 por face
  - `std::vector<uint8_t> render::gerarXadrez(int lado)` — RGBA, quadrados de 8 px alternando `#FF00FF` e `#000000`, começando pelo rosa em (0, 0)
  - `GLuint render::compilarPrograma(const std::filesystem::path& vert, const std::filesystem::path& frag)` — lança `std::runtime_error` com o log do GLSL
  - `GLuint render::carregarArrayTexturas(const std::vector<std::filesystem::path>& arquivos)` — cada PNG precisa ser 64×64 (forçar RGBA com stb); se faltar ou tiver outro tamanho, usar `gerarXadrez(64)` e `logar`; filtro `GL_NEAREST`, `GL_REPEAT`
  - `class render::MalhaGPU { void enviar(const std::vector<Vertice>& v); void desenhar() const; }`

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("célula isolada gera 4 paredes, chão e teto") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    CHECK(render::construirGeometria(m.grade).size() == 6 * 6);
}
TEST_CASE("duas células vizinhas não geram parede entre elas") {
    auto m = mundo::carregarDeTexto({"####", "#@.#", "####"});
    CHECK(render::construirGeometria(m.grade).size() == 10 * 6);   // 6 paredes + 2 chãos + 2 tetos
}
TEST_CASE("normal da parede aponta para a célula aberta") {
    auto m = mundo::carregarDeTexto({"###", "#@#", "###"});
    for (const auto& v : render::construirGeometria(m.grade))
        if (v.camada == render::ParedeEsgoto) {
            glm::vec3 centro{1.5f, 0.5f, 1.5f};
            CHECK(glm::dot(v.normal, centro - v.posicao) > 0.0f);
        }
}
TEST_CASE("projeção tem FOV horizontal de 75 graus") {
    auto proj = render::matrizProjecao(16.0f / 9.0f);
    float t = std::tan(glm::radians(37.5f));
    glm::vec4 borda = proj * glm::vec4(t, 0.0f, -1.0f, 1.0f);   // espaço da câmera
    CHECK(borda.x / borda.w == doctest::Approx(1.0f).epsilon(0.001));
}
TEST_CASE("câmera com ângulo 0 olha para +X") {
    auto v = render::matrizVisao({2.0f, 3.0f}, 0.0f);
    glm::vec4 frente = v * glm::vec4(3.0f, 0.5f, 3.0f, 1.0f);
    CHECK(frente.z < 0.0f);                                   // à frente da câmera
    CHECK(frente.x == doctest::Approx(0.0f).epsilon(0.001));
}
TEST_CASE("xadrez de reserva") {
    auto px = render::gerarXadrez(64);
    REQUIRE(px.size() == 64 * 64 * 4);
    CHECK(px[0] == 255); CHECK(px[1] == 0); CHECK(px[2] == 255); CHECK(px[3] == 255);
    CHECK(px[8 * 4] == 0);   // pixel (8, 0) é preto
}
```

- [ ] **Passo 2: Rodar `ctest`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `Camera`, `GeometriaMasmorra` e `Xadrez`.**
- [ ] **Passo 4: Rodar `ctest`.** Esperado: tudo passa.
- [ ] **Passo 5: Implementar `Shader`, `Texturas` e `MalhaGPU`, e escrever os shaders.** Uniforms: `uVisao`, `uProjecao`, `uTexturas` (`sampler2DArray`), `uPosLanterna` (vec3 = olho do jogador), `uCorLanterna = (1.0, 0.75, 0.45)`, `uAlcanceLanterna = 6.0`, `uCorAmbiente = (0.10, 0.09, 0.06)` (esgoto), `uFimNeblina = 12.0`. Fórmula do fragmento:

```glsl
float d = distance(vPos, uPosLanterna);
float aten = pow(clamp(1.0 - d / uAlcanceLanterna, 0.0, 1.0), 2.0);
float lambert = max(dot(normalize(vNormal), normalize(uPosLanterna - vPos)), 0.0);
vec3 luz = uCorAmbiente + uCorLanterna * aten * (0.35 + 0.65 * lambert);
vec4 tex = texture(uTexturas, vec3(vUv, vCamada));
if (tex.a < 0.5) discard;
vec3 cor = tex.rgb * luz;
cor = mix(cor, vec3(0.0), clamp(d / uFimNeblina, 0.0, 1.0));
```

- [ ] **Passo 6: Ligar no `main`:** profundidade ligada, descarte de faces traseiras, desenho direto na janela (a tela baixa entra na Tarefa 7), câmera com posição e ângulo interpolados. Erro de shader → `mostrarErroFatal`.
- [ ] **Passo 7: Compilar e rodar.** Esperado: a masmorra em 3D com xadrez rosa (ainda sem arte); andar pelo mapa; luz em volta e preto ao longe.

---

### Tarefa 7: Tela 480×270 ampliada e captura

**Arquivos:**
- Criar: `src/render/Letterbox.h/.cpp`, `src/render/TelaBaixa.h/.cpp`, `assets/shaders/tela.vert`, `assets/shaders/tela.frag`
- Modificar: `src/main.cpp`
- Testar: `testes/teste_letterbox.cpp`

**Interfaces:**
- Consome: `plataforma::Janela::tamanhoDesenhavel()`, `render::compilarPrograma`.
- Produz:
  - `struct render::Retangulo { int x, y, largura, altura; };`
  - `Retangulo render::calcularLetterbox(int janelaL, int janelaA, int baseL, int baseA)` — maior retângulo com a proporção da base, centralizado; janela com largura ou altura ≤ 0 → `{0, 0, 0, 0}`
  - `class render::TelaBaixa { TelaBaixa(int largura = 480, int altura = 270); void comecar(); void apresentar(glm::ivec2 tamanhoDesenhavel); }` — FBO com cor `GL_RGBA8` (filtro `GL_NEAREST`) + profundidade; `apresentar` limpa a janela de preto e desenha a textura num quadrado no retângulo do letterbox; com retângulo zerado, não desenha nada
  - opção de linha de comando `--captura <arquivo.bmp>`: depois de 30 quadros, ler a janela (`glReadPixels`), inverter as linhas, salvar com `SDL_SaveBMP` e fechar

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("letterbox 16:9 numa tela 16:9 ocupa tudo") {
    auto r = render::calcularLetterbox(1920, 1080, 480, 270);
    CHECK(r.x == 0); CHECK(r.y == 0); CHECK(r.largura == 1920); CHECK(r.altura == 1080);
}
TEST_CASE("letterbox numa tela 16:10 põe faixas em cima e embaixo") {
    auto r = render::calcularLetterbox(1920, 1200, 480, 270);
    CHECK(r.x == 0); CHECK(r.y == 60); CHECK(r.largura == 1920); CHECK(r.altura == 1080);
}
TEST_CASE("letterbox numa tela 4:3 põe faixas nas laterais") {
    auto r = render::calcularLetterbox(1024, 768, 480, 270);
    CHECK(r.largura == 1024); CHECK(r.altura == 576); CHECK(r.y == 96);
}
TEST_CASE("letterbox com janela zerada") {
    auto r = render::calcularLetterbox(0, 0, 480, 270);
    CHECK(r.largura == 0); CHECK(r.altura == 0);
}
```

- [ ] **Passo 2: Rodar `ctest`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `calcularLetterbox`.**
- [ ] **Passo 4: Rodar `ctest`.** Esperado: tudo passa.
- [ ] **Passo 5: Implementar `TelaBaixa`, os shaders da tela e `--captura`; ligar no `main`** (a masmorra é desenhada no FBO com viewport 480×270; a projeção usa aspecto 480/270).
- [ ] **Passo 6: Rodar `Roguelike.exe --captura capturas\etapa1.bmp` e abrir a imagem.** Esperado: pixels grandes e nítidos, sem borrão; proporção correta; maximizar e minimizar a janela não trava.

---

### Tarefa 8: Texturas de esgoto

**Arquivos:**
- Criar: `assets/texturas/esgoto_parede.png`, `esgoto_chao.png`, `esgoto_teto.png`
- Modificar: `assets/LICENCAS.txt`

- [ ] **Passo 1: Procurar em OpenGameArt.org e Kenney.nl** texturas de tijolo úmido, pedra molhada e teto de pedra/grade, preferindo **CC0** (CC-BY se não houver CC0), em pixel art de 64×64 ou que fiquem boas reduzidas para 64×64.
- [ ] **Passo 2: Mostrar ao Nidragnnar** as opções (link, licença, autor, prévia) e esperar o ok antes de baixar.
- [ ] **Passo 3: Baixar** para uma pasta temporária vazia, recortar/reduzir para 64×64 PNG RGBA e copiar para `assets/texturas/` com os nomes acima.
- [ ] **Passo 4: Registrar em `assets/LICENCAS.txt`:** arquivo, autor, licença, link e alteração feita.
- [ ] **Passo 5: Rodar com `--captura capturas\etapa1.bmp` e conferir a imagem.** Esperado: sem xadrez rosa; tijolos legíveis de perto e somem na neblina.

---

### Tarefa 9: Verificação da etapa, entrega para teste e commit

- [ ] **Passo 1: Compilar em Release** (`cmake --build --preset release`) **e rodar `ctest --preset release`.** Esperado: zero avisos novos do `/W4` no nosso código; todos os testes passando.
- [ ] **Passo 2: Verificações de robustez** (Foco de revisão), cada uma com resultado anotado:
  - abrir `build\Release\Roguelike.exe` com a pasta de trabalho em `C:\` (ex.: `Start-Process -WorkingDirectory C:\`) → a masmorra aparece com texturas;
  - renomear `esgoto_chao.png`, abrir → chão em xadrez rosa e linha no `log.txt`; desfazer a renomeação;
  - corromper `masmorra.frag` numa cópia da pasta de saída → caixa de mensagem com o erro; restaurar;
  - capturas em 1280×720 e em tela maximizada.
- [ ] **Passo 3: Deixar `build\Release\Roguelike.exe` aberto para o Nidragnnar**, com esta tabela:

| O que testar | Como | Esperado |
|---|---|---|
| Andar e olhar | clicar na janela, WASD + mouse | movimento suave, sem tremer, em qualquer taxa de quadros |
| Colisão | andar contra paredes, quinas e pilares; entrar no corredor de 1 célula | desliza na parede, não atravessa nem trava na quina |
| Clima | olhar o corredor comprido e a sala de pilares | luz quente perto, escuro ao longe |
| Visual retrô | olhar a parede bem de perto | pixels grandes e nítidos |
| Foco | Alt+Tab e voltar; Esc | mouse solto, jogo para; clique recaptura sem pulo na câmera |
| Janela | maximizar, redimensionar, minimizar | faixas pretas corretas, nada trava |
| Velocidade e sensibilidade | sensação geral | dizer se 3 células/s e a sensibilidade estão boas |

- [ ] **Passo 4: Perguntar "Depois de testar, faço o commit e envio?"** e esperar o "sim".
- [ ] **Passo 5: Com o "sim", fazer o commit** (autor Nidragnnar, mensagem em português, sem atribuição ao Claude):

```bash
git add .
git commit -m "Etapa 1: janela SDL2/OpenGL, masmorra fixa em 3D com colisão, lanterna e neblina"
```

Envio (push) só se já houver um repositório remoto e com o ok explícito do Nidragnnar.

---

## Próximas etapas

Os planos das etapas 2 a 5 da spec (seção 13) são escritos um de cada vez, depois que o Nidragnnar testar a etapa anterior, aproveitando os ajustes pedidos (velocidade, sensibilidade, visual).
