package com.sivin.fastplayer;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.view.Surface;
import static android.media.AudioFormat.CHANNEL_OUT_STEREO;

/**
 *
 * Created by Sivin on 2017/3/31.
 */
public class FastPlayer {


    /**
     *设置播放视音频的url
     * @param url url
     */
    public native void setDataResource(String url);

    /**
     * 播放器准备
     */
    public native void prepare();


    /**
     * 设置setSuface
     * @param suface suface
     */
    public native void setSuface(Surface suface);



    public native void start();


    /**
     * 创建音频播放对象
     * 这个方法将会被jni层主动回调
     */
    public AudioTrack createAudioTrack(){
        int sampleRateInHz = 44100;
        int bufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRateInHz,CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT );
        AudioTrack audioTrack = new AudioTrack(
                AudioManager.STREAM_MUSIC,
                sampleRateInHz,
                CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSizeInBytes,
                AudioTrack.MODE_STREAM);
        return audioTrack;
    }

}
