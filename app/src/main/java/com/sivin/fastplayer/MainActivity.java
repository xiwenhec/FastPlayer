package com.sivin.fastplayer;

import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;

import com.sivin.fastplayer.opengles.GLFrameRenderer;

import static android.opengl.GLSurfaceView.RENDERMODE_WHEN_DIRTY;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    private static final String TAG = "MainActivity";

    private Button mPlayerBtn;
    private Button mMagnifyBtn;
    private Button mShrinkBtn;

    private FastPlayer2 mPlayer;
    private GLFrameRenderer mRender;

    private GLSurfaceView mglView;
    private String mFilePath;
    private float scale = 1.0f;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initData();
        initView();
        initGLes();
    }


    private void initData() {

        String fileName = "input";

        mFilePath = String.format(Environment.getExternalStorageDirectory().getPath() + "/test/%s.mp4",fileName);


    }


    private void initView() {


        mPlayerBtn = findViewById(R.id.id_player);
        mMagnifyBtn = findViewById(R.id.id_magnify);
        mShrinkBtn = findViewById(R.id.id_shrink);

        mPlayerBtn.setOnClickListener(this);
        mMagnifyBtn.setOnClickListener(this);
        mShrinkBtn.setOnClickListener(this);

    }


    @Override
    public void onClick(View v) {

        switch (v.getId()) {

            case R.id.id_player:
                startVideo();
                break;

            case R.id.id_magnify:
                scale += 0.5f;
                if (scale > 5.0f) {
                    scale = 1.0f;
                }
                mRender.scaleViewPort(scale);
                break;
        }
    }

    private void initGLes() {
        mglView = findViewById(R.id.gl_view);
        mglView.setEGLContextClientVersion(2);
        mRender = new GLFrameRenderer(mglView);
        mglView.setRenderer(mRender);
        mglView.setRenderMode(RENDERMODE_WHEN_DIRTY);
        mPlayer = new FastPlayer2(mRender);
    }


    private void startVideo() {
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                mPlayer.setDataResource(mFilePath);
            }
        });
        thread.start();
    }
}
