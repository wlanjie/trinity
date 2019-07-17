//
// Created by wlanjie on 2019-06-29.
//

#include "video_player.h"
#include "android_xlog.h"

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* Minimum audio buffer size, in samples. */
#define AUDIO_MIN_BUFFER_SIZE 512

static double rdftspeed = 0.02;

namespace trinity {

static int AudioCallback(uint8_t* buffer, size_t buffer_size, void* context) {
    VideoPlayer* player = (VideoPlayer*) context;
    return player->ReadAudio(buffer, buffer_size);
}

VideoPlayer::VideoPlayer() {
    video_event_ = nullptr;
    media_decode_ = nullptr;
    player_state_ = nullptr;
    audio_render_ = nullptr;
    player_handler_ = nullptr;
    message_queue_ = nullptr;
    handler_ = nullptr;
    message_queue_ = nullptr;
    video_play_state_ = kNone;
    core_ = nullptr;
    render_surface_ = EGL_NO_SURFACE;
    render_screen_ = nullptr;
    surface_width_ = 0;
    surface_height_ = 0;
    frame_width_ = 0;
    frame_height_ = 0;
    yuv_render_ = nullptr;
    window_ = nullptr;
    egl_context_exists_ = false;
    egl_destroy_ = false;
    destroy_ = false;

    audio_render_ = new AudioRender();
    audio_render_->Init(1, 44100, AudioCallback, this);
    audio_render_->Start();
    message_queue_ = new MessageQueue("Video Render Message Queue");
    handler_ = new VideoRenderHandler(this, message_queue_);
    vertex_coordinate_ = new GLfloat[8];
    texture_coordinate_ = new GLfloat[8];

    vertex_coordinate_[0] = -1.0f;
    vertex_coordinate_[1] = -1.0f;
    vertex_coordinate_[2] = 1.0f;
    vertex_coordinate_[3] = -1.0f;

    vertex_coordinate_[4] = -1.0f;
    vertex_coordinate_[5] = 1.0f;
    vertex_coordinate_[6] = 1.0f;
    vertex_coordinate_[7] = 1.0f;

    texture_coordinate_[0] = 0.0f;
    texture_coordinate_[1] = 1.0f;
    texture_coordinate_[2] = 1.0f;
    texture_coordinate_[3] = 1.0f;

    texture_coordinate_[4] = 0.0f;
    texture_coordinate_[5] = 0.0f;
    texture_coordinate_[6] = 1.0f;
    texture_coordinate_[7] = 0.0f;
}

VideoPlayer::~VideoPlayer() {
    audio_render_->Stop();
    delete audio_render_;
    audio_render_ = nullptr;
}

void SetClockAt(Clock* c, double pts, int serial, double time) {
    c->pts = pts;
    c->last_update = time;
    c->pts_drift = c->pts - time;
    c->serial = serial;
}

void SetClock(Clock* c, double pts, int serial) {
    double time = av_gettime_relative() / 1000000.0;
    SetClockAt(c, pts, serial, time);
}

double GetClock(Clock *c) {
    if (*c->queue_serial != c->serial) {
        return NAN;
    }
    if (c->paused) {
        return c->pts;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        return c->pts_drift + time - (time - c->last_update) * (1.0 - c->speed);
    }
}

void SetClockSpeed(Clock* c, double speed) {
    SetClock(c, GetClock(c), c->serial);
    c->speed = speed;
}

void SyncClockToSlave(Clock* c, Clock* slave) {
    double clock = GetClock(c);
    double slave_clock = GetClock(slave);
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD)) {
        SetClock(c, slave_clock, slave->serial);
    }
}

void InitClock(Clock* clock, int* serial) {
    clock->speed = 1.0;
    clock->paused = 0;
    clock->queue_serial = serial;
    SetClock(clock, NAN, -1);
}

