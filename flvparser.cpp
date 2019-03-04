#include "flvparser.h"
#include "h264decode.h"
#include "aacdecode.h"

#include <iostream>
#include <fstream>
#include <bitset>
#include <assert.h>
#include <vector>

#if (defined(_WIN32) || defined(_WIN64))
#include<Winsock2.h>

#pragma comment(lib, "ws2_32.lib")
#endif  // _WIN32

using namespace std;

const int MAX_BUF_LEN = 4096;

enum FLVTagType
{
    FTT_AUDIO = 8,
    FTT_VIDEO = 9,
    FTT_SCRIPTDATAOBJECT = 18,
};

enum AVCPacketType
{
    APT_SEQUENCE_HEADER = 0,
    APT_NALU = 1,
    APT_SEQUENCE_END = 2,
};

namespace {

const int32_t h264_start_code = 0x01000000;

uint32_t data_size = 0;

// avc
int32_t nalu_len_size = 0;
char * avc_sequence_header_buf = nullptr;
uint32_t avc_sequence_header_buf_len = 0;

char * avc_head_buf = nullptr;
int avc_head_buf_size = 0;
bool is_first_avc_nalu_frame = true;

// aac
int aac_profile = 0;
int sample_rate_index = 0;
int channel_config = 0;;

uint16_t get_big_endian_16(const char * buf)
{
    char enough_buf[4] = { 0 };

    enough_buf[2] = buf[0];
    enough_buf[3] = buf[1];

    return static_cast<uint16_t>(ntohl(*(reinterpret_cast<const uint32_t *>(enough_buf))));
}

uint32_t get_big_endian_24(const char * buf)
{
    char enough_buf[4] = { 0 };

    enough_buf[1] = buf[0];
    enough_buf[2] = buf[1];
    enough_buf[3] = buf[2];

    return ntohl(*(reinterpret_cast<const uint32_t *>(enough_buf)));
}

int32_t get_big_endian_24i(const char * buf)
{
    char enough_buf[4] = { 0 };

    enough_buf[1] = buf[0];
    enough_buf[2] = buf[1];
    enough_buf[3] = buf[2];

    return ntohl(*(reinterpret_cast<const int32_t *>(enough_buf)));
}

uint32_t get_big_endian_32(const char * buf)
{
    return ntohl(*(reinterpret_cast<const uint32_t *>(buf)));
}

int32_t get_big_endian_32i(const char * buf)
{
    return ntohl(*(reinterpret_cast<const int32_t *>(buf)));
}

void write_64(uint64_t & x, int length, int value)
{
    uint64_t mask = 0xFFFFFFFFFFFFFFFF >> (64 - length);
    x = (x << length) | ((uint64_t)value & mask);
}

}   // namespace

void on_script_data_object(const char * buf, int64_t buf_size)
{
    // ObjectName

    // Type
    int name_type = (int)buf[0];
    assert(name_type == 2);
    // StringLength
    uint16_t str_len = get_big_endian_16(buf + 1);
    //cout << "ObjectName StringLength: " << str_len << endl;
    // StringData
    std::string object_name(buf + 3, buf + 3 + str_len);
    //cout << "ObjectName: " << object_name << endl;

    // ObjectData

    // Type
    int data_type = (int)buf[3 + str_len];
    //cout << "ObjectData Type: " << data_type << endl;
}

void on_avc_sequence_header(char * buf, int64_t buf_size, int64_t dts, int64_t pts)
{
    // AVC ISO
    // https://blog.csdn.net/k1988/article/details/5654631
    // https://github.com/riverlight/FlvParser/blob/master/FlvParser.cpp#L501

    nalu_len_size = (buf[5] & 0x03) + 1;

    const int offset = 6;

    int32_t sps_size = get_big_endian_16(buf + offset);
    int32_t pps_size = get_big_endian_16(buf + offset + (2 + sps_size) + 1);

    int len = 4 + sps_size + 4 + pps_size;
    char * buf_264 = new char[len];

    // SPS
    memcpy(buf_264, &h264_start_code, 4);
    memcpy(buf_264 + 4, buf + offset + 2, sps_size);
    //PPS
    memcpy(buf_264 + 4 + sps_size, &h264_start_code, 4);
    memcpy(buf_264 + 4 + sps_size + 4, buf + offset + 2 + sps_size + 2 + 1, pps_size);

    avc_head_buf = buf_264;
    avc_head_buf_size = len;
    // on_264_packet(buf_264, len, dts, pts);

    // delete[] buf_264;
}

