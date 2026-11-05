# Etapa 6A — Terreno com alturas, pulo e iluminação: Plano de implementação

> **Para agentes:** SUB-SKILL OBRIGATÓRIA: usar superpowers:subagent-driven-development (recomendado) ou superpowers:executing-plans para executar este plano tarefa por tarefa. Os passos usam caixas (`- [ ]`) para acompanhamento.

**Objetivo:** lampiões nas salas e corredores; depois, andares com degraus, rampas, plataformas, poços e caixas, com pulo, dano de queda, inimigos que seguem o terreno e visão e tiros em 3D.

**Arquitetura:**
- O terreno continua sendo a grade 64×64. Cada célula ganha altura de chão e de teto (em quartos de bloco), uma direção de rampa e um "detalhe" (caixa, escada).
- Tudo o que pergunta "dá para passar / ver / acertar" passa por um único módulo puro, `mundo/Terreno`, usado pelo jogador, pelos inimigos, pela IA, pelas armas e pelo gerador.
- O gerador mantém o desenho 2D de hoje e acrescenta alturas, detalhes e luzes em sorteios separados (`mt19937` próprio). Assim os andares de hoje mudam só na vertical.
- O `render/` desenha o que a grade diz.

**Tecnologia:** a mesma da v1 (C++20, SDL2, SDL_mixer, OpenGL 3.3, glm, doctest).

**Spec:** `docs/superpowers/specs/2026-11-03-etapa6a-terreno-alturas-design.md` (inteira) e a geral `2026-11-01-roguelike-design.md`.

## Restrições globais

- Tudo da v1 continua valendo. Em especial:
  - testes que gravam placar usam `--dados`;
  - programas de teste próprios são compilados em Release;
  - roteiros automáticos usam `SDL_AUDIODRIVER=dummy`;
  - poucas execuções curtas com janela, porque o Nidragnnar está usando a máquina.
- **Unidade de altura:** `QUARTO = 0,25` bloco; alturas guardadas em quartos (inteiros).
- **Chão:** de 0 a 12 quartos (0 a 3 blocos). **Teto:** até 20 quartos (5 blocos), com `teto ≥ chão + 5` nos andares gerados.
- **Mapas de texto sem camada de altura:** chão 0 e teto 4 (o mundo de hoje).
- **Rampa:** sobe 4 quartos (1 bloco) ao longo da célula, nas direções N, S, L ou O.
- **Corpo:** olhos a `pés + 0,5`; corpo de 0,7.
- **Física:**
  - gravidade 24 blocos/s²;
  - velocidade do pulo `√(2·24·1,1) ≈ 7,27` (cerca de 1,1 bloco), só com os pés no chão;
  - degrau automático de até 1 quarto.
- **Queda:** até 2 blocos sem dano; acima, **5 de dano por meio bloco a mais**, arredondando para baixo (2,5 → 5; 3 → 10; 4 → 20). Ignora a defesa. A causa no placar é "Caiu de muito alto".
- **Inimigos:**
  - não levam dano de queda;
  - os **andarilhos** sobem 1 quarto e descem qualquer altura;
  - o **Rato** sobe 4 quartos (1 bloco), pulando.
- **Mesmo nível:** diferença de altura dos pés **< 0,75** para corpo a corpo, pegar item, separação e empurrão.
- **Caminho obrigatório (a pé):** sobe no máximo 1 quarto por célula e desce no máximo 8 quartos (2 blocos); nunca pula.
- **Gerador:**
  - nível de sala de 0 a 3 blocos; no **andar 1**, de 0 a 1 bloco;
  - **esgoto**: desnível de até 1 bloco;
  - teto da sala: nível + 2 a 3 blocos, limitado a 5;
  - teto do corredor: chão + 1,5 bloco;
  - comportas e elevador em trecho plano;
  - poços com saída e fora da sala inicial;
  - **garantias testadas em 1.000 sementes**.
- **Luzes:**
  - salas: 1 luz nas pequenas, 2 a 3 nas grandes;
  - corredores: uma a cada 8 a 10 células;
  - presas na parede, 0,8 acima do chão local;
  - alcance 5, com atenuação até zero;
  - tremulação de ±10%;
  - 16 luzes mais próximas por quadro;
  - cores: esgoto amarelada, fábrica alaranjada, minas branca-amarelada.
- **Desempenho:** 60 quadros por segundo no `--demo` em Release.
- **Arte nova** (caixa, degrau, lampião): primeiro dos pacotes já baixados (Tiny Texture Pack e Zortch); na falta, provisória por script. Sem downloads novos sem ok.
- **Commits:** um por tarefa concluída, só com o "sim" do Nidragnnar. A 6A inteira pode virar vários commits se ele preferir; o padrão é **um commit no fim**, como nas etapas anteriores.

## Decisões de projeto

