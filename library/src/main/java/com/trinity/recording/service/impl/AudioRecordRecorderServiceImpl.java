package com.trinity.recording.service.impl;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import com.trinity.PacketBufferTimeEnum;
import com.trinity.Songstudio;
import com.trinity.recording.exception.AudioConfigurationException;
import com.trinity.recording.exception.StartRecordingException;
import com.trinity.recording.processor.RecordProcessor;
import com.trinity.recording.service.PlayerService;
import com.trinity.recording.service.RecorderService;

public class AudioRecordRecorderServiceImpl implements RecorderService {

	private static final String TAG = "AudioRecordRecorderServiceImpl";
	
	public static final int WRITE_FILE_FAIL = 9208911;
	protected AudioRecord audioRecord;
	private Thread recordThread;

	private RecordProcessor recordProcessor;

	private int recordVolume = 0;
	private int AUDIO_SOURCE = MediaRecorder.AudioSource.MIC;
	public static int SAMPLE_RATE_IN_HZ = 44100;
	private final static int CHANNEL_CONFIGURATION = AudioFormat.CHANNEL_IN_MONO;
	private final static int AUDIO_FORMAT = AudioFormat.ENCODING_PCM_16BIT;

	private int bufferSizeInBytes = 0;
	private int bufferSizeInShorts = 0;

	private boolean isRecording = false;
	//音频清唱会暂停
	private volatile boolean isPause = false;

	private static final AudioRecordRecorderServiceImpl instance = new AudioRecordRecorderServiceImpl();

	protected AudioRecordRecorderServiceImpl() {
	}

	public static AudioRecordRecorderServiceImpl getInstance() {
		return instance;
	}

