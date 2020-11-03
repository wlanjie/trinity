package com.trinity.sample;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.Toast;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;

public class HomeActivity extends AppCompatActivity {
    RelativeLayout camera;
    RelativeLayout creation;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_home);
        camera = findViewById(R.id.rl_gallery);
        creation = findViewById(R.id.rl_creation);

        camera.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(HomeActivity.this,RecordActivity.class);
                startActivity(intent);
            }
        });
        creation.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(HomeActivity.this,MyCreationActivity.class);
                startActivity(intent);
            }
        });
    }
    @SuppressLint("NewApi")
    public void muxing(String video,String audio) {

        String outputFile = "";
        String root = Environment.getExternalStorageDirectory().toString();

        try {

            root = Environment.getExternalStorageDirectory().toString();
//            String audio = root + "/" + "audio.mp3";
//            String video = root + "/" + "rohit.mp4";


            File file = new File(Environment.getExternalStorageDirectory() + File.separator + "final2.mp4");
            file.createNewFile();
            outputFile = file.getAbsolutePath();


            MediaExtractor videoExtractor = new MediaExtractor();
            videoExtractor.setDataSource(video);

            MediaExtractor audioExtractor = new MediaExtractor();
            audioExtractor.setDataSource(audio);

            Log.d("TAG", "Video Extractor Track Count " + videoExtractor.getTrackCount());
            Log.d("TAG", "Audio Extractor Track Count " + audioExtractor.getTrackCount());

            MediaMuxer muxer = new MediaMuxer(outputFile, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);

            videoExtractor.selectTrack(0);
            MediaFormat videoFormat = videoExtractor.getTrackFormat(0);
            int videoTrack = muxer.addTrack(videoFormat);

            audioExtractor.selectTrack(0);
            MediaFormat audioFormat = audioExtractor.getTrackFormat(0);
            int audioTrack = muxer.addTrack(audioFormat);

            Log.d("TAG", "Video Format " + videoFormat.toString());
            Log.d("TAG", "Audio Format " + audioFormat.toString());

            boolean sawEOS = false;
            int frameCount = 0;
            int offset = 100;
            int sampleSize = 256 * 1024;
            ByteBuffer videoBuf = ByteBuffer.allocate(sampleSize);
            ByteBuffer audioBuf = ByteBuffer.allocate(sampleSize);
            MediaCodec.BufferInfo videoBufferInfo = new MediaCodec.BufferInfo();
            MediaCodec.BufferInfo audioBufferInfo = new MediaCodec.BufferInfo();


            videoExtractor.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
            audioExtractor.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);

            muxer.start();

            while (!sawEOS) {
                videoBufferInfo.offset = offset;
                videoBufferInfo.size = videoExtractor.readSampleData(videoBuf, offset);


                if (videoBufferInfo.size < 0 || audioBufferInfo.size < 0) {
                    Log.d("TAG", "saw input EOS.");
                    sawEOS = true;
                    videoBufferInfo.size = 0;

                } else {
                    videoBufferInfo.presentationTimeUs = videoExtractor.getSampleTime();
                    videoBufferInfo.flags = MediaCodec.BUFFER_FLAG_KEY_FRAME;
                    muxer.writeSampleData(videoTrack, videoBuf, videoBufferInfo);
                    videoExtractor.advance();


                    frameCount++;
                    Log.d("TAG", "Frame (" + frameCount + ") Video PresentationTimeUs:" + videoBufferInfo.presentationTimeUs + " Flags:" + videoBufferInfo.flags + " Size(KB) " + videoBufferInfo.size / 1024);
                    Log.d("TAG", "Frame (" + frameCount + ") Audio PresentationTimeUs:" + audioBufferInfo.presentationTimeUs + " Flags:" + audioBufferInfo.flags + " Size(KB) " + audioBufferInfo.size / 1024);

                }
            }

            Toast.makeText(getApplicationContext(), "frame:" + frameCount, Toast.LENGTH_SHORT).show();


            boolean sawEOS2 = false;
            int frameCount2 = 0;
            while (!sawEOS2) {
                frameCount2++;

                audioBufferInfo.offset = offset;
                audioBufferInfo.size = audioExtractor.readSampleData(audioBuf, offset);

                if (videoBufferInfo.size < 0 || audioBufferInfo.size < 0) {
                    Log.d("TAG", "saw input EOS.");
                    sawEOS2 = true;
                    audioBufferInfo.size = 0;
                } else {
                    audioBufferInfo.presentationTimeUs = audioExtractor.getSampleTime();
                    audioBufferInfo.flags = MediaCodec.BUFFER_FLAG_KEY_FRAME;
                    muxer.writeSampleData(audioTrack, audioBuf, audioBufferInfo);
                    audioExtractor.advance();


                    Log.d("TAG", "Frame (" + frameCount + ") Video PresentationTimeUs:" + videoBufferInfo.presentationTimeUs + " Flags:" + videoBufferInfo.flags + " Size(KB) " + videoBufferInfo.size / 1024);
                    Log.d("TAG", "Frame (" + frameCount + ") Audio PresentationTimeUs:" + audioBufferInfo.presentationTimeUs + " Flags:" + audioBufferInfo.flags + " Size(KB) " + audioBufferInfo.size / 1024);

                }
            }

            Toast.makeText(getApplicationContext(), "frame:" + frameCount2, Toast.LENGTH_SHORT).show();

            muxer.stop();
            muxer.release();


        } catch (IOException e) {
            Log.d("TAG", "Mixer Error 1 " + e.getMessage());
        } catch (Exception e) {
            Log.d("TAG", "Mixer Error 2 " + e.getMessage());
        }
    }

}
