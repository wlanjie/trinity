package com.trinity.recording.service;

import android.os.Handler;

import java.util.Timer;
import java.util.TimerTask;

public abstract class AbstractPlayerServiceImpl implements PlayerService {

	private boolean isPlaying = false;
	private Timer mTimer;
	private TimerTask myTimerTask = null;
	private Handler mHander;

	public void setHandler(Handler mTimeHandler) {
		this.mHander = mTimeHandler;
	}

	public boolean isPlaying() {
		return isPlaying;
	}

	/**
	 * 由于播放开始需要设置isPlaying以及启动定时器来刷新时间，所以放在公共部分
	 */
	@Override
	public void start() {
		timerStop();
		isPlaying = true;
		timerStart();
	}

	/**
	 * 由于播放停止需要设置isPlaying以及停止定时器来刷新时间，所以放在公共部分
	 */
	@Override
	public void stop() {
		timerStop();
		isPlaying = false;
	}

	protected void timerStop() {
		if (mTimer != null) {
			mTimer.cancel();
			mTimer = null;
		}
		if (myTimerTask != null) {
			myTimerTask.cancel();
			myTimerTask = null;
		}
	}

	protected void timerStart() {
		if (mTimer == null) {
			mTimer = new Timer();
			myTimerTask = new MusicTimerTask();
			mTimer.schedule(myTimerTask, 0, 50);
		}
	}

	class MusicTimerTask extends TimerTask {
		@Override
		public void run() {
			int playerTotalTimeMills = getPlayerCurrentTimeMills();
			int playedAccompanyTimeMills = getPlayedAccompanyTimeMills();
			if (playerTotalTimeMills != 0) {
				mHander.sendMessage(mHander.obtainMessage(
						UPDATE_PLAY_VOICE_PROGRESS, playerTotalTimeMills, playedAccompanyTimeMills));
			}
		}
	}

}
