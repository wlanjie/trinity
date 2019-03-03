package com.trinity.decoder;

public class MusicDecoder implements Mp3Decoder {

    private long mId;

    public MusicDecoder() {
        mId = create();
    }

    private native long create();

    /**
     * 主播开始播放 初始化播放器
     **/
    @Override
    public void init(float packetBufferTimePercent, int vocalSampleRate) {
        if (mId <= 0) {
            return;
        }
        initDecoder(mId, packetBufferTimePercent, vocalSampleRate);
    }

    private native void initDecoder(long id, float packetBufferTimePercent, int vocalSampleRate);

    /**
     * 读取要播放的数据
     **/
    @Override
    public int readSamples(short[] samples, int[] slientSizeArr, int[] extraSampleType) {
        return readSamples(mId, samples, samples.length, slientSizeArr, extraSampleType);
    }

    private native int readSamples(long id, short[] samples, int size, int[] slientSizeArr, int[] extraSampleType);

    /**
     * 由于允许在唱歌的时候可以调节伴奏音量并且要实时听到效果  这里就是设置音量的接口
     **/
    @Override
    public void setAccompanyVolume(float volume, float accompanyMax) {
        setVolume(mId, volume, accompanyMax);
    }

    private native void setVolume(long id, float volume, float accompanyMax);

    /**
     * 开启一个新的伴奏
     **/
    @Override
    public void startAccompany(String path) {
        start(mId, path);
    }

    private native void start(long id, String path);

    /**
     * 暂停当前播放的伴奏
     **/
    @Override
    public void pauseAccompany() {
        pause(mId);
    }

    private native void pause(long id);

    /**
     * 继续播放当前的伴奏
     **/
    @Override
    public void resumeAccompany() {
        resume(mId);
    }

    private native void resume(long id);

    /**
     * 结束当前播放的伴奏
     **/
    @Override
    public void stopAccompany() {
        stop(mId);
    }

    private native void stop(long id);

    /**
     * 主播结束直播 关掉播放器
     **/
    @Override
    public void destory() {
        destroyDecoder(mId);
        release();
        mId = 0;
    }

    private native void destroyDecoder(long id);

    private native void release();
}