int GetMasterSyncType(MediaDecode *media_decode, PlayerState* player_state) {
    if (player_state->av_sync_type == AV_SYNC_VIDEO_MASTER) {
        if (media_decode->video_stream) {
            return AV_SYNC_VIDEO_MASTER;
        } else {
            return AV_SYNC_AUDIO_MASTER;
        }
    } else if (player_state->av_sync_type == AV_SYNC_AUDIO_MASTER) {
        if (media_decode->audio_stream) {
            return AV_SYNC_AUDIO_MASTER;
        } else {
            return AV_SYNC_EXTERNAL_CLOCK;
        }
    } else {
        return AV_SYNC_EXTERNAL_CLOCK;
    }
}

double GetMasterClock(MediaDecode *media_decode, PlayerState* player_state) {
    double val;
    switch (GetMasterSyncType(media_decode, player_state)) {
        case AV_SYNC_VIDEO_MASTER:
            val = GetClock(&player_state->video_clock);
            break;

        case AV_SYNC_AUDIO_MASTER:
            val = GetClock(&player_state->sample_clock);
            break;
        default:
            val = GetClock(&player_state->external_clock);
            break;
    }
    return val;
}

void CheckExternalClockSpeed(MediaDecode *media_decode, PlayerState* player_state) {
    if ((media_decode->audio_stream_index >= 0 && media_decode->audio_packet_queue.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES) ||
        (media_decode->video_stream_index >= 0 && media_decode->video_packet_queue.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES)) {
        SetClockSpeed(&player_state->external_clock, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, player_state->external_clock.speed - EXTERNAL_CLOCK_SPEED_STEP));
    } else if ((media_decode->video_stream_index < 0 || media_decode->video_packet_queue.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
               (media_decode->audio_stream_index < 0 || media_decode->audio_packet_queue.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES)) {
        SetClockSpeed(&player_state->external_clock, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, player_state->external_clock.speed + EXTERNAL_CLOCK_SPEED_STEP));
    } else {
        double speed = player_state->external_clock.speed;
        if (speed != 1.0) {
            SetClockSpeed(&player_state->external_clock, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
        }
    }
}

void VideoDisplay(MediaDecode* media_decode, VideoEvent* video_event) {
    if (video_event && media_decode->video_frame_queue.size > 0) {
        video_event->render_video_frame(video_event);
    }
}

double FrameDuration(MediaDecode *media_decode, Frame *vp, Frame *nextvp) {
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > media_decode->max_frame_duration) {
            return vp->duration;
        } else {
            return duration;
        }
    } else {
        return 0.0;
    }
}

