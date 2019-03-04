#include "yuvrender.h"
#include "rendercommon.h"

#include <iostream>
#include <thread>
#include <SDL.h>

using namespace std;

namespace {

int64_t video_pst_drift = INIT_PST_DRIFT;

int screen_w = 0;
int screen_h = 0;

SDL_Renderer * sdlRenderer = nullptr;
SDL_Texture * sdlTexture = nullptr;

SDL_Rect sdlRect;

}   // namespace 

void render_yuv(uint8_t ** buf, int32_t * linesize, int64_t pts)
{
    int64_t wait_ms = 0;
    
    // synchroniza to audio
    if (audio_pts_drift != INIT_PST_DRIFT)
    {
        wait_ms = pts - (audio_pts_drift + get_steady_time_ms());
    }
    else if(video_pst_drift != INIT_PST_DRIFT)
    {
        wait_ms = pts - ((video_pst_drift) + get_steady_time_ms());
    }

    if (wait_ms > 0)
    {
        this_thread::sleep_for(chrono::milliseconds(wait_ms));
    }

    video_pst_drift = pts - get_steady_time_ms();

#if 0
    if (start_play_pts == 0)
    {
        render_yuv(avframe->data, avframe->linesize);

        start_play_pts = avframe->pts;
        start_play_tm = get_now_tp();
    }
    else
    {
        auto now_tm = get_now_tp();
        int64_t tm_diff = chrono::duration_cast<chrono::milliseconds>(now_tm - start_play_tm).count();

        int64_t pts_diff = avframe->pts - start_play_pts;

        cout << "pts: " << avframe->pts << endl;
        //cout << "diff: " << tm_diff << " " << frame->pts << " " << start_play_pts << " " << frame->pts - start_play_pts << endl;

        if (pts_diff > tm_diff)
        {
            this_thread::sleep_for(chrono::milliseconds(pts_diff - tm_diff));
        }

        render_yuv(avframe->data, avframe->linesize);
    }
#endif 

    SDL_UpdateYUVTexture(sdlTexture, NULL, buf[0], linesize[0], buf[1], linesize[1], buf[2], linesize[2]);

    sdlRect.x = 0;
    sdlRect.y = 0;
    sdlRect.w = screen_w;
    sdlRect.h = screen_h;

    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
    SDL_RenderPresent(sdlRenderer);
}

int yuv_render_init(int width, int height)
{
    SDL_SetWindowSize(screen, width, height);

    SDL_ShowWindow(screen);

    sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
    
    Uint32 pixformat = SDL_PIXELFORMAT_IYUV;

    sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_STREAMING, width, height);

    screen_w = width;
    screen_h = height;
    
    return 0;
}