- **Sorteios separados:** alturas, detalhes e luzes usam `mt19937` próprios, derivados da semente do andar. O desenho 2D (salas, corredores, comportas, tranca, elevador, chave) fica idêntico ao da v1, então os testes e cenários de hoje continuam válidos.
- **Alturas dos corredores:** cada sala tem um "platô" (o interior, a entrada e uma célula além dela) no nível da sala. Uma busca a partir de todos os platôs dá a cada célula de corredor `h = min(nível da sala + distância em células)`, em quartos. Isso garante subida máxima de 1 quarto por célula (escada). Se duas salas próximas têm níveis incompatíveis com a distância entre elas, a mais alta é rebaixada até caber (repetindo até estabilizar).
- **Rampas ficam nas salas.** Elas aparecem nos acessos de plataformas e nas saídas de poços. Nos corredores, a diferença de nível vira **escada** (1 quarto por célula), porque uma rampa de 1 bloco por célula num corredor em L exigiria trechos retos garantidos que o gerador de hoje não oferece. *É um desvio da spec (seção 4 cita "rampas ou escadas" nos corredores): avisar o Nidragnnar na entrega.*
- **Os detalhes são validados um a um.** Cada plataforma, poço ou caixa só fica se o caminho a pé até a chave e o elevador continuar existindo; senão, é desfeito.
- **Lampião sempre visível:** os sprites de lampião vão numa malha própria, desenhada com `uEmissivo = 1` (sem lanterna e com só 30% da neblina).
- **Campos de fluxo:** o campo é calculado a partir do jogador, então a busca percorre as arestas **ao contrário**. Ela vai de B para A quando quem anda pode ir de A para B.

## Foco de revisão

1. **Pular encostado numa quina ou sob teto baixo:** não atravessa a parede nem fica preso dentro do chão ou do teto. → teste "pulo na quina sob teto baixo" (Tarefa 3).
2. **Descer no elevador no meio de um pulo ou queda:** o andar novo começa com os pés no chão do início, velocidade vertical zero e sem dano de queda herdado. → teste "descer no ar zera a queda" (Tarefa 3).
3. **Inimigo empurrado pela borda (martelo):** cai até o chão de baixo e continua a perseguir, sem flutuar. → teste "empurrado da plataforma cai e persegue" (Tarefa 5).
4. **Atirador no alto atirando em quem está colado embaixo da borda:** o projétil bate na borda e não atravessa. → teste "borda protege de quem atira de cima" (Tarefa 5).
5. **Largar um item em cima de uma caixa ou plataforma, ou na beira de um poço:** o item fica no chão real onde caiu e só é pego desse nível. → teste "item largado na plataforma fica no alto" (Tarefa 6).

---

## Estrutura de arquivos

```
src/mundo/Grade.h/.cpp        + chão, teto, rampa e detalhe por célula; carregarDeTexto com camadas de altura
src/mundo/Terreno.h/.cpp      NOVO: alturaDoChao/Teto (contínuas, com rampas), podePassar, distanciasComAltura,
                              temLinhaDeVisao3D, danoDeQueda, física vertical comum (CorpoVertical)
src/mundo/Colisao.h/.cpp      moverComColisao com altura dos pés
src/mundo/Luzes.h/.cpp        NOVO: struct Luz, gerarLuzes
src/mundo/Gerador.h/.cpp      Andar + luzes + pontos secretos; alturas, detalhes, garantias
src/jogo/Jogador.*            pés, velocidade vertical, pulo, queda
src/jogo/Comandos.h           + pular
src/jogo/Inimigos.h, IA.*     altura, física, dois campos, Rato pula, mesmo nível
src/jogo/Projeteis.*          posição 3D
src/jogo/Armas.*              tiro da altura dos olhos, golpe no mesmo nível
src/jogo/Partida.*            dano de queda, pegar no mesmo nível, luzes do andar
src/jogo/Itens.cpp            itens nos pontos secretos
src/jogo/Demo.*               campo a pé com alturas
src/render/Luzes.h/.cpp       NOVO: luzesMaisProximas, tremulacao
src/render/GeometriaMasmorra.* chão/teto em altura, faces de degrau e beiral, rampas, UV em unidades de mundo
src/render/Camera.*, Sprites.*, Automapa.*, Icones.h   altura nos olhos e sprites, tons por altura, lampião
assets/shaders/masmorra.frag  luzes pontuais + emissivo
src/plataforma/Entrada.cpp    Espaço
src/main.cpp                  liga tudo
ferramentas/                  arte provisória se faltar (caixa, degrau, lampião)
testes/teste_terreno.cpp, teste_luzes.cpp (+ casos em teste_grade, teste_colisao, teste_jogador, teste_ia,
teste_projeteis, teste_armas, teste_partida, teste_gerador, teste_geometria, teste_demo, teste_automapa)
```

---

### Tarefa 1: Lampiões (luzes do andar e desenho)

**Arquivos:**
- Criar: `src/mundo/Luzes.h/.cpp`, `src/render/Luzes.h/.cpp`, `testes/teste_luzes.cpp`
- Modificar: `src/mundo/Gerador.h/.cpp` (`Andar::luzes`), `assets/shaders/masmorra.frag`, `src/render/Icones.h` (camada do lampião), `src/main.cpp`, `assets/LICENCAS.txt`, `CMakeLists.txt`

**Interfaces:**
- Produz:
  ```cpp
  namespace mundo {
  struct Luz { glm::vec3 posicao; glm::vec3 cor; float alcance; float fase; };
  constexpr float ALCANCE_LUZ = 5.0f, ALTURA_LUZ = 0.8f;
  // Salas e corredores do andar; usa a altura do chão da célula (Tarefa 2 a preenche; antes dela, 0)
  std::vector<Luz> gerarLuzes(const Andar& a, uint32_t semente);
  }
  namespace render {
  constexpr int MAXIMO_LUZES = 16;
  std::vector<mundo::Luz> luzesMaisProximas(const std::vector<mundo::Luz>& luzes, glm::vec3 ponto);  // até 16, da mais perto
  float tremulacao(float tempo, float fase);  // 0,9 … 1,1
  }
  ```