double ComputeTargetDelay(double delay, MediaDecode *media_decode, PlayerState* player_state) {
    double sync_threshold, diff = 0;
    /* update delay to follow master synchronisation source */
    if (GetMasterSyncType(media_decode, player_state) != AV_SYNC_VIDEO_MASTER) {
        /* if video is slave, we try to correct big delays by
         duplicating or deleting a frame */
        diff = GetClock(&player_state->video_clock) - GetMasterClock(media_decode, player_state);

        /* skip or repeat frame. We take into account the
         delay to compute the threshold. I still don't know
         if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!isnan(diff) && fabs(diff) < media_decode->max_frame_duration) {
            if (diff <= -sync_threshold)
                delay = FFMAX(0, delay + diff);
            else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
                delay = delay + diff;
            else if (diff >= sync_threshold)
                delay = 2 * delay;
        }
    }
    av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n", delay, -diff);
    return delay;
}

void UpdateVideoPts(PlayerState* player_state, double pts, int64_t pos, int serial) {
    /* update current video pts */
    SetClock(&player_state->video_clock, pts, serial);
    SyncClockToSlave(&player_state->external_clock, &player_state->video_clock);
}

void Retry(MediaDecode* media_decode,
        PlayerState* player_state,
        VideoEvent* video_event,
        double* remaining_time, double* time) {
    if (frame_queue_nb_remaining(&media_decode->video_frame_queue) == 0) {
        // nothing to do, no picture to display in the queue
    } else {
        Frame* last = frame_queue_peek_last(&media_decode->video_frame_queue);
        Frame* frame = frame_queue_peek(&media_decode->video_frame_queue);
        if (frame->serial != media_decode->video_packet_queue.serial) {
            frame_queue_next(&media_decode->video_frame_queue);
            Retry(media_decode, player_state, video_event, remaining_time, time);
            return;
        }
        if (last->serial != frame->serial) {
            player_state->frame_timer = av_gettime_relative() / 1000000.0;
        }
        if (media_decode->paused) {
            if (player_state->force_refresh && media_decode->video_frame_queue.rindex_shown) {
                VideoDisplay(media_decode, video_event);
            }
            return;
        }
        double last_duration = FrameDuration(media_decode, last, frame);
        double delay = ComputeTargetDelay(last_duration, media_decode, player_state);
        *time = av_gettime_relative() / 1000000.0;
        if (*time < player_state->frame_timer + delay) {
            *remaining_time = FFMIN(player_state->frame_timer + delay - *time, *remaining_time);
            if (player_state->force_refresh && media_decode->video_frame_queue.rindex_shown) {
                VideoDisplay(media_decode, video_event);
            }
            return;
        }
        player_state->frame_timer += delay;
        if (delay > 0 && *time - player_state->frame_timer > AV_SYNC_THRESHOLD_MAX) {
            player_state->frame_timer = *time;
        }
        pthread_mutex_lock(&media_decode->video_frame_queue.mutex);
        if (!isnan(frame->pts)) {
            UpdateVideoPts(player_state, frame->pts, frame->pos, frame->serial);
        }
        pthread_mutex_unlock(&media_decode->video_frame_queue.mutex);
        frame_queue_next(&media_decode->video_frame_queue);
        player_state->force_refresh = 1;

        if (player_state->step && !media_decode->paused) {
//            StreamTogglePause(media_decode, player_state);
        }
    }

    if (player_state->force_refresh && media_decode->video_frame_queue.rindex_shown) {
        VideoDisplay(media_decode, video_event);
    }
    player_state->force_refresh = 0;
}

void VideoRefresh(MediaDecode* media_decode,
        PlayerState* player_state,
        VideoEvent* video_event,
        double *remaining_time) {
    if (!media_decode->paused && GetMasterSyncType(media_decode, player_state) == AV_SYNC_EXTERNAL_CLOCK) {
        CheckExternalClockSpeed(media_decode, player_state);
    }
    double time;
    if (media_decode->audio_stream) {
        time = av_gettime_relative() / 1000000.0;
        if (player_state->force_refresh || player_state->last_vis_time + rdftspeed < time) {
            VideoDisplay(media_decode, video_event);
            player_state->last_vis_time = time;
        }
        *remaining_time = FFMIN(*remaining_time, player_state->last_vis_time + rdftspeed - time);
    }

    if (media_decode->video_stream) {
        Retry(media_decode, player_state, video_event, remaining_time, &time);
    }
}

void VideoPlayer::StreamTogglePause(MediaDecode* media_decode, PlayerState* player_state) {
    if (media_decode->paused) {
        player_state->frame_timer += av_gettime_relative() / 1000000.0 - player_state->video_clock.last_update;
        if (media_decode->read_pause_return != AVERROR(ENOSYS)) {
            player_state->video_clock.paused = 0;
        }
        SetClock(&player_state->video_clock, GetClock(&player_state->video_clock), player_state->video_clock.serial);
    }
    SetClock(&player_state->external_clock, GetClock(&player_state->external_clock), player_state->external_clock.serial);
    pthread_mutex_lock(&sync_mutex_);
    media_decode->paused = player_state->sample_clock.paused = player_state->video_clock.paused = player_state->external_clock.paused = !media_decode->paused;
    pthread_cond_signal(&sync_cond_);
    pthread_mutex_unlock(&sync_mutex_);
}

void VideoPlayer::OnSeekEvent(SeekEvent* event, int seek_flag) {
    VideoPlayer* video_player = (VideoPlayer*) event->context;
    if (seek_flag & AVSEEK_FLAG_BYTE) {
        SetClock(&video_player->player_state_->external_clock, NAN, 0);
    } else {
        int64_t seek_target = video_player->media_decode_->seek_pos * (AV_TIME_BASE / 1000);
        SetClock(&video_player->player_state_->external_clock, seek_target / (double) AV_TIME_BASE, 0);
    }
    if (video_player->media_decode_->paused) {
        video_player->StreamTogglePause(video_player->media_decode_, video_player->player_state_);
        video_player->player_state_->step = 1;
    }
}

void VideoPlayer::OnAudioPrepareEvent(AudioEvent *event, int size) {
    VideoPlayer* player = (VideoPlayer*) event->context;
    PlayerState* player_state = player->player_state_;
    player_state->audio_hw_buf_size = size;
    player_state->audio_buf_size = 0;
    player_state->audio_buf_index = 0;
    /* init averaging filter */
    player_state->audio_diff_avg_coef = exp(log(0.1) / AUDIO_DIFF_AVG_NB);
    player_state->audio_diff_avg_count = 0;
    /* since we do not have a precmedia_decodee anough audio fifo fullness,
             we correct audio sync only if larger than thmedia_decode threshold */
    player_state->audio_diff_threshold = (double) (size / player->media_decode_->audio_tgt.bytes_per_sec);
}

int VideoPlayer::Init() {
    int result = pthread_create(&render_thread_, nullptr, RenderThread, this);
    if (result != 0) {
        LOGE("Init render thread error: %d", result);
        return false;
    }

    result = pthread_mutex_init(&render_mutex_, nullptr);
    if (result != 0) {
        LOGE("init render mutex error");
        return result;
    }
    result = pthread_cond_init(&render_cond_, nullptr);
    if (result != 0) {
        LOGE("init render cond error");
        return result;
    }
    return result;
}

int VideoPlayer::Start(const char *file_name, uint64_t start_time, uint64_t end_time, StateEvent* state_event) {
    video_event_ = (VideoEvent*) av_malloc(sizeof(VideoEvent));
    memset(video_event_, 0, sizeof(VideoEvent));
    video_event_->context = this;
    video_event_->render_video_frame = RenderVideoFrame;

    player_state_ = (PlayerState*) av_malloc(sizeof(PlayerState));
    memset(player_state_, 0, sizeof(PlayerState));
    player_state_->av_sync_type = AV_SYNC_AUDIO_MASTER;

    media_decode_ = (MediaDecode*) av_malloc(sizeof(MediaDecode));
    memset(media_decode_, 0, sizeof(MediaDecode));
    media_decode_->start_time = start_time;
    media_decode_->end_time = end_time;
    media_decode_->loop = 1;
    media_decode_->precision_seek = 1;

    SeekEvent* seek_event = (SeekEvent*) av_malloc(sizeof(SeekEvent));
    memset(seek_event, 0, sizeof(SeekEvent));
    seek_event->on_seek_event = OnSeekEvent;
    seek_event->context = this;
    media_decode_->seek_event = seek_event;

    AudioEvent* audio_event = (AudioEvent*) av_malloc(sizeof(AudioEvent));
    memset(audio_event, 0, sizeof(AudioEvent));
    audio_event->on_audio_prepare_event = OnAudioPrepareEvent;
    audio_event->context = this;
    media_decode_->audio_event = audio_event;

    media_decode_->state_event = state_event;
    int result = av_decode_start(media_decode_, file_name);
    if (result != 0) {
        return result;
    }
    InitClock(&player_state_->video_clock, &media_decode_->video_packet_queue.serial);
    InitClock(&player_state_->sample_clock, &media_decode_->audio_packet_queue.serial);
    InitClock(&player_state_->external_clock, &player_state_->external_clock.serial);

    pthread_mutex_init(&sync_mutex_, nullptr);
    pthread_cond_init(&sync_cond_, nullptr) ;
    pthread_create(&sync_thread_, nullptr, SyncThread, this);

    return 0;
}

void *VideoPlayer::SyncThread(void* arg) {
    VideoPlayer* video_player = (VideoPlayer*) arg;
    video_player->Sync();
    pthread_exit(0);
}

void VideoPlayer::Sync() {
    double remaining_time = 0.0;
    while (!media_decode_->abort_request) {
        pthread_mutex_lock(&sync_mutex_);
        if (media_decode_->paused) {
            LOGE("Sync paused");
            pthread_cond_wait(&sync_cond_, &sync_mutex_);
        }
        pthread_mutex_unlock(&sync_mutex_);
        if (remaining_time > 0.0) {
            av_usleep((int64_t)(remaining_time * 1000000.0));
        }
        remaining_time = REFRESH_RATE;
        if (!media_decode_->paused || player_state_->force_refresh) {
            VideoRefresh(media_decode_, player_state_, video_event_, &remaining_time);
        }
    }
}

void VideoPlayer::Resume() {
//    StreamTogglePause(media_decode_, player_state_);
}

void VideoPlayer::Pause() {
//    StreamTogglePause(media_decode_, player_state_);
}

void VideoPlayer::Stop() {
    // TODO nullpoint
    if (nullptr != player_state_->swr_context) {
        swr_free(&player_state_->swr_context);
        player_state_->swr_context = nullptr;
    }
    pthread_mutex_lock(&sync_mutex_);
    media_decode_->paused = 0;
    pthread_cond_signal(&sync_cond_);
    pthread_mutex_unlock(&sync_mutex_);

    av_decode_destroy(media_decode_);
    pthread_join(sync_thread_, nullptr);

    pthread_mutex_destroy(&sync_mutex_);
    pthread_cond_destroy(&sync_cond_);

    if (nullptr != player_state_) {
        av_free(player_state_);
        player_state_ = nullptr;
    }
    if (nullptr != media_decode_) {
        av_free(media_decode_);
        media_decode_ = nullptr;
    }
}

void VideoPlayer::Destroy() {
    LOGI("enter Destroy");
    destroy_ = true;
    handler_->PostMessage(new Message(kDestroyEGLContext));
    handler_->PostMessage(new Message(MESSAGE_QUEUE_LOOP_QUIT_FLAG));
    pthread_join(render_thread_, nullptr);
}

void VideoPlayer::StartPlayer() {

}

int SynchronizeAudio(MediaDecode* media_decode, PlayerState* player_state, int nb_samples) {
    int wanted_nb_samples = nb_samples;
    if (GetMasterSyncType(media_decode, player_state) != AV_SYNC_AUDIO_MASTER) {
        double diff = GetClock(&player_state->sample_clock) - GetMasterClock(media_decode, player_state);
        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
            player_state->audio_diff_cum = diff + player_state->audio_diff_avg_coef * player_state->audio_diff_cum;
            if (player_state->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
                player_state->audio_diff_avg_count++;
            } else {
                double avg_diff = player_state->audio_diff_cum * (1.0 - player_state->audio_diff_avg_coef);

                if (fabs(avg_diff) >= player_state->audio_diff_threshold) {
                    wanted_nb_samples = nb_samples + (int) (diff * media_decode->audio_src.freq);
                    int min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    int max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    wanted_nb_samples = av_clip_c(wanted_nb_samples, min_nb_samples, max_nb_samples);
                }
                av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",
                       diff, avg_diff, wanted_nb_samples - nb_samples,
                       player_state->audio_clock, player_state->audio_diff_threshold);
            }
        } else {
            player_state->audio_diff_avg_count = 0;
            player_state->audio_diff_cum = 0;
        }
    }
    return wanted_nb_samples;
}

