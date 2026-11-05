# Etapa 6B — Inimigos voadores: Plano de implementação

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Objetivo:** pôr no jogo a vespa a corda (mergulha e morde) e o dirigível a vapor (paira e solta bombas).

**Arquitetura:** os voadores são `Inimigo` comuns com a marca `voa` na tabela. Andam por um terceiro campo de
distâncias (`VOADOR`, sem limite de subida/descida) e têm a altura controlada à parte, sem gravidade enquanto vivos.
A bomba é um `Projetil` com gravidade e raio de explosão.

**Tecnologia:** C++20, SDL2/SDL_mixer, OpenGL 3.3, glm, doctest; MSVC, presets vs/debug e vs/release.

**Especificação:** `docs/superpowers/specs/2026-11-05-etapa6b-voadores-design.md` (ler junto).

## Restrições globais

- Comentários curtos em português, mesma densidade do código existente; nomes em português.
- Compilar e testar com `.\ferramentas\compilar.ps1 release` (Debug abre caixas de erro na tela do Nidragnnar).
- Roteiros automáticos com `SDL_AUDIODRIVER=dummy`; testes que podem gravar placar usam `--dados <scratchpad>`.
- **Sem commit nem push sem o "sim" explícito do Nidragnnar.** Commits em português, autor Nidragnnar, **sem**
  `Co-Authored-By` nem menção ao Claude. As etapas "Commit" deste plano ficam todas para o fim, num commit só, depois
  do teste dele.
- O desenho do andar não muda: o teste de impressão digital (14210815957670128114) continua passando.
- Valores da spec (andar 1): Vespa vida 6, def 0, dano 2–4, veloc 3,0, XP 5, preparo 0,3, recup 0,8, altura 0,30;
  Dirigível vida 18, def 1, dano 5–8, veloc 1,0, XP 12, altura 0,60, intervalo 2,5 s.

## Foco da revisão

1. Voador num corredor de teto baixo (teto − chão = 1,25): não pode ficar preso dentro do teto nem tremer entre
   alvos; deve passar baixo. → teste em Tarefa 2 ("voador passa por corredor baixo").
2. Dirigível exatamente em cima do jogador (distância horizontal 0): a bomba nasce sem direção e não pode gerar
   NaN. → teste em Tarefa 3 ("bomba com dirigível em cima do jogador").
3. Bomba que nasce dentro do teto/parede (dirigível encostado): explode na hora, sem atravessar. → teste em Tarefa 3.
4. Voador destruído sobre um poço: os destroços caem até o fundo, não ficam no ar. → teste em Tarefa 2.
5. Andar sem voadores: o campo `VOADOR` não é calculado (custo zero) e nada muda no comportamento dos outros. →
   teste em Tarefa 4 ("sem voador, campo voador vazio").

---

### Tarefa 1: Dados, sorteio e marcas

**Arquivos:**
- Modificar: `src/jogo/Inimigos.h`, `src/jogo/Inimigos.cpp`
- Modificar: `src/render/Icones.h` (`PRIMEIRO_ITEM = PRIMEIRO_INIMIGO + 6 * QUADROS_POR_INIMIGO`)
- Teste: `testes/teste_inimigos.cpp`

**Interfaces:**
- Produz: `enum class TipoInimigo { Rato, Automato, Atirador, Colosso, Vespa, Dirigivel };`
  `DadosInimigo` ganha `bool voa;` como último campo; `constexpr int TOTAL_TIPOS_INIMIGO = 6;`
  Marcas `v` (Vespa) e `d` (Dirigível) em `inimigosDasMarcas`.

- [ ] **Passo 1: testes que falham**

```cpp
TEST_CASE("voadores na tabela") {
    CHECK(dados(TipoInimigo::Vespa).voa);  CHECK(dados(TipoInimigo::Dirigivel).voa);
    CHECK_FALSE(dados(TipoInimigo::Rato).voa);
    CHECK(dados(TipoInimigo::Vespa).vida == 6);  CHECK(dados(TipoInimigo::Vespa).xp == 5);
    CHECK(dados(TipoInimigo::Dirigivel).vida == 18);  CHECK(dados(TipoInimigo::Dirigivel).defesa == 1);
    CHECK(dados(TipoInimigo::Dirigivel).danoMin == 5);  CHECK(dados(TipoInimigo::Dirigivel).danoMax == 8);
}
TEST_CASE("voadores só a partir do andar certo") {
    // 200 sementes: andar 1 sem voador; andar 2 com vespa em alguma e sem dirigível; andar 3 com os dois em alguma
}
TEST_CASE("marcas v e d") { /* mapa com 'v' e 'd' gera Vespa e Dirigivel nas células marcadas */ }
```

