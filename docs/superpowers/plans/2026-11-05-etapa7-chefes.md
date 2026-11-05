# Etapa 7 — Chefes e vitória: Plano de implementação

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Objetivo:** três chefes (andares 2, 4 e 6) em arenas que trancam, e vitória ao derrotar a Grande Máquina.

**Arquitetura:** chefe é um `Inimigo` com `chefe = true` e raio próprio na tabela; a lógica de cada um fica em
`jogo/Chefes.cpp` (chamada pela IA como a vespa e o dirigível). O gerador marca a maior sala como arena nos andares
de chefe; a `Partida` tranca/destranca a arena, aplica invocações e ondas e decide a vitória.

**Tecnologia:** C++20, SDL2/SDL_mixer, OpenGL 3.3, glm, doctest; MSVC, presets vs/release.

**Especificação:** `docs/superpowers/specs/2026-11-05-etapa7-chefes-design.md` (ler junto).

## Restrições globais

- Comentários curtos em português, nomes em português, mesma densidade do código existente.
- Compilar e testar com `.\ferramentas\compilar.ps1 release` (nunca Debug: abre caixas de erro na tela).
- Roteiros automáticos com `SDL_AUDIODRIVER=dummy`; testes que gravam placar usam `--dados <scratchpad>`.
- **Sem commit nem push sem o "sim" explícito do Nidragnnar**; um commit só no fim; sem `Co-Authored-By`.
- Andares 1, 3 e 5 não mudam de desenho; o placar não muda de formato.
- Valores da spec: Rainha 140/0, vel 2,5, raio 0,5, altura 0,8, XP 60; Capataz 240/3, vel 1,0, raio 0,6,
  altura 1,4, XP 120; Grande Máquina 420/2, vel 0, raio 0,8, altura 1,8, XP 250; +500 pontos por chefe.

## Foco da revisão

1. Chefe empurrado contra a comporta trancada ou preso num canto da arena (raio grande numa sala estreita): deve
   continuar se mexendo e atacando. → teste na Tarefa 4 ("investida não atravessa nem trava na comporta").
2. Jogador que sai da arena pela comporta no exato quadro em que ela tranca: não pode ficar dentro da parede. →
   teste na Tarefa 3 ("trancar não prende o jogador dentro da comporta").
3. Invocações nascendo dentro de parede ou fora da arena: devem nascer em célula de chão livre da arena. → teste na
   Tarefa 4 ("ratos chamados nascem no chão da arena").
4. Grande Máquina destruída por inimigo/explosão enquanto o jogador está morto: morte vence, sem vitória. → teste na
   Tarefa 3 ("morto não vence").
5. Andar de chefe com tranca de válvula-chave na arena (gerador antigo): a arena continua alcançável a pé. → teste na
   Tarefa 2 ("arena alcançável a pé nos andares de chefe").

---

### Tarefa 1: Chefes na tabela e raio por inimigo

**Arquivos:** `src/jogo/Inimigos.h/.cpp`, `src/jogo/IA.cpp`, `src/jogo/Armas.cpp`, `src/jogo/Partida.cpp`,
`src/render/Icones.h`, `src/render/AnimacaoInimigos.cpp`, `src/main.cpp` (lista de sprites), testes
`teste_inimigos.cpp`, `teste_armas.cpp`, `teste_hud.cpp`.

**Interfaces:**
- Produz: `TipoInimigo` += `RainhaRatos, Capataz, GrandeMaquina` (`TOTAL_TIPOS_INIMIGO = 9`); `DadosInimigo` ganha
  `bool chefe; float raio;` no fim; `float raioDe(const Inimigo&)` em `Inimigos.h`; letras de mapa `R`, `P`, `M`.
  Chefes não entram no sorteio comum (peso 0).

- [ ] Testes: "chefes na tabela" (valores da spec, `chefe`, `raio`); "inimigos comuns com raio 0,25";
  "tiro acerta o chefe pela borda" (revólver mirando 0,45 ao lado do centro: acerta a Rainha, erra o rato);
  `teste_hud`: `PRIMEIRO_ITEM == PRIMEIRO_INIMIGO + 9 * QUADROS_POR_INIMIGO`.
- [ ] Ver falhar; trocar todo `RAIO_INIMIGO` de corpo de inimigo por `raioDe(i)` (colisão, separação, golpe,
  traçado do tiro, empurrão do jogador); sprites provisórios mínimos (cópias do colosso) só para o array fechar —
  a arte de verdade é da Tarefa 7.
- [ ] Suite inteira passa (os inimigos de hoje não mudam).

### Tarefa 2: Arena no gerador

