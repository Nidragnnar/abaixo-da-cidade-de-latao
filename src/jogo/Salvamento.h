#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "jogo/Partida.h"

namespace jogo {

// "Salvar e sair": a partida em texto (uma coisa por linha) e de volta. O andar é refeito pela semente; o arquivo
// guarda só as células da grade que mudaram, a exploração e o estado da partida. Tiros em voo, explosões, comportas
// subindo e o preparo dos ataques ficam de fora (menos de um segundo de ação).
struct Salvamento {
    // Vazio quando a partida não pode ser gravada (morto, venceu ou contando a vitória)
    static std::optional<std::string> escrever(const Partida& p);
    // Vazio se o texto não for um salvamento válido desta versão
    static std::optional<Partida> ler(std::string_view texto);
};

constexpr int VERSAO_SALVAMENTO = 1;

inline std::optional<std::string> escreverPartida(const Partida& p) { return Salvamento::escrever(p); }
inline std::optional<Partida> lerPartida(std::string_view texto) { return Salvamento::ler(texto); }

// --- No disco (partida.sav na pasta de dados) ---

// Esta execução pode ler e gravar o salvamento? Nunca na demo, na captura nem invencível; um cenário que abre direto
// na partida (--semente, --andar, --posicao...) só com uma pasta de dados de teste (--dados), para nunca gravar por
// cima da partida salva de verdade do jogador
constexpr bool podeUsarSalvamento(bool automatico, bool invencivel, bool direto, bool temDadosDeTeste) {
    return !automatico && !invencivel && (!direto || temDadosDeTeste);
}

// Preso: o arquivo não pôde ser apagado (outro programa o segura); a partida não continua, para a morte seguir
// permanente, e o arquivo fica para tentar de novo
enum class ResultadoCarga { Nenhum, Carregou, Estragado, Preso };

// A partida pode ser gravada agora? Falso morto, vencido ou contando a vitória
bool podeGravarAgora(const Partida& p);

// Gravação atômica; falso se a partida não pode ser gravada (morto, venceu) ou se o disco recusou
bool gravarPartida(const std::filesystem::path& arquivo, const Partida& p);
bool existePartidaSalva(const std::filesystem::path& arquivo);
// Carregou: devolve a partida e apaga o arquivo (um salvamento só, sem voltar atrás).
// Estragado: o arquivo vira partida.sav.corrompido. Nenhum: não havia arquivo.
std::pair<ResultadoCarga, std::optional<Partida>> continuarPartida(const std::filesystem::path& arquivo);
// "Nova partida" confirmada
void apagarPartidaSalva(const std::filesystem::path& arquivo);

}