void on_avc_nalu(char * buf, int64_t buf_size, int64_t dts, int64_t pts)
{
    // AVC ISO
    // https://blog.csdn.net/k1988/article/details/5654631
    // https://github.com/riverlight/FlvParser/blob/master/FlvParser.cpp#L501

    char * buf_264 = new char[data_size + 10];
    int32_t len = 0;

    uint32_t offset = 0;

    while (true)
    {
        if (offset >= data_size - 5)
        {
            break;
        }

        int nalu_real_len = 0;

        switch (nalu_len_size)
        {
        case 4:
            nalu_real_len = get_big_endian_32(buf + offset);
            break;
        case 3:
            nalu_real_len = get_big_endian_24(buf + offset);
            break;
        case 2:
            nalu_real_len = get_big_endian_16(buf + offset);
            break;
        default:
            nalu_real_len = *(unsigned char *)(buf + offset);
            break;
        }

        cout << nalu_real_len << endl;

        if(nalu_real_len > 0)
        { 
            memcpy(buf_264 + len, &h264_start_code, 4);
            memcpy(buf_264 + len + 4, buf + offset + nalu_len_size, nalu_real_len);

            len += (4 + nalu_real_len);
        }

        offset += (nalu_len_size + nalu_real_len);
    };

    cout << "offset: " << data_size - offset << endl;
    
    if (is_first_avc_nalu_frame)
    {
        char * full_buf = new char[len + avc_head_buf_size];

        memcpy(full_buf, avc_head_buf, avc_head_buf_size);
        memcpy(full_buf + avc_head_buf_size, buf_264, len);

        delete [] avc_head_buf;
        delete[] buf_264;

        buf_264 = full_buf;
        len += avc_head_buf_size;

        is_first_avc_nalu_frame = false;
    }

    on_264_packet(buf_264, len, dts, pts);

    // delete[] buf_264;
}

void on_avc_packet(char * buf, int64_t buf_size, int64_t dts)
{
    uint8_t pkt_type = (uint8_t)buf[0];
    cout << "AVCPacketType: " << (int)pkt_type << endl;

    int32_t composition_time = get_big_endian_24i(buf + 1);
    cout << "CompositionTime: " << composition_time << endl;

    switch (pkt_type)
    {
    case APT_SEQUENCE_HEADER:
        on_avc_sequence_header(buf + 4, buf_size - 4, dts, dts + composition_time);
        break;
    case APT_NALU:
        on_avc_nalu(buf + 4, buf_size - 4, dts, dts + composition_time);
        break;
    case APT_SEQUENCE_END:
        break;
    default:
        break;
    }
}

void on_aac_squence_header(char * buf, int64_t buf_size)
{
    aac_profile = ((buf[0] & 0xf8) >> 3) - 1;
    cout << "aac_profile: " << aac_profile << endl;

    sample_rate_index = ((buf[0] & 0x07) << 1) | (buf[1] >> 7);
    cout << "sample_rate_index: " << sample_rate_index << endl;

    channel_config = (buf[1] >> 3) & 0x0f;
    cout << "channel_config: " << channel_config << endl;
}

void on_aac_raw_data(char * buf, int64_t buf_size, int64_t dts)
{
    uint64_t bits = 0;
    int aac_frame_size = data_size - 2;

    // https://www.cnblogs.com/lihaiping/p/5284547.html
    // ADTS frame header

    write_64(bits, 12, 0xFFF);
    write_64(bits, 1, 0);
    write_64(bits, 2, 0);
    write_64(bits, 1, 1);
    write_64(bits, 2, aac_profile);
    write_64(bits, 4, sample_rate_index);
    write_64(bits, 1, 0);
    write_64(bits, 3, channel_config);
    write_64(bits, 1, 0);
    write_64(bits, 1, 0);
    write_64(bits, 1, 0);
    write_64(bits, 1, 0);
    write_64(bits, 13, 7 + aac_frame_size);
    write_64(bits, 11, 0x7FF);
    write_64(bits, 2, 0);

    int adts_packet_len = aac_frame_size + 7;

    char * buf_adts = new char[adts_packet_len];

    unsigned char p64[8];

    // p64[0] = (unsigned char)(bits >> 56);
    p64[1] = (unsigned char)((bits >> 48) & 0xFF);
    p64[2] = (unsigned char)((bits >> 40) & 0xFF);
    p64[3] = (unsigned char)((bits >> 32) & 0xFF);
    p64[4] = (unsigned char)((bits >> 24) & 0xFF);
    p64[5] = (unsigned char)((bits >> 16) & 0xFF);
    p64[6] = (unsigned char)((bits >> 8) & 0xFF);
    p64[7] = (unsigned char)(bits & 0xFF);

    memcpy(buf_adts, p64 + 1, 7);
    memcpy(buf_adts + 7, buf, aac_frame_size);

    on_aac_packet(buf_adts, adts_packet_len, dts);

    // delete[] buf_adts;
}