- [ ] **Passo 2:** compilar; os testes falham (tipos inexistentes).
- [ ] **Passo 3:** tabela com as duas linhas (vespa `aDistancia=false`, dirigível `aDistancia=true`), pesos da
  spec (andar 2: 35/30/20/0/15/0; andar 3+: 25/25/20/12/10/8), vespas em 1–2 na mesma sala (como o bando de ratos),
  marcas, `PRIMEIRO_ITEM` com 6 inimigos. Voadores nascem com `corpo.pes` no chão (a IA os levanta).
- [ ] **Passo 4:** `.\ferramentas\compilar.ps1 release` → todos passam, inclusive a impressão digital do gerador e
  `teste_hud`.

### Tarefa 2: Voo (caminho, altura, vespa)

**Arquivos:**
- Modificar: `src/mundo/Terreno.h` (`constexpr RegrasPassagem VOADOR{99, 99, false, false};`)
- Modificar: `src/jogo/IA.h`, `src/jogo/IA.cpp`
- Criar: `testes/teste_voadores.cpp`

**Interfaces:**
- Consome: Tarefa 1.
- Produz: `MundoIA` ganha `const std::vector<int>* campoVoador = nullptr;`
  `constexpr float VELOCIDADE_VERTICAL_VOO = 2.5f, ALTURA_VOO_VESPA = 1.2f, ALTURA_PEITO = 0.3f,
  DISTANCIA_MERGULHO = 1.5f, DESCIDA_DIRIGIVEL = 1.2f;` em `IA.h`.
  `float alvoDeVoo(const Inimigo&, const mundo::Grade&, glm::ivec2 proxima, float pesJogador)` (anônima em IA.cpp).

- [ ] **Passo 1: testes que falham** (mapas de texto com alturas, padrão de `teste_ia.cpp`)

```cpp
TEST_CASE("voador atravessa poço e caixa")        // vespa do outro lado de um poço de 2 blocos chega ao jogador em ≤ 6 s
TEST_CASE("parede e comporta fechada barram o voador")  // campo VOADOR: célula atrás da parede só pela volta
TEST_CASE("altura do voador entre chão e teto")    // em todo passo: pes ≥ chão + 0,1 − 1e-3 e pes + 0,30 ≤ teto − 0,1 + 1e-3
TEST_CASE("voador passa por corredor baixo")       // corredor teto − chão = 1,25: atravessa e a altura não oscila > 0,05 em 10 passos parados
TEST_CASE("vespa mergulha, morde e sobe")          // a 1,4 do jogador, vendo: Preparando; no fim pes ≈ pesJogador + 0,3; 1 golpe 2–4; depois de Recuperando pes ≈ pesJogador + 1,2
TEST_CASE("chave inglesa só acerta a vespa embaixo") // golpear() com vespa a pes+1,2: 0 acertos; a pes+0,3: 1 acerto
TEST_CASE("tiro acerta vespa no alto")             // disparar() com inclinação para ela: 1 acerto
TEST_CASE("voador destruído cai até o chão")       // sobre poço: depois de 3 s, pes == chão do fundo
TEST_CASE("plataforma não protege do voador")      // jogador numa plataforma de 2 blocos: vespa chega e morde
```

- [ ] **Passo 2:** compilar; falham.
- [ ] **Passo 3:** na IA, para `dados(t).voa`:
  - Caminho: `andarPeloCampo` usa `*m.campoVoador` e `podePassar(..., VOADOR)`; aproximação em linha reta vale com
    `temPassagemLivre` **sem** exigir mesmo nível.
  - Altura: `alvoDeVoo` = altura do estado (vespa: `pesJogador + 1,2`, ou `+ 0,3` em Preparando; dirigível:
    `teto − 1,2`), presa entre `max(chão atual, chão da próxima) + 0,1` e
    `min(teto atual, teto da próxima) − altura − 0,1`; se o mínimo passar do máximo, o mínimo. `corpo.pes` anda até
    o alvo a no máximo `2,5 × dt`. Vivo: **não** chamar `atualizarVertical`; destruído: chamar (cai).
  - Movimento: `moverComColisao(..., corpo.pes, 0)`.
  - Visão: sem `nivel` para voadores. Vespa: Perseguindo → Preparando quando `ve` e distância horizontal ≤ 1,5;
    golpe no fim do preparo se distância 3D (centros) ≤ `ALCANCE_GOLPE`.
  - `separar`: sem mudança (já compara níveis).