**Arquivos:** `src/mundo/Gerador.h/.cpp`, `src/mundo/Relevo.cpp`, `src/jogo/Inimigos.cpp`,
`testes/teste_gerador.cpp`, `testes/teste_terreno.cpp` ou `teste_gerador.cpp`.

**Interfaces:**
- Produz: `bool andarDeChefe(int andar)` (2, 4, 6) e `constexpr int ULTIMO_ANDAR = 6` em `Gerador.h`;
  `Andar::arena` (`std::optional<size_t>`, índice da sala); `TipoInimigo chefeDoAndar(int andar)`.

- [ ] **Antes de mexer:** calcular a impressão digital do layout só com `n ∈ {1, 3}` no código atual e trocar o
  teste "layout 2D igual ao da v1" para esse laço e esse valor (passa sem mudança de código).
- [ ] Testes: "arena na maior sala fora a inicial" (sementes 1–30, andares 2/4/6); "arena plana e com teto alto"
  (todas as células da sala com o chão da sala, sem rampa/detalhe, teto ≥ chão + 16 quartos ou o máximo);
  "chefe no centro da arena e nenhum inimigo comum dentro"; "arena alcançável a pé nos andares de chefe"
  (`distanciasComAltura(..., A_PE)` do início chega ao elevador); "andar sem chefe sem arena".
- [ ] Ver falhar; elevador na maior sala (área; empate: a mais distante) só em andar de chefe; relevo pula
  plataforma/poço/caixas na arena e usa teto = nível + 16 (limitado ao máximo); `gerarInimigos` não sorteia posição
  na arena e acrescenta o chefe no centro.
- [ ] Suite passa, inclusive a impressão digital nova.

### Tarefa 3: Luta, recompensa e vitória na Partida

**Arquivos:** `src/jogo/Partida.h/.cpp`, `src/jogo/Itens.h/.cpp`, `src/jogo/Placar.*` (só a pontuação),
`testes/teste_partida.cpp`, `testes/teste_itens.cpp`.

**Interfaces:**
- Consome: Tarefas 1–2.
- Produz: `const Inimigo* Partida::chefe() const` (vivo ou nulo); `bool lutaComChefe() const`; `bool venceu() const`;
  `bool terminou() const` (morto ou venceu); `float tempoDeJogo() const`; `int chefesDerrotados() const`;
  `Item gerarObraPrima(int andar, std::mt19937&)` (arma ou armadura, qualidade ObraPrima, bônus como em gerarItem);
  `calcularPontuacao` ganha `int chefes` (+500 cada). Constantes `CAUSA_VITORIA = "Venceu a Grande Máquina"`,
  `ESPERA_VITORIA = 3.0f`.

- [ ] Testes: "entrar na arena tranca e acorda o chefe"; "trancar não prende o jogador dentro da comporta" (o
  jogador na célula da comporta não a tranca até sair dela); "chefe destruído destranca e solta obra-prima e 30
  engrenagens"; "elevador com chefe vivo avisa 'Derrote o chefe primeiro'"; "Grande Máquina destruída: vitória
  depois de 3 s, causa e +500"; "morto não vence"; "obra-prima sorteada nunca é tônico".
- [ ] Ver falhar; implementar (comportas da arena: as entradas da sala, guardadas ao começar a luta; trancadas viram
  `ComportaTrancada`; ao destrancar voltam a `Comporta`).
- [ ] Suite passa.

### Tarefa 4: Rainha dos Ratos e invocações

**Arquivos:** criar `src/jogo/Chefes.h/.cpp`; `src/jogo/IA.h/.cpp`, `src/jogo/Partida.cpp`, `CMakeLists.txt`;
criar `testes/teste_chefes.cpp`.

**Interfaces:**
- Consome: Tarefas 1–3.
- Produz: `EventosIA` ganha `std::vector<Invocacao> invocacoes` (`struct Invocacao { TipoInimigo tipo; glm::vec2
  posicao; }`) e `std::vector<Onda> ondas` (`struct Onda { glm::vec2 centro; float raio; int dano; }`);
  `void atualizarChefe(Inimigo&, const MundoIA&, EventosIA&, bool ve, float dt)` em `Chefes.h`; `Inimigo` ganha
  `glm::vec2 direcao{0}` e `float recarga = 0, recarga2 = 0` (relógios dos ataques). `MundoIA` ganha
  `int ratosVivos = 0, voadoresVivos = 0` e `std::optional<mundo::Sala> arena`.
  Constantes: `INVESTIDA_MIRA 0.6`, `INVESTIDA_VELOCIDADE 7`, `INVESTIDA_DURACAO 1.2`, `INVESTIDA_ALCANCE 6`,
  `INVESTIDA_INTERVALO 3`, `TONTURA 1.5`, `NINHADA_INTERVALO 8`, `NINHADA_TAMANHO 3`, `MAXIMO_RATOS 6`.

