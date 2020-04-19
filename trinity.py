# -*- coding: utf-8 -*- 
# by Appetizer v1.4.9
import sys

if sys.version_info < (3,0):
    print('Python2 is no longer supported since uiautomator2 0.2.3, upgrade to Python3 or use older version')
import uiautomator2 as u2
import time
import os
import random
from datetime import datetime
import unittest

device = input('输入设备名: ')
print(device)

d = u2.connect_usb(device)
xp = d.ext_xpath

effects = ["闪白", "两屏", "三屏", "四屏", "六屏", "九屏", "黑白分屏", "模糊分屏", "灵魂出窍", "抖动", "毛刺", "缩放", "70s", "x-signal"]
effect_tab_names = ["梦幻", "动感", "分屏", "转场"]

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
            # d.app_start(self.package_name)
            self.runApp()
            # d.app_stop(self.package_name)

    def clickPhoto(self):
        # 选择相机中的视频, 并点击完成
        d(resourceId="com.trinity.sample:id/photo", className="android.widget.ImageView").click()
        time.sleep(2)
        try:
            elements = d.xpath("//android.widget.FrameLayout//androidx.recyclerview.widget.RecyclerView//android.widget.FrameLayout//android.widget.TextView").all()
            random_value = random.randint(0, len(elements) - 1)
            elements[random_value].click()
            elements = d.xpath("//android.widget.RelativeLayout//android.widget.ImageView").all()
            for item in elements:
                attrib = item.attrib
                if 'done' in attrib['resource-id']:
                    item.click()
        except Exception:
            print('clickPhoto error: ' + str(e))
            self.runTest()             

    def clickRecordEffect(self):
        try:
            # 选择录制中的特效
            d(resourceId="com.trinity.sample:id/effect", className="android.widget.ImageView").click()
            time.sleep(2)
            effect_elements = d.xpath("//android.view.ViewGroup//androidx.recyclerview.widget.RecyclerView//android.widget.LinearLayout").all()
            effect_index = random.randint(0, len(effect_elements) - 1)
            print(effect_elements)
            print(effect_index)
            time.sleep(1)
            effect_elements[effect_index].click()
            time.sleep(2)
            d.swipe_ext("down", box=(200, 0, self.displayWidth, self.displayHeight), scale=0.7)
        except Exception:
            d.swipe_ext("down", box=(200, 0, self.displayWidth, self.displayHeight), scale=0.7)
            print('click record effect error')
            # self.runTest()    

    def runApp(self):
        # 录制5段视频,每段5秒
        for num in range(0, 5): 
            # 随机去选一次相册中的视频
            album_random_value = random.randint(0, 5)
            if (album_random_value == num):
                self.clickPhoto()
                continue
            # self.clickRecordEffect()
            # 长按录制按钮5秒
            d(resourceId="com.trinity.sample:id/record_button", className="android.view.View").long_click(5)
            # 切换摄像头
            # d(resourceId="com.trinity.sample:id/switch_camera", className="android.widget.ImageView").click()

        # 点击完成,跳转到编辑页
        d(resourceId="com.trinity.sample:id/done", className="android.widget.ImageView").click()

        # 点击滤镜
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

        # # 点击隐藏滤镜列表
        d(resourceId="com.trinity.sample:id/root_view", className="android.widget.RelativeLayout").click()

        # 点击特效
        d(text="特效").click()
        for effect_tab in effect_tab_names:
            try:
                d.xpath(effect_tab).click()
                time.sleep(2)
                effect_elements = d.xpath("//androidx.recyclerview.widget.RecyclerView//android.widget.LinearLayout//android.widget.FrameLayout//android.widget.TextView").all()
                for effect in effect_elements:
                    attrib = effect.attrib
                    print(attrib['text'])
                    d(text=attrib['text']).long_click(2)
                    # 往左滑动
                    d.swipe_ext("left", box=(0, self.displayHeight - 500, self.displayWidth, self.displayHeight), scale=0.1) 
            except Exception:
                self.runTest()

        # 遍历特效, 所有特效执行一次
        # for effect_name in effects:
        #     try:
        #         # 每个特效显示2秒
        #         d(text=effect_name).long_click(2)
        #     except Exception:
        #         print('click effect exception')
        #         self.runTest()
        #     # 往左滑动
        #     d.swipe_ext("left", box=(0, self.displayHeight - 200, 260, self.displayHeight), scale=0.7)                        

        time.sleep(40)
         # 点击隐藏特效列表
        d(resourceId="com.trinity.sample:id/root_view", className="android.widget.RelativeLayout").click()

        # 点击下一步
        d(text="下一步").click()
        time.sleep(60)
        d.press("back")
        d.press("back")
        os.system('adb -s ' + device + " shell rm /sdcard/Android/data/com.trinity.sample/cache/*.mp4")
        # d.press("back")

        for index in range(0, 40):
            try:
                d(resourceId="com.trinity.sample:id/delete", className="android.widget.ImageView").click()
            except Exception:
                print('delete exception')

        current_time = time.strftime("%Y-%m-%d-%H-%M-%S", time.localtime())
        os.system('adb -s ' + device + ' pull /sdcard/export.mp4 ~/Desktop/trinity_video/' + current_time + ".mp4")
        os.system('adb -s ' + device + ' pull /sdcard/Android/data/com.trinity.sample/cache/resource.json ~/Desktop/trinity_video/' + current_time + ".json")
    def tearDown(self):
        d.set_fastinput_ime(False)
        d.app_stop(self.package_name)
        # d.screen_off()

if __name__ == "__main__":
    unittest.main()