- [ ] **Passo 4:** todos os testes passam.

### Tarefa 3: Dirigível e bomba

**Arquivos:**
- Modificar: `src/jogo/Projeteis.h`, `src/jogo/Projeteis.cpp`, `src/jogo/IA.cpp`
- Teste: `testes/teste_projeteis.cpp`, `testes/teste_voadores.cpp`

**Interfaces:**
- Consome: Tarefa 2.
- Produz: `Projetil` ganha `float gravidade = 0; float raioExplosao = 0;` (no fim, para os inicializadores atuais
  continuarem valendo). `constexpr float GRAVIDADE_BOMBA = 9.0f, RAIO_BOMBA = 1.0f, INTERVALO_BOMBA = 2.5f,
  ALCANCE_BOMBA = 2.0f;` `ResultadoProjeteis` ganha `std::vector<glm::vec3> explosoes;`

- [ ] **Passo 1: testes que falham**

```cpp
TEST_CASE("bomba cai com gravidade")           // parada a 2 blocos: após 0,3 s, velocidade.y ≈ −2,7 e altura < inicial
TEST_CASE("bomba explode no chão com dano pela distância") // centro do jogador a 0,5 da explosão: golpe = round(dano × 0,5); a 1,1: nenhum golpe; explosoes.size() == 1
TEST_CASE("bomba que nasce no teto explode na hora")       // posição acima do teto: some no primeiro passo com 1 explosão
TEST_CASE("tiro do atirador continua reto")    // gravidade 0: velocidade.y constante
TEST_CASE("dirigível solta bomba com intervalo") // vendo e a ≤ 2 na horizontal: 1 bomba; nenhuma outra antes de 2,5 s; outra depois
TEST_CASE("dirigível sem ver não solta bomba")
TEST_CASE("bomba com dirigível em cima do jogador") // distância horizontal 0: bomba com velocidade (0,0,0), sem NaN
```

- [ ] **Passo 2:** compilar; falham.
- [ ] **Passo 3:** `atualizarProjeteis`: `velocidade.y −= gravidade × dt` por subpasso. Com `raioExplosao > 0`,
  ao bater (chão, teto, parede, jogador) registra a explosão e dá `golpe = round(dano × (1 − d/raio))` se
  `d < raio` e o resultado ≥ 1 (d = distância 3D ao centro do jogador). Sem raio, o comportamento de hoje.
  IA do dirigível: Perseguindo segue o campo voador até a célula do jogador; com `ve`, distância horizontal ≤ 2 e
  `tempoNoEstado ≥ 2,5` desde a última bomba (usar Recuperando por 2,5 s como recarga), empurra em `e.tiros` uma
  bomba em `(x, pes, z)` do dirigível, velocidade zero, dano sorteado `danoMin..danoMax`.
- [ ] **Passo 4:** todos passam.

### Tarefa 4: Partida, sons e eventos

**Arquivos:**
- Modificar: `src/jogo/Partida.h`, `src/jogo/Partida.cpp`, `src/jogo/Sons.h`, `src/jogo/Sons.cpp`,
  `src/jogo/IA.h` (`EventosIA` ganha `std::vector<TipoInimigo> alertas;` — voador que passou de Parado a
  Perseguindo)
- Modificar: `ferramentas/gerar_sons.cpp`, `ferramentas/preparar_sons.ps1`; gerar `assets/sons/zumbido.wav`,
  `motor.wav`, `explosao.wav`
- Teste: `testes/teste_partida.cpp`

**Interfaces:**
- Consome: Tarefas 2–3.
- Produz: `Som::Zumbido, Som::Motor, Som::Explosao` (antes de `TOTAL`);
  `struct Explosao { glm::vec3 posicao; float tempo; };` e `const std::vector<Explosao>& Partida::explosoes() const`
  (cada uma vive `DURACAO_EXPLOSAO = 0.3f` s); `campoVoador_` na Partida.

- [ ] **Passo 1: testes que falham**

```cpp
TEST_CASE("sem voador, campo voador vazio")     // andar só com rato: campoVoador() vazio depois de 1 s
TEST_CASE("com voador, campo voador calculado") // tamanho = largura × altura
TEST_CASE("explosão de bomba fere o jogador e aparece") // vida cai, explosoes().size() == 1, some depois de 0,3 s
```

