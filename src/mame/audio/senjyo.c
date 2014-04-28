#include "emu.h"
#include "includes/senjyo.h"


const z80_daisy_config senjyo_daisy_chain[] =
{
	{ "z80ctc" },
	{ "z80pio" },
	{ NULL }
};


/* z80 pio */

READ8_MEMBER(senjyo_state::pio_pa_r)
{
	return m_sound_cmd;
}

Z80PIO_INTERFACE( senjyo_pio_intf )
{
	DEVCB_CPU_INPUT_LINE("sub", INPUT_LINE_IRQ0),
	DEVCB_DRIVER_MEMBER(senjyo_state,pio_pa_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

WRITE_LINE_MEMBER(senjyo_state::sound_line_clock)
{
	if (state != 0)
	{
		m_dac->write_signed16(2184 * 2 * ((m_sound_state & 8) ? m_single_volume : 0));
		m_sound_state++;
	}
}

WRITE8_MEMBER(senjyo_state::senjyo_volume_w)
{
	m_single_volume = data & 0x0f;
}