- `Andar` ganha `std::vector<Luz> luzes`, preenchido em `gerarAndar` (vazio em `andarDeTexto`).
- **Cor da luz:** esgoto `(1,0; 0,85; 0,5)`, fábrica `(1,0; 0,6; 0,3)`, minas `(1,0; 0,92; 0,75)`.
- **Uniforms novos no shader:**
  - `uLuzPos[16]`, `uLuzCor[16]` (cor já × tremulação), `uNumLuzes`, `uAlcanceLuzes`;
  - `uEmissivo` (1: cor = textura e neblina × 0,3).
  - **Atenuação:** igual à da lanterna (`pow(1 − d/alcance, 2)` × (0,35 + 0,65·lambert)).
- **Sala:** área < 30 células → 1 luz; senão 2 ou 3. Ficam em células da sala vizinhas a parede, empurradas 0,45 em direção à parede, em `chão + 0,8`.
- **Corredores** (chão fora de salas): percorridos em ordem de varredura; põe-se uma luz quando nenhuma outra está a 4,5 células.
- **Sprite do lampião:** camada `SPRITE_LAMPIAO = TOTAL_SPRITES` antigo (o array de sprites cresce 1, no fim); tamanho 0,25 × 0,3; base `altura − 0,15`.

- [ ] **Passo 1: Testes que falham** (`teste_luzes.cpp`)

```cpp
TEST_CASE("toda sala tem ao menos uma luz") { /* 200 sementes × andares 1–6: para cada sala, alguma luz com (x,z) dentro da sala ou a 0,5 da borda */ }
TEST_CASE("corredor nunca fica mais de 10 células sem luz") { /* 200 sementes: toda célula de corredor tem luz a ≤ 5 células (distância euclidiana do centro) */ }
TEST_CASE("luz presa na parede, acima do chão") { /* célula da luz é chão; vizinha na direção do empurrão é Parede; y == chão + 0,8 */ }
TEST_CASE("mesma semente, mesmas luzes; layout 2D da v1 intacto") { /* gerarAndar(7,3) duas vezes: luzes iguais; grade igual à gerada sem luzes (comparar com uma cópia guardada antes da mudança: hash das células de 10 sementes) */ }
TEST_CASE("luzes mais próximas: no máximo 16, da mais perto para a mais longe") { /* 30 luzes em linha; ponto na origem → 16 primeiras em ordem; 3 luzes → 3 */ }
TEST_CASE("tremulação fica entre 0,9 e 1,1") { /* 1000 tempos × 3 fases */ }
```

- [ ] **Passo 2: Rodar e ver falhar.** Antes de mexer no gerador, gravar o hash das grades de 10 sementes (1–10, andares 1–3) num teste, para provar que o 2D não muda.
- [ ] **Passo 3: Implementar** `Luzes` (mundo e render), o campo em `Andar`, o shader com as 16 luzes e o emissivo.
  - **Arte do lampião:** procurar em `arte_fonte/` (Tiny/Zortch) algo de lâmpada ou lampião; na falta, `ferramentas/gerar_lampiao.ps1` faz uma arandela de latão com chama de 16×16, ampliada para 64×64.
  - **`main`:**
    - envia as 16 luzes mais próximas do olho a cada quadro, com `tremulacao(relogio, fase)`;
    - desenha os lampiões numa `MalhaGPU` própria com `uEmissivo = 1`;
    - `uEmissivo = 0` para o resto.
- [ ] **Passo 4: Rodar.** `.\ferramentas\compilar.ps1` e `release`: tudo passa. Captura (`capturar.ps1 luzes -Release --semente 1 --quadro 10`) de uma sala e de um corredor. Conferir que o lampião aparece e que as paredes em volta ficam claras.

---

### Tarefa 2: Alturas na grade e consultas de terreno

**Arquivos:**
- Criar: `src/mundo/Terreno.h/.cpp`, `testes/teste_terreno.cpp`
- Modificar: `src/mundo/Grade.h/.cpp`, `src/mundo/Gerador.cpp` (`andarDeTexto` com camadas), `testes/teste_grade.cpp`, `CMakeLists.txt`

**Interfaces:**
- Produz:
  ```cpp
  namespace mundo {
  constexpr float QUARTO = 0.25f;
  constexpr int TETO_PADRAO = 4;  // quartos acima do chão, nos mapas de texto
  enum class Rampa : uint8_t { Nenhuma, Norte, Sul, Leste, Oeste };  // direção em que sobe (Norte = −y)
  enum class Detalhe : uint8_t { Nenhum, Caixa, Escada };
  // Grade: int chao(x,y), int teto(x,y), Rampa rampa(x,y), Detalhe detalhe(x,y) (fora: 0, 0, Nenhuma, Nenhum);
  //        void definirAltura(x, y, int chao, int teto); void definirRampa(x, y, Rampa); void definirDetalhe(x, y, Detalhe)
  // carregarDeTexto(linhas, alturas = {}, tetos = {}); andarDeTexto(linhas, numero, alturas = {}, tetos = {})
  float alturaDoChao(const Grade& g, glm::vec2 p);   // em blocos; rampa interpolada
  float alturaDoTeto(const Grade& g, glm::vec2 p);
  float chaoNaBorda(const Grade& g, glm::ivec2 c, glm::ivec2 direcao);  // altura do chão da célula c na borda voltada para direcao
  struct RegrasPassagem { int subida; int descida; bool atravessaTrancada; bool atravessaElevador; };
  constexpr RegrasPassagem A_PE{1, 8, false, false}, ANDARILHO{1, 99, false, false}, SALTADOR{4, 99, false, false};
  // Pode ir da célula a para a vizinha b: b não bloqueia; chão de b na borda − chão de a na borda ∈ [−descida, subida]
  // (em quartos); teto de b ≥ chão de b + 3 quartos (corpo de 0,7)
  bool podePassar(const Grade& g, glm::ivec2 a, glm::ivec2 b, const RegrasPassagem& r);
  }
  ```
