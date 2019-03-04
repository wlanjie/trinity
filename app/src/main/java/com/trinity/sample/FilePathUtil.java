package com.trinity.sample;

import android.os.Environment;

import java.io.File;

public class FilePathUtil { 
	
	public static int sPathIndex = 0;
	
	public static String getVideoRecordingFilePath() {
		return getKTVConfigFileDir() + "/recording.flv";
	}
	
	public static String getVideoPlayFilePath() {
		return "/mnt/sdcard/a_songstudio/huahua.mp4";
	}

	public static File getKTVConfigFileDir() {
		File sdDir = getSDPath();
		if (sdDir != null && sdDir.isDirectory()) {
			File ktv = new File(sdDir, "a_songstudio");
			if (!ktv.exists())
				ktv.mkdirs();
			return ktv;
		}
		return null;
	}

	public static File getSDPath() {
		File sdDir = null;
		boolean sdCardExist = Environment.getExternalStorageState().equals(
				Environment.MEDIA_MOUNTED); // 判断sd卡是否存在
		if (sdCardExist) {
			sdDir = Environment.getExternalStorageDirectory();// 获取跟目录
		}
		return sdDir;
	}
	
}
