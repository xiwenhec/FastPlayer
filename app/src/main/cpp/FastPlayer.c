#include <jni.h>
#include <android/native_window_jni.h>
#include <unistd.h>
#include <libyuv.h>
#include <libavutil/imgutils.h>
#include "FastPlayer.h"

#define MAX_AUDIO_FRME_SIZE 48000 * 4

struct _videoState *is;

int videoWidth = 0;

int videoHeight = 0;


/**
 * 初始化封装格式上下文，获取音频和视频流的索引
 * @return
 */
static int init_format_ctx(struct _videoState *is, const char *input_str) {
    LOGE("%s", input_str);
    input_str = av_strdup(input_str);

    //注册组件
    av_register_all();
    avformat_network_init();

    //封装格式上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //设置读取中断时长
    // av_dict_set(&opts, "timeout", "6000", 0);


    LOGE("%s", "开始打开视频文件");
    int ret = avformat_open_input(&pFormatCtx, input_str, NULL, NULL);
    if (ret < 0) {
        LOGE("%s", "打开输入视频文件失败");
        return -1;
    }
    LOGE("%s", "打开视频文件成功");
    //3.获取视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "获取视频信息失败");
        return -1;
    }


    LOGE("%s", "获取文件信息成功");
    //获取视频流和音频流索引
    int video_stream_idx = -1;
    int audio_stream_idx = -1;

    video_stream_idx = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    audio_stream_idx = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

    LOGE("%s,%d,%d", "成功查找到解码器", video_stream_idx, audio_stream_idx);

    is->video_stream_index = video_stream_idx;
    is->audio_stream_index = audio_stream_idx;
    is->pFormatCtx = pFormatCtx;

    return 0;
}


/**
 * 初始化解码器上下文
 * @param sp
 */
static int init_codec_context_type(struct _videoState *is, int index) {
    //4.查找解码器
    AVStream *stream = is->pFormatCtx->streams[index];

    AVCodec *pCodec = NULL;
    pCodec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (pCodec == NULL) {
        LOGE("%s", "无法解码");
        return -1;
    }
    //为decoder context分配内存
    AVCodecContext *pCodeCtx = avcodec_alloc_context3(pCodec);

    //decoder context 复制数据
    if (avcodec_parameters_to_context(pCodeCtx, stream->codecpar) < 0) {
        LOGE("复制codec parameters 到decoder context失败");
        return -1;
    }

    //5.打开解码器
    if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
        LOGE("%s", "解码器无法打开");
        return -1;
    }

    is->pCodecCtxArray[index] = pCodeCtx;

    return 0;
}


/**
 * 初始化解码器上下文
 * @param is
 * @return 0 表示成功,-1表示失败
 */
static int init_codec_context(struct _videoState *is) {
    if (is->video_stream_index >= 0) {
        if (init_codec_context_type(is, is->video_stream_index) < 0) {
            return -1;
        }
    }

    if (is->audio_stream_index >= 0) {
        if (init_codec_context_type(is, is->audio_stream_index) < 0) {
            return -1;
        }
    }

    return 0;
}


static void obtainVideoSize(struct _videoState *is) {

    if (is->video_stream_index >= 0) {
        videoWidth = is->pCodecCtxArray[is->video_stream_index]->width;
        videoHeight = is->pCodecCtxArray[is->video_stream_index]->height;
        //TODO:这里获取到视频的宽高之后,应该回调java层进行视频尺寸的改变
    }
}


void jni_audio_prepare(JNIEnv *env, jobject instance, struct _videoState *sp) {
    if (sp->state < 0) {
        return;
    }
    jclass jCls = (*env)->GetObjectClass(env, instance);

    jmethodID create_audio_track_mid = (*env)->GetMethodID(env, jCls, "createAudioTrack",
                                                           "()Landroid/media/AudioTrack;");

    //得到java层的audio_track对象
    jobject audio_track = (*env)->CallObjectMethod(env, instance, create_audio_track_mid);


    jclass audio_track_class = (*env)->GetObjectClass(env, audio_track);

    jmethodID play_mid = (*env)->GetMethodID(env, audio_track_class, "play", "()V");
    //调用java层的audioTrack的play()方法
    (*env)->CallVoidMethod(env, audio_track, play_mid);
    jmethodID write_mid = (*env)->GetMethodID(env, audio_track_class, "write", "([BII)I");

    //--------JNI end----------
    sp->audioParams.audio_track = (*env)->NewGlobalRef(env, audio_track);
    sp->audioParams.audio_track_write_mid = write_mid;
}


