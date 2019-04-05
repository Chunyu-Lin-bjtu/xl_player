//
// Created by gutou on 2017/4/25.
//

#ifndef XL_XL_PLAYER_TYPES_H
#define XL_XL_PLAYER_TYPES_H

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <android/native_window_jni.h>
#include "xl_video_render_types.h"
#include "xl_macro.h"
#include <libavfilter/avfilter.h>
#include <android/looper.h>

#if __ANDROID_API__ >= NDK_MEDIACODEC_VERSION

#include <media/NdkMediaCodec.h>

#endif

struct struct_xl_play_data;
typedef struct struct_xl_java_class {
//    jclass XLPlayer_class;
    jmethodID player_onPlayStatusChanged;
    jmethodID player_onPlayError;

    jclass HwDecodeBridge;
    jmethodID codec_init;
    jmethodID codec_stop;
    jmethodID codec_flush;
    jmethodID codec_dequeueInputBuffer;
    jmethodID codec_queueInputBuffer;
    jmethodID codec_getInputBuffer;
    jmethodID codec_dequeueOutputBufferIndex;
    jmethodID codec_formatChange;
    __attribute__((unused))
    jmethodID codec_getOutputBuffer;
    jmethodID codec_releaseOutPutBuffer;
    jmethodID codec_release;

    jclass SurfaceTextureBridge;
    jmethodID texture_getSurface;
    jmethodID texture_updateTexImage;
    jmethodID texture_getTransformMatrix;
    __attribute__((unused))
    jmethodID texture_release;

} xl_java_class;

typedef struct struct_xl_clock {
    int64_t update_time;
    int64_t pts;
} xl_clock;

typedef enum {
    UNINIT = -1, IDEL = 0, PLAYING, PAUSED, BUFFER_EMPTY, BUFFER_FULL
} PlayStatus;

typedef struct struct_xl_audio_filter_context {
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;
    AVFilter *abuffersrc;
    AVFilter *abuffersink;
//    AVFilterLink *outlink;
    AVFilterInOut *outputs, *inputs;
    pthread_mutex_t *filter_lock;
    int channels;
    uint64_t channel_layout;
} xl_audio_filter_context;

typedef struct struct_xl_packet_pool {
    int index;
    int size;
    int count;
    AVPacket **packets;
} xl_pakcet_pool;

typedef struct struct_xl_mediacodec_context {
    JNIEnv *jniEnv;
#if __ANDROID_API__ >= NDK_MEDIACODEC_VERSION
    AMediaCodec *codec;
    AMediaFormat *format;
#endif
    size_t nal_size;
    int width, height;
    enum AVPixelFormat pix_format;
    enum AVCodecID codec_id;
} xl_mediacodec_context;

typedef struct struct_xl_packet_queue {
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    AVPacket **packets;
    int readIndex;
    int writeIndex;
    int count;
    int total_bytes;
    unsigned int size;
    uint64_t duration;
    uint64_t max_duration;
    AVPacket flush_packet;

    void (*full_cb)(void *);

    void (*empty_cb)(void *);

    void *cb_data;
} xl_packet_queue;

typedef struct struct_xl_frame_pool {
    int index;
    int size;
    int count;
    AVFrame *frames;
} xl_frame_pool;


typedef struct struct_xl_frame_queue {
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    AVFrame **frames;
    int readIndex;
    int writeIndex;
    int count;
    unsigned int size;
    AVFrame flush_frame;
} xl_frame_queue;

typedef struct struct_xl_audio_player_context {
    pthread_mutex_t *lock;
    unsigned int play_pos;
    int buffer_size;
    uint8_t * buffer;
    int frame_size;

    void (*play)(struct struct_xl_play_data *pd);

    void (*shutdown)();

    void (*release)(struct struct_xl_audio_player_context * ctx);

    void (*player_create)(int rate, int channel, struct struct_xl_play_data *pd);

    int64_t (*get_delta_time)(struct struct_xl_audio_player_context * ctx);
} xl_audio_player_context;

typedef struct struct_xl_statistics {
    int64_t last_update_time;
    int64_t last_update_bytes;
    int64_t last_update_frames;
    int64_t bytes;
    int64_t frames;
    uint8_t *ret_buffer;
    jobject ret_buffer_java;
} xl_statistics;

typedef struct struct_xl_play_data {
    JavaVM *vm;
    JNIEnv *jniEnv;
    int run_android_version;
    int best_samplerate;
    jobject *xlPlayer;
    xl_java_class *jc;

    //用户设置
    int buffer_size_max;
    float buffer_time_length;
    bool force_sw_decode;
    float read_timeout;

    // 播放器状态
    PlayStatus status;

    // 各个thread
    pthread_t read_stream_thread;
    pthread_t audio_decode_thread;
    pthread_t video_decode_thread;
    pthread_t gl_thread;

    // 封装
    AVFormatContext *format_context;
    int video_index, audio_index;
    //是否有 音频0x1 视频0x2 字幕0x4
    uint8_t av_track_flags;
    uint64_t timeout_start;

    // packet容器
    xl_packet_queue *video_packet_queue, *audio_packet_queue;
    xl_pakcet_pool *packet_pool;
    // frame容器
    xl_frame_pool *audio_frame_pool;
    xl_frame_queue *audio_frame_queue;
    xl_frame_pool *video_frame_pool;
    xl_frame_queue *video_frame_queue;

    // 音频
    xl_audio_player_context *audio_player_ctx;
    xl_audio_filter_context *audio_filter_ctx;
    AVCodecContext *audio_codec_ctx;
    AVCodec *audio_codec;
    AVFrame *audio_frame;

    // 软硬解公用
    xl_video_render_context *video_render_ctx;
    AVFrame *video_frame;
    int width, height;
    int frame_rotation;

    //软解视频
    AVCodecContext *video_codec_ctx;
    AVCodec *video_codec;
    // 硬解
    xl_mediacodec_context *mediacodec_ctx;

    // 音视频同步
    xl_clock *video_clock;
    xl_clock *audio_clock;

    // 是否软解
    bool is_sw_decode;

    // SEEK
    float seek_to;
    uint8_t seeking;

    // play background
    bool just_audio;

    //end of file
    bool eof;

    // error code
    // -1 stop by user
    // -2 read stream time out
    // 1xx init
    // 2xx format and stream
    // 3xx audio decode
    // 4xx video decode sw
    // 5xx video decode hw
    // 6xx audio play
    // 7xx video play  openGL
    int error_code;

    // 统计
    xl_statistics *statistics;

    // message
    ALooper *main_looper;
    int pipe_fd[2];

    void (*send_message)(struct struct_xl_play_data *pd, int message);

    void (*change_status)(struct struct_xl_play_data *pd, PlayStatus status);

    void (*on_error)(struct struct_xl_play_data * pd, int error_code);
} xl_play_data;
#endif //XL_XL_PLAYER_TYPES_H
