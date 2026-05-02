/*
 * gen_sounds.c — Generates all WAV sound assets for Snake Xenzia.
 *
 * Compile & run ONCE to produce assets/sounds/*.wav:
 *   gcc -o gen_sounds gen_sounds.c -lm && ./gen_sounds
 *
 * Produces:
 *   assets/sounds/eat.wav      — short pop / blip
 *   assets/sounds/die.wav      — descending crash tone
 *   assets/sounds/powerup.wav  — ascending chime
 *   assets/sounds/music.wav    — simple loopable background beat
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define SAMPLE_RATE  44100
#define NUM_CHANNELS 1
#define BITS_PER_SAMPLE 16
#define PI 3.14159265358979323846

/* ── WAV writer ─────────────────────────────────────────────────────── */

typedef struct {
    FILE    *f;
    long     data_pos;
    uint32_t num_samples;
} WavWriter;

static void write_u16(FILE *f, uint16_t v){ fwrite(&v,2,1,f); }
static void write_u32(FILE *f, uint32_t v){ fwrite(&v,4,1,f); }

static WavWriter wav_open(const char *path)
{
    WavWriter w = {NULL, 0, 0};
    w.f = fopen(path, "wb");
    if (!w.f) { fprintf(stderr,"Cannot open %s\n",path); return w; }

    /* RIFF header (sizes filled in on close) */
    fwrite("RIFF",1,4,w.f); write_u32(w.f,0);   /* chunk size placeholder */
    fwrite("WAVE",1,4,w.f);
    fwrite("fmt ",1,4,w.f); write_u32(w.f,16);
    write_u16(w.f,1);                              /* PCM */
    write_u16(w.f,NUM_CHANNELS);
    write_u32(w.f,SAMPLE_RATE);
    write_u32(w.f,SAMPLE_RATE * NUM_CHANNELS * BITS_PER_SAMPLE/8);
    write_u16(w.f,NUM_CHANNELS * BITS_PER_SAMPLE/8);
    write_u16(w.f,BITS_PER_SAMPLE);

    fwrite("data",1,4,w.f);
    w.data_pos = ftell(w.f);
    write_u32(w.f,0);   /* data size placeholder */
    return w;
}

static void wav_write_sample(WavWriter *w, double sample)
{
    /* Clamp and convert to 16-bit signed */
    if (sample >  1.0) sample =  1.0;
    if (sample < -1.0) sample = -1.0;
    int16_t s = (int16_t)(sample * 32767.0);
    fwrite(&s, 2, 1, w->f);
    w->num_samples++;
}

static void wav_close(WavWriter *w)
{
    if (!w->f) return;
    uint32_t data_bytes = w->num_samples * (BITS_PER_SAMPLE/8);
    /* Patch data chunk size */
    fseek(w->f, w->data_pos, SEEK_SET);
    write_u32(w->f, data_bytes);
    /* Patch RIFF chunk size */
    fseek(w->f, 4, SEEK_SET);
    write_u32(w->f, 36 + data_bytes);
    fclose(w->f);
    w->f = NULL;
}

/* ── Waveform helpers ─────────────────────────────────────────────── */

static double sine(double freq, double t)   { return sin(2.0*PI*freq*t); }
static double square(double freq, double t) { return sine(freq,t)>=0?1.0:-1.0; }
static double noise(void) { return ((double)rand()/RAND_MAX)*2.0-1.0; }
static double env_adsr(double t, double total,
                        double a, double d, double s_lvl, double r)
{
    /* Simple linear ADSR envelope */
    if (t < a)              return t/a;
    if (t < a+d)            return 1.0 - (1.0-s_lvl)*((t-a)/d);
    if (t < total-r)        return s_lvl;
    return s_lvl * (1.0-(t-(total-r))/r);
}

/* ── Sound: EAT (bright blip) ───────────────────────────────────────── */
static void gen_eat(const char *path)
{
    WavWriter w = wav_open(path);
    if (!w.f) return;
    double dur = 0.12;
    int n = (int)(dur * SAMPLE_RATE);
    for (int i=0; i<n; i++){
        double t = (double)i/SAMPLE_RATE;
        double freq = 880.0 + (1.0 - t/dur)*440.0;   /* descending chirp */
        double env  = env_adsr(t, dur, 0.005, 0.03, 0.5, 0.06);
        double s    = sine(freq, t) * 0.6 + square(freq*2, t) * 0.15;
        wav_write_sample(&w, s * env * 0.7);
    }
    wav_close(&w);
    printf("  Generated: %s\n", path);
}

