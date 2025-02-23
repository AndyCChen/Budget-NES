#ifndef CBLIP_BUFFER_H
#define CBLIP_BUFFER_H

#ifdef __cplusplus
	#define CBLIP_EXTERN extern "C"
#else
	#define CBLIP_EXTERN extern
#endif

#define CBLIP_BUFFER_API CBLIP_EXTERN


typedef long blip_time_t;
typedef short blip_sample_t;
typedef struct Blip_Buffer CBlip_Buffer;
typedef void* CBlipSynth;

typedef const char* blargg_err_t;

// blip buffer

CBLIP_BUFFER_API CBlip_Buffer* create_cblip_buffer(void);
CBLIP_BUFFER_API void free_cblip_buffer(CBlip_Buffer *in);
CBLIP_BUFFER_API void cblip_buffer_clock_rate(CBlip_Buffer *in, long cps);
CBLIP_BUFFER_API blargg_err_t cblip_buffer_set_sample_rate(CBlip_Buffer* in, long samples_per_sec, int msec_length);
CBLIP_BUFFER_API void cblip_buffer_end_frame(CBlip_Buffer* in, blip_time_t time);
CBLIP_BUFFER_API long cblip_buffer_read_samples(CBlip_Buffer* in, blip_sample_t* dest, long max_samples);

// blip synth

CBLIP_BUFFER_API CBlipSynth create_cblip_synth(void);
CBLIP_BUFFER_API void free_cblip_synth(CBlipSynth in);
CBLIP_BUFFER_API void cblip_synth_volume(CBlipSynth in, double v);
CBLIP_BUFFER_API void cblip_synth_output(CBlipSynth in, CBlip_Buffer *b);
CBLIP_BUFFER_API void cblip_synth_update(CBlipSynth in, blip_time_t time, int amplitude);


#endif