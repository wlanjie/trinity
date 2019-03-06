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

  private Handler mTimeHandler;
  private PlayerService.OnCompletionListener onComletionListener;

  public CommonVideoRecordingStudio(RecordingImplType recordingImplType, Handler mTimeHandler,
                                    PlayerService.OnCompletionListener onComletionListener, RecordingStudioStateCallback recordingStudioStateCallback) {
    super(recordingImplType, recordingStudioStateCallback);
    // 伴奏的初始化
    this.onComletionListener = onComletionListener;
    this.mTimeHandler = mTimeHandler;
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
    recorderService = MediaRecorderServiceFactory.getInstance().getRecorderService(scheduler, recordingImplType);
    if (recorderService != null) {
      recorderService.initMetaData();
    }
    if (recorderService != null && !recorderService.initMediaRecorderProcessor()) {
      throw new InitRecorderFailException();
    }
    // 初始化伴奏带额播放器 实例化以及init播放器
    playerService = PlayerServiceFactory.getInstance().getPlayerService(onComletionListener,
        RecordingImplType.ANDROID_PLATFORM, mTimeHandler);
    if (playerService != null) {
      boolean result = playerService.setAudioDataSource(AudioRecordRecorderServiceImpl.SAMPLE_RATE_IN_HZ);
      if (!result) {
        throw new InitPlayerFailException();
      }
    }
  }

  public boolean isPlayingAccompany() {
    boolean ret = false;
    if (null != playerService) {
      ret = playerService.isPlayingAccompany();
    }
    return ret;
  }

  @Override
  protected int startConsumer(final String outputPath, final int videoWidth, final int videoHeight, final int audioSampleRate,
                              int qualityStrategy, int adaptiveBitrateWindowSizeInSecs, int adaptiveBitrateEncoderReconfigInterval,
                              int adaptiveBitrateWarCntThreshold, int adaptiveMinimumBitrate,
                              int adaptiveMaximumBitrate) {

    qualityStrategy = ifQualityStrayegyEnable(qualityStrategy);
    return VideoStudio.getInstance().startVideoRecord(outputPath,
        videoWidth, videoHeight, VIDEO_FRAME_RATE, COMMON_VIDEO_BIT_RATE,
        audioSampleRate, audioChannels, audioBitRate,
        qualityStrategy, adaptiveBitrateWindowSizeInSecs, adaptiveBitrateEncoderReconfigInterval,
        adaptiveBitrateWarCntThreshold, adaptiveMinimumBitrate, adaptiveMaximumBitrate);
  }

  private int ifQualityStrayegyEnable(int qualityStrategy) {
    qualityStrategy = (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) ? 0 : qualityStrategy;
    return qualityStrategy;
  }

  @Override
  protected boolean startProducer(final int videoWidth, int videoHeight, boolean useHardWareEncoding, int strategy) throws StartRecordingException {
    if (playerService != null) {
      playerService.start();
    }
    if (recorderService != null) {
      return recorderService.start(videoWidth, videoHeight, VideoRecordingStudio.getInitializeVideoBitRate(), VIDEO_FRAME_RATE, useHardWareEncoding, strategy);
    }

    return false;
  }

  @Override
  public void destroyRecordingResource() {
    // 销毁伴奏播放器
    if (playerService != null) {
      playerService.stop();
      playerService = null;
    }
    super.destroyRecordingResource();
  }

  /**
   * 播放一个新的伴奏
   **/
  public void startAccompany(String musicPath) {
    if (null != playerService) {
      playerService.startAccompany(musicPath);
    }
  }

  /**
   * 停止播放
   **/
  public void stopAccompany() {
    if (null != playerService) {
      playerService.stopAccompany();
    }
  }

  /**
   * 暂停播放
   **/
  public void pauseAccompany() {
    if (null != playerService) {
      playerService.pauseAccompany();
    }
  }

  /**
   * 继续播放
   **/
  public void resumeAccompany() {
    if (null != playerService) {
      playerService.resumeAccompany();
    }
  }

  /**
   * 设置伴奏的音量大小
   **/
  public void setAccompanyVolume(float volume, float accompanyMax) {
    if (null != playerService) {
      playerService.setVolume(volume, accompanyMax);
    }
  }

}