- **Camada de alturas no texto:** `0`–`9` = chão em quartos; `n s l o` = rampa a partir de 0 e `N S L O` = a partir de 4 quartos, subindo para norte (−y), sul, leste e oeste. **Tetos:** dígito × 1 quarto acima de 0, e `0` = chão + 4. Linhas de tamanho diferente do mapa geram `std::invalid_argument`.

- [ ] **Passo 1: Testes que falham** (`teste_terreno.cpp`, `teste_grade.cpp`)

```cpp
TEST_CASE("mapa sem camada de altura fica no nível 0 com teto 4") { /* chao 0, teto 4 em todas as células abertas */ }
TEST_CASE("camada de alturas: dígitos e rampas") {
    /* linhas {"#####","#...#","#####"}, alturas {"00000","02l40","00000"}: chao(1,1)=2; rampa(2,1)=Leste;
       alturaDoChao({2.0f,1.5f}) == 0, alturaDoChao({2.5f,1.5f}) ≈ 0,5, alturaDoChao({2.99f,1.5f}) ≈ 0,99 */
}
TEST_CASE("camada com tamanho diferente é inválida") { /* REQUIRE_THROWS_AS(..., std::invalid_argument) */ }
TEST_CASE("passagem a pé: degrau de 1/4 sim, de 1/2 não; descida até 2 blocos") {
    /* chão 0 → 1: sim; 0 → 2: não; 8 → 0: sim (A_PE); 9 → 0: não (A_PE), sim (ANDARILHO);
       SALTADOR sobe 4 mas não 5; teto baixo (chão 0, teto 2) bloqueia todos */
}
TEST_CASE("rampa liga níveis de 1 bloco pela borda certa") { /* "0l4": de (0)→rampa→(4) passa a pé nos dois sentidos; entrar pela lateral da rampa (vindo do norte) com chão 0 → borda da rampa em altura média, sobe 2 > 1: não */ }
```

- [ ] **Passo 2: Rodar e ver falhar.**
- [ ] **Passo 3: Implementar.** As alturas ficam em `std::vector<int8_t>` dentro da `Grade`; o padrão é chão 0 e teto 4. `podePassar` usa `chaoNaBorda` dos dois lados da borda comum.
- [ ] **Passo 4: Rodar** — tudo passa; os testes de hoje continuam verdes (mapas sem camada).

---

### Tarefa 3: Jogador com altura, pulo e queda

**Arquivos:**
- Modificar: `src/mundo/Colisao.h/.cpp`, `src/mundo/Terreno.h/.cpp` (física vertical comum), `src/jogo/Jogador.h/.cpp`, `src/jogo/Comandos.h`, `src/jogo/Partida.h/.cpp`, `src/plataforma/Entrada.cpp`, `src/render/Camera.h/.cpp`, `src/main.cpp`
- Testar: `testes/teste_colisao.cpp`, `testes/teste_jogador.cpp`, `testes/teste_partida.cpp`, `testes/teste_terreno.cpp`

**Interfaces:**
- Consome: Tarefa 2.
- Produz:
  ```cpp
  namespace mundo {
  constexpr float GRAVIDADE = 24.0f, ALTURA_CORPO = 0.7f, VELOCIDADE_PULO = 7.27f;
  // Desliza nas células sólidas E nas que têm chão acima de pés + subida ou teto abaixo de pés + ALTURA_CORPO
  glm::vec2 moverComColisao(const Grade& g, glm::vec2 posicao, glm::vec2 deslocamento, float raio, float pes = 0.0f,
                            float subida = QUARTO);
  struct CorpoVertical { float pes = 0; float velocidade = 0; bool noChao = true; float maisAlto = 0; };
  // Gravidade, chão (degrau automático já resolvido pelo movimento horizontal), teto; devolve a queda em blocos ao pousar (0 se não pousou)
  float atualizarVertical(CorpoVertical& c, const Grade& g, glm::vec2 posicao, float raio, float dt);
  int danoDeQueda(float blocos);  // 0 até 2; depois 5 por meio bloco a mais (para baixo)
  }
  // jogo::Jogador ganha CorpoVertical corpo; jogo::Comandos ganha bool pular (evento)
  // jogo::atualizarJogador devolve float (queda em blocos ao pousar)
  // render::matrizVisao(glm::vec2 posicao, float alturaOlhos, float angulo, float inclinacao)
  ```
- **`Partida`:**
  - dano de queda ignora a defesa; causa `"Caiu de muito alto"`; toca `Som::Dano`;
  - ao `descer()`, o corpo é zerado nos pés do chão do início.
- **`Entrada`:** Espaço vira `pular`, contando uma vez por aperto.
- **`main`:** olhos em `pés + 0,5`, interpolando os pés como a posição.

