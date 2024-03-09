#ifndef _ZSMKIT_H_
#define _ZSMKIT_H_
#include <stdint.h>

// These values are used to tell zsm_setloop to loop or not
#define DO_LOOP 1
#define NO_LOOP 0

// These values are used to tell zsm_tick what to update
#define MUSIC_PCM 0	// Music and PCM
#define PCM_ONLY 1	// Only PCM
#define MUSIC_ONLY 2	// Only Music

//    void callbackfunction(uint8_t eventtype, uint8_t priority, uint8_t paramval)
// eventtype = .Y - priority = .X - paramval = .A
typedef void(*zsm_callback)(uint8_t, uint8_t, uint8_t);

typedef struct zsm_state {
	uint8_t playing;
	uint8_t not_playable;
	uint16_t loopcnt;
};

extern void __fastcall__ zsm_init_engine(uint8_t bank);
extern void __fastcall__ zsm_tick(uint8_t what);
extern void __fastcall__ zsm_play(uint8_t priority);
extern void __fastcall__ zsm_stop(uint8_t priority);
extern void __fastcall__ zsm_rewind(uint8_t priority);
extern void __fastcall__ zsm_close(uint8_t priority);
extern void __fastcall__ zsm_fill_buffers();
extern void __fastcall__ zsm_setlfs(uint8_t lfnsa, uint8_t priority, uint8_t device);
extern void __fastcall__ zsm_setfile(uint8_t priority, char *filename);
extern uint16_t __fastcall__ zsm_loadpcm(uint8_t priority, uint16_t addr, uint8_t bank);
extern void __fastcall__ zsm_setmem(uint8_t priority, uint16_t addr, uint8_t bank);
extern void __fastcall__ zsm_setatten(uint8_t priority, uint8_t attenuation);
extern void __fastcall__ zsm_setcb(uint8_t priority, zsm_callback, uint8_t bank);
extern void __fastcall__ zsm_clearcb(uint8_t priority);
extern struct zsm_state __fastcall__ zsm_getstate(uint8_t priority);
extern void __fastcall__ zsm_setrate(uint8_t priority, uint16_t tickrate);
extern uint16_t __fastcall__ zsm_getrate(uint8_t priority);
extern void __fastcall__ zsm_setloop(uint8_t priority, uint8_t loop);
extern void __fastcall__ zsm_opmatten(uint8_t priority, uint8_t channel, uint8_t val);
extern void __fastcall__ zsm_psgatten(uint8_t priority, uint8_t channel, uint8_t val);
extern void __fastcall__ zsm_pcmatten(uint8_t priority, uint8_t channel, uint8_t val);
extern void __fastcall__ zsm_set_int_rate(uint8_t herz, uint8_t in256th);

extern void __fastcall__ zcm_setmem(uint8_t priority, uint16_t addr, uint8_t bank);
extern void __fastcall__ zcm_play(uint8_t slot, uint8_t volume);
extern void __fastcall__ zcm_stop();
extern void __fastcall__ zsmkit_setisr();
extern void __fastcall__ zsmkit_clearisr();


#endif
