package com.sivin.fastplayer;

/**
 * Created by Administrator on 2017/12/2.
 */

public class FastPlayer2 {

    static {
        System.loadLibrary("FastPlayer");
    }

    /**
     *设置播放视音频的url
     * @param url url
     */
    public native void setDataResource(String url);



}