- [ ] **Passo 1: Testes que falham**

```cpp
TEST_CASE("dano de queda") { CHECK(mundo::danoDeQueda(2.0f) == 0); CHECK(mundo::danoDeQueda(2.49f) == 0);
    CHECK(mundo::danoDeQueda(2.5f) == 5); CHECK(mundo::danoDeQueda(3.0f) == 10); CHECK(mundo::danoDeQueda(4.0f) == 20); }
TEST_CASE("anda e sobe degrau de 1/4, para no de 1/2") { /* alturas "0013" (3 = 3/4, meio bloco acima do 1): andando +X, sobe para a célula de 1 quarto (pés 0,25) e para antes da de 3 quartos (x < 3 − raio) */ }
TEST_CASE("rampa acompanha o chão") { /* "0l4": andando +X sobre a rampa, pés == alturaDoChao a cada passo (±0,01) */ }
TEST_CASE("pulo alcança 1 bloco mas não 1,25") { /* plataforma de 4 quartos: pular + andar → pés 1,0 em cima; de 5 quartos: não sobe */ }
TEST_CASE("teto corta o pulo") { /* teto 6 quartos: no pulo a cabeça (pés + 0,7) nunca passa de 1,5 */ }
TEST_CASE("queda de 2 blocos sem dano, de 3 com 10") { /* Partida em mapa com plataforma de 8 e 12 quartos: andar para fora; vida 30 → 30 e → 20; causa "Caiu de muito alto" ao morrer */ }
TEST_CASE("pulo na quina sob teto baixo não prende") { /* Foco 1: canto entre parede e degrau, teto 5; pular e andar contra a quina por 2 s: pés sempre ≥ chão sob o centro e cabeça ≤ teto; posição sai da quina ao andar para trás */ }
TEST_CASE("descer no ar zera a queda") { /* Foco 2: pular diante do elevador e apertar E no ar: novo andar com pés == chão do início, velocidade 0, sem dano no pouso */ }
```

- [ ] **Passo 2: Rodar e ver falhar.**
- [ ] **Passo 3: Implementar.**
  - `moverComColisao` testa os quatro cantos da caixa em cada subpasso contra `alturaDoChao` e `alturaDoTeto`.
  - `atualizarVertical`:
    - aplica a gravidade;
    - prende os pés ao chão quando a distância até ele é menor que 1 quarto e a velocidade não é positiva (subida de degrau e rampa);
    - guarda o ponto mais alto desde que saiu do chão e devolve a queda.
- [ ] **Passo 4: Rodar** — tudo passa (os mapas de hoje ficam no plano: nenhum teste antigo muda). Uma captura com `--semente 1` para conferir que a câmera não mudou.

---

### Tarefa 4: Caminhos e visão em 3D

**Arquivos:**
- Modificar: `src/mundo/Terreno.h/.cpp`, `src/mundo/Consultas.h/.cpp`
- Testar: `testes/teste_terreno.cpp`, `testes/teste_consultas.cpp`

**Interfaces:**
- Produz:
  ```cpp
  namespace mundo {
  // Distância (em células) de cada célula até `alvo` para quem anda com as regras r; −1 = inalcançável.
  // Calculada a partir do alvo pelas arestas ao contrário (vai de b para a quando podePassar(a, b, r)).
  std::vector<int> distanciasComAltura(const Grade& g, glm::ivec2 alvo, const RegrasPassagem& r);
  // Amanatides–Woo na grade; barrado por célula sólida ou quando o raio passa abaixo do chão ou acima do teto da
  // célula (testado na entrada e na saída de cada célula)
  bool temLinhaDeVisao3D(const Grade& g, glm::vec3 de, glm::vec3 para);
  }
  ```
- `distanciasNaGrade` continua existindo, para a exploração e o automapa.

- [ ] **Passo 1: Testes que falham**

```cpp
TEST_CASE("campo do andarilho contorna pela rampa; o do saltador sobe direto") { /* sala com plataforma de 4 quartos com rampa numa ponta e alvo em cima: ANDARILHO chega pela rampa (distância maior), SALTADOR pela frente (menor) */ }
TEST_CASE("campo a pé não desce poço fundo") { /* poço de 12 quartos com saída: A_PE não entra pela borda, entra pela rampa */ }
TEST_CASE("borda de plataforma esconde quem está atrás e abaixo") { /* olho em (1,5; 1,5; 1,5) sobre plataforma de 4 quartos; alvo agachado logo além da borda (y 0,35): false; alvo longe (y 0,5, 4 células): true */ }
TEST_CASE("teto baixo barra a visão de cima") { /* raio de 1,8 de altura atravessando célula de teto 6 quartos: false */ }
TEST_CASE("visão 3D em mapa plano igual à 2D") { /* 200 pares aleatórios em mapa sem alturas, olhos a 0,5: mesmo resultado de temLinhaDeVisao */ }
```

- [ ] **Passo 2: Rodar e ver falhar.** **Passo 3: Implementar.** **Passo 4: Rodar** — tudo passa.

---

### Tarefa 5: Inimigos com altura, IA em 3D e projéteis 3D

**Arquivos:**
- Modificar: `src/jogo/Inimigos.h/.cpp`, `src/jogo/IA.h/.cpp`, `src/jogo/Projeteis.h/.cpp`, `src/jogo/Partida.cpp`, `src/main.cpp` (sprites dos inimigos e projéteis na altura)
- Testar: `testes/teste_ia.cpp`, `testes/teste_projeteis.cpp`, `testes/teste_inimigos.cpp`