	@Override
	public void initMetaData() throws AudioConfigurationException {
		if (null != audioRecord) {
			audioRecord.release();
		}
		try {
			// 首先利用我们标准的44_1K的录音频率初是录音器
			bufferSizeInBytes = AudioRecord.getMinBufferSize(SAMPLE_RATE_IN_HZ,
					CHANNEL_CONFIGURATION, AUDIO_FORMAT);
			audioRecord = new AudioRecord(AUDIO_SOURCE, SAMPLE_RATE_IN_HZ,
					CHANNEL_CONFIGURATION, AUDIO_FORMAT, bufferSizeInBytes);
		} catch (Exception e) {
			e.printStackTrace();
		}
		// 如果初始化不成功的话,则降低为16K的采样率来初始化录音器
		if (audioRecord == null
				|| audioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
			try {
				SAMPLE_RATE_IN_HZ = 16000;
				bufferSizeInBytes = AudioRecord.getMinBufferSize(
						SAMPLE_RATE_IN_HZ, CHANNEL_CONFIGURATION, AUDIO_FORMAT);
				audioRecord = new AudioRecord(AUDIO_SOURCE, SAMPLE_RATE_IN_HZ,
						CHANNEL_CONFIGURATION, AUDIO_FORMAT, bufferSizeInBytes);
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		if (audioRecord == null || audioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
			throw new AudioConfigurationException();
		}
		bufferSizeInShorts = bufferSizeInBytes / 2;
	}

	@Override
	public void start() throws StartRecordingException {
		if (audioRecord != null
				&& audioRecord.getState() == AudioRecord.STATE_INITIALIZED) {
			try{
				audioRecord.startRecording();
			}catch (Exception e){
				throw new StartRecordingException();
			}
		} else {
			throw new StartRecordingException();
		}
		isRecording = true;
		isPause = false;
		recordThread = new Thread(new RecordThread(), "RecordThread");
		try{
			recordThread.start();
		} catch(Exception e){
			throw new StartRecordingException();
		}
	}

	@Override
	public boolean initAudioRecorderProcessor() {
		boolean result = true;
		Songstudio songstudio = Songstudio.getInstance();
		songstudio.resetPacketBufferTime();
		recordProcessor = songstudio.getAudioRecorder();
		PacketBufferTimeEnum packetBufferTime = songstudio
				.getPacketBufferTime();
		float percent = packetBufferTime.getPercent();
		while (true) {
			packetBufferTime = songstudio.getPacketBufferTime();
			percent = packetBufferTime.getPercent();
			if (packetBufferTime == PacketBufferTimeEnum.ERROR_STATE) {
				result = false;
				break;
			}
			if (SAMPLE_RATE_IN_HZ * percent < bufferSizeInShorts) {
				songstudio.upPacketBufferTimeLevel();
			} else {
				break;
			}
		}
		recordProcessor
				.initAudioBufferSize(SAMPLE_RATE_IN_HZ, (int) (SAMPLE_RATE_IN_HZ * percent));
		return result;
	}

	@Override
	public void destoryAudioRecorderProcessor() {
		if (recordProcessor != null) {
			recordProcessor.destroy();
		}
	}

	@Override
	public int getAudioBufferSize() {
		Songstudio songstudio = Songstudio.getInstance();
		PacketBufferTimeEnum packetBufferTime = songstudio
				.getPacketBufferTime();
		float percent = packetBufferTime.getPercent();
		return (int) (SAMPLE_RATE_IN_HZ * percent);
	}

	@Override
	public void stop() {
		try {
			if (audioRecord != null) {
				isRecording = false;
				isPause = false;
				try {
					if (recordThread != null) {
						recordThread.join();
						recordThread = null;
					}
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				releaseAudioRecord();
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	private void releaseAudioRecord() {
		if (audioRecord.getState() == AudioRecord.STATE_INITIALIZED)
			audioRecord.stop();
		audioRecord.release();
		audioRecord = null;
	}

	private boolean isUnAccom = false;

	public void enableUnAccom() {
		isUnAccom = true;
	}

	class RecordThread implements Runnable {

		@Override
		public void run() {
			int scoringBufferMaxSize = bufferSizeInShorts;
			short[] audioSamples = new short[scoringBufferMaxSize];
			int offset = (int) (((long) scoringBufferMaxSize) * 1000 / SAMPLE_RATE_IN_HZ);
			while (isRecording) {
				boolean localPaused = isPause;
//				android.util.Log.d(TAG, "RecordThread run() localPaused="+localPaused+"  isUnAccom="+isUnAccom);
				if (isUnAccom && localPaused) { //只有是清唱才能暂停录制
					continue;
				}
				int audioSampleSize = getAudioRecordBuffer(
						scoringBufferMaxSize, audioSamples);
				if (audioSampleSize > 0) {
					if (isUnAccom) {
						float x = (float) Math.abs(audioSamples[0])
								/ Short.MAX_VALUE;
						recordVolume = Math.round((2 * x - x * x) * 9);
						// Log.i("problem", "recordVolume"+recordVolume);
					}
					recordProcessor.pushAudioBufferToQueue(audioSamples,
							audioSampleSize);
				}
			}
			recordProcessor.flushAudioBufferToQueue();
		}
	}

	protected int getAudioRecordBuffer(int scoringBufferMaxSize,
			short[] audioSamples) {
		if (audioRecord != null) {
			int size = audioRecord.read(audioSamples, 0,
					scoringBufferMaxSize);
			return size;
		} else {
			return 0;
		}
	}
	
	private int getPlayCurrentTimeMills() {
		if (null != playerService) {
			return playerService.getPlayerCurrentTimeMills();
		}
		return 0;
	}

	@Override
	public int getSampleRate() {
		return SAMPLE_RATE_IN_HZ;
	}

	protected PlayerService playerService = null;

	@Override
	public int getRecordVolume() {
		return recordVolume;
	}

	@Override
	public void pause() {
		isPause = true;
	}

	@Override
	public void continueRecord() {
		isPause = false;
	}

	@Override
	public boolean isPaused() {
		return isPause;
	}
	
}