- [ ] Testes: "investida em linha reta e dano ao encostar" (7–10); "tonta ao bater na parede" (Recuperando 1,5 s,
  sem golpe); "ninhada de 3 a cada 8 s e nunca mais de 6 ratos"; "ratos chamados nascem no chão da arena";
  "investida não atravessa nem trava na comporta".
- [ ] Ver falhar; implementar; a Partida acrescenta as invocações (`acrescentarInimigo`) e já aplica as ondas.
- [ ] Suite passa.

### Tarefa 5: Capataz a Vapor

**Arquivos:** `src/jogo/Chefes.cpp`, `src/jogo/Partida.cpp`, `testes/teste_chefes.cpp`.

**Interfaces:** constantes `JATO_ALCANCE 7`, `JATO_INTERVALO 3`, `JATO_PREPARO 0.5`, `JATO_ANGULOS {-20,-10,0,10,20}`
graus, `PANCADA_ALCANCE 2.5`, `PANCADA_INTERVALO 5`, `PANCADA_PREPARO 0.8`, dano da pancada 9–13.

- [ ] Testes: "leque de 5 projéteis nos ângulos certos" (direções a −20°…+20° da linha até o jogador, dano 4–6);
  "onda fere no chão"; "onda não fere no ar" (jogador com `noChao = false`); "onda não fere fora do raio".
- [ ] Ver falhar; implementar (a Partida aplica a onda só com `jogador_.corpo.noChao` e distância < raio).
- [ ] Suite passa.

### Tarefa 6: Grande Máquina

**Arquivos:** `src/jogo/Chefes.cpp`, `testes/teste_chefes.cpp`.

**Interfaces:** constantes `RAJADA_TIROS 3`, `RAJADA_ESPACO 0.2`, `RAJADA_INTERVALO 2`, `RAJADA_INTERVALO_FASE2 1.2`,
`CHAMADO_INTERVALO 12`, `MAXIMO_VOADORES 4`.

- [ ] Testes: "não sai do lugar nem com empurrão"; "rajada de 3 a cada 2 s"; "fase 2 com metade da vida: rajada
  a cada 1,2 s"; "chama 2 vespas e depois 1 dirigível, nunca mais de 4 voadores".
- [ ] Ver falhar; implementar (empurrão zerado para ela; separação não a move).
- [ ] Suite passa.

### Tarefa 7: Tela, música e arte

**Arquivos:** `src/render/Hud.h/.cpp`, `src/plataforma/Audio.h/.cpp`, `src/main.cpp`, `ferramentas/gerar_sons.cpp`,
criar `ferramentas/gerar_chefes.ps1`, `assets/LICENCAS.txt`, testes `teste_hud.cpp`.

**Interfaces:** `EstadoHud` ganha `std::string_view nomeChefe; int vidaChefe = 0, vidaMaximaChefe = 0;`;
`DadosFim` ganha `bool vitoria; int segundos;`; `Audio::tocarMusica(std::string_view nome = "musica")` (troca se
for outra); `assets/sons/musica_chefe.wav` (32 s, loop).

- [ ] Testes: "barra do chefe só durante a luta e proporcional à vida"; "tela de vitória diz Vitória! e o tempo
  m:ss".
- [ ] Ver falhar; implementar; `main.cpp` usa `terminou()` no lugar de `morto()` para a tela de fim e o placar,
  troca a música pela luta; gerar sprites (7 quadros × 3) e a música.
- [ ] Suite passa; capturas de cada arena com `--andar 2/4/6` mostram chefe e barra.

### Tarefa 8: Cenários, documentação e pacote

**Arquivos:** `build\Release\16_Rainha_dos_Ratos.bat`, `17_Capataz_a_Vapor.bat`, `18_Grande_Maquina.bat`,
`19_Vitoria.bat`; `src/main.cpp` (opção de teste `--vida-chefe <n>` para o cenário 19); `docs/COMO_JOGAR.txt`,
`README.md`, spec geral (zonas e fim do jogo).

- [ ] Escolher sementes/posições com `--descrever` (de frente para a entrada de cada arena); escrever os `.bat`.
- [ ] `.\ferramentas\empacotar.ps1` → "teste de fumaça ok" (o piloto da demo continua no andar 1).
- [ ] Abrir o cenário 16; tabela do que olhar em cada cenário.
- [ ] Commit único só com o "sim" do Nidragnnar.