/* ── Sound: DIE (crashing buzz) ─────────────────────────────────────── */
static void gen_die(const char *path)
{
    WavWriter w = wav_open(path);
    if (!w.f) return;
    double dur = 0.55;
    int n = (int)(dur * SAMPLE_RATE);
    for (int i=0; i<n; i++){
        double t    = (double)i/SAMPLE_RATE;
        double freq = 220.0 * (1.0 - t/dur * 0.7);   /* falling pitch */
        double env  = env_adsr(t, dur, 0.002, 0.05, 0.4, 0.3);
        double s    = square(freq, t) * 0.5
                    + noise()         * 0.3 * (t/dur);   /* adds crunch */
        wav_write_sample(&w, s * env * 0.75);
    }
    wav_close(&w);
    printf("  Generated: %s\n", path);
}

/* ── Sound: POWER-UP (ascending arpeggio) ─────────────────────────── */
static void gen_powerup(const char *path)
{
    WavWriter w = wav_open(path);
    if (!w.f) return;
    double freqs[4] = {440.0, 554.4, 659.3, 880.0};
    double note_dur = 0.09;
    for (int note=0; note<4; note++){
        int n = (int)(note_dur * SAMPLE_RATE);
        for (int i=0; i<n; i++){
            double t   = (double)i/SAMPLE_RATE;
            double env = env_adsr(t, note_dur, 0.005, 0.02, 0.6, 0.04);
            double s   = sine(freqs[note], t) * 0.55
                       + sine(freqs[note]*2, t) * 0.2;
            wav_write_sample(&w, s * env * 0.8);
        }
    }
    wav_close(&w);
    printf("  Generated: %s\n", path);
}

/* ── Sound: MUSIC (8-bar loopable background) ─────────────────────── */
/*
 * Simple synthesised chiptune-style loop using a bass line + hi-hat.
 * 120 BPM, 4/4 time, 2 bars = 4 seconds.
 */
static void gen_music(const char *path)
{
    WavWriter w = wav_open(path);
    if (!w.f) return;

    double bpm   = 118.0;
    double beat  = 60.0 / bpm;
    double bars  = 8.0;
    double total = bars * 4.0 * beat;

    /* Bass pattern: C2 G2 A2 F2 repeated */
    double bass_notes[] = {65.41, 98.00, 110.00, 87.31};
    int    bass_steps   = 4;

    /* Hi-hat: every 8th note */
    int n = (int)(total * SAMPLE_RATE);
    for (int i=0; i<n; i++){
        double t = (double)i / SAMPLE_RATE;

        /* Which beat/step are we on? */
        double beat_pos  = t / beat;
        int    beat_idx  = (int)beat_pos % (bass_steps);
        double beat_frac = beat_pos - (int)beat_pos;

        /* Bass synth */
        double bfreq = bass_notes[beat_idx];
        double benv  = (beat_frac < 0.45) ?
                        env_adsr(beat_frac, 0.45, 0.01, 0.05, 0.5, 0.3) : 0.0;
        double bass  = (square(bfreq, t)*0.5 + sine(bfreq*2,t)*0.3) * benv * 0.35;

        /* Chord pad (simple fifth) */
        double pad_env = 0.15 * (0.5 + 0.5*sine(0.25, t));
        double pad = sine(bfreq*2, t)*0.4 + sine(bfreq*3, t)*0.2;
        pad *= pad_env;

        /* Hi-hat: noise burst every 8th note */
        double hihat_period = beat / 2.0;
        double hfrac = fmod(t, hihat_period) / hihat_period;
        double hihat = (hfrac < 0.06) ? noise() * 0.15 * (1.0 - hfrac/0.06) : 0.0;

        /* Kick: every beat */
        double kick_frac = fmod(t, beat) / beat;
        double kick_freq = 80.0 * exp(-kick_frac * 20.0);
        double kick_env  = exp(-kick_frac * 12.0) * 0.5;
        double kick = sine(kick_freq, t) * kick_env;

        double mix = bass + pad + hihat + kick;
        /* Soft clip */
        mix = tanh(mix * 0.9);
        wav_write_sample(&w, mix * 0.7);
    }
    wav_close(&w);
    printf("  Generated: %s  (%.1f seconds)\n", path, total);
}

/* ── Entry point ────────────────────────────────────────────────────── */
int main(void)
{
    printf("Generating sound assets...\n");

    /* Create output directory if needed (best-effort) */
#ifdef _WIN32
    system("if not exist assets\\sounds mkdir assets\\sounds");
#else
    system("mkdir -p assets/sounds");
#endif

    gen_eat    ("assets/sounds/eat.wav");
    gen_die    ("assets/sounds/die.wav");
    gen_powerup("assets/sounds/powerup.wav");
    gen_music  ("assets/sounds/music.wav");

    printf("\nDone. All sound files written to assets/sounds/\n");
    return 0;
}
