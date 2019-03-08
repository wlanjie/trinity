package com.trinity.recording.video;

import android.os.Build;
import android.os.Handler;
import com.trinity.VideoStudio;
import com.trinity.camera.PreviewScheduler;
import com.trinity.recording.RecordingImplType;
import com.trinity.recording.exception.*;
import com.trinity.recording.service.PlayerService;
import com.trinity.recording.service.factory.PlayerServiceFactory;
import com.trinity.recording.service.impl.AudioRecordRecorderServiceImpl;
import com.trinity.recording.video.service.factory.MediaRecorderServiceFactory;

public class CommonVideoRecordingStudio extends VideoRecordingStudio {

  private PlayerService.OnCompletionListener onComletionListener;

  public CommonVideoRecordingStudio(RecordingImplType recordingImplType,
                                    PlayerService.OnCompletionListener onComletionListener, RecordingStudioStateCallback recordingStudioStateCallback) {
    super(recordingImplType, recordingStudioStateCallback);
    // 伴奏的初始化
    this.onComletionListener = onComletionListener;
  }

  @Override
  public void initRecordingResource(PreviewScheduler scheduler) throws RecordingStudioException {
    /**
     * 这里一定要注意顺序，先初始化record在初始化player，因为player中要用到recorder中的samplerateSize
     **/
    if (scheduler == null) {
      throw new RecordingStudioNullArgumentException("null argument exception in initRecordingResource");
    }

    scheduler.resetStopState();
    setRecorderService(MediaRecorderServiceFactory.getInstance().getRecorderService(scheduler, getRecordingImplType()));
    if (getRecorderService() != null) {
      getRecorderService().initMetaData();
    }
    if (getRecorderService() != null && !getRecorderService().initMediaRecorderProcessor()) {
      throw new InitRecorderFailException();
    }
    // 初始化伴奏带额播放器 实例化以及init播放器
    setPlayerService(PlayerServiceFactory.getInstance().getPlayerService(onComletionListener,
        RecordingImplType.ANDROID_PLATFORM));
    if (getPlayerService() != null) {
      boolean result = getPlayerService().setAudioDataSource(AudioRecordRecorderServiceImpl.Companion.getSAMPLE_RATE_IN_HZ());
      if (!result) {
        throw new InitPlayerFailException();
      }
    }
  }

  public boolean isPlayingAccompany() {
    boolean ret = false;
    if (null != getPlayerService()) {
      ret = getPlayerService().isPlayingAccompany();
    }
    return ret;
  }

  @Override
  protected int startConsumer(final String outputPath, final int videoBitRate, final int videoWidth, final int videoHeight, final int audioSampleRate,
                              int qualityStrategy, int adaptiveBitrateWindowSizeInSecs, int adaptiveBitrateEncoderReconfigInterval,
                              int adaptiveBitrateWarCntThreshold, int adaptiveMinimumBitrate,
                              int adaptiveMaximumBitrate) {

    qualityStrategy = ifQualityStrayegyEnable(qualityStrategy);
    return VideoStudio.getInstance().startVideoRecord(outputPath,
        videoWidth, videoHeight, VIDEO_FRAME_RATE, videoBitRate,
        audioSampleRate, audioChannels, audioBitRate,
        qualityStrategy);
  }

  private int ifQualityStrayegyEnable(int qualityStrategy) {
    qualityStrategy = (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) ? 0 : qualityStrategy;
    return qualityStrategy;
  }

  @Override
  protected boolean startProducer(final int videoWidth, int videoHeight, int videoBitRate, boolean useHardWareEncoding, int strategy) throws StartRecordingException {
    if (getPlayerService() != null) {
      getPlayerService().start();
    }
    if (getRecorderService() != null) {
      return getRecorderService().start(videoWidth, videoHeight, videoBitRate, VIDEO_FRAME_RATE, useHardWareEncoding, strategy);
    }

    return false;
  }

  @Override
  public void destroyRecordingResource() {
    // 销毁伴奏播放器
    if (getPlayerService() != null) {
      getPlayerService().stop();
      setPlayerService(null);
    }
    super.destroyRecordingResource();
  }

  /**
   * 播放一个新的伴奏
   **/
  public void startAccompany(String musicPath) {
    if (null != getPlayerService()) {
      getPlayerService().startAccompany(musicPath);
    }
  }

  /**
   * 停止播放
   **/
  public void stopAccompany() {
    if (null != getPlayerService()) {
      getPlayerService().stopAccompany();
    }
  }

  /**
   * 暂停播放
   **/
  public void pauseAccompany() {
    if (null != getPlayerService()) {
      getPlayerService().pauseAccompany();
    }
  }

  /**
   * 继续播放
   **/
  public void resumeAccompany() {
    if (null != getPlayerService()) {
      getPlayerService().resumeAccompany();
    }
  }

  /**
   * 设置伴奏的音量大小
   **/
  public void setAccompanyVolume(float volume, float accompanyMax) {
    if (null != getPlayerService()) {
      getPlayerService().setVolume(volume, accompanyMax);
    }
  }

}
