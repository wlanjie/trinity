# -*- coding: utf-8 -*- 
# by Appetizer v1.4.9
import sys

if sys.version_info < (3,0):
    print('Python2 is no longer supported since uiautomator2 0.2.3, upgrade to Python3 or use older version')
import uiautomator2 as u2
import time
import os
from datetime import datetime
import unittest

device = input('输入设备名: ')
print(device)

d = u2.connect_usb(device)
xp = d.ext_xpath

class TrinityTestCase(unittest.TestCase):
    def setUp(self):
        self.package_name = "com.trinity.sample"
        d.screen_on()
        # 点击home
        d.press("home")
        # 停止当前应用
        d.app_stop(self.package_name)
        # 清空数据
        # d.app_clear(self.package_name)
        # 启动App
        d.app_start(self.package_name)
        # 打印信息
        info_json = d.info
        print(info_json)
        # 获取设备分辨率
        window_size = d.window_size()
        self.displayWidth = window_size[0]
        self.displayHeight = window_size[1]
        # 权限同意
        for index in range(0, 3):
            exists = d(resourceId="com.android.packageinstaller:id/permission_allow_button", className="android.widget.Button").exists()
            print(exists)
            if (exists):
                d(resourceId="com.android.packageinstaller:id/permission_allow_button", className="android.widget.Button").click()


    def runTest(self):
        for index in range(0, 100):
            self.runApp()

    def runApp(self):
        # 录制20段视频,每段2秒
        for num in range(0, 20): 
            # 长按录制按钮2秒
            d(resourceId="com.trinity.sample:id/record_button", className="android.view.View").long_click(2)
            # 切换摄像头
            d(resourceId="com.trinity.sample:id/switch_camera", className="android.widget.ImageView").click()

        # 点击完成,跳转到编辑页
        d(resourceId="com.trinity.sample:id/done", className="android.widget.ImageView").click()

        # 点击滤镜
        # xp.when("滤镜").click()
        d(text="滤镜").click()
        # 遍历滤镜,所有滤镜切换一次
        for num in range(1, 8):
            for index in range(1, 8):
                try:
                    d.xpath('//*[@resource-id="com.trinity.sample:id/effect_list_filter"]/android.widget.RelativeLayout[' + str(index) +']').click()
                    # 每个滤镜显示2秒
                    time.sleep(2)
                except Exception:
                    print('click filter exception')
                    self.runTest()
            # 往左滑动
            d.swipe_ext("left", box=(0, self.displayHeight - 200, self.displayWidth, self.displayHeight), scale=0.7)

        # 点击隐藏滤镜列表
        d(resourceId="com.trinity.sample:id/root_view", className="android.widget.RelativeLayout").click()

        # 点击下一步
        d(text="下一步").click()
        time.sleep(40)
        d.press("back")
        d.press("back")
        for index in range(0, 40):
            d(resourceId="com.trinity.sample:id/delete", className="android.widget.ImageView").click()

    def tearDown(self):
        d.set_fastinput_ime(False)
        d.app_stop(self.package_name)
        # d.screen_off()

if __name__ == "__main__":
    unittest.main()