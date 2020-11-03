package com.trinity.sample;

import android.app.Dialog;
import android.media.MediaMetadataRetriever;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.trinity.sample.adapter.CreatedVideoListAdapter;

import java.io.File;
import java.util.ArrayList;

public class MyCreationActivity extends AppCompatActivity {

    GridLayoutManager gridLayoutManager;
    LinearLayoutManager linearLayoutManager;

    CreatedVideoListAdapter createVideoListAdapter;
    ArrayList<AlbumFile> createVideoList = new ArrayList<>();

    int audioPlayingPosition = -1;
    int audioPausePosition = -1;
    Boolean isSelectedVideo = true;

    MediaPlayer mediaPlayer,mPlayer;
    Dialog songDialog;
    boolean isAudioPlay= false;

    MediaMetadataRetriever retriever = new MediaMetadataRetriever();

    RecyclerView createVideoRecyclerView;
    TextView creationErrorView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_my_creation);

        createVideoRecyclerView = findViewById(R.id.rv_save_image_list);
        creationErrorView = findViewById(R.id.tv_no_creation);
        Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                MyCreationActivity.this.setLayoutManagerVideo();
                MyCreationActivity.this.setCreationsList();
            }
        }, 300);
    }

    void addVideoPath() {
        String filepath = Environment.getExternalStorageDirectory().toString();

        File dir = new File(filepath + "/" + "Video Creator");
        File file = new File(String.valueOf(dir));
        File[] files = file.listFiles();
        try {
            if (file.exists()) {
                for (File savedFiles : files) {
                    AlbumFile albumFile = new AlbumFile();
                    String uri = savedFiles.getAbsolutePath();
                    String extension = uri.substring(uri.lastIndexOf("."));
                    try {
                        retriever.setDataSource(this, Uri.fromFile(new File(uri)));
                        String time = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);
                        if (time != null) {
                            if (".mp4".equals(extension)) {
                                albumFile.setPath(savedFiles.getAbsolutePath());
                                createVideoList.add(albumFile);
                            }
                        }
                    } catch (Exception e) {
                    }
                }
            }
        } catch (Exception e) {
            Log.e("Error: ", e.toString());
        }
        setCreationsList();
    }

    void setLayoutManagerVideo() {
        gridLayoutManager = new GridLayoutManager(this, 2);
        createVideoRecyclerView.setLayoutManager(gridLayoutManager);
        createVideoListAdapter = new CreatedVideoListAdapter(this, createVideoList);
        addVideoPath();
        createVideoRecyclerView.setAdapter(createVideoListAdapter);
    }

    public void setCreationsList() {
        if (createVideoList.size() == 0 && isSelectedVideo) {
            createVideoRecyclerView.setVisibility(View.GONE);
            creationErrorView.setVisibility(View.VISIBLE);
        }else {
            creationErrorView.setVisibility(View.GONE);
        }
    }

}