void decode_audio_prepare(struct _videoState *is) {

    AVCodecContext *pCodecCtx = is->pCodecCtxArray[is->audio_stream_index];
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt = pCodecCtx->sample_fmt;
    //输出采样格式16bit PCM
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate = pCodecCtx->sample_rate;
    //输出采样率
    int out_sample_rate = in_sample_rate;

    uint64_t in_ch_layout = pCodecCtx->channel_layout;

    //输出的声道布局（立体声）
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    if (in_ch_layout == 4)
        out_ch_layout = AV_CH_LAYOUT_4POINT0;

    SwrContext *swrCtx = swr_alloc();
    //重采样设置选项
    swr_alloc_set_opts(swrCtx,
                       out_ch_layout, out_sample_fmt, out_sample_rate,
                       in_ch_layout, in_sample_fmt, in_sample_rate,
                       0, NULL);
    swr_init(swrCtx);

    //输出的声道个数
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    //-------------重采样设置参数end------------------

    is->audioParams.in_sample_fmt = in_sample_fmt;
    is->audioParams.out_sample_fmt = out_sample_fmt;
    is->audioParams.in_sample_rate = in_sample_rate;
    is->audioParams.out_sample_rate = out_sample_rate;
    is->audioParams.out_channel_nb = out_channel_nb;
    is->audioParams.swr_ctx = swrCtx;
}


JNIEXPORT void JNICALL
Java_com_sivin_fastplayer_FastPlayer_setDataResource(JNIEnv *env, jobject instance, jstring url_) {

    const char *inputStr = (*env)->GetStringUTFChars(env, url_, 0);

    is = malloc(sizeof(struct _videoState));
    //初始化播放器状态标识
    is->state = 0;

    //获取vm实例
    (*env)->GetJavaVM(env, &(is->vm));

    int ret = init_format_ctx(is, inputStr);

    if (ret < 0) {
        is->state = PLAY_ERROR;
        return;
    }

    //初始化解码信息
    ret = init_codec_context(is);
    if (ret < 0) {
        is->state = PLAY_ERROR;
        return;
    }

    //获取视频尺寸,并回调通知java层
    obtainVideoSize(is);

    //释放资源
    (*env)->ReleaseStringUTFChars(env, url_, inputStr);
}


static void *decode_video(void *state) {

    struct _videoState *is = (struct _videoState *) state;

    //编码数据包分配内存
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    av_init_packet(packet);

    //像素数据（解码数据）
    AVFrame *yuv_frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();

    //绘制时的缓冲区
    ANativeWindow_Buffer windowBuffer;

    AVCodecContext *pCodecCtx = is->pCodecCtxArray[is->video_stream_index];
    AVFormatContext *pFormatCtx = is->pFormatCtx;

    if (ANativeWindow_setBuffersGeometry(is->nativeWindow, pCodecCtx->width, pCodecCtx->height,
                                         WINDOW_FORMAT_RGBA_8888));

    // Determine required buffer size and allocate buffer
    // buffer中数据就是用于渲染的,且格式为RGBA
    //这里的align:可以根据视频像素的宽高进行计算,一般的原则是宽高都能被align整除,而且是2的倍数,指定align可以提高渲染效率
    //使用1表示通用,效率比较低
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height,
                                            1);
    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    //Change the format and size of the window buffers.设置缓冲区的属性（宽、高、像素格式）
    int num = 0;
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        // Is this a packet from the video stream?
        if (packet->stream_index == is->video_stream_index) {
            //开始解码--> 将原始数据包读入解码器
            int ret = avcodec_send_packet(pCodecCtx, packet);
            if (ret < 0) {
                av_packet_unref(packet);
                continue;
            }
            //从解码器中读取yuv_frame数据
            ret = avcodec_receive_frame(pCodecCtx, yuv_frame);
            if (ret < 0 && ret != AVERROR_EOF) {
                LOGI("读取数据帧错误=%d", ret);
                av_packet_unref(packet);
                continue;
            }
            // 并不是decode一次就可解码出一帧
            if (ret == 0) {
                LOGE("解码%d", num++);
                // lock native window buffer
                ANativeWindow_lock(is->nativeWindow, &windowBuffer, 0);
                av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer,
                                     AV_PIX_FMT_RGBA,
                                     yuv_frame->width, yuv_frame->height, 1);
                // 格式转换
                //YUV->RGBA_8888
                I420ToARGB(yuv_frame->data[0], yuv_frame->linesize[0],
                           yuv_frame->data[2], yuv_frame->linesize[2],
                           yuv_frame->data[1], yuv_frame->linesize[1],
                           rgb_frame->data[0], rgb_frame->linesize[0],
                           pCodecCtx->width, pCodecCtx->height);

                // 获取stride
                uint8_t *dst = windowBuffer.bits;
                int dstStride = windowBuffer.stride * 4;
                uint8_t *src = (uint8_t *) (rgb_frame->data[0]);
                int srcStride = rgb_frame->linesize[0];

                // 由于window的stride和帧的stride不同,因此需要逐行复制
                int h;
                for (h = 0; h < videoHeight; h++) {
                    memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(is->nativeWindow);
                usleep(1000 * 40);
            }
        }
        av_packet_unref(packet);
    }

