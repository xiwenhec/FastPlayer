package com.sivin.fastplayer;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    private Button mPlayerBtn;
    private FastPlayer2 mPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mPlayerBtn = findViewById(R.id.id_player);
        mPlayer = new FastPlayer2();

        final String path = Environment.getExternalStorageDirectory().getPath()+"/test/test.mp4";

        mPlayerBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.e(TAG, "onClick: "+path);
                mPlayer.setDataResource(path);
            }
        });




    }


}