int AudioResample(MediaDecode* media_decode, PlayerState* player_state) {
    Frame* frame;
    do {
        frame = frame_queue_peek_readable(&media_decode->sample_frame_queue);
        if (!frame) {
            LOGE("frame_queue_peek_readable error");
            return 0;
        }
        frame_queue_next(&media_decode->sample_frame_queue);
    } while (frame->serial != media_decode->audio_packet_queue.serial);

    int wanted_nb_sample;
    int data_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(frame->frame),
                                               frame->frame->nb_samples, (AVSampleFormat) frame->frame->format, 1);
    uint64_t dec_channel_layout = (frame->frame->channel_layout && av_frame_get_channels(frame->frame) == av_get_channel_layout_nb_channels(frame->frame->channel_layout)) ?
                                  frame->frame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(frame->frame));
    wanted_nb_sample = SynchronizeAudio(media_decode, player_state, frame->frame->nb_samples);
    if (frame->frame->format != media_decode->audio_src.fmt ||
        dec_channel_layout != media_decode->audio_src.channel_layout ||
        frame->frame->sample_rate != media_decode->audio_src.freq ||
        (wanted_nb_sample != frame->frame->nb_samples && !player_state->swr_context)) {
        swr_free(&player_state->swr_context);
        player_state->swr_context = swr_alloc_set_opts(NULL, media_decode->audio_tgt.channel_layout, media_decode->audio_tgt.fmt, media_decode->audio_tgt.freq,
                                                        dec_channel_layout, (AVSampleFormat) frame->frame->format, frame->frame->sample_rate, 0, NULL);
        if (!player_state->swr_context || swr_init(player_state->swr_context) < 0) {
            av_log(NULL, AV_LOG_ERROR,
                   "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                   frame->frame->sample_rate, av_get_sample_fmt_name((AVSampleFormat) frame->frame->format), av_frame_get_channels(frame->frame),
                   media_decode->audio_tgt.freq, av_get_sample_fmt_name(media_decode->audio_tgt.fmt), media_decode->audio_tgt.channels);
            swr_free(&player_state->swr_context);
            return -1;
        }
        media_decode->audio_src.channel_layout = dec_channel_layout;
        media_decode->audio_src.channels = av_frame_get_channels(frame->frame);
        media_decode->audio_src.freq = frame->frame->sample_rate;
        media_decode->audio_src.fmt = (AVSampleFormat) frame->frame->format;
    }

    int resample_data_size;
    if (player_state->swr_context) {
        const uint8_t **in = (const uint8_t **) frame->frame->extended_data;
        uint8_t **out = &player_state->audio_buf1;
        int out_count = wanted_nb_sample * media_decode->audio_tgt.freq / frame->frame->sample_rate + 256;
        int out_size = av_samples_get_buffer_size(NULL, media_decode->audio_tgt.channels, out_count, media_decode->audio_tgt.fmt, 0);
        int len2;
        if (out_size < 0) {
            av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
            return -1;
        }
        if (wanted_nb_sample != frame->frame->nb_samples) {
            if (swr_set_compensation(player_state->swr_context, (wanted_nb_sample - frame->frame->nb_samples) * media_decode->audio_tgt.freq / frame->frame->sample_rate,
                                     wanted_nb_sample * media_decode->audio_tgt.freq / frame->frame->sample_rate) < 0) {
                av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
                return -1;
            }
        }
        LOGE("audio_buf1: %d audio_buf1_size: %d out_size: %d", player_state->audio_buf1, player_state->audio_buf1_size, out_size);
        av_fast_malloc(&player_state->audio_buf1, &player_state->audio_buf1_size, out_size);
        if (!player_state->audio_buf1) {
            return AVERROR(ENOMEM);
        }
        len2 = swr_convert(player_state->swr_context, out, out_count, in, frame->frame->nb_samples);
        if (len2 < 0) {
            av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
            return -1;
        }
        if (len2 == out_count) {
            av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too samll.\n");
            if (swr_init(player_state->swr_context) < 0) {
                swr_free(&player_state->swr_context);
            }
        }
        player_state->audio_buf = player_state->audio_buf1;
        resample_data_size = len2 * media_decode->audio_tgt.channels * av_get_bytes_per_sample(media_decode->audio_tgt.fmt);
    } else {
        player_state->audio_buf = frame->frame->data[0];
        resample_data_size = data_size;
    }
    double audio_clock0 = player_state->audio_clock;

    /* update the audio clock with the pts */
    if (!isnan(frame->pts)) {
        player_state->audio_clock = frame->pts + (double) frame->frame->nb_samples / frame->frame->sample_rate;
    } else {
        player_state->audio_clock = NAN;
    }