**Interfaces:**
- Consome: Tarefas 2–4.
- Produz:
  - `Inimigo` ganha `mundo::CorpoVertical corpo` (pés no chão de onde nasce).
  - `MundoIA` troca `campo` por `campoAndarilho` e `campoSaltador` (`const std::vector<int>&`) e ganha `float pesJogador`.
  - `Projetil` passa a `glm::vec3 posicao, velocidade` (y = altura). O Atirador aponta para `(jogador.x, pés + 0,35, jogador.z)`.
  - `atualizarProjeteis(p, grade, glm::vec3 centroJogador, float raioJogador, float dt)` some quando y < `alturaDoChao` ou y > `alturaDoTeto`, ou numa célula sólida.
- **Regras:**
  - inimigo usa `moverComColisao(..., pes, QUARTO)` e `atualizarVertical`, e ignora a queda;
  - o Rato, quando o próximo passo do campo saltador exige subir mais de 1 quarto, pula com `VELOCIDADE_PULO` (se estiver no chão);
  - visão do inimigo: `temLinhaDeVisao3D(olho do inimigo, olho do jogador)`;
  - golpe corpo a corpo, separação e empurrão só com `|Δpés| < 0,75`.

- [ ] **Passo 1: Testes que falham**

```cpp
TEST_CASE("autômato sobe pela rampa até o jogador na plataforma") { /* 10 s: chega a ≤ 1,2 do jogador e pés ≈ 1,0 */ }
TEST_CASE("rato pula 1 bloco") { /* plataforma de 4 quartos sem rampa: rato chega em cima; autômato não */ }
TEST_CASE("corpo a corpo não alcança 1 bloco acima") { /* jogador na plataforma de 4 quartos colado à borda, autômato embaixo encostado: 5 s, vida intacta */ }
TEST_CASE("empurrado da plataforma cai e persegue") { /* Foco 3: inimigo na plataforma com empurrao para fora: pousa no chão de baixo (pés ≈ 0) e o campo volta a guiá-lo até o jogador */ }
TEST_CASE("projétil do atirador voa em 3D e bate no degrau") { /* atirador no chão, jogador em plataforma alta atrás de degrau: projétil some no degrau, jogador sem dano */ }
TEST_CASE("borda protege de quem atira de cima") { /* Foco 4: atirador em plataforma de 8 quartos; jogador colado embaixo da borda: sem linha de visão, não atira; se atirar, o projétil bate na borda */ }
TEST_CASE("inimigos não levam dano de queda") { /* derrubado de 12 quartos: vida intacta */ }
```

- [ ] **Passo 2: Rodar e ver falhar.** **Passo 3: Implementar** (`Partida` calcula os dois campos com `distanciasComAltura(..., ANDARILHO)` e `(..., SALTADOR)` no mesmo ritmo de hoje). **Passo 4: Rodar** — tudo passa, inclusive os testes de IA de hoje (mapas planos).

---

### Tarefa 6: Combate e coleta do jogador em 3D

**Arquivos:**
- Modificar: `src/jogo/Armas.h/.cpp`, `src/jogo/Partida.cpp`
- Testar: `testes/teste_armas.cpp`, `testes/teste_partida.cpp`

**Interfaces:**
- `atualizarArmas(..., glm::vec2 posicao, float pes, float angulo, float inclinacao, ...)` (`pes` novo, depois da posição).
- **`tracar`:**
  - o raio sai de `pés + 0,5` com a inclinação;
  - acerta o inimigo cujo intervalo vertical (pés … pés + altura do sprite) o raio cruza, com a tolerância de 4° de hoje;
  - para no primeiro ponto em que o raio fica abaixo de `alturaDoChao` ou acima de `alturaDoTeto`.
- **Golpe:** só inimigos com `|Δpés| < 0,75`.
- **Pegar** item (E), munição, engrenagens e válvula: só com `|pés − alturaDoChao(posição da coisa)| < 0,75`.
- **Largar:** a coisa fica em `(x, z)`; a altura vem sempre de `alturaDoChao`.

- [ ] **Passo 1: Testes que falham**

```cpp
TEST_CASE("tiro de cima acerta inimigo embaixo") { /* jogador em plataforma de 8 quartos olhando −30°: acerta autômato 3 células à frente no chão 0 */ }
TEST_CASE("tiro para baixo que erra para no chão") { /* mesma cena sem inimigo na linha: não acerta o inimigo 6 células à frente atrás do ponto de impacto */ }
TEST_CASE("golpe não alcança quem está 1 bloco abaixo") { /* jogador na plataforma, rato embaixo colado: golpe sem acerto */ }
TEST_CASE("item na plataforma só se pega lá em cima") { /* item no chão de 4 quartos; jogador embaixo olhando para ele a 1 célula: E não pega; em cima: pega */ }
TEST_CASE("item largado na plataforma fica no alto") { /* Foco 5: jogador na plataforma larga item à frente sobre a plataforma: altura do item 1,0; à beira de poço, largado para dentro do poço: altura do fundo */ }
```

- [ ] **Passo 2: Rodar e ver falhar.** **Passo 3: Implementar.** **Passo 4: Rodar** — tudo passa.

---

### Tarefa 7: Gerador com alturas, detalhes e garantias

