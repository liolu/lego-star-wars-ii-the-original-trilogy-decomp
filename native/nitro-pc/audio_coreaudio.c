/* Son sur macOS : AudioQueue (AudioToolbox). Meme contrat que le waveOut de os_win32.c (os.h) :
 * le fil du son de snd.c remplit un tampon (os_audio_acquire, mix, os_audio_submit) chaque fois que la file en rend un.
 * La file tire donc le mixeur a son rythme : quand elle a joue un tampon, son rappel le rend libre et reveille
 * os_audio_acquire. Format : stereo 16 bits a la frequence du mixeur (RATE de snd.c), CoreAudio convertit pour la sortie.
 * --mute : snd.c n'ouvre pas la sortie (os_audio_open n'est jamais appele) et cadence le sequenceur sur os_sleep. */
#if !defined(__APPLE__)
#error "audio_coreaudio.c : macOS seulement"
#endif

#include <AudioToolbox/AudioQueue.h>
#include <dispatch/dispatch.h>

#include "nitro.h"
#include "os.h"

/* six tampons comme sous Windows : 6 x 384 images a 32768 Hz = 70 ms en file */
#define BUFFERS 6

struct OsAudio {
    AudioQueueRef queue;
    AudioQueueBufferRef buf[BUFFERS];
    dispatch_semaphore_t free_bufs; /* tampons libres : BUFFERS au depart, puis un par tampon joue */
    u32 frames, rate;
    u32 cur;       /* prochain tampon a remplir : la file les rend dans l'ordre ou ils sont entres */
    u32 submitted; /* la file demarre quand tous les tampons d'amorce sont entres */
};
static struct OsAudio audio;

/* fil interne de la file : un tampon vient d'etre joue */
static void on_played(void *user, AudioQueueRef q, AudioQueueBufferRef b) {
    dispatch_semaphore_signal(((OsAudio *)user)->free_bufs);
}

OsAudio *os_audio_open(u32 rate, u32 frames) {
    OsAudio *a = &audio;
    if (a->queue || !frames) return 0;
    AudioStreamBasicDescription fmt = {
        .mSampleRate = rate,
        .mFormatID = kAudioFormatLinearPCM,
        .mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked,
        .mBytesPerPacket = 4,
        .mFramesPerPacket = 1,
        .mBytesPerFrame = 4,
        .mChannelsPerFrame = 2,
        .mBitsPerChannel = 16,
    };
    /* run loop 0 : le rappel tourne sur un fil interne de la file, pas besoin de boucle d'evenements */
    if (AudioQueueNewOutput(&fmt, on_played, a, 0, 0, 0, &a->queue) != 0) {
        a->queue = 0;
        return 0;
    }
    for (int i = 0; i < BUFFERS; i++) {
        if (AudioQueueAllocateBuffer(a->queue, frames * 4, &a->buf[i]) != 0) {
            AudioQueueDispose(a->queue, 1);
            a->queue = 0;
            return 0;
        }
        a->buf[i]->mAudioDataByteSize = frames * 4;
    }
    a->free_bufs = dispatch_semaphore_create(BUFFERS);
    a->frames = frames;
    a->rate = rate;
    a->cur = 0;
    a->submitted = 0;
    return a;
}

u32 os_audio_depth(OsAudio *a) { return BUFFERS; }

s16 *os_audio_acquire(OsAudio *a) {
    dispatch_semaphore_wait(a->free_bufs, DISPATCH_TIME_FOREVER);
    return a->buf[a->cur]->mAudioData;
}

void os_audio_submit(OsAudio *a) {
    AudioQueueBufferRef b = a->buf[a->cur];
    b->mAudioDataByteSize = a->frames * 4;
    if (AudioQueueEnqueueBuffer(a->queue, b, 0, 0) != 0) {
        /* tampon refuse (sortie perdue) : il redevient libre apres sa duree, le sequenceur garde son rythme sans son */
        os_sleep(a->frames * 1000 / a->rate);
        dispatch_semaphore_signal(a->free_bufs);
    }
    a->cur = (a->cur + 1) % BUFFERS;
    if (++a->submitted == BUFFERS) AudioQueueStart(a->queue, 0);
}
