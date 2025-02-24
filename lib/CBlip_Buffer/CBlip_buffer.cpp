#include "Blip_Buffer.h"
#include "CBlip_buffer.h"

CBLIP_BUFFER_API CBlip_Buffer* create_cblip_buffer(void)
{
	return new Blip_Buffer();
}

CBLIP_BUFFER_API void free_cblip_buffer(CBlip_Buffer* in)
{
	delete in;
}

CBLIP_BUFFER_API void cblip_buffer_clock_rate(CBlip_Buffer* in, long cps)
{
	in->clock_rate(cps);
}

CBLIP_BUFFER_API blargg_err_t cblip_buffer_set_sample_rate(CBlip_Buffer* in, long samples_per_sec, int msec_length)
{
	return in->set_sample_rate(samples_per_sec, msec_length);
}

CBLIP_BUFFER_API void cblip_buffer_end_frame(CBlip_Buffer* in, blip_time_t time)
{
	in->end_frame(time);
}

CBLIP_BUFFER_API long cblip_buffer_read_samples(CBlip_Buffer* in, blip_sample_t* dest, long max_samples)
{
	return in->read_samples(dest, max_samples);
}

CBLIP_BUFFER_API void cblip_buffer_clear(CBlip_Buffer* in)
{
	in->clear();
}

CBLIP_BUFFER_API CBlipSynth create_cblip_synth(void)
{
	return (Blip_Synth<blip_good_quality, 20>*) new Blip_Synth<blip_good_quality, 20>;
}

CBLIP_BUFFER_API void free_cblip_synth(CBlipSynth in)
{
	delete (Blip_Synth<blip_good_quality, 20>*) in;
}

CBLIP_BUFFER_API void cblip_synth_volume(CBlipSynth in, double v)
{
	((Blip_Synth<blip_good_quality, 20>*) in)->volume(v);
}

CBLIP_BUFFER_API void cblip_synth_output(CBlipSynth in, CBlip_Buffer* b)
{
	((Blip_Synth<blip_good_quality, 20>*) in)->output(b);
}

CBLIP_BUFFER_API void cblip_synth_update(CBlipSynth in, blip_time_t time, int amplitude)
{
	((Blip_Synth<blip_good_quality, 20>*) in)->update(time, amplitude);
}