- [ ] **Passo 2:** falham.
- [ ] **Passo 3:** Partida recalcula `campoVoador_` junto dos outros campos só se houver voador vivo (senão
  `clear()`), passa `&campoVoador_` no `MundoIA`, toca `Som::Explosao` e liga `clarao_` em cada explosão, toca
  `Zumbido`/`Motor` em cada alerta (com origem), e guarda as explosões. `gerar_sons.cpp`: três sínteses curtas
  (zumbido 0,5 s em ~180 Hz com vibrato; motor 0,8 s grave pulsado; explosão 0,7 s ruído com queda) seguindo as
  existentes; `preparar_sons.ps1` com as três entradas `sintetico:`. Rodar o preparo (precisa de `gerar_sons`
  compilado em Release).
- [ ] **Passo 4:** todos passam; `--demo` curto sem erro.

### Tarefa 5: Desenho

**Arquivos:**
- Criar: `ferramentas/gerar_voadores.ps1` → `assets/sprites/vespa_0..6.png`, `dirigivel_0..6.png`, `bomba.png`,
  `explosao.png` (64×64, mesmo estilo do `gerar_lampiao.ps1`; vespa: quadros 1 e 2 com asas diferentes)
- Modificar: `src/main.cpp` (`arquivosDeSprites`: inimigos `vespa`, `dirigivel` depois de `colosso`; `bomba.png` e
  `explosao.png` no fim), `src/render/Icones.h` (`SPRITE_BOMBA = SPRITE_LUSTRE + 1; SPRITE_EXPLOSAO = SPRITE_BOMBA + 1;
  TOTAL_SPRITES = SPRITE_EXPLOSAO + 1`), `src/render/AnimacaoInimigos.cpp` (larguras: vespa 0,4; dirigível 0,9),
  `assets/LICENCAS.txt`
- Teste: `testes/teste_hud.cpp` (ajustar a contagem esperada de sprites), `testes/teste_animacao*.cpp` se existir

- [ ] **Passo 1:** ajustar o teste de contagem (`TOTAL_SPRITES == PRIMEIRO_ITEM + 14 + 4`) → falha.
- [ ] **Passo 2:** gerar os sprites, ligar no `main.cpp`: projétil com `raioExplosao > 0` desenhado com
  `SPRITE_BOMBA` (0,25 × 0,25, pisca alternando visível a cada 0,1 s); explosões na malha emissiva com
  `SPRITE_EXPLOSAO` (1,0 × 1,0, centrada na posição).
- [ ] **Passo 3:** todos os testes passam; capturas `.\ferramentas\capturar.ps1 vespa -Release` e `dirigivel` com
  os mapas da Tarefa 6 mostram os sprites no ar.

### Tarefa 6: Cenários, documentação e pacote

**Arquivos:**
- Modificar: `src/main.cpp` — opção de teste `--inimigo <letra> <x> <y>` (repetível; letras das marcas de mapa)
  que acrescenta o inimigo ao andar inicial via `void Partida::acrescentarInimigo(TipoInimigo, glm::vec2)` (cria
  com `criarInimigo` no andar atual e põe os pés no chão da célula)
- Criar: `build\Release\14_Vespas_sobre_poco.bat` (semente 2, andar 5, como o cenário 11, com 2 vespas do outro
  lado do poço) e `15_Dirigivel_sala_alta.bat` (semente 10, andar 3, como o cenário 12, com 1 dirigível na sala alta)
- Modificar: `docs/COMO_JOGAR.txt`, `README.md` (inimigos novos), spec geral (tabela de inimigos)
- Teste: `testes/teste_partida.cpp` — `TEST_CASE("acrescentarInimigo põe o inimigo no chão da célula")`
- [ ] **Passo 1:** teste de `acrescentarInimigo` (falha → passa), opção `--inimigo`, posições com `--descrever` e
  os dois `.bat`.
- [ ] **Passo 2:** `.\ferramentas\empacotar.ps1` → "teste de fumaça ok".
- [ ] **Passo 3:** abrir o cenário 14 para o Nidragnnar; tabela do que olhar em cada cenário.
- [ ] **Passo 4 (só com o "sim" dele):** um commit "Etapa 6B: inimigos voadores (vespa a corda e dirigível a
  vapor)" com spec, plano, código, testes, sprites e sons.
