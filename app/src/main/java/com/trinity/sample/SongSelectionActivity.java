package com.trinity.sample;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class SongSelectionActivity extends Activity {

	private Button song_1;
	private Button song_2;
	private Button bgm_1;
	
	public static final String SONG_ID = "song_id";
	public static final int ACCOMPANY_TYPE = 0;
	public static final int BGM_TYPE = 1;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.song_selection_layout);
		findView();
		bindListener();
	}
	private void findView() {
		song_1 = (Button) findViewById(R.id.song_1);
		song_2 = (Button) findViewById(R.id.song_2);
		bgm_1 = (Button) findViewById(R.id.bgm_1);
	}
	private void bindListener() {
		song_1.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				Intent intent = new Intent();
				Bundle bundle = new Bundle();
				Song song = new Song();
				song.setArtist("陶喆");
				song.setSongId(32541);
				song.setName("算你狠-陶喆版本");
				song.setType(ACCOMPANY_TYPE);
				bundle.putSerializable(SONG_ID, song);
				intent.putExtras(bundle);
				setResult(RESULT_OK, intent);
				finish();
			}
		});
		song_2.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				Intent intent = new Intent();
				Bundle bundle = new Bundle();
				Song song = new Song();
				song.setArtist("陈小春");
				song.setSongId(199);
				song.setName("算你狠");
				song.setType(ACCOMPANY_TYPE);
				bundle.putSerializable(SONG_ID, song);
				intent.putExtras(bundle);
				setResult(RESULT_OK, intent);
				finish();
			}
		});
		bgm_1.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				Intent intent = new Intent();
				Bundle bundle = new Bundle();
				Song song = new Song();
				song.setArtist("陈小春");
				song.setSongId(1999);
				song.setName("算你狠");
				song.setType(BGM_TYPE);
				bundle.putSerializable(SONG_ID, song);
				intent.putExtras(bundle);
				setResult(RESULT_OK, intent);
				finish();
			}
		});
	}
	
}
