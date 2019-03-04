#include "pcmrender.h"
#include "audio_common.h"
#include "thread_queue.h"
#include "rendercommon.h"

#include <iostream>

using namespace std;


//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
// static  uint8_t  * audio_chunk = nullptr;

namespace {

struct PCMPacket
{
    uint8_t * buf = nullptr;
    int len = 0;
    int64_t pts = 0;
};

ThreadQueue<PCMPacket> pcmpacket_queue;

uint8_t * current_chunk = nullptr;
int32_t  remain_audio_len = 0;
uint8_t  * remain_audio_pos = nullptr;

}

void  fill_audio(void *udata, uint8_t *stream, int len)
{
    SDL_memset(stream, 0, len);

    // play remain buf
    if (remain_audio_len > 0)
    {
        len = (len > remain_audio_len ? remain_audio_len : len);

        SDL_MixAudio(stream, remain_audio_pos, len, SDL_MIX_MAXVOLUME);

        remain_audio_pos += len;
        remain_audio_len -= len;

        return;
    }

    // free current_pkt
    if (current_chunk)
    {
        delete [] current_chunk;

        current_chunk = nullptr;
    }

    // get pcm packet
    if (pcmpacket_queue.empty())
    {
        return;
    }

    PCMPacket pkt;
    bool got_pkt = pcmpacket_queue.pop_front(pkt, 1);

    if (!got_pkt)
    {
        return;
    }

    current_chunk = pkt.buf;
    remain_audio_len = pkt.len;
    remain_audio_pos = pkt.buf;

    len = (len > remain_audio_len ? remain_audio_len : len);

    SDL_MixAudio(stream, remain_audio_pos, len, SDL_MIX_MAXVOLUME);

    remain_audio_pos += len;
    remain_audio_len -= len;

    audio_pts_drift = pkt.pts - get_steady_time_ms();

#if 0    
    //SDL 2.0
    SDL_memset(stream, 0, len);
    if (audio_len == 0)
    {
        return;
    }

    len = (len > audio_len ? audio_len : len);

    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
#endif
}

int pcm_render_init(int freq, SDL_AudioFormat format, uint8_t channels,  uint16_t samples)
{
    SDL_AudioSpec wanted_spec;

    wanted_spec.freq = freq;
    wanted_spec.format = format;
    wanted_spec.channels = channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = samples;
    wanted_spec.callback = fill_audio;

    if (SDL_OpenAudio(&wanted_spec, NULL)<0) {        
        cerr << "can't open audio" << endl;
        return -1;
    }

    //Play
    SDL_PauseAudio(0);

    return 0;
}

void on_pcm_packet(uint8_t * buf, int len, int64_t pts)
{   
    PCMPacket pkt;

    pkt.buf = buf;
    pkt.len = len;
    pkt.pts = pts;

    pcmpacket_queue.push_back(pkt);

#if 0
    while (audio_len > 0)
    {
        SDL_Delay(1);
    }    

    audio_pos = buf;
    audio_len = len;
#endif
}
