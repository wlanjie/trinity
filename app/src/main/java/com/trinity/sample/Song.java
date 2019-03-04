package com.trinity.sample;

import java.io.File;
import java.io.Serializable;


public class Song implements Serializable {
	public int getId() {
		return id;
	}

	public void setId(int id) {
		this.id = id;
	}

	/*
	 * 1. 修改了此文件，必须更新SongSqliteHelper DATABASE_VERSION ，否则会出现诡异的异常，后果很严重
	 * 
	 * 2. 目前歌曲列表是song.db直接下载到app中。该song的数据结构只能增加字段，不能删除，否则后果很严重，高洁会生气。 data
	 * 2012-08-07
	 */
	private static final long serialVersionUID = 1L;
	int id;
	private int songId;
	private String name;
	private String artist;
	private String mp3;// 原唱文件
	private String music = "";// 伴奏文件
	private String zbd;
	private String zrc;
	private String singcount;
	private String pinyin;

	private String tag;
	private int listenedNum;
	private float size; // 文件大小,单位M，保留小数点一位
	private float hotvalue; // 热度值
	private int scorecount; // 评分的用户数

	// 上传人信息
	private String uploader;
	private String uploader_name; // 上传人姓名
	private String uploader_photo; // 上传人头像

	private long finishTime;

	private boolean isHD;
	private boolean hasHD;

	public boolean isHD() {
		return isHD;
	}

	public void setHD(boolean isHD) {
		this.isHD = isHD;
	}

	public boolean hasHD() {
		return hasHD;
	}

	public void setHasHD(boolean hasHD) {
		this.hasHD = hasHD;
	}

	public float getSize() {
		return size;
	}

	public void setSize(float size) {
		this.size = size;
	}

	public float getHotvalue() {
		return hotvalue;
	}

	public int getScorecount() {
		return scorecount;
	}

	public String getUploader_name() {
		return uploader_name;
	}

	public String getUploader_photo() {
		return uploader_photo;
	}

	public int getSongId() {
		return songId;
	}

	public String getName() {
		return name;
	}

	public String getArtist() {
		return artist;
	}

	// 原唱
	public String getMp3() {
		return mp3;
	}

	// 伴奏
	public String getMusic() {
		return music;
	}

	public String getZbd() {
		return zbd;
	}

	public String getPinyin() {
		return pinyin;
	}

	public int getListenedNum() {
		return listenedNum;
	}

	public String getUploader() {
		return uploader;
	}

	public void setSongId(int songId) {
		this.songId = songId;
	}

	public void setName(String name) {
		this.name = name;
	}

	public void setArtist(String artist) {
		this.artist = artist;
	}

	public void setMp3(String mp3) {
		this.mp3 = mp3;
	}

	public void setMusic(String music) {
		this.music = music;
	}

	public void setZbd(String zbd) {
		this.zbd = zbd;
	}

	public void setZrc(String zrc) {
		this.zrc = zrc;
	}

	public void setPinyin(String pinyin) {
		this.pinyin = pinyin;
	}

	public String getSingcount() {
		return singcount;
	}

	public void setSingcount(String singcount) {
		this.singcount = singcount;
	}

	public void setUploader(String uploader) {
		this.uploader = uploader;
	}

	public String getTag() {
		return tag;
	}

	public void setTag(String tag) {
		this.tag = tag;
	}

	public long getFinishTime() {
		return finishTime;
	}

	public void setFinishTime(long finishTime) {
		this.finishTime = finishTime;
	}

	/***************** custom function ****************************/
	public String getFileSize() {
		return size + "M";
	}

	public String getZrc() {
		if (null != zrc) {
			if (zrc.endsWith("zrce")) {
				return zrc;
			}
			return zrc + "e";
		}
		return zrc;
	}


	public boolean isZrcExist() {
		return !StringUtil.isEmpty(getZrc()) && getZrc().endsWith(".zrce");
	}

	public boolean isZbdExist() {
		return !StringUtil.isEmpty(getZbd()) && getZbd().endsWith(".zbd");
	}

	public boolean isMusicExist() {
		return !StringUtil.isEmpty(getMusic()) && getMusic().endsWith(".mp3");
	}

	public boolean isStandardSong() {
		return "0".equals(uploader);
	}

	public String getKey() {
		return songId > 0 ? songId + "" : "_" + name;
	}

	/***************** custom params ****************************/
	private boolean isSelected;
	private int progress;
	private int type;
	private String localSongPath;

	/** 演唱本地音乐路径 **/
	public void setLocalSongPath(String localSongPath) {
		this.localSongPath = localSongPath;
	}

	public String getLocalSongPath() {
		return localSongPath;
	}

	public int getType() {
		return type;
	}

	public void setType(int type) {
		this.type = type;
	}

	public int getProgress() {
		return progress;
	}

	public void setProgress(int progress) {
		this.progress = progress;
	}

	public boolean isSelected() {
		return isSelected;
	}

	public void setSelected(boolean isSelected) {
		this.isSelected = isSelected;
	}

	public static Song getInstance(int type, String name) {
		Song song = new Song();
		song.setType(type);
		song.setName(name);
		return song;
	}

	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + songId;
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		Song other = (Song) obj;
		return songId == other.songId;
	}
	
	public String getMelpPath() {
		File dirpath = FilePathUtil.getKTVConfigFileDir();
		String result = dirpath.getAbsolutePath() + "/" + getKey() + ".melp";
		File melFile = new File(result);
		if(melFile.exists()){
			return melFile.getAbsolutePath();
		} else{
			return "";
		}
	}
	
	public String getSongFilePath(String type)
	{
		File dirpath = FilePathUtil.getKTVConfigFileDir();
		String ext = type;
		if(ext.equals("music")) ext = "mp3";
		return dirpath.getAbsolutePath() + "/" + getKey() + "." + ext;
	}
	public String getOriginalSongFilePath(String type)
	{
		File dirpath = FilePathUtil.getKTVConfigFileDir();
		String ext = type;
		if(ext.equals("music")) ext = "mp3";
		return dirpath.getAbsolutePath() + "/" + getKey() + "_1." + ext;
	}
}
