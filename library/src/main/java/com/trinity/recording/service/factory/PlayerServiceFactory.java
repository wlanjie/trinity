package com.trinity.recording.service.factory;

import android.os.Handler;
import com.trinity.recording.RecordingImplType;
import com.trinity.recording.service.PlayerService;
import com.trinity.recording.service.impl.AudioTrackPlayerServiceImpl;

public class PlayerServiceFactory {

	private static PlayerServiceFactory instance = new PlayerServiceFactory();

	private PlayerServiceFactory() {
	}

	public static PlayerServiceFactory getInstance() {
		return instance;
	}

	public PlayerService getPlayerService(final PlayerService.OnCompletionListener onComletionListener, RecordingImplType recordingImplType,
										  Handler mTimeHandler) {
		PlayerService result = null;
		switch (recordingImplType) {
		case ANDROID_PLATFORM:
			result = new AudioTrackPlayerServiceImpl() {
				@Override
				public void onCompletion() {
					onComletionListener.onCompletion();
				}
			};
			break;
		default:
			break;
		}
		result.setHandler(mTimeHandler);
		return result;
	}
}
