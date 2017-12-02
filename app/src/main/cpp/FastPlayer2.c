//
// Created by Administrator on 2017/12/2.
//
#include <jni.h>
#include <android/log.h>
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"


#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"fastplayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"fastplayer",FORMAT,##__VA_ARGS__);


static double r2d(AVRational r) {
    return r.num == 0 || r.den == 0 ? 0 : (double) r.num / (double) r.den;
}


JNIEXPORT void JNICALL
Java_com_sivin_fastplayer_FastPlayer2_setDataResource(JNIEnv *env, jobject instance, jstring url_) {
    const char *url = (*env)->GetStringUTFChars(env, url_, 0);

    //注册所有的解码器
    av_register_all();

    AVFormatContext *ic = NULL;

    //获取封装格式上下文信息
    int ret = avformat_open_input(&ic, url, NULL, NULL);

    if (ret != 0) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf));
        LOGE("open %s failed: %s", url, buf);
        return;
    }
    //获取视频总时长(second)
    int64_t totalSec = ic->duration / AV_TIME_BASE;
    LOGE("totalSec %lld", totalSec);


    //获取视频解码器，和视频流信息的index
    int videoStream = 0;
    AVCodec *videoCodec = NULL;
    videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);
    if (videoStream < 0) {
        LOGE("find videoCodec failed");
        return;
    }


   //初始化解码器上下文
    AVCodecContext *videoCtx = avcodec_alloc_context3(videoCodec);
    avcodec_parameters_to_context(videoCtx,ic->streams[videoStream]->codecpar);
    int err = avcodec_open2(videoCtx,videoCodec,NULL);
    if (err<0){
        char buf[1024] = {0};
        av_strerror(err, buf, sizeof(buf));
        LOGE("open videoCodec failed: %s",buf);
        return;
    }


    AVFrame *yuv = av_frame_alloc();
    int pFrameIndex = 0;
    //将原始数据读取到pkt中
    AVPacket *pkt = av_packet_alloc();
    while (av_read_frame(ic, pkt) >= 0){
        //如果是视频数据包
        if(pkt->stream_index == videoStream){
            ret = avcodec_send_packet(videoCtx,pkt);
            if(ret != 0){
                av_packet_unref(pkt);
                continue;
            }
            while (avcodec_receive_frame(videoCtx,yuv) == 0){
                LOGE("读取视频帧:%d",pFrameIndex);
                pFrameIndex ++;
            }
        }
        int pts = pkt->pts * r2d(ic->streams[pkt->stream_index]->time_base) * 1000;
       // LOGE("pts = %d", pts);
        //释放包
        av_packet_unref(pkt);
    }
    av_packet_unref(pkt);
    avcodec_free_context(&videoCtx);
    avformat_close_input(&ic);
    (*env)->ReleaseStringUTFChars(env, url_, url);
}