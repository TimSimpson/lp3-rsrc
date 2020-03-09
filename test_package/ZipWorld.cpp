// --------------------------------------------------------------------
// This example shows using `lp3::sdl`'s helper classes to create
// a simple app.
// --------------------------------------------------------------------
#include <SDL_image.h>
#include <fstream>
#include <iostream>
#include <lp3/main.hpp>
#include <lp3/rsrc.hpp>
#include <lp3/sdl.hpp>
#include <memory>

namespace sdl = lp3::sdl;

int _main(lp3::main::PlatformLoop & loop) {
    sdl::SDL2 sdl2(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    sdl::Window window = SDL_CreateWindow(
            "SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_Log("Creating renderer...");
    sdl::Renderer renderer
            = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Log("Loading texture...");
    lp3::rsrc::Directory dir{"media"};
    lp3::rsrc::ZipFile zip(dir.load("Earth.zip"));
    zip.load("Earth.png");

    sdl::RWops png_file = zip.load("Earth.png");
    sdl::Surface bitmap = IMG_LoadTyped_RW(png_file, 0, "PNG");

    sdl::Texture tex = SDL_CreateTextureFromSurface(renderer, bitmap);

    SDL_Log("start itr");
    loop.run([&renderer, &tex]() {
        SDL_Event e;
        bool quit = false;
        if (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        return !quit;
    });

    return 0;
}

LP3_MAIN(_main)
// ~end-doc
