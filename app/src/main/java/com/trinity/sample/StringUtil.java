package com.trinity.sample;

import java.util.regex.Pattern;

public class StringUtil {

	public static boolean containsEnglishCharacter(String content) {
		byte[] buf = content.getBytes();
        for (byte aBuf : buf) {
            if ((aBuf & 0x80) == 0) {
                return true;
            }
        }
		return false;
	}
	
	public static boolean containsChineseCharacter(String content){
		for(int i = 0;i<content.length();i++){
			String temStr = content.substring(i, i+1);
			if(Pattern.matches("[\u4E00-\u9FA5]", temStr)){
				return true;
			}
		}
		return false;
	}
	
	public static boolean isEmpty(String str){
        return str == null || str.length() == 0 || str.trim().length() == 0;
    }
}
