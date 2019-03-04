#include "h264decode.h"
#include "yuvrender.h"
#include "thread_queue.h"

#include <stdio.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
};  // extern "C"

using namespace std;

namespace {

struct H264Packet
{
    char * buf = nullptr;
    int64_t buf_size = 0;
    int64_t dts = 0;
    int64_t pts = 0;
};

bool render_inited = false;

AVCodecContext * avctx = nullptr;
AVFrame * avframe = nullptr;

thread * video_thread = nullptr;
bool is_stop = false;

ThreadQueue<H264Packet> h264packet_queue;

}   // namespace

void run_video_thread();

int h264_init()
{
    av_log_set_level(AV_LOG_DEBUG);

    //avcodec_register_all();

    AVCodec * codec = avcodec_find_decoder(AV_CODEC_ID_H264);
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

    video_thread = new thread(std::bind(run_video_thread));

    return 0;
}

void h264_fina()
{
    is_stop = true;

    if (video_thread)
    {
        if (video_thread->joinable())
        {

            video_thread->join();
        }

        delete video_thread;
        video_thread = nullptr;
    }

    av_frame_free(&avframe);

    avcodec_free_context(&avctx);
}

void decodeh264(H264Packet & h264pkt)
{
    AVPacket pkt;

    av_init_packet(&pkt);

    pkt.data = reinterpret_cast<uint8_t *>(h264pkt.buf);
    pkt.size = static_cast<int32_t>(h264pkt.buf_size);
    pkt.dts = h264pkt.dts;
    pkt.pts = h264pkt.pts;

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
        cerr << "Error during decoding: " << ret << endl;

        return;
    }

    delete[] h264pkt.buf;

    cout << "saving frame " << avctx->frame_number << endl;
}

void render_avframe()
{
    if (!render_inited)
    {
        yuv_render_init(avframe->width, avframe->height);

        render_inited = true;
    }

    render_yuv(avframe->data, avframe->linesize, avframe->pts);

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
#endif  // #if 0
}

void on_264_packet(char * buf, int64_t buf_size, int64_t dts, int64_t pts)
{   
#if 1
    H264Packet pkt;
    pkt.buf = buf;
    pkt.buf_size = buf_size;
    pkt.dts = dts;
    pkt.pts = pts;

    h264packet_queue.push_back(pkt);
#endif

#if 0
    AVPacket pkt;

    av_init_packet(&pkt);
    
    pkt.data = reinterpret_cast<uint8_t *>(buf);
    pkt.size = static_cast<int32_t>(buf_size);
    pkt.dts = dts;
    pkt.pts = pts;
    
    /*    
    int got_picture = 0;
    int ret = avcodec_decode_video2(avctx, frame, &got_picture, &pkt);

    if (ret <= 0)
    {
        cerr << "avcodec_decode_video2 return: " << ret << endl;

        return;
    }
    */
    
    int ret = avcodec_send_packet(avctx, &pkt);
    if (0 != ret)
    {
        cerr << "avcodec_send_packet return: " << ret << endl;

        return;
    }

    ret = avcodec_receive_frame(avctx, avframe);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
        return;
    }        
    else if (ret < 0) 
    {
        cerr << "Error during decoding: " << ret << endl;

        return;
    }

    cout << "saving frame " << avctx->frame_number << endl;

    if (!render_inited)
    {
        yuv_render_init(avframe->width, avframe->height);

        render_inited = true;
    }

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

#endif  // if 0
}

void run_video_thread()
{
    while (!is_stop)
    {
        // get h264 packet
        H264Packet h264pkt;
        bool got_pkt =  h264packet_queue.pop_front(h264pkt, 200);

        if (!got_pkt)
        {
            continue;
        }
        
        // decode
        decodeh264(h264pkt);

        // render
        render_avframe();
    }
}
