# Etapa 4 — Itens e progressão: Plano de implementação

> **Para agentes:** SUB-SKILL OBRIGATÓRIA: usar superpowers:subagent-driven-development (recomendado) ou superpowers:executing-plans para executar este plano tarefa por tarefa. Os passos usam caixas (`- [ ]`) para acompanhamento.

**Objetivo:** encher a masmorra de coisas para pegar — as outras 4 armas (sabre, martelo a vapor, bacamarte, pistola Tesla), 3 armaduras, tônicos, munição e engrenagens —, com qualidade e modificações, inventário de 16 espaços (tecla I), itens rápidos (1–4), XP e níveis e o HUD completo. Antes, fechar as pendências da Etapa 3.

**Arquitetura:** tudo que é regra fica em `jogo/` e é testado sem janela: `Itens` (tabelas, qualidade, bônus, nomes, geração, população do chão e o que os inimigos soltam), `Inventario` (espaços, equipamentos, atributos derivados) e `Armas` (reescrita para usar a arma equipada e a munição por tipo). A `Partida` ganha o chão de itens, pegar, XP e a API usada pela tela de inventário. No `render/`, construtores puros do HUD completo e da tela de inventário (com a conversão do mouse da janela para a tela baixa); o `main` só liga as peças.

**Tecnologia:** a mesma das etapas anteriores.

**Spec:** `docs/superpowers/specs/2026-11-01-roguelike-design.md` (seções 4 "Atributos", "Armas brancas", "Armas de fogo", 6 inteira, 8 "HUD", 13 etapa 4)

## Restrições globais

- Tudo das etapas 1–3 continua valendo (inclusive: testes que podem gravar placar usam `--dados`; o classificador não deixa apagar nada no `%APPDATA%`).
- **Armas brancas:** chave inglesa 4–6 / 0,35 s / alcance 1,1; sabre 6–9 / 0,5 s / 1,3; martelo a vapor 11–15 / 0,9 s / 1,1, empurra bem mais longe. Arco de 60°, empurram e interrompem.
- **Armas de fogo:** revólver 7–10 / 0,4 s / carga 6 / recarga 1,5 s / balas / dispersão 2°; bacamarte 7 chumbos × 2–4 / 0,6 s / carga 2 / recarga 2,2 s / cartuchos / leque de 12° / dano pela metade depois de 4 células; pistola Tesla 14–20 / 0,7 s / sem recarga / 1 célula elétrica por disparo / alcance 15. Bacamarte a partir do andar 2, Tesla a partir do 4.
- **Munição:** contador por tipo, fora do inventário; máximos balas 48, cartuchos 20, células 12; caixas no chão: balas 6, cartuchos 4, células 3; pegar acima do máximo deixa o resto no chão.
- **Inventário:** 16 espaços + 3 equipamentos (arma branca, arma de fogo, armadura); tônicos se empilham.
- **Qualidade:** comum 70%, modificada 25% (1 bônus), obra-prima 5% (2 bônus), com chance de modificada/obra-prima subindo 2% por andar.
- **Bônus:** +dano, +defesa, +vida máxima, +velocidade de ataque; em armas de fogo também +carga e recarga mais rápida. Nome gerado (ex.: "Revólver Calibrado +3").
- **Armaduras:** casaco de couro 1; colete blindado 3 (andar 3+); armadura de latão 5 (andar 5+).
- **Tônico restaurador** cura 15. **Engrenagens:** pilhas de `5–20 × andar`, somam na pontuação.
- **Por andar:** 4–7 itens e 3–5 caixas de munição no chão; inimigo destruído: 25% de soltar item, 35% de soltar munição; Atiradores sempre soltam balas.
- **Pegar item:** olhar para ele a até 1,5 e apertar E; inventário cheio → mensagem e o item fica no chão.
- **XP para o próximo nível** = `20 × nível`; cada nível: +8 de vida máxima, +1 de dano base, vida cheia.
- **Início:** chave inglesa comum, revólver comum carregado + 12 balas, casaco de couro comum, 2 tônicos.
- **HUD:** manômetro de vida, nível e barra de XP, arma na mão, carga e reserva, engrenagens, andar, mensagens.
- **Constantes desta etapa** (não fixadas pela spec; ajustar após o teste):
  - tônicos empilham até **9** por espaço;
  - **martelo empurra 0,6** (chave e sabre 0,3);
  - qualidade no andar `a`: obra-prima `min(5 + 0,5·(a−1), 15)`%, modificada `min(25 + 1,5·(a−1), 45)`% (os 2% por andar divididos 1,5 + 0,5), resto comum;
  - valor dos bônus no andar `a` (divisão inteira): dano `1–3 + a/3`; defesa `1–2 + a/4`; vida máxima `5–10 + a`; velocidade de ataque `10–20`% (menos intervalo); carga `+1–2` (revólver) ou `+1` (bacamarte); recarga `15–30`% mais rápida. A Tesla não recebe carga nem recarga; armaduras recebem só defesa e vida máxima; armas brancas, só dano e velocidade;
  - pesos de sorteio de item: tônico 35, arma branca 20 (sabre 50, martelo 35, chave 15), arma de fogo 20 (revólver 50, bacamarte 35, Tesla 15 — só os disponíveis no andar, renormalizados), armadura 25 (casaco 50, colete 30, latão 20 — idem);
  - munição sorteada: balas 60, cartuchos 30 (andar 2+), células 10 (andar 4+);
  - 3–6 pilhas de engrenagens por andar; itens, munição e engrenagens nunca na sala inicial nem no elevador;
  - **munição e engrenagens são pegas automaticamente** ao passar a até 0,6 (como a válvula-chave); armas, armaduras e tônicos pedem E;
  - **itens rápidos 1–4 = espaços 1–4 do inventário** (a primeira linha da grade); dentro do inventário, arrastar troca dois espaços.
- **Arte:** ícones/sprites de itens e as armas novas na mão são **provisórios por script**, como os da Etapa 3 (sem download). Downloads novos só com ok.
- **Commit:** um único, no fim, com o "sim" do Nidragnnar.

## Decisões de projeto

- **Pegar munição e engrenagens sem apertar E:** a spec só exige E para "item"; andar por cima é o padrão de Doom e evita apertar E em cada caixa. A válvula-chave já funciona assim.
- **A carga fica na arma:** cada arma de fogo guarda a própria carga; trocar de revólver por outro revólver não "teletransporta" balas. Armas achadas vêm carregadas.
- **Equipar troca no mesmo espaço:** equipar um item do inventário coloca o equipado anterior no lugar dele, então nunca falta espaço para trocar. Não dá para ficar sem arma branca, sem arma de fogo ou sem armadura.
- **Tela de inventário com mouse:** abrir com I pausa o jogo e solta o mouse; clique esquerdo equipa/usa, clique direito larga no chão, arrastar troca espaços.
- **Verticalidade (Etapa 6):** nada nesta etapa supõe que o chão é sempre `Y = 0` além do que já existe (os itens no chão guardam só `x, z`, como os inimigos).