#ifdef DEBUG
    {
//        static double last_clock;
//        printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n",
//               is->audio_clock - last_clock,
//               is->audio_clock, audio_clock0);
//        last_clock = is->audio_clock;
    }
#endif
    return resample_data_size;
}

int VideoPlayer::ReadAudio(uint8_t* buffer, int buffer_size) {
    if (!media_decode_) {
        return buffer_size;
    }
    if (media_decode_->paused || media_decode_->abort_request) {
        return 0;
    }
    player_state_->audio_callback_time = av_gettime_relative();
    int audio_size, len1 = 0;
    while (buffer_size > 0) {
        if (player_state_->audio_buf_index >= player_state_->audio_buf_size) {
            audio_size = AudioResample(media_decode_, player_state_);
            if (audio_size < 0) {
                player_state_->audio_buf = nullptr;
                player_state_->audio_buf_size = AUDIO_MIN_BUFFER_SIZE / media_decode_->audio_tgt.frame_size * media_decode_->audio_tgt.frame_size;
            } else {
                player_state_->audio_buf_size = audio_size;
            }
            player_state_->audio_buf_index = 0;
        }
        len1 = player_state_->audio_buf_size - player_state_->audio_buf_index;
        if (len1 > buffer_size) {
            len1 = buffer_size;
        }
        if (nullptr == player_state_->audio_buf) {
            memset(buffer, 0, len1);
            break;
        } else {
            memcpy(buffer, player_state_->audio_buf + player_state_->audio_buf_index, len1);
        }
        buffer_size -= len1;
        buffer += len1;
        player_state_->audio_buf_index += len1;
    }
    player_state_->audio_write_buf_size = player_state_->audio_buf_size - player_state_->audio_buf_index;
    /* Let's assume the audio driver that is used by SDL has two periods. */
    if (!isnan(player_state_->audio_clock)) {
        SetClockAt(&player_state_->sample_clock, player_state_->audio_clock - (double) (2 * player_state_->audio_hw_buf_size + player_state_->audio_write_buf_size) / media_decode_->audio_tgt.bytes_per_sec,
                     player_state_->audio_clock_serial, player_state_->audio_callback_time / 1000000.0);
        SyncClockToSlave(&player_state_->external_clock, &player_state_->sample_clock);
    }
    return len1;
}