end:
    if (buffer)
        av_free(buffer);
    if (rgb_frame)
        av_free(rgb_frame);
    if (yuv_frame)
        av_free(yuv_frame);
    if (packet)
        av_free(packet);

}


void *decode_audio(void *arg) {

    struct _videoState *is = (struct _videoState *) arg;

    //关联当前线程的JNIEnv
    JavaVM *javaVM = is->vm;
    JNIEnv *env;
    (*javaVM)->AttachCurrentThread(javaVM, &env, NULL);
    //16bit 44100 PCM 数据
    uint8_t *out_buffer = (uint8_t *) av_malloc(MAX_AUDIO_FRME_SIZE);
    //编码数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    AVFormatContext *pFormatCtx = is->pFormatCtx;
    AVCodecContext *pCodeCtx = is->pCodecCtxArray[is->audio_stream_index];
    //解压缩数据
    AVFrame *frame = av_frame_alloc();
    //不断的读取压缩数据
    while (av_read_frame(pFormatCtx, packet) == 0) {
        if (packet->stream_index == is->audio_stream_index) {
            int ret = avcodec_send_packet(pCodeCtx, packet);
            if (ret < 0) {
                av_packet_unref(packet);
                continue;
            }
            ret = avcodec_receive_frame(pCodeCtx, frame);

            if (ret < 0) {
                LOGI("读取数据错误=%d", ret);
                av_packet_unref(packet);
                continue;
            }

            //非零，正在解码
            //ret ==0 解码成功
            if (ret == 0) {
                //重采样
                swr_convert(is->audioParams.swr_ctx, &out_buffer, MAX_AUDIO_FRME_SIZE,
                            (const uint8_t **) frame->data, frame->nb_samples);
                //获取sample的size
                int out_buffer_size = av_samples_get_buffer_size(NULL, is->audioParams.out_channel_nb,
                                                                 frame->nb_samples,
                                                                 is->audioParams.out_sample_fmt,
                                                                 0);
                //out_buffer缓冲区数据，转成byte数组
                jbyteArray audio_sample_array = (*env)->NewByteArray(env, out_buffer_size);

                //将out_buffer数组数据复制到audio_sample_array中,想要实现这样数据,需要一个jbyte指针才能操作
                jbyte *psample_byte = (*env)->GetByteArrayElements(env, audio_sample_array, NULL);
                //out_buffer的数据复制到psample_byte
                memcpy(psample_byte, out_buffer, out_buffer_size);

                //同步指针和数组里的内容
                (*env)->ReleaseByteArrayElements(env, audio_sample_array, psample_byte, 0);

                //AudioTrack.write PCM数据
                (*env)->CallIntMethod(env, is->audioParams.audio_track, is->audioParams.audio_track_write_mid,
                                      audio_sample_array, 0, out_buffer_size);
                //释放局部引用
                (*env)->DeleteLocalRef(env, audio_sample_array);
                usleep(1000 * 16);
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_free(out_buffer);
    swr_free(&(is->audioParams.swr_ctx));
    (*javaVM)->DetachCurrentThread(javaVM);
}



JNIEXPORT void JNICALL
Java_com_sivin_fastplayer_FastPlayer_prepare(JNIEnv *env, jobject instance) {
    jni_audio_prepare(env, instance, is);
    decode_audio_prepare(is);
}

JNIEXPORT void JNICALL
Java_com_sivin_fastplayer_FastPlayer_setSuface(JNIEnv *env, jobject instance, jobject suface) {
    is->nativeWindow = ANativeWindow_fromSurface(env, suface);
}


/**
 * 开始解码播放
 */
JNIEXPORT void JNICALL
Java_com_sivin_fastplayer_FastPlayer_start(JNIEnv *env, jobject instance) {

    if (is->video_stream_index >= 0) {
        LOGE("%s", "创建视频解码线程");
        pthread_create(&(is->decodeThreads[is->video_stream_index]), NULL, decode_video, is);
    }
    if (is->audio_stream_index >= 0) {
        LOGE("%s", "创建音频解码线程");
        pthread_create(&(is->decodeThreads[is->audio_stream_index]), NULL, decode_audio, is);
    }

}
