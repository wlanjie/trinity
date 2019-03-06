package com.trinity.player;

import android.media.AudioFormat;
import android.media.AudioTrack;
import android.util.Log;
import com.trinity.SongStudio;
import com.trinity.decoder.Mp3Decoder;
import com.trinity.recording.service.PlayerService;

public class AudioTrackPlayer {

  public static final String TAG = "AudioTrackPlayer";
  private static final int BITS_PER_BYTE = 8;
  private static final int BITS_PER_CHANNEL = 16;
  private static final int CHANNEL_PER_FRAME = 2;
  private Mp3Decoder decoder;

  private AudioTrack audioTrack;
  /** 用于在暂停时候，播放空数据 **/

  /**
   * default mp3's decode buffer is 2 times than play buffer
   **/
  private static int DECODE_BUFFER_SIZE = 16 * 1024;
  private int sampleRateInHz = -1;
  private static int channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
  private static int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
  private int audioStreamType = -1;
  private PlayerService.OnCompletionListener onCompletionListener = null;
  private Thread playerThread;

  /**
   * 播放状态
   */
  private volatile boolean isStop = false;
  private volatile boolean isPlaying = false;

  public void setAudioStreamType(int audioStreamType) {
    this.audioStreamType = audioStreamType;
  }

  public void setOnCompletionListener(
      PlayerService.OnCompletionListener onCompletionListener) {
    this.onCompletionListener = onCompletionListener;
  }

  /**
   * 开始播放线程
   **/
  public boolean prepare(int vocalSampleRate) {
    boolean result = true;
    this.sampleRateInHz = vocalSampleRate;
    decoder = SongStudio.getInstance().getMp3Decoder();
    initMetaData();
    SongStudio songstudio = SongStudio.getInstance();
    float percent = songstudio.getPacketBufferTime().getPercent();
    decoder.init(percent, vocalSampleRate);
    result = initAudioTrack();
    if (result) {
      startPlayerThread();
    }
    return result;
  }

  private void initMetaData() {
    int byteCountPerSec = sampleRateInHz * CHANNEL_PER_FRAME * BITS_PER_CHANNEL / BITS_PER_BYTE;
    float percent = SongStudio.getInstance().getPacketBufferTime().getPercent();
    DECODE_BUFFER_SIZE = (int) ((byteCountPerSec / 2) * percent);
    isPlaying = false;
    isStop = false;
  }

  private boolean initAudioTrack() {
    boolean result = true;
    int bufferSize = AudioTrack.getMinBufferSize(sampleRateInHz,
        channelConfig, audioFormat);
    audioTrack = new AudioTrack(this.audioStreamType, sampleRateInHz,
        channelConfig, audioFormat, bufferSize,
        AudioTrack.MODE_STREAM);
    audioTrack.setStereoVolume(1, 1);
    if (null == audioTrack || audioTrack.getState() == AudioTrack.STATE_UNINITIALIZED) {
      result = false;
    }
    return result;
  }

  private void startPlayerThread() {
    playerThread = new Thread(new PlayerThread(), "NativeMp3PlayerThread");
    playerThread.start();
  }

  public void start() {
    synchronized (AudioTrackPlayer.class) {
      try {
        if (null != audioTrack) {
          audioTrack.play();
          short[] samples = new short[DECODE_BUFFER_SIZE];
          audioTrack.write(samples, 0, DECODE_BUFFER_SIZE);
        }
      } catch (Throwable t) {
      }
      isPlaying = true;
    }
  }

  public void stop() {
    if (!isStop && null != audioTrack) {
      if (null != audioTrack && audioTrack.getState() != AudioTrack.STATE_UNINITIALIZED) {
        try {
          audioTrack.stop();
        } catch (Throwable t) {
          t.printStackTrace();
        }
      }
      isPlaying = true;
      isStop = true;
      try {
        Log.i(TAG, "join decodeMp3Thread....");
        if (null != playerThread) {
          playerThread.join();
          playerThread = null;
        }
        Log.i(TAG, " decodeMp3Thread ended....");
      } catch (Throwable t) {
        t.printStackTrace();
      }
      closeAudioTrack();
      destroy();
    }
  }

  public void destroy() {
    playerThread = null;
    audioTrack = null;
  }

  private void closeAudioTrack() {
    try {
      if (null != audioTrack
          && audioTrack.getState() != AudioTrack.STATE_UNINITIALIZED) {
        audioTrack.release();
      }
    } catch (Throwable t) {
      t.printStackTrace();
    }
  }

  public static final int ACCOMPANY_QUEUE_USING_UP = -2;
  public static final int SILIENT_SAMPLE_TYPE = 0;
  public static final int ACCOMPANY_SAMPLE_TYPE = 1;

  private int lastSampleType = SILIENT_SAMPLE_TYPE;

  class PlayerThread implements Runnable {
    private short[] samples;

