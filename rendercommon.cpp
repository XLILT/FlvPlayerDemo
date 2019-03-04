#include "rendercommon.h"

#include <iostream>
#include <SDL.h>
#include <stdint.h>
#include <chrono>

using namespace std;

SDL_Window * screen = nullptr;
int64_t audio_pts_drift = INIT_PST_DRIFT;

namespace {

using SteadyTimePoint = std::chrono::time_point<std::chrono::steady_clock>;

SteadyTimePoint get_now_tp()
{
    return std::chrono::steady_clock::now();
}

const SteadyTimePoint base_time_point = get_now_tp();

}   // namespace 

int64_t get_steady_time_ms()
{
    return chrono::duration_cast<chrono::milliseconds>(get_now_tp() - base_time_point).count();
}

int render_init()
{
    // if (SDL_Init(SDL_INIT_VIDEO| SDL_INIT_AUDIO | SDL_INIT_TIMER))
    if (SDL_Init(SDL_INIT_EVERYTHING))
    {
        cerr << "Could not initialize SDL: " << SDL_GetError() << endl;
        return -1;
    }

    //SDL 2.0 Support for multiple windows
    screen = SDL_CreateWindow("FLVPlayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        1, 1, SDL_WINDOW_OPENGL);

    if (!screen)
    {
        printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
        return -1;
    }

    SDL_HideWindow(screen);

    return 0;
}

void render_fina()
{
    SDL_Quit();
}

void render_event_loop()
{
    bool running = true;
    SDL_Event ev;

    while (running)
    {
        // Event loop
        while (SDL_WaitEvent(&ev) != 0)
        {
            cout << "SDL_PolLEvent type: " << ev.type << endl;

            // check event type
            switch (ev.type)
            {
            case SDL_QUIT:
                // shut down
                running = false;
                break;
            default:
                break;
            }

            if (!running)
            {
                break;
            }
        }
    }
}