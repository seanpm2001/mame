/***************************************************************************

    8080-based black and white hardware

****************************************************************************/

#include "includes/mw8080bw.h"

/* for games in 8080bw.c */
#define CABINET_PORT_TAG                  "CAB"


class _8080bw_state : public mw8080bw_state
{
public:
	_8080bw_state(const machine_config &mconfig, device_type type, const char *tag)
		: mw8080bw_state(mconfig, type, tag) { }

	/* misc game specific */
	emu_timer   *m_schaser_effect_555_timer;
	attotime    m_schaser_effect_555_time_remain;
	INT32       m_schaser_effect_555_time_remain_savable;
	int         m_schaser_effect_555_is_low;
	int         m_schaser_explosion;
	int         m_schaser_last_effect;
	UINT8       m_polaris_cloud_speed;
	UINT8       m_polaris_cloud_pos;
	UINT8       m_schaser_background_disable;
	UINT8       m_schaser_background_select;
	UINT8       m_c8080bw_flip_screen;
	UINT8       m_color_map;
	UINT8       m_screen_red;

	device_t *m_speaker;

	DECLARE_CUSTOM_INPUT_MEMBER(sflush_80_r);
	DECLARE_READ8_MEMBER(indianbt_r);
	DECLARE_WRITE8_MEMBER(steelwkr_sh_port_3_w);
	DECLARE_WRITE8_MEMBER(invadpt2_sh_port_1_w);
	DECLARE_WRITE8_MEMBER(invadpt2_sh_port_2_w);
	DECLARE_WRITE8_MEMBER(spcewars_sh_port_w);
	DECLARE_WRITE8_MEMBER(lrescue_sh_port_1_w);
	DECLARE_WRITE8_MEMBER(lrescue_sh_port_2_w);
	DECLARE_WRITE8_MEMBER(cosmo_sh_port_2_w);
	DECLARE_WRITE8_MEMBER(ballbomb_sh_port_1_w);
	DECLARE_WRITE8_MEMBER(ballbomb_sh_port_2_w);
	DECLARE_WRITE8_MEMBER(indianbt_sh_port_1_w);
	DECLARE_WRITE8_MEMBER(indianbt_sh_port_2_w);
	DECLARE_WRITE8_MEMBER(schaser_sh_port_1_w);
	DECLARE_WRITE8_MEMBER(schaser_sh_port_2_w);
	DECLARE_WRITE8_MEMBER(rollingc_sh_port_w);
	DECLARE_WRITE8_MEMBER(invrvnge_sh_port_w);
	DECLARE_WRITE8_MEMBER(lupin3_sh_port_1_w);
	DECLARE_WRITE8_MEMBER(lupin3_sh_port_2_w);
	DECLARE_WRITE8_MEMBER(schasercv_sh_port_1_w);
	DECLARE_WRITE8_MEMBER(schasercv_sh_port_2_w);
	DECLARE_WRITE8_MEMBER(yosakdon_sh_port_1_w);
	DECLARE_WRITE8_MEMBER(yosakdon_sh_port_2_w);
	DECLARE_WRITE8_MEMBER(shuttlei_sh_port_1_w);
	DECLARE_WRITE8_MEMBER(shuttlei_sh_port_2_w);
	DECLARE_READ8_MEMBER(invmulti_eeprom_r);
	DECLARE_WRITE8_MEMBER(invmulti_eeprom_w);
	DECLARE_WRITE8_MEMBER(invmulti_bank_w);
	DECLARE_DRIVER_INIT(invmulti);
	DECLARE_DRIVER_INIT(vortex);
	DECLARE_MACHINE_START(extra_8080bw);
	DECLARE_MACHINE_START(schaser);
	DECLARE_MACHINE_RESET(schaser);
	DECLARE_MACHINE_START(polaris);
	DECLARE_MACHINE_START(darthvdr);
	DECLARE_MACHINE_RESET(darthvdr);
	DECLARE_MACHINE_RESET(invmulti);
	DECLARE_MACHINE_START(extra_8080bw_sh);
	DECLARE_MACHINE_START(extra_8080bw_vh);
	DECLARE_MACHINE_START(schaser_sh);
	DECLARE_MACHINE_RESET(schaser_sh);
};


/*----------- defined in audio/8080bw.c -----------*/



extern const samples_interface lrescue_samples_interface;

WRITE8_DEVICE_HANDLER( indianbt_sh_port_3_w );
DISCRETE_SOUND_EXTERN( indianbt );

WRITE8_DEVICE_HANDLER( polaris_sh_port_1_w );
WRITE8_DEVICE_HANDLER( polaris_sh_port_2_w );
WRITE8_DEVICE_HANDLER( polaris_sh_port_3_w );
DISCRETE_SOUND_EXTERN( polaris );



extern const sn76477_interface schaser_sn76477_interface;
DISCRETE_SOUND_EXTERN( schaser );


/*----------- defined in video/8080bw.c -----------*/



SCREEN_UPDATE_RGB32( invadpt2 );
SCREEN_UPDATE_RGB32( ballbomb );
SCREEN_UPDATE_RGB32( schaser );
SCREEN_UPDATE_RGB32( schasercv );
SCREEN_UPDATE_RGB32( rollingc );
SCREEN_UPDATE_RGB32( polaris );
SCREEN_UPDATE_RGB32( lupin3 );
SCREEN_UPDATE_RGB32( cosmo );
SCREEN_UPDATE_RGB32( indianbt );
SCREEN_UPDATE_RGB32( shuttlei );
SCREEN_UPDATE_RGB32( sflush );

