
package com.trinity.editor

import android.view.SurfaceView
import com.trinity.listener.OnRenderListener

interface TrinityVideoEditor {

  /**
   * Set preview view
   */
  fun setSurfaceView(surfaceView: SurfaceView)

  /**
   * Get video duration
   */
  fun getVideoDuration(): Long

  fun getCurrentPosition(): Long

  /**
   * Get the number of video clips
   */
  fun getClipsCount(): Int

  /**
   * Get video clip
   * @param index Subscript of current fragment
   * @return Return null when the subscript is invalid
   */
  fun getClip(index: Int): MediaClip?

  /**
   * Broadcast a clip to the queue
   * @param clip Inserted clip object
   * @return Insert successful return 0
   */
  fun insertClip(clip: MediaClip): Int

  /**
   * Broadcast a clip to the queue
   * @param index Broadcast subscript
   * @param clip Inserted clip object
   * @return Insert successful return 0
   */
  fun insertClip(index: Int, clip: MediaClip): Int

  /**
   * Delete a fragment
   * @param index Delete the subscript of the fragment
   */
  fun removeClip(index: Int)

  /**
   * Replace a fragment
   * @param index Subscript to be replaced
   * @param clip The object whose fragment needs to be replaced
   * @return Return 0 if the replacement is successful
   */
  fun replaceClip(index: Int, clip: MediaClip): Int

  /**
   * Get all fragments
   * @return Return all current clips
   */
  fun getVideoClips(): List<MediaClip>

  /**
   * Get the time period of the current clip
   * @param index Obtained subscript
   * @return Returns the time period of the current clip
   */
  fun getClipTimeRange(index: Int): TimeRange

  fun getVideoTime(index: Int, clipTime: Long): Long

  fun getClipTime(index: Int, videoTime: Long): Long

  /**
   * Get fragment subscripts based on time
   * @param time Incoming time
   * @return Coordinates found, Return if not found-1
   */
  fun getClipIndex(time: Long): Int

  /**
   * Add filter
   * Such as: the absolute path of content.json is /sdcard/Android/com.trinity.sample/cache/filters/config.json
   * The incoming path only needs /sdcard/Android/com.trinity.sample/cache/filters Can
   * If the current path does not contain config.json, the addition fails
   * The specific json content is as follows:
   * {
   *  "type": 0,
   *  "intensity": 1.0,
   *  "lut": "lut_124/lut_124.png"
   * }
   *
   * json Parameter explanation:
   * type: reserved text, Currently no effect
   * intensity: Filter transparency, No difference between 0.0 and camera capture
   * lut: The address of the filter color table, Must be a local address, And relative path
   *      Path splicing will be performed inside the SDK
   * @param configPath The parent directory of the filter config.json
   * @return Returns the unique id of the current filter
   */
  fun addFilter(configPath: String): Int

  /**
   * Update filter
   * @param configPath The path of config.json, currently symmetric addFilter description
   * @param startTime Start time of the filter
   * @param endTime The end time of the filter
   * @param actionId Which filter needs to be updated, Must be the actionId returned by addFilter
   */
  fun updateFilter(configPath: String, startTime: Int, endTime: Int, actionId: Int)

  /**
   * Remove filter
   * @param actionId Which filter needs to be removed, Must be the actionId returned when addFilter
   */
  fun deleteFilter(actionId: Int)

  /**
   * Add background music
   * * The specific json content is as follows:
   * {
   *    "path": "/sdcard/trinity.mp3",
   *    "startTime": 0,
   *    "endTime": 2000
   * }
   * json Parameter explanation:
   * path: Music local address
   * startTime: Start time of this music
   * endTime: End time of this music 2000 means this music will only play for 2 seconds
   * @param config Background music json content
   */
  fun addMusic(config: String): Int

  /**
   * Update background music
   * * The specific json content is as follows:
   * {
   *    "path": "/sdcard/trinity.mp3",
   *    "startTime": 2000,
   *    "endTime": 4000
   * }
   * json Parameter explanation:
   * path: Music local address
   * startTime: Start time of this music
   * endTime: The end time of this music 4000 means that the music is played for 2 seconds from the start time to the end time
   * @param config Background music json content
   */
  fun updateMusic(config: String, actionId: Int)

  /**
   * Remove background music
   * @param actionId Must be the actionId returned when adding background music
   */
  fun deleteMusic(actionId: Int)

  /**
   * Add special effects
   * Such as: content.json The absolute path is/sdcard/Android/com.trinity.sample/cache/effects/config.json
   * The incoming path only needs /sdcard/Android/com.trinity.sample/cache/effects Can
   * If the current path does not contain config.json Add failed
   * @param configPath Filter config.json Parent directory
   * @return Returns the unique id of the current effect
   */
  fun addAction(configPath: String): Int

  /**
   * Update specific effects
   * @param startTime The start time of the special effect
   * @param endTime The end time of the special effect
   * @param actionId Which special effect needs to be updated,Must be the actionId returned by addAction
   */
  fun updateAction(startTime: Int, endTime: Int, actionId: Int)

  /**
   * Delete a special effect
   * @param actionId Which special effect needs to be deleted, Must be the actionId returned by addAction
   */
  fun deleteAction(actionId: Int)

  /**
   * Set background color 255 255 255 0 白色
   * @param clipIndex Which segment needs to be updated
   * @param red Int red
   * @param green Int green
   * @param blue Int blue
   * @param alpha Int Transparent color
   */
  fun setBackgroundColor(clipIndex: Int, red: Int, green: Int, blue: Int, alpha: Int)

  /**
   * Set video background image
   * @param clipIndex Which segment needs to be updated
   * @param path String The map's address
   * @return 0 success other failed
   */
  fun setBackgroundImage(clipIndex: Int, path: String): Int

  /**
   * Set display size
   * @param width Int Need to display the width
   * @param height Int Need to display the height
   */
  fun setFrameSize(width: Int, height: Int)

  fun setOnRenderListener(l: OnRenderListener)

  /**
   * seek operating
   * @param time Set the playing time, In milliseconds
   */
  fun seek(time: Int)

  /**
   * Start playing
   * @param repeat Whether to loop
   * @return Return 0 if the playback is successful
   */
  fun play(repeat: Boolean): Int

  /**
   * Pause playback
   */
  fun pause()

  /**
   * Keep playing
   */
  fun resume()

  /**
   * Stop play, Release resources
   */
  fun stop()

  /**
   * Release edited resources
   */
  fun destroy()
}