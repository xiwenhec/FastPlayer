package com.sivin.fastplayer;

import android.opengl.GLSurfaceView;
import android.util.Log;

import com.sivin.fastplayer.opengles.GLFrameRenderer;

/**
 * Created by Administrator on 2017/12/2.
 */

public class FastPlayer2 {

    GLFrameRenderer renderer;

    static {
        System.loadLibrary("FastPlayer");
    }

    public FastPlayer2(GLFrameRenderer renderer) {
        this.renderer = renderer;
    }

    /**
     *设置播放视音频的url
     * @param url url
     */
    public native void setDataResource(String url);


    public void nativeCallback(int width ,int height ,byte[] yData ,byte[] uData ,byte[] vData){
        Log.e("FastPlayer2", "nativeCallback: width:"+width+" height="+height);
        renderer.update(yData,uData,vData);
    }


    public void onGetVideoSize(int width ,int height){
        renderer.updateSize(width,height);
    }


}
