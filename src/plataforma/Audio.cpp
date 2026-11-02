#include "plataforma/Audio.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

#include <SDL.h>
#include <SDL_mixer.h>

#include "plataforma/Caminhos.h"
#include "plataforma/Log.h"

namespace plataforma {

namespace {

constexpr int CANAIS = 32;

// .ogg tem preferência (pacote baixado); senão o .wav provisório; vazio se nenhum existe
std::filesystem::path acharSom(std::string_view nome) {
    for (const char* extensao : {".ogg", ".wav"}) {
        const std::filesystem::path p = caminhoAsset("sons/" + std::string(nome) + extensao);
        std::error_code erro;
        if (std::filesystem::exists(p, erro)) return p;
    }
    return {};
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
        const std::string_view nome = jogo::nomeDoSom(static_cast<jogo::Som>(i));
        const std::filesystem::path arquivo = acharSom(nome);
        if (arquivo.empty()) {
            logar("Som ausente (fica em silêncio): sons/" + std::string(nome));
            continue;
        }
        Mix_Chunk* c = Mix_LoadWAV(reinterpret_cast<const char*>(arquivo.u8string().c_str()));
        if (!c) logar("Som inválido (fica em silêncio): " + std::string(nome) + ": " + Mix_GetError());
        efeitos_[static_cast<size_t>(i)] = c;
    }
}

void Audio::tocar(jogo::Som s, float volume, float pan) {
    if (!ativo_ || s == jogo::Som::TOTAL || volume <= 0.0f) return;
    Mix_Chunk* c = efeitos_[static_cast<size_t>(s)];
    if (!c) return;
    const int canal = Mix_PlayChannel(-1, c, 0);
    if (canal < 0) return;  // todos os canais ocupados: este som se perde
    Mix_Volume(canal, volumeMixer(volume * volumeEfeitos_));
    const jogo::GanhosEstereo g = jogo::ganhosEstereo(pan);
    Mix_SetPanning(canal, g.esquerda, g.direita);
}

void Audio::tocarMusica() {
    if (!ativo_) return;
    if (!musica_) {
        const std::filesystem::path arquivo = acharSom("musica");
        if (arquivo.empty()) {
            logar("Música ausente (fica em silêncio): sons/musica");
            return;
        }
        musica_ = Mix_LoadMUS(reinterpret_cast<const char*>(arquivo.u8string().c_str()));
        if (!musica_) {
            logar(std::string("Música inválida (fica em silêncio): ") + Mix_GetError());
            return;
        }
    }
    if (!Mix_PlayingMusic()) Mix_PlayMusic(musica_, -1);
}

void Audio::definirVolumes(int efeitos, int musica) {
    volumeEfeitos_ = std::clamp(efeitos, 0, 10) / 10.0f;
    if (ativo_) Mix_VolumeMusic(volumeMixer(std::clamp(musica, 0, 10) / 10.0f));
}

}
