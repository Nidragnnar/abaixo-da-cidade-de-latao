#include "plataforma/Audio.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_mixer.h>

#include "plataforma/Caminhos.h"
#include "plataforma/Log.h"

namespace plataforma {

namespace {

constexpr int CANAIS = 32;
constexpr float VOLUME_BASE_MUSICA = 0.6f;  // a faixa baixada vem bem mais alta que os efeitos

// Arquivos que existem para o som, na ordem de preferência: .ogg, depois .wav
std::vector<std::filesystem::path> arquivosDoSom(std::string_view nome) {
    std::vector<std::filesystem::path> achados;
    for (const char* extensao : {".ogg", ".wav"}) {
        const std::filesystem::path p = caminhoAsset("sons/" + std::string(nome) + extensao);
        std::error_code erro;
        if (std::filesystem::exists(p, erro)) achados.push_back(p);
    }
    return achados;
}

std::string utf8(const std::filesystem::path& p) {
    const std::u8string s = p.u8string();
    return {reinterpret_cast<const char*>(s.data()), s.size()};
}

// Tenta cada arquivo (um .ogg inválido cai para o .wav); nulo e log se nenhum carregar
template <typename T, typename Carregar>
T* carregarPrimeiro(std::string_view nome, std::string_view tipo, Carregar carregar) {
    const auto arquivos = arquivosDoSom(nome);
    if (arquivos.empty()) logar(std::string(tipo) + " ausente (fica em silêncio): sons/" + std::string(nome));
    for (const std::filesystem::path& arquivo : arquivos) {
        if (T* t = carregar(utf8(arquivo).c_str())) return t;
        logar(std::string(tipo) + " inválido: " + utf8(arquivo.filename()) + ": " + Mix_GetError());
    }
    return nullptr;
}

// 0–10 → 0–128 (MIX_MAX_VOLUME)
int volumeMixer(float fracao) { return static_cast<int>(std::lround(std::clamp(fracao, 0.0f, 1.0f) * MIX_MAX_VOLUME)); }

}

Audio::Audio() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        logar(std::string("Áudio indisponível (o jogo segue sem som): ") + SDL_GetError());
        return;
    }
    Mix_Init(MIX_INIT_OGG);
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) != 0) {
        logar(std::string("Áudio indisponível (o jogo segue sem som): ") + Mix_GetError());
        Mix_Quit();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return;
    }
    Mix_AllocateChannels(CANAIS);
    ativo_ = true;
}

Audio::~Audio() {
    if (!ativo_) return;
    Mix_HaltChannel(-1);
    Mix_HaltMusic();
    for (Mix_Chunk* c : efeitos_)
        if (c) Mix_FreeChunk(c);
    if (musica_) Mix_FreeMusic(musica_);
    Mix_CloseAudio();
    Mix_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void Audio::carregar() {
    if (!ativo_) return;
    for (int i = 0; i < static_cast<int>(jogo::Som::TOTAL); ++i) {
        efeitos_[static_cast<size_t>(i)] = carregarPrimeiro<Mix_Chunk>(
            jogo::nomeDoSom(static_cast<jogo::Som>(i)), "Som", [](const char* a) { return Mix_LoadWAV(a); });
    }
}

void Audio::tocar(jogo::Som s, float volume, float pan) {
    if (!ativo_ || s == jogo::Som::TOTAL || volume <= 0.0f) return;
    Mix_Chunk* c = efeitos_[static_cast<size_t>(s)];
    if (!c) return;
    // Volume e lado vão no canal antes de tocar (senão o começo sai com os do som anterior daquele canal)
    const int canal = Mix_GroupAvailable(-1);
    if (canal < 0) return;  // todos os canais ocupados: este som se perde
    Mix_Volume(canal, volumeMixer(volume * volumeEfeitos_));
    const jogo::GanhosEstereo g = jogo::ganhosEstereo(pan);
    Mix_SetPanning(canal, g.esquerda, g.direita);
    Mix_PlayChannel(canal, c, 0);
}

void Audio::tocarMusica(std::string_view nome) {
    if (!ativo_) return;
    if (nome != nomeMusica_) {  // outra música: para e troca
        Mix_HaltMusic();
        if (musica_) Mix_FreeMusic(musica_);
        musica_ = carregarPrimeiro<Mix_Music>(nome, "Música", [](const char* a) { return Mix_LoadMUS(a); });
        nomeMusica_ = nome;
    }
    if (!musica_) return;
    if (!Mix_PlayingMusic()) Mix_PlayMusic(musica_, -1);
}

void Audio::definirVolumes(int efeitos, int musica) {
    volumeEfeitos_ = std::clamp(efeitos, 0, 10) / 10.0f;
    if (ativo_) Mix_VolumeMusic(volumeMixer(std::clamp(musica, 0, 10) / 10.0f * VOLUME_BASE_MUSICA));
}

}