    @Override
    public void run() {
      int sample_count = 0;
      boolean isPlayTemp = isPlaying = false;
      int sampleType = SILIENT_SAMPLE_TYPE;
      try {
        samples = new short[DECODE_BUFFER_SIZE];
        int[] extraSlientSampleSize = new int[1];
        int[] extraSampleType = new int[1];
        while (!isStop) {
          extraSlientSampleSize[0] = 0;
          extraSampleType[0] = SILIENT_SAMPLE_TYPE;
          sample_count = decoder.readSamples(samples, extraSlientSampleSize, extraSampleType);
          if (sample_count == ACCOMPANY_QUEUE_USING_UP) {
            try {
              Thread.sleep(10);
            } catch (InterruptedException e) {
              e.printStackTrace();
            }
            Log.i(TAG, "WARN : no play data");
            continue;
          }
          while (true) {
            synchronized (AudioTrackPlayer.class) {
              isPlayTemp = isPlaying;
            }
            if (isPlayTemp)
              break;
            else
              Thread.yield();
          }
          sampleType = extraSampleType[0];
          if (SILIENT_SAMPLE_TYPE == lastSampleType && ACCOMPANY_SAMPLE_TYPE == sampleType) {
            if (isInPauseState) {
              //点击了继续播放按钮之后音乐播放
              audioTrackStartPlayBaseHeadPosition = audioTrack.getPlaybackHeadPosition();
              isInPauseState = false;
            } else {
              //开始播放伴奏之后音乐播放
              audioTrackStartPlayBaseHeadPosition = audioTrack.getPlaybackHeadPosition();
              isInPauseState = false;
              isInSwitching = false;
              hasPlayedTimeMills = 0;
            }
          }
          if (ACCOMPANY_SAMPLE_TYPE == lastSampleType && SILIENT_SAMPLE_TYPE == sampleType) {
            if (pauseRequest) {
              //点击了暂停按钮
              Log.i("problem", "点击了暂停按钮");
              long playedHeadPosition = audioTrack.getPlaybackHeadPosition() - audioTrackStartPlayBaseHeadPosition;
              hasPlayedTimeMills += (int) ((float) playedHeadPosition * 1000 / sampleRateInHz);
              isInPauseState = true;
              pauseRequest = false;
            } else if (isInSwitching) {
              //正在切换到一个新的伴奏
              Log.i("problem", "正在切换到一个新的伴奏");
            } else if (isInStopping) {
              //正在关闭一个伴奏
              isInStopping = false;
              Log.i("problem", "正在关闭一个伴奏");
            } else {
              //播放结束了这个伴奏了
              Log.i("problem", "播放结束了这个伴奏了");
              onCompletionListener.onCompletion();
            }
          }

          if (null != audioTrack && audioTrack.getState() != AudioTrack.STATE_UNINITIALIZED) {
            if (extraSlientSampleSize[0] > 0) {
              int totalSize = extraSlientSampleSize[0] + sample_count;
              short[] playSamples = new short[totalSize];
              System.arraycopy(samples, 0, playSamples, extraSlientSampleSize[0], sample_count);
              audioTrack.write(playSamples, 0, totalSize);
            } else {
              audioTrack.write(samples, 0, sample_count);
            }
          }
          lastSampleType = sampleType;
        }
        decoder.destroy();
      } catch (Error e) {
        e.printStackTrace();
      }
      samples = null;
      //Log.i(TAG, "end Native Mp3 player thread...");
    }
  }

  public int getAccompanySampleRate() {
    return sampleRateInHz;
  }

  public void setVolume(float volume, float accompanyMax) {
    if (null != decoder) {
      decoder.setAccompanyVolume(volume, accompanyMax);
    }
  }

  public int getCurrentTimeMills() {
    int currentTimeMills = 0;
    try {
      long curPos = audioTrack.getPlaybackHeadPosition();
      currentTimeMills = (int) ((float) curPos * 1000 / sampleRateInHz);
    } catch (Throwable e) {
    }
    return currentTimeMills;
  }

  /**
   * Mini播放器的播放暂停状态
   **/
  private boolean isInPauseState = false;
  /**
   * 用于计算Mini播放器的时间刷新
   **/
  private volatile long audioTrackStartPlayBaseHeadPosition = 0;
  private volatile long hasPlayedTimeMills = 0;
  private boolean pauseRequest = false;
  private boolean isInSwitching = false;
  private boolean isInStopping = false;

  public void startAccompany(String path) {
    pauseRequest = false;
    isInSwitching = true;
    isInStopping = false;
    hasPlayedTimeMills = 0;
    isInPauseState = false;
    decoder.startAccompany(path);
  }

  public void pauseAccompany() {
    pauseRequest = true;
    isInStopping = false;
    decoder.pauseAccompany();
  }

  public void resumeAccompany() {
    decoder.resumeAccompany();
    isInStopping = false;
  }

  public void stopAccompany() {
    isInStopping = true;
    decoder.stopAccompany();
  }

  public int getPlayedAccompanyTimeMills() {
    int playedAccompanyTimeMills = 0;
    try {
      long curHeadPosition = audioTrack.getPlaybackHeadPosition();
      if (isInPauseState) {
        playedAccompanyTimeMills = (int) hasPlayedTimeMills;
      } else {
        if (SILIENT_SAMPLE_TYPE == lastSampleType || isInSwitching) {
          playedAccompanyTimeMills = 0;
        } else {
          long subHeadPosition = curHeadPosition - audioTrackStartPlayBaseHeadPosition;
          playedAccompanyTimeMills = (int) hasPlayedTimeMills + (int) ((float) subHeadPosition * 1000 / sampleRateInHz);
        }
      }
    } catch (Throwable e) {
    }
    return playedAccompanyTimeMills;
  }

  public boolean isPlayingAccompany() {
    if (ACCOMPANY_SAMPLE_TYPE == lastSampleType) {
      return true;
    }
    return false;
  }

}