## Foco de revisão

1. **Inventário cheio** ao pegar com E, ao equipar e ao inimigo soltar item: nada some, nada duplica. → testes `cheio: E avisa e o item fica`, `equipar troca no mesmo espaço` (Tarefas 3 e 5).
2. **Munição acima do máximo** e caixa parcialmente pega: a sobra fica no chão com a quantidade certa; pegar de novo depois de gastar funciona. → teste `munição acima do máximo deixa o resto` (Tarefa 5).
3. **Trocar de arma de fogo no meio da recarga ou com carga zero; equipar arma nova durante a recarga:** sem balas fantasmas nem perdidas. → testes da Tarefa 4.
4. **Subir vários níveis de uma vez** (Colosso vale 20 XP no nível 1): todos os ganhos aplicados, vida cheia. → teste `XP que passa de dois níveis` (Tarefa 5).
5. **Mouse da tela de inventário com escala de tela e letterbox:** o clique cai no espaço certo com faixas pretas e com janela em 125%/150%. → testes `janela para tela baixa` (Tarefa 6).

---

## Estrutura de arquivos

```
src/jogo/IA.*, Projeteis.*, Partida.*, Placar.*   pendências da Etapa 3
src/jogo/Itens.h/.cpp        tipos, tabelas de armas e armaduras, qualidade, bônus, nome, geração, chão, o que inimigos soltam
src/jogo/Inventario.h/.cpp   16 espaços, equipamentos, empilhar, equipar, usar, atributos derivados
src/jogo/Armas.h/.cpp        reescrita: arma equipada, munição por tipo, sabre, martelo, bacamarte, Tesla, bônus
src/jogo/Comandos.h          + usarRapido (1–4)
src/jogo/Partida.h/.cpp      chão, pegar, soltar, XP e nível, tônicos, API do inventário, pontuação completa
src/render/Hud.h/.cpp        HUD completo
src/render/Inventario.h/.cpp tela do inventário: layout, espaço sob o mouse, construção (puro)
src/render/Letterbox.h/.cpp  + janelaParaTelaBaixa
src/render/Sprites.h         + camadas de itens no chão
src/plataforma/Entrada.*     I, 1–4, mouse livre no inventário (posição, pressionar/soltar, botão direito)
src/main.cpp                 liga tudo; pausa com o inventário
ferramentas/gerar_itens.ps1  ícones/sprites provisórios dos itens e armas novas na mão
testes/teste_itens.cpp, teste_inventario.cpp, teste_inventario_tela.cpp (+ casos em teste_armas, teste_partida,
teste_ia, teste_placar, teste_hud, teste_letterbox)
```

---

### Tarefa 1: Pendências da Etapa 3

**Arquivos:**
- Modificar: `src/jogo/IA.h/.cpp`, `src/jogo/Projeteis.h/.cpp`, `src/jogo/Partida.cpp`, `src/jogo/Placar.h/.cpp`, `src/main.cpp`
- Testar: `testes/teste_ia.cpp`, `testes/teste_projeteis.cpp`, `testes/teste_partida.cpp`, `testes/teste_placar.cpp`

**Interfaces:**
- Produz:
  - `struct jogo::Golpe { int dano; TipoInimigo autor; };`
  - `EventosIA`: troca `dano`/`autor` por `std::vector<Golpe> golpes`; `ResultadoProjeteis`: idem (`std::vector<Golpe> golpes`)
  - `Partida`: aplica `max(1, dano − defesa)` **por golpe**, na ordem; a causa da morte é o golpe que zerou a vida
  - Atirador: se o recuo não o moveu (encurralado), vai para `Preparando` e atira mesmo de perto
  - Jogador não atravessa inimigos: depois de mover, a `Partida` empurra o jogador para fora de cada inimigo vivo a menos de `RAIO_JOGADOR + RAIO_INIMIGO` (com `moverComColisao`)
  - `LeituraPlacar::renomeado` (o `.corrompido` foi mesmo criado); o log diz o que aconteceu
  - `main`: Enter só vale 0,5 s depois da morte

- [ ] **Passo 1: Escrever os testes**

