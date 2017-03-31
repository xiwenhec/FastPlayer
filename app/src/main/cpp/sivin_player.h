//
// Created by Sivin on 2017/3/10.
//

#ifndef FFMEGDEMO_SIVIN_PLAYER_H
#define FFMEGDEMO_SIVIN_PLAYER_H

#include <android/log.h>
#include <unistd.h>
#include <libavformat/avformat.h>
#include <pthread.h>
#include <android/native_window.h>
#include <libswresample/swresample.h>
#include <jni.h>


#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"sivin",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"sivin",FORMAT,##__VA_ARGS__);

#define MAX_STREAM 3

#define PLAY_ERROR -1;


struct _audioParams{

    SwrContext *swr_ctx;
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt;
    //输出采样格式16bit PCM
    enum AVSampleFormat out_sample_fmt;
    //输入采样率
    int in_sample_rate;
    //输出采样率
    int out_sample_rate;

    //输出的声道个数
    int out_channel_nb;

    //JNI
    jobject audio_track;

    jmethodID audio_track_write_mid;

    pthread_t thread_read_from_stream;

};


struct _videoState{

    int state;

    JavaVM *vm;


    /*封装格式上下文*/
    AVFormatContext *pFormatCtx;

    /*音频视频流索引位置*/
    int video_stream_index;
    int audio_stream_index;

    /*解码器上下文数组*/
    AVCodecContext *pCodecCtxArray[MAX_STREAM];

    /*解码线程ID数组*/
    pthread_t decodeThreads[MAX_STREAM];



    /*音频相关参数*/
    struct _audioParams audioParams;


    ANativeWindow* nativeWindow;




    /*音频，视频队列数组*/
    //Queue *packets[MAX_STREAM];

};



#endif //FFMEGDEMO_SIVIN_PLAYER_H