bool VideoPlayer::CreateEGLContext(ANativeWindow *window) {
    core_ = new EGLCore();
    bool result = core_->InitWithSharedContext();
    if (!result) {
        LOGE("Create EGLContext failed");
        return false;
    }
    CreateWindowSurface(window);
    core_->DoneCurrent();
    egl_context_exists_ = true;
    return result;
}

void VideoPlayer::CreateWindowSurface(ANativeWindow *window) {
    LOGI("Enter CreateWindowSurface");
    if (core_ == nullptr || window == nullptr) {
        LOGE("CreateWindowSurface core_ is null");
        return;
    }
    render_surface_ = core_->CreateWindowSurface(window);
    if (render_surface_ != NULL && render_surface_ != EGL_NO_SURFACE) {
        core_->MakeCurrent(render_surface_);

        render_screen_ = new OpenGL(surface_width_, surface_height_, DEFAULT_VERTEX_MATRIX_SHADER, DEFAULT_FRAGMENT_SHADER);
//        image_process_->AddFlashWhiteAction(2000, 0, 3000);
    }
    LOGE("Leave CreateWindowSurface");
}

void VideoPlayer::DestroyWindowSurface() {
    LOGE("enter DestroyWindowSurface");
    if (EGL_NO_SURFACE != render_surface_) {
        if (nullptr != yuv_render_) {
            delete yuv_render_;
            yuv_render_ = nullptr;
        }
        if (nullptr != render_screen_) {
            delete render_screen_;
            render_screen_ = nullptr;
        }
        if (nullptr != core_) {
            core_->ReleaseSurface(render_surface_);
        }
        render_surface_ = EGL_NO_SURFACE;
        if (nullptr != window_) {
            ANativeWindow_release(window_);
            window_ = nullptr;
        }
    }
    LOGE("Leave DestroyWindowSurface");
}