void on_aac_packet_adapt(char * buf, int64_t buf_size, int64_t dts)
{
    uint8_t pkt_type = (uint8_t)buf[0];
    cout << "AACPacketType: " << (int)pkt_type << endl;

    switch (pkt_type)
    {
    case 0:
        on_aac_squence_header(buf + 1, buf_size - 1);
        break;
    case 1:
        on_aac_raw_data(buf + 1, buf_size - 1, dts);
        break;
    default:
        break;
    }
}

void on_video_data(char * buf, int64_t buf_size, int64_t dts)
{
    // FrameType
    int frame_type = (buf[0] & 0xF0) >> 4;
    cout << "FrameType: " << frame_type << endl;
    // CodecID    
    int codec_id = buf[0] & 0x0F;
    cout << "CodecID: " << codec_id << endl;

    assert(codec_id == 7);

    on_avc_packet(buf + 1, buf_size - 1, dts);
}

void on_audio_data(char * buf, int64_t buf_size, int64_t dts)
{
    // SoundFormat
    int sound_format = (buf[0] >> 4) & 0x0F;
    cout << "SoundFormat: " << sound_format << endl;
    // SoundRate
    int sound_rate = (buf[0] >> 2) & 0x03;
    cout << "SoundRate: " << sound_rate << endl;
    // SoundSize
    int sound_size = (buf[0] >> 1) & 0x01;
    cout << "SoundSize: " << sound_size << endl;
    // SoundType
    int sound_type = buf[0] & 0x01;
    cout << "SoundType: " << sound_type << endl;

    assert(sound_format == 10);

    on_aac_packet_adapt(buf + 1, buf_size - 1, dts);
}

void on_tag_data(int tag_type, char * buf, int64_t buf_size, int64_t dts)
{
    switch (tag_type)
    {
    case FTT_SCRIPTDATAOBJECT:
        on_script_data_object(buf, buf_size);
        break;
    case FTT_VIDEO:
        on_video_data(buf, buf_size, dts);
        break;
    case FTT_AUDIO:
        on_audio_data(buf, buf_size, dts);
        break;
    default:
        break;
    }
}

bool parse_body(ifstream & ifs)
{
    char c[MAX_BUF_LEN] = { 0 };

    // PreviousTagSize
    ifs.read(c, 4);

    auto pre_read_count = ifs.gcount();
    if (pre_read_count != 4)
    {
        cerr << "PreviousTagSize parse failed: " << pre_read_count << endl;

        return false;
    }

    cout << "previous tag size: " << ntohl(*(reinterpret_cast<uint32_t *>(c))) << endl;

    // FLV Tag

    // TagType
    ifs.read(c, 1);

    if (ifs.gcount() != 1)
    {
        cout << "parse over" << endl;

        return false;
    }

    int tag_type = (int)c[0];
    cout << "TagType: " << tag_type << endl;
    // DataSize
    ifs.read(c, 3);
    data_size = get_big_endian_24(c);
    cout << "DataSize: " << data_size << endl;
    // Timestamp
    ifs.read(&c[1], 3);
    ifs.read(c, 1);
    int32_t dts = get_big_endian_32i(c);
    cout << "Timestamp: " << dts << endl;
    // StreamID
    ifs.read(c, 3);
    cout << "StreamID: " << get_big_endian_24(c) << endl;

    char * data_buf = c;
    vector<char> dyn_buf;

    if (data_size > MAX_BUF_LEN)
    {
        dyn_buf.resize(data_size);
        data_buf = &dyn_buf[0];
    }

    // Data
    ifs.read(data_buf, data_size);

    auto data_read_count = ifs.gcount();
    if (data_read_count != data_size)
    {
        cout << ifs.eof() << endl;
        cerr << data_read_count << " != " << data_size << endl;

        return false;
    }

    on_tag_data(tag_type, data_buf, data_size, dts);
    
    return true;
}