**Arquivos:**
- Modificar: `src/mundo/Gerador.h/.cpp`, `src/jogo/Itens.cpp` (pontos secretos), `src/mundo/Luzes.cpp` (já usa o chão)
- Testar: `testes/teste_gerador.cpp`, `testes/teste_itens.cpp`

**Interfaces:**
- Consome: Tarefas 1, 2 e 4.
- Produz:
  - `Andar` ganha `std::vector<glm::vec2> pontosSecretos` (topo das plataformas só por pulo).
  - `gerarChao` põe um item em cada ponto secreto, além dos de hoje.
- **Passos novos em `gerarAndar`**, depois do 2D e com `std::mt19937 alturas(base ^ 0x5A17u)`:
  1. **Nível das salas** (múltiplo de 4 quartos): andar 1 de 0 a 4; esgoto de 0 a 4; senão de 0 a 12.
  2. **Platôs:** interior + entradas (`entradas()`) + a célula seguinte para fora.
  3. **Rebaixar a sala mais alta** de cada par incompatível (`nível_a − nível_b > distância a pé entre platôs`), até estabilizar.
  4. **Corredores:** `h = min(nível + distância)` por busca a partir dos platôs. Células cujo chão difere de uma vizinha aberta recebem `Detalhe::Escada`.
  5. **Tetos:** sala = nível + 8 a 12, limitado a 20; corredor = h + 6, limitado a 20.
  6. **Comportas** com vizinhas de alturas diferentes voltam a ser chão. Se a tranca ficar torta, a tentativa falha (`nullopt`), como hoje.
- **Detalhes** (sorteio próprio, `base ^ 0xDE7Au`), nunca a menos de 2 células da entrada da sala, do início, do elevador ou da chave. Cada um é **desfeito se a garantia a pé falhar**.
  - **Plataforma** (fábrica, e minas com chance menor), em salas ≥ 6×6:
    - faixa de 1 a 2 células junto a uma parede, com chão +4;
    - o teto da sala sobe para ≥ chão da plataforma + 5;
    - acesso por uma célula de rampa numa ponta, ou, em 35% dos casos, sem acesso: vira **ponto secreto**.
  - **Poço** (minas; fábrica com 15%), em salas ≥ 7×7 com nível ≥ 4:
    - retângulo interno com profundidade de 4 a 12 (≤ nível) e margem de 1 célula;
    - saída por uma fila de `profundidade/4` células de rampa ao longo do lado maior; precisa de comprimento ≥ fila + 1.
  - **Caixas:** esgoto 0 a 1, fábrica 2 a 4, minas 0 a 2 por sala:
    - chão +4 e `Detalhe::Caixa`;
    - em 20%, pilha de 2 (chão +8) se o teto deixar 5 quartos acima.
- **Garantia a pé:** `distanciasComAltura(g, elevador, A_PE)` alcança o início. Com tranca, `(g, chave, A_PE sem atravessar)` alcança o início e o elevador é alcançado com a tranca aberta.

- [ ] **Passo 1: Testes que falham** (em 1.000 sementes × andares 1–6, salvo indicação)

```cpp
TEST_CASE("caminho a pé até a chave e o elevador") { /* A_PE: início → chave (sem tranca) e → elevador (com tranca aberta) */ }
TEST_CASE("todo teto pelo menos 1,25 acima do chão") { /* teto − chão ≥ 5 em toda célula aberta */ }
TEST_CASE("comportas e elevador em trecho plano") { /* comporta: as duas vizinhas abertas com o mesmo chão; elevador: as 4 vizinhas abertas com o mesmo chão */ }
TEST_CASE("todo poço tem saída a pé") { /* toda célula aberta alcança o início por A_PE (nada vira armadilha) */ }
TEST_CASE("andar 1 e esgoto com no máximo 1 bloco de desnível") { /* max(chão) − min(chão) ≤ 4 */ }
TEST_CASE("chão e teto dentro dos limites") { /* 0 ≤ chão ≤ 12, teto ≤ 20 */ }
TEST_CASE("andares têm desníveis de verdade") { /* em 100 sementes do andar 3: ≥ 80% com chão máximo ≥ 4 e ≥ 50% com alguma Caixa */ }
TEST_CASE("layout 2D igual ao da v1") { /* o hash do Passo 2 da Tarefa 1 continua igual */ }
TEST_CASE("ponto secreto tem item e só se alcança pulando") { /* A_PE não alcança a célula; gerarChao tem item nela */ }
```

- [ ] **Passo 2: Rodar e ver falhar.** **Passo 3: Implementar.** **Passo 4: Rodar** em Debug e Release; anotar o tempo do teste de 1.000 sementes (se passar de 15 s em Debug, usar 200 em Debug, como no `teste_demo`).

---

### Tarefa 8: Desenho com alturas e automapa

**Arquivos:**
- Modificar: `src/render/GeometriaMasmorra.h/.cpp`, `src/render/Comportas.cpp` (na altura do chão), `src/render/Sprites.*` (base em altura), `src/render/Automapa.h/.cpp`, `src/main.cpp`, `assets/LICENCAS.txt`
- Testar: `testes/teste_geometria.cpp`, `testes/teste_automapa.cpp`, `testes/teste_sprites.cpp`