void VideoPlayer::DestroyEGLContext() {
    LOGI("enter DestroyEGLContext");
    if (EGL_NO_SURFACE != render_surface_) {
        core_->MakeCurrent(render_surface_);
    }
    DestroyWindowSurface();
    if (nullptr != core_) {
        core_->Release();
        delete core_;
        core_ = nullptr;
    }
    egl_context_exists_ = false;
    egl_destroy_ = true;
    LOGI("leave DestroyEGLContext");
}

void VideoPlayer::RenderVideo() {
    if (nullptr != core_ && EGL_NO_SURFACE != render_surface_) {
        Frame *vp = frame_queue_peek_last(&media_decode_->video_frame_queue);
        if (! core_->MakeCurrent(render_surface_)) {
            LOGE("eglSwapBuffers MakeCurrent error: %d", eglGetError());
        }
        if (!vp->uploaded) {
            int width = MIN(vp->frame->linesize[0], vp->frame->width);
            int height = vp->frame->height;
            if (frame_width_ != width || frame_height_ != height) {
                frame_width_ = width;
                frame_height_ = height;
                if (nullptr != yuv_render_) {
                    delete yuv_render_;
                }
                yuv_render_ = new YuvRender(vp->width, vp->height, surface_width_, surface_height_, 0);
            }
            int texture_id = yuv_render_->DrawFrame(vp->frame);
            uint64_t current_time = (uint64_t) (vp->frame->pts * av_q2d(media_decode_->video_stream->time_base) * 1000);
//            texture_id = image_process_->Process(texture_id, current_time, vp->width, vp->height, 0, 0);
            render_screen_->ProcessImage(texture_id, vertex_coordinate_, texture_coordinate_);
            if (!core_->SwapBuffers(render_surface_)) {
                LOGE("eglSwapBuffers error: %d", eglGetError());
            }
            pthread_mutex_lock(&render_mutex_);
            if (handler_->GetQueueSize() <= 1) {
                pthread_cond_signal(&render_cond_);
            }
            pthread_mutex_unlock(&render_mutex_);
            vp->uploaded = 1;
        }
    }
}

