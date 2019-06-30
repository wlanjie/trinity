//
// Created by wlanjie on 2019-06-29.
//

#include "video_player.h"

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

static double rdftspeed = 0.02;

namespace trinity {

VideoPlayer::VideoPlayer() {
    video_render_context_ = nullptr;
    media_decode_ = nullptr;
    player_state_ = nullptr;
}

VideoPlayer::~VideoPlayer() {

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

void VideoDisplay(MediaDecode* media_decode, VideoRenderContext* video_render_context) {
    if (video_render_context && media_decode->video_frame_queue.size > 0) {
        video_render_context->render_video_frame(video_render_context);
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
        VideoRenderContext* video_render_context,
        double* remaining_time, double* time) {
    if (frame_queue_nb_remaining(&media_decode->video_frame_queue) == 0) {
        // nothing to do, no picture to display in the queue
    } else {
        Frame* last = frame_queue_peek_last(&media_decode->video_frame_queue);
        Frame* frame = frame_queue_peek(&media_decode->video_frame_queue);
        if (frame->serial != media_decode->video_packet_queue.serial) {
            frame_queue_next(&media_decode->video_frame_queue);
            Retry(media_decode, player_state, video_render_context, remaining_time, time);
            return;
        }
        if (last->serial != frame->serial) {
            player_state->frame_timer = av_gettime_relative() / 1000000.0;
        }
        if (media_decode->paused) {
            if (player_state->force_refresh && media_decode->video_frame_queue.rindex_shown) {
                VideoDisplay(media_decode, video_render_context);
            }
            return;
        }
        double last_duration = FrameDuration(media_decode, last, frame);
        double delay = ComputeTargetDelay(last_duration, media_decode, player_state);
        *time = av_gettime_relative() / 1000000.0;
        if (*time < player_state->frame_timer + delay) {
            *remaining_time = FFMIN(player_state->frame_timer + delay - *time, *remaining_time);
            if (player_state->force_refresh && media_decode->video_frame_queue.rindex_shown) {
                VideoDisplay(media_decode, video_render_context);
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

    }

    if (player_state->force_refresh && media_decode->video_frame_queue.rindex_shown) {
        VideoDisplay(media_decode, video_render_context);
    }
}

void VideoRefresh(MediaDecode* media_decode,
        PlayerState* player_state,
        VideoRenderContext* video_render_context,
        double *remaining_time) {
    if (!media_decode->paused && GetMasterSyncType(media_decode, player_state) == AV_SYNC_EXTERNAL_CLOCK) {
        CheckExternalClockSpeed(media_decode, player_state);
    }
    double time;
    if (media_decode->audio_stream) {
        time = av_gettime_relative() / 1000000.0;
        if (player_state->force_refresh || player_state->last_vis_time + rdftspeed < time) {
            VideoDisplay(media_decode, video_render_context);
            player_state->last_vis_time = time;
        }
        *remaining_time = FFMIN(*remaining_time, player_state->last_vis_time + rdftspeed - time);
    }

    if (media_decode->video_stream) {
        Retry(media_decode, player_state, video_render_context, remaining_time, &time);
    }
}

int VideoPlayer::Start(const char *file_name, VideoRenderContext* render_context) {
    pthread_mutex_init(&sync_mutex_, nullptr);
    pthread_cond_init(&sync_cond_, nullptr) ;
    pthread_create(&sync_thread_, nullptr, SyncThread, this);

    video_render_context_ = render_context;

    player_state_ = (PlayerState*) av_malloc(sizeof(PlayerState));
    player_state_->av_sync_type = AV_SYNC_AUDIO_MASTER;

    media_decode_ = (MediaDecode*) av_malloc(sizeof(MediaDecode));
    media_decode_->start_time = 0;
    media_decode_->end_time = INT64_MAX;
    media_decode_->loop = 1;
    media_decode_->precision_seek = 1;
    int result = av_decode_start(media_decode_, file_name);
    if (result != 0) {
        return result;
    }
    InitClock(&player_state_->video_clock, &media_decode_->video_packet_queue.serial);
    InitClock(&player_state_->sample_clock, &media_decode_->audio_packet_queue.serial);
    InitClock(&player_state_->external_clock, &player_state_->external_clock.serial);
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
            pthread_cond_wait(&sync_cond_, &sync_mutex_);
        }
        pthread_mutex_unlock(&sync_mutex_);
        if (remaining_time > 0.0) {
            av_usleep((int64_t)(remaining_time * 1000000.0));
        }
        remaining_time = REFRESH_RATE;
        if (!media_decode_->paused || player_state_->force_refresh) {
            VideoRefresh(media_decode_, player_state_, video_render_context_, &remaining_time);
        }
    }
}

void VideoPlayer::Stop() {
    pthread_mutex_destroy(&sync_mutex_);
    pthread_cond_destroy(&sync_cond_);
}

}