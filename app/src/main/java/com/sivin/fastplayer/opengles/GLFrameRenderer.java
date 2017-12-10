package com.sivin.fastplayer.opengles;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.util.DisplayMetrics;
import android.util.Log;

import java.nio.ByteBuffer;
import java.util.WeakHashMap;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GLFrameRenderer implements Renderer {

    private GLSurfaceView mTargetSurface;

    private GLProgram prog = new GLProgram(0);

    //屏幕的宽高
    private int mScreenWidth, mScreenHeight;

    //视频的宽高
    private int mVideoWidth, mVideoHeight;

    private ByteBuffer y;
    private ByteBuffer u;
    private ByteBuffer v;


    private float mScale = 1.0f;
    private boolean mScaleChange = false;


    public GLFrameRenderer(GLSurfaceView surface) {
        mTargetSurface = surface;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        if (!prog.isProgramBuilt()) {
            prog.buildProgram();
        }
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log.e("onSurfaceChanged", "onSurfaceChanged: " + width + " height：" + height);
        mScreenWidth = width;
        mScreenHeight = height;
        GLES20.glViewport(0, 0, width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        synchronized (this) {
            if (y != null) {
                // reset position, have to be done
                if(mScaleChange){
                    changeGLViewport(gl);
                }
                y.position(0);
                u.position(0);
                v.position(0);
                prog.buildTextures(y, u, v, mVideoWidth, mVideoHeight);
                GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
                prog.drawFrame();
            }
        }
    }

    private int viewportOffset = 0;
    private int maxOffset = 400;


    private void changeGLViewport(GL10 gl) {
        mScaleChange = false;
        int offSetX = (int) (-mScreenWidth * (mScale-1) / 2);
        int offSetY = (int) (-mScreenHeight*(mScale-1)/2);
        int width = (int)(mScreenWidth * mScale);
        int height = (int)(mScreenHeight * mScale);
        gl.glViewport(offSetX, offSetY,width , height);

    }


    /**
     * this method will be called from native code, it happens when the video is about to play or
     * the video size changes.
     */
    public void updateSize(int w, int h) {
        if (w > 0 && h > 0) {
            // 调整比例
            if (mScreenWidth > 0 && mScreenHeight > 0) {
                float f1 = 1f * mScreenHeight / mScreenWidth;
                float f2 = 1f * h / w;
                if (f1 == f2) {
                    prog.createBuffers(GLProgram.squareVertices);
                } else if (f1 < f2) {
                    float widScale = f1 / f2;
                    prog.createBuffers(new float[]{-widScale, -1.0f, widScale, -1.0f, -widScale, 1.0f, widScale,
                            1.0f,});
                } else {
                    float heightScale = f2 / f1;
                    prog.createBuffers(new float[]{-1.0f, -heightScale, 1.0f, -heightScale, -1.0f, heightScale, 1.0f,
                            heightScale,});
                }
            }
            // 初始化容器
            if (w != mVideoWidth && h != mVideoHeight) {
                this.mVideoWidth = w;
                this.mVideoHeight = h;
                int yArraySize = w * h;
                int uvArraySize = yArraySize / 4;
                synchronized (this) {
                    y = ByteBuffer.allocate(yArraySize);
                    u = ByteBuffer.allocate(uvArraySize);
                    v = ByteBuffer.allocate(uvArraySize);
                }
            }
        }

    }

    /**
     * this method will be called from native code, it's used for passing yuv data to me.
     */
    public void update(byte[] ydata, byte[] udata, byte[] vdata) {
        synchronized (this) {
            y.clear();
            u.clear();
            v.clear();
            y.put(ydata, 0, ydata.length);
            u.put(udata, 0, udata.length);
            v.put(vdata, 0, vdata.length);
        }

        // request to render
        mTargetSurface.requestRender();
    }


    public void scaleViewPort(float scale){
        synchronized (this) {
            mScaleChange = true;
            mScale = scale;
        }
        mTargetSurface.requestRender();
    }


}
