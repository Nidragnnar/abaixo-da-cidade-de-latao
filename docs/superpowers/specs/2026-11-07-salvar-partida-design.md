# Salvar e sair — Especificação de design

Data: 2026-11-07 · Autor: Nidragnnar · Status: aguardando revisão
Complementa a especificação geral (`2026-11-01-roguelike-design.md`, "Fora da v1": salvar no meio da partida).

## 1. Objetivo

Poder parar uma partida e continuar depois, sem perder a morte permanente.

**Critério de sucesso:** no menu de pausa, "Salvar e sair" grava e fecha; fechar a janela no meio da partida também
grava; ao abrir o jogo, "Continuar" volta ao mesmo andar, no mesmo lugar, com o mesmo inventário, inimigos, itens no
chão, comportas e mapa explorado; o salvamento é apagado ao continuar (um só, sem voltar atrás); um arquivo estragado
não trava o jogo.

### Decisões tomadas

| Tema | Decisão |
|---|---|
| Modelo | **Salvar e sair** (estilo roguelike): um salvamento, apagado ao continuar |
| Fechar a janela | **Grava sozinho**, como "Salvar e sair" |
| Abordagem | **A**: o andar é refeito pela semente; grava-se só o que mudou nele e o estado da partida |
| Formato | Texto em linhas, com versão no topo; gravação atômica como o placar |

## 2. Quando grava, quando não

- **Grava:** "Salvar e sair" (pausa) e fechar a janela com uma partida em andamento.
- **Não grava:** jogador morto ou vitória; partida de fundo do menu; partidas de teste (`--demo`, `--captura`,
  `--invencivel`); `--dados` também vale para o salvamento (os testes gravam fora do `%APPDATA%`).
- **Continuar:** lê o arquivo, monta a partida e **apaga o arquivo** logo depois de carregar.
- **Nova partida** com salvamento guardado: pergunta "Apagar a partida salva?" (Sim apaga e começa; Não volta ao
  menu).

## 3. O que vai no arquivo

Arquivo `partida.sav` na pasta de dados (a mesma do placar e das opções).

- **Cabeçalho:** `versao 1`, semente, número do andar.
- **Jogador:** posição, ângulo, inclinação, pés (altura); vida, nível, XP, engrenagens; inimigos destruídos, chefes
  derrotados, tempo de jogo, trocas de andar; tem a válvula-chave.
- **Inventário e armas:** cada espaço (tipo, qualidade, bônus, quantidade, carga), os equipados, arma ativa e munição
  de reserva.
- **Andar (o que mudou desde a geração):** estado de cada comporta (fechada, aberta, trancada); válvula-chave ainda no
  chão ou não; arena trancada e as aberturas guardadas; células exploradas.
- **Inimigos:** todos, com tipo, posição, pés, vida, estado (Parado, Perseguindo, Destruído; os outros viram
  Perseguindo), já recompensado, relógios dos chefes (recarga, chamados) e se o voador já avisou.
- **Chão:** cada item, caixa de munição e engrenagens, com posição.

**Fica de fora** (menos de um segundo de ação): tiros e bombas em voo, explosões, comportas no meio da abertura (ficam
fechadas), preparo de ataque dos inimigos, mensagem na tela, clarão.

## 4. Erros

- Arquivo que não abre, de outra versão ou com linha inválida: o jogo avisa **"Partida salva não pôde ser lida"**,
  renomeia para `partida.sav.corrompido` e segue para o menu sem "Continuar".
- Se a gravação falhar (sem permissão, disco cheio): avisa "Não foi possível salvar" e não fecha, para o jogador
  decidir (no X da janela, fecha assim mesmo, porque a janela já está fechando).

## 5. Interfaces

- `jogo/Salvamento.h/.cpp`: `std::string escreverPartida(const Partida&)` e
  `std::optional<Partida> lerPartida(std::string_view texto)` (testáveis sem disco), mais `gravarPartida(caminho,
  Partida)` e `carregarPartida(caminho)` usando a gravação atômica existente.
- `Partida` ganha o necessário para ser montada a partir do arquivo (construtor ou função amiga do salvamento).
- Menus: "Continuar" (só com arquivo), "Salvar e sair" na pausa, confirmação de "Nova partida".

## 6. Testes

- Gravar e ler de volta dá a mesma partida: jogador, inventário (com bônus e carga), armas, inimigos (inclusive chefe
  e voadores), chão, comportas abertas, válvula pega, exploração, arena trancada, contadores e tempo.
- Ler um arquivo de outra versão, cortado ou com lixo: devolve nada, sem travar.
- Continuar apaga o arquivo; morto ou vencido não grava.
- Depois de continuar, a partida segue funcionando: andar, descer de elevador, lutar.

**Cenário pronto** em `build\Release`: `20_Continuar_partida_salva.bat`, que copia uma partida salva (andar 4, perto
do Capataz) para uma pasta de dados de teste e abre o jogo nela.

## 7. Fora deste trabalho

Vários espaços de salvamento, salvar a qualquer hora, salvamento na nuvem, salvar automaticamente a cada andar.