void *VideoPlayer::RenderThread(void *context) {
    VideoPlayer* video_player = (VideoPlayer*) context;
    if (video_player->destroy_) {
        return nullptr;
    }
    video_player->ProcessMessage();
    pthread_exit(0);
}

void VideoPlayer::ProcessMessage() {
    bool rendering = true;
    while (rendering) {
        if (destroy_) {
            break;
        }
        Message *msg = NULL;
        if (message_queue_->DequeueMessage(&msg, true) > 0) {
            if (msg == NULL) {
                return;
            }
            if (MESSAGE_QUEUE_LOOP_QUIT_FLAG == msg->Execute()) {
                rendering = false;
            }
            delete msg;
        }
    }
}

void VideoPlayer::OnSurfaceCreated(ANativeWindow *window) {
    if (nullptr != handler_) {
        if (!egl_context_exists_) {
            handler_->PostMessage(new Message(kCreateEGLContext, window));
        } else {
            handler_->PostMessage(new Message(kCreateWindowSurface, window));
            handler_->PostMessage(new Message(kRenderFrame));
        }
    }
}

void VideoPlayer::OnSurfaceChanged(int width, int height) {
    surface_width_ = width;
    surface_height_ = height;
}

void VideoPlayer::OnSurfaceDestroyed() {
    if (nullptr != handler_) {
        handler_->PostMessage(new Message(kDestroyWindowSurface));
    }
}

void VideoPlayer::RenderVideoFrame(VideoEvent* event) {
    if (nullptr != event && nullptr != event->context) {
        VideoPlayer* video_player = (VideoPlayer*) event->context;
        if (!video_player->egl_context_exists_) {
            return;
        }
        video_player->video_play_state_ = kPlaying;
        VideoRenderHandler* handler = video_player->handler_;
        if (nullptr != handler) {
            pthread_mutex_lock(&video_player->render_mutex_);
            if (handler->GetQueueSize() > 1) {
                pthread_cond_wait(&video_player->render_cond_, &video_player->render_mutex_);
            }
            pthread_mutex_unlock(&video_player->render_mutex_);
            handler->PostMessage(new Message(kRenderFrame));
        }
    }
}

void VideoPlayer::Seek(int start_time) {
    av_seek(media_decode_, start_time);
}

}
