package com.trinity.sample.adapter;

import android.content.Context;
import android.content.Intent;
import android.media.MediaMetadataRetriever;
import android.net.Uri;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.bumptech.glide.Glide;
import com.trinity.sample.AlbumFile;
import com.trinity.sample.R;

import java.io.File;
import java.util.ArrayList;

public class CreatedVideoListAdapter extends RecyclerView.Adapter<CreatedVideoListAdapter.MyViewHolder>  {
    private ArrayList<AlbumFile> videoList;
    private Context context;
    MediaMetadataRetriever retriever = new MediaMetadataRetriever();

    public CreatedVideoListAdapter(Context context, ArrayList videoList) {
        this.context = context;
        this.videoList = videoList;
    }

    public void replaceData(ArrayList<AlbumFile> arrayList) {
        videoList = arrayList;
        notifyDataSetChanged();
    }

    @NonNull
    @Override
    public CreatedVideoListAdapter.MyViewHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int i) {
        LayoutInflater layoutInflater = LayoutInflater.from(viewGroup.getContext());
        View view = layoutInflater.inflate(R.layout.adapter_createdvideo, viewGroup, false);
        return new MyViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull CreatedVideoListAdapter.MyViewHolder myViewHolder, final int i) {
        final AlbumFile albumFile = videoList.get(i);
        Glide.with(context)
                .load(albumFile.getPath())
//                .override(540, 960)
//                .fitCenter()
                .into(myViewHolder.videoImage);


        myViewHolder.videoImage.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

            }
        });
    }

    @Override
    public int getItemCount() {
        return videoList.size();
    }

    class MyViewHolder extends RecyclerView.ViewHolder {

        ImageView videoImage;

        RelativeLayout mainLayout;

        MyViewHolder(@NonNull View itemView) {
            super(itemView);
            videoImage = itemView.findViewById(R.id.iv_create_video_Image);
        }
    }

    public boolean getVideoTime(String getPath) {
        try {
            retriever.setDataSource(context, Uri.fromFile(new File(getPath)));
            String time = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);
            if (time == null) {
                return true;
            }
        } catch (Exception e) {
            return true;
        }
        return false;
    }
}
