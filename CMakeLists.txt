cmake_minimum_required(VERSION 3.25)
project(Roguelike LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Runtime estático: os amigos não precisam instalar o Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(cmake/Dependencias.cmake)

# Avisos e acentos só no nosso código
function(configurar_alvo alvo)
    target_compile_options(${alvo} PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(${alvo} PRIVATE NOMINMAX)
    target_include_directories(${alvo} PRIVATE ${CMAKE_SOURCE_DIR}/src)
endfunction()

# Lógica pura (sem SDL nem OpenGL), testada sem janela
add_library(nucleo STATIC
    src/jogo/PassoFixo.cpp
    src/mundo/Grade.cpp
    src/mundo/MapaFixo.cpp
    src/mundo/Colisao.cpp
    src/jogo/Jogador.cpp
    src/render/Camera.cpp
    src/render/GeometriaMasmorra.cpp
    src/render/Xadrez.cpp
    src/render/Letterbox.cpp
)
configurar_alvo(nucleo)
target_include_directories(nucleo PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(nucleo PUBLIC glm::glm)

# Jogo
add_executable(Roguelike WIN32
    src/main.cpp
    src/plataforma/Log.cpp
    src/plataforma/Caminhos.cpp
    src/plataforma/Janela.cpp
    src/plataforma/Entrada.cpp
    src/render/Shader.cpp
    src/render/Texturas.cpp
    src/render/MalhaGPU.cpp
    src/render/TelaBaixa.cpp
)
configurar_alvo(Roguelike)
target_link_libraries(Roguelike PRIVATE nucleo SDL2::SDL2main SDL2::SDL2 glad stb opengl32)
set_target_properties(Roguelike PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:Roguelike>)

# SDL2.dll e assets ao lado do exe
add_custom_command(TARGET Roguelike POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:Roguelike>
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:Roguelike>/assets)

# Testes automáticos
file(GLOB ARQUIVOS_TESTE CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/testes/*.cpp)
add_executable(testes_roguelike ${ARQUIVOS_TESTE})
configurar_alvo(testes_roguelike)
target_link_libraries(testes_roguelike PRIVATE nucleo doctest::doctest)

enable_testing()
add_test(NAME testes COMMAND testes_roguelike)
