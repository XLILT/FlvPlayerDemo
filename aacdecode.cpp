#include "aacdecode.h"
#include "pcmrender.h"
#include "thread_queue.h"
#include "audio_common.h"

#include <assert.h>
#include <iostream>
#include <fstream>
#include <thread>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libswresample\swresample.h"
};  // extern "C"

using namespace std;


#define MAX_AUDIO_FRAME_SIZE 192000

namespace {

AVCodecContext * avctx = nullptr;
AVFrame * avframe = nullptr;
SwrContext * swrctx = nullptr;

uint8_t * swr_buf = nullptr;
int swr_buf_size = 0;

bool is_first_packet = true;

thread * audio_thread = nullptr;
bool is_stop = false;

struct AACPacket
{
    char * buf = nullptr;
    int64_t buf_size = 0;
    int64_t dts = 0;
};

ThreadQueue<AACPacket> aacpacket_queue;

SDL_AudioFormat to_SDL_AudioFormat(AVSampleFormat av_sample_format)
{    
    switch (av_sample_format)
    {
    case AV_SAMPLE_FMT_FLTP:
        return AUDIO_F32SYS;
    default:
        return AUDIO_S16SYS;
    }
}

// ofstream pcm_ofs("./c.pcm", ios_base::out|ios_base::binary);

//void write_pcm_packet(uint8_t * buf, int len)
//{
//    pcm_ofs.write((char *)buf, len);
//}

}   // namespace

void run_audio_thread();

int aac_init()
{
    av_log_set_level(AV_LOG_DEBUG);

    //avcodec_register_all();

    AVCodec * codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (!codec)
    {
        cerr << "Codec not found" << endl;

        return -1;
    }

    avctx = avcodec_alloc_context3(codec);
    if (!avctx)
    {
        cerr << "Could not allocate video codec context" << endl;

        return -1;
    }

    if (avcodec_open2(avctx, codec, nullptr) < 0)
    {
        cerr << "Could not open codec" << endl;

        return -1;
    }
    
    avframe = av_frame_alloc();

    // swr_buf = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

    audio_thread = new thread(std::bind(run_audio_thread));

    return 0;
}

void aac_fina()
{
    is_stop = true;

    if (audio_thread)
    {
        if (audio_thread->joinable())
        {
            audio_thread->join();
        }

        delete audio_thread;
        audio_thread = nullptr;
    }

    // av_free(swr_buf);

    swr_free(&swrctx);

    av_frame_free(&avframe);

    avcodec_free_context(&avctx);

    // pcm_ofs.flush();
    // pcm_ofs.close();
}

void decodeaac(AACPacket & aac_pkt)
{
    AVPacket pkt;

    av_init_packet(&pkt);

    pkt.data = reinterpret_cast<uint8_t *>(aac_pkt.buf);
    pkt.size = static_cast<int32_t>(aac_pkt.buf_size);
    pkt.dts = aac_pkt.dts;
    pkt.pts = aac_pkt.dts;

    int ret = avcodec_send_packet(avctx, &pkt);
    if (0 != ret)
    {
        cerr << "avcodec_send_packet return: " << hex << -ret << dec << endl;

        assert(0);

        return;
    }

    ret = avcodec_receive_frame(avctx, avframe);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
        return;
    }
    else if (ret < 0)
    {
        cerr << "AAC Error during decoding: " << ret << endl;

        assert(0);

        return;
    }

    cout << "saving aac frame " << avctx->frame_number << endl;

    if (is_first_packet)
    {
        if (pcm_render_init(avframe->sample_rate, AUDIO_S16SYS,
            avframe->channels, avframe->nb_samples) != 0)
        {
            return;
        }

        is_first_packet = false;
    }

    if (!swrctx)
    {
        swrctx = swr_alloc();

        int64_t channelslayout = av_get_default_channel_layout(avframe->channels);
        AVSampleFormat out_fmt = AV_SAMPLE_FMT_S16;

        swrctx = swr_alloc_set_opts(swrctx, channelslayout, out_fmt, avframe->sample_rate,
            av_get_default_channel_layout(avframe->channels), avctx->sample_fmt, avctx->sample_rate, 0, NULL);

        swr_init(swrctx);
    }

    int out_nb_samples = avframe->nb_samples;
    swr_buf_size = av_samples_get_buffer_size(NULL, avctx->channels, out_nb_samples, AV_SAMPLE_FMT_S16, 1);

    swr_buf = new uint8_t[swr_buf_size];

    swr_convert(swrctx, &swr_buf, swr_get_out_samples(swrctx, avframe->nb_samples),
        (const uint8_t **)(avframe->data), avframe->nb_samples);

    
}

void play_pcm()
{
    on_pcm_packet(swr_buf, swr_buf_size, avframe->pts);
}

void on_aac_packet(char * buf, int64_t buf_size, int64_t dts)
{
    AACPacket aac_pkt;

    aac_pkt.buf = buf;
    aac_pkt.buf_size = buf_size;
    aac_pkt.dts = dts;

    aacpacket_queue.push_back(aac_pkt);

#if 0
    AVPacket pkt;

    av_init_packet(&pkt);

    pkt.data = reinterpret_cast<uint8_t *>(buf);
    pkt.size = static_cast<int32_t>(buf_size);
    // pkt.dts = dts;
    // pkt.pts = dts;

    int ret = avcodec_send_packet(avctx, &pkt);
    if (0 != ret)
    {
        cerr << "avcodec_send_packet return: " << hex << -ret << dec << endl;

        assert(0);

        return;
    }

    ret = avcodec_receive_frame(avctx, avframe);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
        return;
    }
    else if (ret < 0)
    {
        cerr << "AAC Error during decoding: " << ret << endl;
        
        assert(0);

        return;
    }    

    cout << "saving aac frame " << avctx->frame_number << endl;

    if (is_first_packet)
    {
        if (pcm_render_init(avframe->sample_rate, AUDIO_S16SYS,
            avframe->channels, avframe->nb_samples) != 0)
        {
            return;
        }

        is_first_packet = false;
    }

    if (!swrctx)
    {
        swrctx = swr_alloc();

        int64_t channelslayout = av_get_default_channel_layout(avframe->channels);
        AVSampleFormat out_fmt = AV_SAMPLE_FMT_S16;

        swrctx = swr_alloc_set_opts(swrctx, channelslayout, out_fmt, avframe->sample_rate,
            av_get_default_channel_layout(avframe->channels), avctx->sample_fmt, avctx->sample_rate, 0, NULL);

        swr_init(swrctx);
    }
    
    swr_convert(swrctx, &swr_buf, swr_get_out_samples(swrctx, avframe->nb_samples),
        (const uint8_t **)(avframe->data),avframe->nb_samples);
    
    int out_nb_samples = avframe->nb_samples;
    auto swr_buf_size = av_samples_get_buffer_size(NULL, avctx->channels, out_nb_samples, AV_SAMPLE_FMT_S16, 1);

    // write_pcm_packet(avframe->data[0], avframe->linesize[0]);
    // on_pcm_packet(avframe->data[0], avframe->linesize[0]);
    on_pcm_packet(swr_buf, swr_buf_size);
#endif
}

void run_audio_thread()
{
    while (!is_stop)
    {
        // get aac(ADTS) packet
        AACPacket aac_pkt;
        bool got_pkt = aacpacket_queue.pop_front(aac_pkt, 200);

        if (!got_pkt)
        {
            continue;
        }

        // decode
        decodeaac(aac_pkt);
        
        // play
        play_pcm();
    }
}