```cpp
// teste_ia.cpp (os testes antigos passam a somar eventos.golpes)
TEST_CASE("dois golpes no mesmo passo chegam separados") {
    auto a = mundo::andarDeTexto({"#####", "#r@r#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    auto e = simular(v, a.grade, {2.5f, 1.5f}, 0.45f);   // aproximam, preparam juntos (0,3 s) e mordem; recuperação 0,6 s
    CHECK(e.golpes.size() == 2);
}
TEST_CASE("atirador encurralado atira de perto") {
    auto a = mundo::andarDeTexto({"########", "#g.@...#", "########"}, 2);
    auto v = jogo::inimigosDasMarcas(a);
    auto e = simular(v, a.grade, {3.5f, 1.5f}, 2.5f);
    CHECK(e.tiros.size() >= 1);
}
// teste_partida.cpp
TEST_CASE("jogador não atravessa inimigo") {
    jogo::Partida p(mundo::andarDeTexto({"#########", "#@..a...#", "#########"}, 1), 1);
    p.tornarInvencivel();
    passos(p, 120, {1, 0, 0});
    CHECK(p.jogador().posicao.x <= p.inimigos()[0].posicao.x - (jogo::RAIO_JOGADOR + jogo::RAIO_INIMIGO) + 0.02f);
}
TEST_CASE("defesa vale para cada golpe do mesmo passo") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#r@r#", "#####"}, 1), 3);
    p.definirPosicao({2.5f, 1.5f});
    passos(p, 25);                                    // ~0,42 s: só a primeira dupla de mordidas
    // cada mordida 2–3 menos defesa 1 = 1–2; somado 2–4 (com a regra antiga, 3–5)
    CHECK(p.vida() >= 30 - 4);
    CHECK(p.vida() <= 30 - 2);
}
// teste_placar.cpp
TEST_CASE("corrompido informa se renomeou") {
    PastaTemporaria pasta;
    { std::ofstream f(pasta.caminho / "placar.txt"); f << "lixo\n"; }
    CHECK(jogo::lerPlacar(pasta.caminho / "placar.txt").renomeado);
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha (compilação: `golpes`, `renomeado`; comportamento: atirador encurralado e jogador atravessando).
- [ ] **Passo 3: Implementar.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 2: Itens — tabelas, qualidade, bônus, nomes, geração e chão

**Arquivos:**
- Criar: `src/jogo/Itens.h/.cpp`
- Testar: `testes/teste_itens.cpp`

**Interfaces:**
- Produz:
  - `enum class jogo::TipoItem { ChaveInglesa, Sabre, Martelo, Revolver, Bacamarte, Tesla, Casaco, Colete, ArmaduraLatao, Tonico };`
  - `enum class jogo::ClasseItem { ArmaBranca, ArmaDeFogo, Armadura, Consumivel };` e `ClasseItem jogo::classe(TipoItem)`
  - `enum class jogo::TipoMunicao { Balas, Cartuchos, Celulas };`
  - `struct jogo::DadosArma { const char* nome; bool feminino; int danoMin, danoMax; float intervalo; float alcance; int carga; float recarga; TipoMunicao municao; int chumbos; float abertura; float empurrao; int andarMinimo; };` (abertura = dispersão total em graus; `carga 0` = sem recarga, gasta da reserva; `alcance` das armas de fogo: 0 = sem limite)
  - `struct jogo::DadosArmadura { const char* nome; bool feminino; int defesa; int andarMinimo; };`
  - `const DadosArma& jogo::dadosArma(TipoItem)`, `const DadosArmadura& jogo::dadosArmadura(TipoItem)`
  - `enum class jogo::Qualidade { Comum, Modificada, ObraPrima };`
  - `enum class jogo::TipoBonus { Dano, Defesa, VidaMaxima, Velocidade, Carga, Recarga };` (Velocidade e Recarga em %)
  - `struct jogo::Bonus { TipoBonus tipo; int valor; };`
  - `struct jogo::Item { TipoItem tipo; Qualidade qualidade = Qualidade::Comum; std::vector<Bonus> bonus; int quantidade = 1; int carga = 0; };`
  - `int jogo::somaBonus(const Item& i, TipoBonus t);`
  - `int jogo::capacidade(const Item& arma);` (carga da tabela + bônus de carga)
  - `jogo::Item jogo::criarItem(TipoItem t);` (comum; arma de fogo já carregada)
  - `std::string jogo::nomeItem(const Item& i);` — nome da tabela; com bônus: `nome + " " + adjetivo(primeiro bônus, gênero) + " +" + valor` (+ `"%"` para Velocidade e Recarga); tônico com quantidade > 1: `"Tônico restaurador ×3"`. Adjetivos (masc./fem.): Dano arma branca "Afiado/Afiada", Dano arma de fogo "Calibrado/Calibrada", Defesa "Reforçado/Reforçada", VidaMaxima "Vital", Velocidade "Veloz", Carga "Ampliado/Ampliada", Recarga "Ágil"
  - Nomes da tabela: "Chave inglesa"(f), "Sabre de oficial", "Martelo a vapor", "Revólver", "Bacamarte", "Pistola Tesla"(f), "Casaco de couro", "Colete blindado", "Armadura de latão"(f), "Tônico restaurador"
  - `jogo::Item jogo::gerarItem(int andar, std::mt19937& rng);` (pesos, disponibilidade, qualidade e bônus das Restrições globais; obra-prima com 2 bônus de tipos diferentes)
  - `struct jogo::MunicaoNoChao { TipoMunicao tipo; int quantidade; };`
  - `struct jogo::CoisaNoChao { glm::vec2 posicao; std::variant<Item, MunicaoNoChao, int> conteudo; };` (`int` = engrenagens)
  - `std::vector<CoisaNoChao> jogo::gerarChao(const mundo::Andar& andar, uint32_t semente);` (rng = `mt19937(sementeDoAndar(semente, n) ^ 0x17E5u)`)
  - `std::vector<CoisaNoChao> jogo::soltarDoInimigo(TipoInimigo t, glm::vec2 posicao, int andar, std::mt19937& rng);`

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("tabelas da spec") {
    CHECK(jogo::dadosArma(jogo::TipoItem::Bacamarte).chumbos == 7);
    CHECK(jogo::dadosArma(jogo::TipoItem::Bacamarte).carga == 2);
    CHECK(jogo::dadosArma(jogo::TipoItem::Tesla).alcance == doctest::Approx(15.0f));
    CHECK(jogo::dadosArma(jogo::TipoItem::Tesla).carga == 0);
    CHECK(jogo::dadosArma(jogo::TipoItem::Martelo).empurrao == doctest::Approx(0.6f));
    CHECK(jogo::dadosArmadura(jogo::TipoItem::ArmaduraLatao).defesa == 5);
    CHECK(jogo::criarItem(jogo::TipoItem::Revolver).carga == 6);
}
TEST_CASE("nomes gerados") {
    CHECK(jogo::nomeItem(jogo::criarItem(jogo::TipoItem::Sabre)) == "Sabre de oficial");
    jogo::Item r{jogo::TipoItem::Revolver, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Dano, 3}}};
    CHECK(jogo::nomeItem(r) == "Revólver Calibrado +3");
    jogo::Item c{jogo::TipoItem::ChaveInglesa, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Velocidade, 15}}};
    CHECK(jogo::nomeItem(c) == "Chave inglesa Veloz +15%");
    jogo::Item l{jogo::TipoItem::ArmaduraLatao, jogo::Qualidade::ObraPrima,
                 {{jogo::TipoBonus::Defesa, 2}, {jogo::TipoBonus::VidaMaxima, 8}}};
    CHECK(jogo::nomeItem(l) == "Armadura de latão Reforçada +2");
    jogo::Item t = jogo::criarItem(jogo::TipoItem::Tonico);
    t.quantidade = 3;
    CHECK(jogo::nomeItem(t) == "Tônico restaurador ×3");
}
TEST_CASE("disponibilidade por andar") {
    std::mt19937 rng(1);
    for (int i = 0; i < 2000; ++i) {
        auto t = jogo::gerarItem(1, rng).tipo;
        REQUIRE(t != jogo::TipoItem::Bacamarte);
        REQUIRE(t != jogo::TipoItem::Tesla);
        REQUIRE(t != jogo::TipoItem::Colete);
        REQUIRE(t != jogo::TipoItem::ArmaduraLatao);
    }
    bool viuTesla = false, viuLatao = false;
    for (int i = 0; i < 5000; ++i) {
        auto t = jogo::gerarItem(5, rng).tipo;
        viuTesla |= t == jogo::TipoItem::Tesla;
        viuLatao |= t == jogo::TipoItem::ArmaduraLatao;
    }
    CHECK(viuTesla);
    CHECK(viuLatao);
}
TEST_CASE("frequência de qualidade no andar 1") {
    std::mt19937 rng(2);
    int comum = 0, obra = 0, total = 0;
    for (int i = 0; i < 10000; ++i) {
        auto it = jogo::gerarItem(1, rng);
        if (it.tipo == jogo::TipoItem::Tonico) continue;
        ++total;
        comum += it.qualidade == jogo::Qualidade::Comum;
        obra += it.qualidade == jogo::Qualidade::ObraPrima;
    }
    CHECK(comum * 100.0 / total == doctest::Approx(70.0).epsilon(0.05));
    CHECK(obra * 100.0 / total == doctest::Approx(5.0).epsilon(0.3));
}
TEST_CASE("bônus só onde cabem") {
    std::mt19937 rng(3);
    for (int i = 0; i < 5000; ++i) {
        auto it = jogo::gerarItem(6, rng);
        auto c = jogo::classe(it.tipo);
        REQUIRE(it.bonus.size() == (it.qualidade == jogo::Qualidade::Comum ? 0u
                                    : it.qualidade == jogo::Qualidade::Modificada ? 1u : 2u));
        if (it.bonus.size() == 2) REQUIRE(it.bonus[0].tipo != it.bonus[1].tipo);
        for (auto b : it.bonus) {
            if (c == jogo::ClasseItem::Armadura)
                REQUIRE((b.tipo == jogo::TipoBonus::Defesa || b.tipo == jogo::TipoBonus::VidaMaxima));
            if (c == jogo::ClasseItem::ArmaBranca)
                REQUIRE((b.tipo == jogo::TipoBonus::Dano || b.tipo == jogo::TipoBonus::Velocidade));
            if (it.tipo == jogo::TipoItem::Tesla)
                REQUIRE((b.tipo != jogo::TipoBonus::Carga && b.tipo != jogo::TipoBonus::Recarga));
        }
    }
}
TEST_CASE("chão do andar") {
    for (uint32_t s = 1; s <= 100; ++s) {
        auto a = mundo::gerarAndar(s, 3);
        auto chao = jogo::gerarChao(a, s);
        int itens = 0, municao = 0, engrenagens = 0;
        for (auto& c : chao) {
            REQUIRE_FALSE(a.salas[0].contem(int(c.posicao.x), int(c.posicao.y)));
            REQUIRE(a.grade.em(int(c.posicao.x), int(c.posicao.y)) == mundo::Celula::Chao);
            if (std::holds_alternative<jogo::Item>(c.conteudo)) ++itens;
            else if (std::holds_alternative<jogo::MunicaoNoChao>(c.conteudo)) ++municao;
            else {
                ++engrenagens;
                int v = std::get<int>(c.conteudo);
                REQUIRE((v >= 5 * 3 && v <= 20 * 3));
            }
        }
        REQUIRE((itens >= 4 && itens <= 7));
        REQUIRE((municao >= 3 && municao <= 5));
        REQUIRE((engrenagens >= 3 && engrenagens <= 6));
    }
}
TEST_CASE("o que os inimigos soltam") {
    std::mt19937 rng(4);
    int itens = 0, municao = 0;
    for (int i = 0; i < 4000; ++i)
        for (auto& c : jogo::soltarDoInimigo(jogo::TipoInimigo::Automato, {1, 1}, 1, rng)) {
            itens += std::holds_alternative<jogo::Item>(c.conteudo);
            municao += std::holds_alternative<jogo::MunicaoNoChao>(c.conteudo);
        }
    CHECK(itens / 4000.0 == doctest::Approx(0.25).epsilon(0.1));
    CHECK(municao / 4000.0 == doctest::Approx(0.35).epsilon(0.1));
    for (int i = 0; i < 200; ++i) {
        bool balas = false;
        for (auto& c : jogo::soltarDoInimigo(jogo::TipoInimigo::Atirador, {1, 1}, 1, rng))
            if (auto m = std::get_if<jogo::MunicaoNoChao>(&c.conteudo)) balas |= m->tipo == jogo::TipoMunicao::Balas;
        REQUIRE(balas);
    }
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `Itens`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 3: Inventário e atributos

**Arquivos:**
- Criar: `src/jogo/Inventario.h/.cpp`
- Testar: `testes/teste_inventario.cpp`

**Interfaces:**
- Consome: `Item`, `classe`, `somaBonus`, `dadosArmadura` (Tarefa 2).
- Produz:
  - `constexpr int jogo::ESPACOS_INVENTARIO = 16, jogo::PILHA_MAXIMA_TONICOS = 9;`
  - `class jogo::Inventario`
    - `Inventario();` — o equipamento e os 2 tônicos iniciais (espaço 0)
    - `const std::optional<Item>& espaco(int i) const;`
    - `const Item& armaBranca() const; const Item& armaDeFogo() const; const Item& armadura() const; Item& armaDeFogo();`
    - `bool adicionar(Item i);` (tônico empilha num espaço com tônico e menos de 9; senão primeiro espaço livre; falso se não coube)
    - `std::optional<Item> remover(int i);`
    - `void trocar(int a, int b);`
    - `bool equipar(int i);` (arma branca, arma de fogo ou armadura: troca com o equipado no mesmo espaço; falso para tônico/vazio)
    - `bool consumirTonico(int i);` (tira 1; o espaço esvazia em 0)
  - `struct jogo::Atributos { int danoBase; int defesa; int vidaMaxima; };`
  - `jogo::Atributos jogo::calcularAtributos(const Inventario& inv, int nivel);` — `danoBase = nivel − 1`; `defesa = armadura + bônus de defesa`; `vidaMaxima = 30 + 8·(nivel − 1) + bônus de vida`

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("inventário inicial") {
    jogo::Inventario inv;
    CHECK(inv.armaBranca().tipo == jogo::TipoItem::ChaveInglesa);
    CHECK(inv.armaDeFogo().tipo == jogo::TipoItem::Revolver);
    CHECK(inv.armaDeFogo().carga == 6);
    CHECK(inv.armadura().tipo == jogo::TipoItem::Casaco);
    REQUIRE(inv.espaco(0).has_value());
    CHECK(inv.espaco(0)->quantidade == 2);
}
TEST_CASE("tônicos empilham até 9") {
    jogo::Inventario inv;
    for (int i = 0; i < 8; ++i) inv.adicionar(jogo::criarItem(jogo::TipoItem::Tonico));
    CHECK(inv.espaco(0)->quantidade == 9);
    CHECK(inv.espaco(1)->quantidade == 1);
}
TEST_CASE("cheio recusa") {
    jogo::Inventario inv;
    for (int i = 1; i < jogo::ESPACOS_INVENTARIO; ++i) REQUIRE(inv.adicionar(jogo::criarItem(jogo::TipoItem::Sabre)));
    CHECK_FALSE(inv.adicionar(jogo::criarItem(jogo::TipoItem::Martelo)));
}
TEST_CASE("equipar troca no mesmo espaço") {
    jogo::Inventario inv;
    inv.adicionar(jogo::criarItem(jogo::TipoItem::Martelo));   // espaço 1
    REQUIRE(inv.equipar(1));
    CHECK(inv.armaBranca().tipo == jogo::TipoItem::Martelo);
    CHECK(inv.espaco(1)->tipo == jogo::TipoItem::ChaveInglesa);
    CHECK_FALSE(inv.equipar(0));                                // tônico não equipa
}
TEST_CASE("consumir tônico") {
    jogo::Inventario inv;
    CHECK(inv.consumirTonico(0));
    CHECK(inv.espaco(0)->quantidade == 1);
    CHECK(inv.consumirTonico(0));
    CHECK_FALSE(inv.espaco(0).has_value());
    CHECK_FALSE(inv.consumirTonico(0));
}
TEST_CASE("atributos com nível e bônus") {
    jogo::Inventario inv;
    inv.adicionar({jogo::TipoItem::Colete, jogo::Qualidade::ObraPrima,
                   {{jogo::TipoBonus::Defesa, 2}, {jogo::TipoBonus::VidaMaxima, 7}}});
    inv.equipar(1);
    auto a = jogo::calcularAtributos(inv, 3);
    CHECK(a.danoBase == 2);
    CHECK(a.defesa == 3 + 2);
    CHECK(a.vidaMaxima == 30 + 16 + 7);
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar `Inventario`.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 4: Armas com equipamento e munição por tipo

**Arquivos:**
- Modificar: `src/jogo/Armas.h/.cpp`, `src/jogo/Partida.*` (só a chamada), `testes/teste_armas.cpp` (os casos antigos passam a montar um `Inventario`)
- Testar: `testes/teste_armas.cpp`

**Interfaces:**
- Consome: `Inventario`, `Atributos`, `dadosArma`, `capacidade`, `somaBonus` (Tarefas 2 e 3).
- Produz:
  - `enum class jogo::ArmaAtiva { Branca, Fogo };` (substitui ChaveInglesa/Revolver)
  - `struct jogo::Municao { int balas = 12, cartuchos = 0, celulas = 0; int& de(TipoMunicao t); };` e `int jogo::maximoMunicao(TipoMunicao t);` (48 / 20 / 12)
  - `struct jogo::Armas { ArmaAtiva ativa = ArmaAtiva::Branca; Municao municao; float intervalo = 0, recarga = 0, animacao = 0; };`
  - `jogo::ResultadoAtaque jogo::atualizarArmas(Armas& a, Inventario& inv, const Atributos& atr, const Comandos& c, glm::vec2 posicao, float angulo, float inclinacao, const mundo::Grade& g, std::vector<Inimigo>& inimigos, std::mt19937& rng, float dt);`
  - **Regras novas:** dano da arma + `danoBase` + bônus de dano; intervalo × `(1 − velocidade%/100)`; recarga × `(1 − recarga%/100)`; capacidade = `capacidade(arma)`; recarga move `min(capacidade − carga, reserva do tipo)`; **Tesla** (`carga 0`): cada disparo gasta 1 da reserva, sem recarga, alcance 15; **bacamarte**: 7 traços com desvio uniforme em `±abertura/2`, cada um com 2–4 (+ bônus/base), dano pela metade se `t > 4`, todos com a mira vertical; empurrão da tabela; trocar de arma de fogo equipada (pelo inventário) cancela a recarga
  - Mensagem sem munição: `"Sem munição — R para recarregar"` quando há reserva, `"Sem munição"` quando não há (Tesla: só `"Sem células elétricas"`)

- [ ] **Passo 1: Escrever os testes** (helper `inventarioCom(TipoItem branca, TipoItem fogo)`; os casos da Etapa 3 continuam, adaptados)

```cpp
TEST_CASE("sabre alcança mais longe que a chave") {
    auto a = mundo::andarDeTexto({"########", "#@.a...#", "########"}, 1);
    // autômato a 1,5 do jogador = 1,25 até a borda: o sabre (1,3) alcança, a chave (1,1) não
    auto v = jogo::inimigosDasMarcas(a);
    v[0].posicao.x = 1.5f + 1.5f;
    auto inv = inventarioCom(jogo::TipoItem::Sabre, jogo::TipoItem::Revolver);
    jogo::Armas armas; std::mt19937 rng(1);
    CHECK(atacar(armas, inv, a, v, rng).acertos == 1);
    auto inv2 = inventarioCom(jogo::TipoItem::ChaveInglesa, jogo::TipoItem::Revolver);
    jogo::Armas armas2;
    CHECK(atacar(armas2, inv2, a, v, rng).acertos == 0);
}
TEST_CASE("martelo empurra 0,6") {
    auto a = mundo::andarDeTexto({"#######", "#@a...#", "#######"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    auto inv = inventarioCom(jogo::TipoItem::Martelo, jogo::TipoItem::Revolver);
    jogo::Armas armas; std::mt19937 rng(1);
    atacar(armas, inv, a, v, rng);
    CHECK(glm::length(v[0].empurrao) == doctest::Approx(0.6f));
}
TEST_CASE("bacamarte: 7 chumbos, dano cai pela metade longe") {
    auto perto = mundo::andarDeTexto({"#######", "#@.c..#", "#######"}, 1);   // colosso a 2
    auto longe = mundo::andarDeTexto({"#########", "#@.....c#", "#########"}, 1);   // colosso a 6
    auto vp = jogo::inimigosDasMarcas(perto), vl = jogo::inimigosDasMarcas(longe);
    auto inv = inventarioCom(jogo::TipoItem::ChaveInglesa, jogo::TipoItem::Bacamarte);
    std::mt19937 rng(5);
    jogo::Armas a1; a1.ativa = jogo::ArmaAtiva::Fogo;
    jogo::Armas a2 = a1;
    atacar(a1, inv, perto, vp, rng);
    atacar(a2, inv, longe, vl, rng);
    int danoPerto = 40 - vp[0].vida, danoLonge = 40 - vl[0].vida;
    CHECK(danoPerto >= 7 * 1);              // 7 × (2–4 − defesa 2, mínimo 1)
    CHECK(danoPerto > danoLonge);
    CHECK(inv.armaDeFogo().carga == 1);
}
TEST_CASE("Tesla gasta célula da reserva e não recarrega") {
    auto a = mundo::andarDeTexto({"####################", "#@.............a...#", "####################"}, 4);
    auto v = jogo::inimigosDasMarcas(a);
    auto inv = inventarioCom(jogo::TipoItem::ChaveInglesa, jogo::TipoItem::Tesla);
    jogo::Armas armas; armas.ativa = jogo::ArmaAtiva::Fogo; armas.municao.celulas = 2;
    std::mt19937 rng(1);
    CHECK(atacar(armas, inv, a, v, rng).acertos == 1);   // a 14 células: dentro do alcance 15
    CHECK(armas.municao.celulas == 1);
    jogo::Comandos r; r.recarregar = true;
    jogo::atualizarArmas(armas, inv, jogo::calcularAtributos(inv, 1), r, a.inicio, 0, 0, a.grade, v, rng, 1.0f / 60);
    CHECK(armas.recarga == 0.0f);
}
TEST_CASE("bônus de velocidade e de carga") {
    auto a = mundo::andarDeTexto({"#####", "#@c.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    inv.adicionar({jogo::TipoItem::ChaveInglesa, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Velocidade, 20}}});
    inv.equipar(1);
    jogo::Armas armas; std::mt19937 rng(1);
    atacar(armas, inv, a, v, rng);
    CHECK(armas.intervalo == doctest::Approx(0.35f * 0.8f).epsilon(0.02));
    jogo::Item rev{jogo::TipoItem::Revolver, jogo::Qualidade::Modificada, {{jogo::TipoBonus::Carga, 2}}};
    CHECK(jogo::capacidade(rev) == 8);
}
TEST_CASE("dano base do nível soma ao golpe") {
    auto a = mundo::andarDeTexto({"#####", "#@c.#", "#####"}, 1);
    auto v = jogo::inimigosDasMarcas(a);
    jogo::Inventario inv;
    jogo::Armas armas; std::mt19937 rng(1);
    jogo::Comandos c; c.atacar = true;
    jogo::atualizarArmas(armas, inv, jogo::calcularAtributos(inv, 5), c, a.inicio, 0, 0, a.grade, v, rng, 1.0f / 60);
    CHECK(v[0].vida <= 40 - (4 + 4 - 2));   // 4–6 + 4 de base − defesa 2
}
```

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Reescrever `Armas`** (o `atacar(...)` dos testes é um helper do arquivo de teste que monta `Comandos{atacar}` e chama `atualizarArmas` com `calcularAtributos(inv, 1)`).
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 5: Partida com chão, pegar, XP, tônicos e inventário

**Arquivos:**
- Modificar: `src/jogo/Comandos.h` (+ `int usarRapido = 0;` — 1 a 4, evento), `src/jogo/Partida.h/.cpp`
- Testar: `testes/teste_partida.cpp`

**Interfaces:**
- Consome: Tarefas 2–4.
- Produz (Partida):
  - `const Inventario& inventario() const; const std::vector<CoisaNoChao>& chao() const; int nivel() const; int xp() const; int xpParaProximo() const; int engrenagens() const; const Atributos& atributos() const;`
  - `void usarEspaco(int i);` (equipa, ou bebe tônico) `void largarEspaco(int i);` (no chão a 0,6 à frente) `void trocarEspacos(int a, int b);`
  - `constexpr int jogo::CURA_TONICO = 15;` `constexpr float jogo::ALCANCE_PEGAR = 1.5f;`
  - **E** pega o item mais próximo à frente (`distância ≤ 1,5`, desvio lateral do raio ≤ 0,4, com linha de visão) antes de tentar comporta/elevador; cheio → `"Inventário cheio"`; pegou → `"Você pegou: <nome>"`
  - munição e engrenagens: automáticas a até `RAIO_PEGAR`; munição respeita o máximo e a sobra fica na caixa
  - inimigo destruído: soma XP (`dados(tipo).xp`) e usa `soltarDoInimigo` na posição dele
  - subir de nível em laço enquanto `xp ≥ 20 × nível`: `xp −= 20 × nível`, `nivel += 1`, recalcula atributos, vida cheia, `"Nível N!"`
  - `usarRapido = n` usa o espaço `n − 1` (`usarEspaco`)
  - tônico: cura 15 até a vida máxima; vida cheia → `"Vida cheia"` e não gasta
  - vida máxima nova (armadura com vida, nível) recalculada ao equipar; se baixar, a vida corta no máximo
  - `pontuacao()` usa nível e engrenagens reais; o chão é limpo e regerado ao descer

- [ ] **Passo 1: Escrever os testes**

```cpp
TEST_CASE("E pega o item à frente") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1), 1);
    p.colocarNoChao({{2.5f, 1.5f}, jogo::criarItem(jogo::TipoItem::Sabre)});   // função de teste
    passos(p, 1, apertarE());
    CHECK(p.inventario().espaco(1)->tipo == jogo::TipoItem::Sabre);
    CHECK(p.chao().empty());
    CHECK(p.mensagem() == "Você pegou: Sabre de oficial");
}
TEST_CASE("cheio: E avisa e o item fica") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1), 1);
    for (int i = 1; i < 16; ++i) p.colocarNoChao({{1.6f, 1.5f}, jogo::criarItem(jogo::TipoItem::Martelo)});
    for (int i = 1; i < 16; ++i) passos(p, 1, apertarE());
    p.colocarNoChao({{2.5f, 1.5f}, jogo::criarItem(jogo::TipoItem::Sabre)});
    passos(p, 1, apertarE());
    CHECK(p.mensagem() == "Inventário cheio");
    CHECK(p.chao().size() == 1);
}
TEST_CASE("munição acima do máximo deixa o resto") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1), 1);
    p.colocarNoChao({{2.5f, 1.5f}, jogo::MunicaoNoChao{jogo::TipoMunicao::Balas, 40}});
    passos(p, 30, {1, 0, 0});
    CHECK(p.armas().municao.balas == 48);
    REQUIRE(p.chao().size() == 1);
    CHECK(std::get<jogo::MunicaoNoChao>(p.chao()[0].conteudo).quantidade == 4);
}
TEST_CASE("engrenagens somam na pontuação") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1), 1);
    p.colocarNoChao({{2.5f, 1.5f}, 17});
    passos(p, 30, {1, 0, 0});
    CHECK(p.engrenagens() == 17);
    CHECK(p.pontuacao() == 100 + 17 + 50);
}
TEST_CASE("XP que passa de dois níveis") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@c.#", "#####"}, 1), 1);
    p.ganharXp(20 + 40 + 5);                          // função de teste: nível 1→2 (20) e 2→3 (40), sobram 5
    CHECK(p.nivel() == 3);
    CHECK(p.xp() == 5);
    CHECK(p.vidaMaxima() == 30 + 16);
    CHECK(p.vida() == p.vidaMaxima());
    CHECK(p.mensagem() == "Nível 3!");
}
TEST_CASE("destruir dá XP") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@r.#", "#####"}, 1), 1);
    jogo::Comandos golpe; golpe.atacar = true;
    for (int i = 0; i < 60; ++i) p.atualizar(golpe, 1.0f / 60);
    CHECK(p.xp() == 3);
}
TEST_CASE("tônico rápido cura e não gasta com vida cheia") {
    jogo::Partida p(mundo::andarDeTexto({"#####", "#@..#", "#####"}, 1), 1);
    jogo::Comandos um; um.usarRapido = 1;
    passos(p, 1, um);
    CHECK(p.mensagem() == "Vida cheia");
    CHECK(p.inventario().espaco(0)->quantidade == 2);
    p.ferirParaTeste(20);                              // função de teste
    passos(p, 1, um);
    CHECK(p.vida() == 25);
    CHECK(p.inventario().espaco(0)->quantidade == 1);
}
TEST_CASE("largar põe à frente e pegar de volta") {
    jogo::Partida p(mundo::andarDeTexto({"######", "#@...#", "######"}, 1), 1);
    p.largarEspaco(0);
    REQUIRE(p.chao().size() == 1);
    CHECK_FALSE(p.inventario().espaco(0).has_value());
    passos(p, 1, apertarE());
    CHECK(p.inventario().espaco(0)->quantidade == 2);
}
TEST_CASE("chão do andar gerado e regerado ao descer") {
    jogo::Partida p(5, 2);
    CHECK(p.chao().size() >= 4 + 3 + 3);
}
```

`colocarNoChao`, `ganharXp` e `ferirParaTeste` são métodos públicos marcados como "só para testes" no header (mesma linha de `tornarInvencivel`).

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 6: HUD completo, tela de inventário e mouse (puros)

**Arquivos:**
- Modificar: `src/render/Hud.h/.cpp`, `src/render/Letterbox.h/.cpp`, `src/render/Sprites.h`, `src/render/AnimacaoInimigos.h` (constante total)
- Criar: `src/render/Inventario.h/.cpp`
- Testar: `testes/teste_hud.cpp` (adaptado), `testes/teste_letterbox.cpp`, `testes/teste_inventario_tela.cpp`

**Interfaces:**
- Produz:
  - `glm::vec2 render::janelaParaTelaBaixa(glm::vec2 mouse, glm::ivec2 tamanhoJanela, glm::ivec2 tamanhoDesenhavel, int baseL, int baseA);` (escala janela→pixels, tira o letterbox, divide pela ampliação; fora da imagem pode dar negativo/maior)
  - `int render::iconeDoItem(jogo::TipoItem t)`, `int render::iconeDaMunicao(jogo::TipoMunicao t)`, `constexpr int render::ICONE_ENGRENAGENS` — camadas no array de sprites a partir de `PRIMEIRO_ITEM = PRIMEIRO_INIMIGO + 4 × QUADROS_POR_INIMIGO` (ordem: os 10 `TipoItem`, balas, cartuchos, células, engrenagens)
  - `QuadroArma` cresce: `0` chave parada, `1` chave golpe, `2` revólver parado, `3` tiro, `4` recarga, `5` sabre parado, `6` sabre golpe, `7` martelo parado, `8` martelo golpe, `9` bacamarte parado, `10` tiro, `11` recarga, `12` Tesla parada, `13` Tesla disparo; `int render::quadroArma(jogo::TipoItem arma, bool ataque, bool recarregando)`
  - `EstadoHud` vira struct com valores padrão e campos nomeados: `vida, vidaMaxima, nivel, xp, xpProximo, engrenagens, andar; TipoItem arma; bool armaDeFogoAtiva; int carga, reserva; TipoMunicao municao; bool semRecarga; bool recarregando; float animacao, tempoDesdeDano; std::string_view mensagem; std::array<RapidoHud, 4> rapidos;` com `struct RapidoHud { int icone = -1; int quantidade = 0; };`
  - HUD: manômetro (moldura latão de 2 px em volta da barra de vida), `"Nv 3"` e barra fina de XP (100 × 3 px) logo acima da vida; munição `"6 | 12"` + rótulo `balas`/`cartuchos`/`células` (Tesla: `"| 12 células"`); `"⚙"` não existe na fonte: engrenagens como `"Engrenagens 120"` em cima à direita; 4 quadrinhos de itens rápidos (16 px, com número 1–4 e quantidade) em baixo, ao lado da vida; mira e flash como antes
  - Tela do inventário: `struct render::LayoutInventario { std::array<Retangulo, 16> espacos; std::array<Retangulo, 3> equipados; Retangulo painel; };`, `LayoutInventario render::layoutInventario(int telaL, int telaA);` (grade 4×4 de 36 px com 4 px de folga, equipados numa coluna à direita), `std::optional<int> render::espacoSob(const LayoutInventario&, glm::vec2 ponto);` (0–15 espaços, 16–18 equipados), `ElementosInventario render::construirInventario(const jogo::Inventario&, const LayoutInventario&, std::optional<int> sobMouse, std::optional<int> arrastando, glm::vec2 mouse)` com cores, texto (título "Inventário", nomes dos equipados, nome e atributos do item sob o mouse colorido pela qualidade: comum branco, modificada azul `(0.45, 0.7, 1)`, obra-prima dourado `(1, 0.8, 0.3)`, ajuda "Clique: usar/equipar  Direito: largar  Arraste: trocar  I: fechar") e ícones

- [ ] **Passo 1: Escrever os testes**

```cpp
// teste_letterbox.cpp
TEST_CASE("janela para tela baixa sem faixas") {
    auto p = render::janelaParaTelaBaixa({640, 360}, {1280, 720}, {1280, 720}, 480, 270);
    CHECK(p.x == doctest::Approx(240)); CHECK(p.y == doctest::Approx(135));
}
TEST_CASE("janela para tela baixa com faixas e escala de tela 150%") {
    // janela lógica 1280×800 (16:10) em 150% → 1920×1200 pixels; imagem 1920×1080 com faixas de 60 px
    auto p = render::janelaParaTelaBaixa({640, 40}, {1280, 800}, {1920, 1200}, 480, 270);
    CHECK(p.y == doctest::Approx(0.0f).epsilon(0.01));    // 40 lógicos = 60 px = topo da imagem
    CHECK(p.x == doctest::Approx(240));
}
// teste_inventario_tela.cpp
TEST_CASE("espaço sob o mouse") {
    auto l = render::layoutInventario(480, 270);
    auto centro = [](render::Retangulo r) { return glm::vec2(r.x + r.largura / 2.0f, r.y + r.altura / 2.0f); };
    CHECK(render::espacoSob(l, centro(l.espacos[5])) == 5);
    CHECK(render::espacoSob(l, centro(l.equipados[2])) == 18);
    CHECK_FALSE(render::espacoSob(l, {0, 0}).has_value());
}
TEST_CASE("todos os espaços cabem na tela baixa sem se sobrepor") {
    auto l = render::layoutInventario(480, 270);
    for (auto& r : l.espacos) { REQUIRE(r.x >= 0); REQUIRE(r.y >= 0); REQUIRE(r.x + r.largura <= 480); REQUIRE(r.y + r.altura <= 270); }
    for (int a = 0; a < 16; ++a) for (int b = a + 1; b < 16; ++b) {
        auto& p = l.espacos[a]; auto& q = l.espacos[b];
        REQUIRE((p.x + p.largura <= q.x || q.x + q.largura <= p.x || p.y + p.altura <= q.y || q.y + q.altura <= p.y));
    }
}
// teste_hud.cpp (casos antigos adaptados ao EstadoHud novo) +
TEST_CASE("barra de XP proporcional") { /* xp 10 de 20 → 50 px de preenchimento */ }
TEST_CASE("rótulo da munição pelo tipo") { /* cartuchos → texto contém "cartuchos" */ }
TEST_CASE("quadro da arma na mão por tipo") {
    CHECK(render::quadroArma(jogo::TipoItem::Martelo, true, false) == 8);
    CHECK(render::quadroArma(jogo::TipoItem::Bacamarte, false, true) == 11);
    CHECK(render::quadroArma(jogo::TipoItem::Tesla, true, false) == 13);
}
```

(Os dois casos com comentário são escritos completos na execução, no mesmo estilo dos existentes: montam o `EstadoHud`, chamam `construirHud` e conferem a largura do 4º retângulo e o texto da munição.)

- [ ] **Passo 2: Rodar `.\ferramentas\compilar.ps1`.** Esperado: falha de compilação.
- [ ] **Passo 3: Implementar.**
- [ ] **Passo 4: Rodar `.\ferramentas\compilar.ps1`.** Esperado: tudo passa.

---

### Tarefa 7: Arte provisória dos itens e das armas novas

**Arquivos:**
- Criar: `ferramentas/gerar_itens.ps1`, `assets/sprites/item_*.png` (14: os 10 tipos, balas, cartuchos, células, engrenagens), `assets/armas/sabre_0..1.png`, `martelo_0..1.png`, `bacamarte_0..2.png`, `tesla_0..1.png`
- Modificar: `assets/LICENCAS.txt`

- [ ] **Passo 1: Script `gerar_itens.ps1`** no estilo de `gerar_inimigos.ps1`: cada item como objeto deitado/de pé no chão, legível também em 16 px no HUD (garrafa verde do tônico, lâmina do sabre, cabeça de martelo com pistão, cano duplo do bacamarte, bobina azul da Tesla, casaco marrom, colete cinza com placas, armadura dourada, caixa de balas, caixa de cartuchos vermelha, células com raio, pilha de engrenagens); armas na mão vistas de trás como as atuais.
- [ ] **Passo 2: Folha de prévia** (como na Etapa 3) e conferência.
- [ ] **Passo 3: `LICENCAS.txt`** — provisórios, sem licença de terceiros.

---

### Tarefa 8: Entrada, inventário na tela e laço principal

**Arquivos:**
- Modificar: `src/plataforma/Entrada.h/.cpp`, `src/main.cpp`

**Interfaces:**
- Produz:
  - `Entrada`: I alterna `inventarioAberto()` (abre: solta o mouse; fecha: recaptura); teclas 1–4 → `usarRapido`; com o inventário aberto: `glm::vec2 posicaoMouse() const` (coordenadas da janela) e eventos `struct CliqueInventario { bool pressionou, soltou, direito; };` via `CliqueInventario coletarCliques()`; com o inventário aberto, o clique **não** captura o mouse
  - `main`: com o inventário aberto a lógica pausa (`passo.zerar()`), o HUD e a tela do inventário são desenhados; pressionar num espaço guarda `arrastando`; soltar no mesmo espaço → `usarEspaco`; soltar noutro espaço → `trocarEspacos`; clique direito num espaço → `largarEspaco`; espaços de equipamento só mostram (não são alvo de troca nesta etapa); itens do chão desenhados como sprites (camada por `iconeDoItem`/`iconeDaMunicao`/`ICONE_ENGRENAGENS`, 0,35 × 0,35); HUD completo
  - opção de teste `--inventario` (abre o inventário já no começo, para capturas)

- [ ] **Passo 1: Implementar.**
- [ ] **Passo 2: Capturas** (com `--dados`): andar 3 com itens à vista (`--posicao` perto de itens do `--descrever`, que passa a listar o chão), HUD completo, inventário aberto (`--inventario`). Esperado: itens no chão, HUD legível, grade do inventário com os ícones e os nomes.

---

### Tarefa 9: Verificação, cenários, revisão e entrega

- [ ] **Passo 1: Release + testes.** Esperado: tudo passa, sem avisos novos.
- [ ] **Passo 2: Cenários** `.bat`: partida aleatória; andar 2 (bacamarte possível); andar 5 (Tesla e armadura de latão possíveis); perto de itens; invencível para juntar coisas.
- [ ] **Passo 3: Revisão independente** (Foco de revisão + Decisões de projeto); corrigir os importantes com teste.
- [ ] **Passo 4: Deixar o jogo aberto** com a tabela do que testar: pegar e largar, inventário cheio, equipar, arrastar, 1–4, subir de nível, cada arma nova, munição no máximo, HUD.
- [ ] **Passo 5: Perguntar "Depois de testar, faço o commit e envio?"** e commitar com o "sim".

---

## Próximas etapas

Etapa 5 (som, menus, opções, `--demo` e o pacote para os amigos) e, depois da v1, a Etapa 6 (verticalidade).
