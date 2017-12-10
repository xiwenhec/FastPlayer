//
// Created by Administrator on 2017/12/2.
//
#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <pthread.h>
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "H264DecodeDefine.h"

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"fastplayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"fastplayer",FORMAT,##__VA_ARGS__);


static double r2d(AVRational r) {
    return r.num == 0 || r.den == 0 ? 0 : (double) r.num / (double) r.den;
}


static int min(int a, int b) {
    return a < b ? a : b;
}


void copyDecodeFrame(unsigned char *src, unsigned char *dist, int lineSize, int width, int height) {
    width = min(lineSize, width);
    for (int i = 0; i < height; i++) {
        memcpy(dist, src, width);
        dist += width;
        src += lineSize;
    }
}

JNIEXPORT void JNICALL
Java_com_sivin_fastplayer_FastPlayer2_setDataResource(JNIEnv *env, jobject instance, jstring url_) {

    const char *url = (*env)->GetStringUTFChars(env, url_, 0);
    jclass renderCls = (*env)->GetObjectClass(env, instance);
    jmethodID updateDataId = (*env)->GetMethodID(env, renderCls, "nativeCallback", "(II[B[B[B)V");
    jmethodID updateSizeMethodId = (*env)->GetMethodID(env, renderCls, "onGetVideoSize", "(II)V");

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
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(videoCodec);
    avcodec_parameters_to_context(pCodecCtx, ic->streams[videoStream]->codecpar);
    int err = avcodec_open2(pCodecCtx, videoCodec, NULL);
    if (err < 0) {
        char buf[1024] = {0};
        av_strerror(err, buf, sizeof(buf));
        LOGE("open videoCodec failed: %s", buf);
        return;
    }

    (*env)->CallVoidMethod(env,instance,updateSizeMethodId,pCodecCtx->width,pCodecCtx->height);

    AVFrame *yuv = av_frame_alloc();
    H264YUV_Frame yuvFrame;

    int pFrameIndex = 0;
    //将原始数据读取到pkt中
    AVPacket *pkt = av_packet_alloc();
    while (av_read_frame(ic, pkt) >= 0) {
        //如果是视频数据包
        if (pkt->stream_index == videoStream) {
            ret = avcodec_send_packet(pCodecCtx, pkt);
            if (ret != 0) {
                av_packet_unref(pkt);
                continue;
            }
            while (avcodec_receive_frame(pCodecCtx, yuv) == 0) {
                LOGE("读取视频帧:%d", pFrameIndex);
                pFrameIndex++;


                unsigned int yDataLength =
                        (unsigned int) (pCodecCtx->height) * (min(yuv->linesize[0], pCodecCtx->width));
                unsigned int uLength =
                        (unsigned int) (pCodecCtx->height / 2) * (min(yuv->linesize[1], pCodecCtx->width / 2));
                unsigned int vLength =
                        (unsigned int) (pCodecCtx->height / 2) * (min(yuv->linesize[2], pCodecCtx->width / 2));

                memset(&yuvFrame, 0, sizeof(H264YUV_Frame));

                yuvFrame.yData.length = yDataLength;
                yuvFrame.uData.length = uLength;
                yuvFrame.vData.length = vLength;

                yuvFrame.yData.dataBufer = (unsigned char *) malloc(yDataLength);
                yuvFrame.uData.dataBufer = (unsigned char *) malloc(uLength);
                yuvFrame.vData.dataBufer = (unsigned char *) malloc(vLength);


                copyDecodeFrame(yuv->data[0], yuvFrame.yData.dataBufer, yuv->linesize[0],

                                pCodecCtx->width, pCodecCtx->height);

                copyDecodeFrame(yuv->data[1], yuvFrame.uData.dataBufer, yuv->linesize[1],
                                pCodecCtx->width / 2, pCodecCtx->height / 2);

                copyDecodeFrame(yuv->data[2], yuvFrame.vData.dataBufer, yuv->linesize[2],
                                pCodecCtx->width / 2, pCodecCtx->height / 2);


                jbyteArray yArr = (*env)->NewByteArray(env, yuvFrame.yData.length);
                (*env)->SetByteArrayRegion(env, yArr, 0, yuvFrame.yData.length,
                                           (jbyte *) yuvFrame.yData.dataBufer);

                jbyteArray uArr = (*env)->NewByteArray(env, yuvFrame.uData.length);
                (*env)->SetByteArrayRegion(env, uArr, 0, yuvFrame.uData.length,
                                           (jbyte *) yuvFrame.uData.dataBufer);

                jbyteArray vArr = (*env)->NewByteArray(env, yuvFrame.vData.length);
                (*env)->SetByteArrayRegion(env, vArr, 0, yuvFrame.vData.length,
                                           (jbyte *) yuvFrame.vData.dataBufer);

                (*env)->CallVoidMethod(env, instance, updateDataId, pCodecCtx->width,
                                       pCodecCtx->height, yArr, uArr, vArr);


                (*env)->DeleteLocalRef(env, yArr);
                (*env)->DeleteLocalRef(env, uArr);
                (*env)->DeleteLocalRef(env, vArr);


                free(yuvFrame.yData.dataBufer);
                free(yuvFrame.uData.dataBufer);
                free(yuvFrame.vData.dataBufer);

                usleep(1000 * 40);
            }
        }

        int pts = (int) (pkt->pts * r2d(ic->streams[pkt->stream_index]->time_base) * 1000);
        // LOGE("pts = %d", pts);
        //释放包
        av_packet_unref(pkt);
    }
    av_packet_unref(pkt);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&ic);
    (*env)->ReleaseStringUTFChars(env, url_, url);
}