**Interfaces:**
- Novas camadas no fim de `CamadaTextura`: `CamadaCaixa`, `CamadaDegrau`. Texturas `texturas/caixa.png` e `texturas/degrau.png`: dos pacotes já baixados ou provisórias por `ferramentas/gerar_terreno.ps1`.
- **`construirGeometria`:**
  - chão de cada célula em `chão` (rampa: os 4 cantos nas alturas da borda) e teto em `teto`;
  - entre duas células abertas vizinhas: face vertical do chão mais baixo ao mais alto, voltada para a mais baixa (camada `CamadaDegrau` se alguma das duas é `Escada`, `CamadaCaixa` se a mais alta é `Caixa`, senão parede da zona); e face do teto mais baixo ao mais alto, voltada para a de teto mais baixo;
  - contra parede: face do chão ao teto da célula;
  - **UV em unidades de mundo:** u = posição ao longo da aresta, v = altura em blocos.
- **Sprites:** `Sprite::base` recebe a altura dos pés ou do chão; o lampião usa `posicao.y − 0,15`.
- **`construirAutomapa`:**
  - o tom da célula vista é multiplicado por `0,6 + 0,4·chão/12`;
  - células de rampa ou escada ganham uma marca de 1 px no centro;
  - fundo de poço (chão < nível da sala) fica 30% mais escuro.

- [ ] **Passo 1: Testes que falham**

```cpp
TEST_CASE("mapa plano gera a mesma quantidade de vértices de antes") { /* o teste de geometria de hoje continua: só chão, teto e paredes */ }
TEST_CASE("degrau gera face vertical voltada para a célula de baixo") { /* "02": existe quadrado em x=1 de y 0 a 0,5 com normal −X */ }
TEST_CASE("rampa: chão inclinado nos cantos") { /* "0l4": vértices do chão da rampa com y 0 em x=1 e 1 em x=2 */ }
TEST_CASE("textura não estica: v vai de 0 a 3 numa parede de 3 blocos") { /* teto 12: face contra parede com uv.y de 0 a 3 */ }
TEST_CASE("automapa mais claro no alto") { /* duas células vistas, chão 0 e 12: a segunda mais clara */ }
```

- [ ] **Passo 2: Rodar e ver falhar.** **Passo 3: Implementar.** **Passo 4: Rodar.** Depois, 3 capturas em Release de andares gerados (fábrica com mezanino, mina com poço e um corredor com escada, achados com `--descrever`, que passa a listar plataformas, poços e caixas). Conferir que não há buraco nem textura esticada.

---

### Tarefa 9: Demo, guia, desempenho e cenários

**Arquivos:**
- Modificar: `src/jogo/Demo.cpp` (campo `A_PE` com `distanciasComAltura`), `testes/teste_demo.cpp`, `docs/COMO_JOGAR.txt`, `README.md`, spec geral (seção 4, controles: Espaço pular), `src/main.cpp` (`--descrever` com detalhes)

- [ ] **Passo 1: Teste** — `teste_demo` passa a rodar também nos andares 2 e 3 (20 sementes cada em Release): o piloto chega ao elevador **sem pular**, ou seja, `Comandos::pular` nunca é verdadeiro.
- [ ] **Passo 2: Ver falhar** (o piloto de hoje usa o campo sem alturas e fica preso). **Passo 3: Implementar.** **Passo 4: Rodar.**
- [ ] **Passo 5: Desempenho** — `--demo 12345` em Release com `SDL_AUDIODRIVER=dummy`; o log ganha a média de quadros por segundo da demo (`demo: N quadros em S s`), e a média precisa passar de 60.
- [ ] **Passo 6: Guia e README** com "Espaço ... pular" e uma dica: "Cair de muito alto machuca; desça pelas escadas e rampas".
- [ ] **Passo 7: Cenários** em `build\Release`:
  - `10_Sala_com_mezanino.bat`
  - `11_Mina_com_poco.bat`
  - `12_Atirador_no_alto.bat`
  - `13_Plataforma_so_por_pulo.bat`
  - `14_Corredor_iluminado.bat`

  Sementes e posições achadas com `--descrever`. A tabela do que testar cobre:
  - pular para a plataforma secreta;
  - cair no poço (dano) e sair pela rampa;
  - lutar embaixo do Atirador no alto, usando a borda como cobertura;
  - Ratos pulando e Colossos dando a volta;
  - lampiões tremulando;
  - Alt-Tab no ar.

---

### Tarefa 10: Verificação, revisão e entrega

- [ ] **Passo 1:** `.\ferramentas\compilar.ps1` e `release`, com tudo verde (anotar a contagem).
- [ ] **Passo 2:** Revisão independente do ramo inteiro (agente em segundo plano, Release, `--dados` no scratchpad, áudio dummy, no máximo 2 execuções com janela), com o Foco de revisão acima. Reclassificar os achados pelo efeito, corrigir os Importantes com teste RED→GREEN e listar os Menores.
- [ ] **Passo 3:** Backup de `%APPDATA%\Roguelike` antes de deixar o jogo com o Nidragnnar. Pacote regerado com `.\ferramentas\empacotar.ps1`.
- [ ] **Passo 4:** Deixar o cenário 10 aberto e perguntar "Depois de testar, faço o commit e envio?". Listar:
  - o que foi testado;
  - o que não foi (sensação do pulo, se a queda dói na medida certa, beleza dos andares);
  - o que é estimativa: as constantes (gravidade, pulo, dano de queda, chances dos detalhes, cores e alcance das luzes);
  - o desvio das rampas nos corredores.

  Release v1.1 no GitHub só com ok.
