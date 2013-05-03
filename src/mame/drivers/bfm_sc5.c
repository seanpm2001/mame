/*

    Scorpion 5

    Skeleton Driver - For note keeping.

    This system is not emulated.

*/


#include "emu.h"

#include "includes/bfm_sc5.h"


#include "machine/mcf5206e.h"
#include "bfm_sc5.lh"
#include "video/awpvid.h"



WRITE16_MEMBER( bfm_sc5_state::sc5_duart_w )
{
	// clearly a duart of some kind, write patterns are the same as SC4 games
//	printf("%s: duart_w %1x %04x %04x\n", machine().describe_context(), offset, data, mem_mask);

	if (mem_mask &0xff00)
	{
		duart68681_w(m_duart,space,offset,(data>>8)&0x00ff);
	}
	else
	{
		logerror("%s: duart_w %1x %04x %04x\n", machine().describe_context(), offset, data, mem_mask);
	}

}

static ADDRESS_MAP_START( sc5_map, AS_PROGRAM, 32, bfm_sc5_state )
	// ROM (max size?)
	AM_RANGE(0x00000000, 0x002fffff) AM_ROM
	// ?
	AM_RANGE(0x01000000, 0x0100ffff) AM_RAM

#if 1
	// dev1
	AM_RANGE(0x01010000, 0x01010003) AM_WRITENOP
	AM_RANGE(0x01010010, 0x01010013) AM_WRITENOP
	AM_RANGE(0x01010020, 0x01010023) AM_WRITENOP
	AM_RANGE(0x01010030, 0x01010033) AM_WRITENOP
	AM_RANGE(0x01010040, 0x01010043) AM_WRITENOP
	AM_RANGE(0x01010050, 0x01010053) AM_WRITENOP
	AM_RANGE(0x01010060, 0x01010063) AM_WRITENOP
	AM_RANGE(0x01010070, 0x01010073) AM_WRITENOP
	AM_RANGE(0x01010080, 0x01010083) AM_WRITENOP
	AM_RANGE(0x01010090, 0x01010093) AM_WRITENOP
	AM_RANGE(0x010100a0, 0x010100a3) AM_WRITENOP
	AM_RANGE(0x010100b0, 0x010100b3) AM_WRITENOP
	AM_RANGE(0x010100c0, 0x010100c3) AM_WRITENOP
	AM_RANGE(0x010100d0, 0x010100d3) AM_WRITENOP
	AM_RANGE(0x010100e0, 0x010100e3) AM_WRITENOP
	AM_RANGE(0x010100f0, 0x010100f3) AM_WRITENOP
	AM_RANGE(0x01010100, 0x01010103) AM_WRITENOP
	AM_RANGE(0x01010110, 0x01010113) AM_WRITENOP
	AM_RANGE(0x01010120, 0x01010123) AM_WRITENOP
	AM_RANGE(0x01010130, 0x01010133) AM_WRITENOP
	AM_RANGE(0x01010140, 0x01010143) AM_WRITENOP
	AM_RANGE(0x01010150, 0x01010153) AM_WRITENOP
	AM_RANGE(0x01010160, 0x01010163) AM_WRITENOP
	AM_RANGE(0x01010170, 0x01010173) AM_WRITENOP
	AM_RANGE(0x01010180, 0x01010183) AM_WRITENOP
	AM_RANGE(0x01010190, 0x01010193) AM_WRITENOP
	AM_RANGE(0x010101a0, 0x010101a3) AM_WRITENOP
	AM_RANGE(0x010101b0, 0x010101b3) AM_WRITENOP
	AM_RANGE(0x010101c0, 0x010101c3) AM_WRITENOP
	AM_RANGE(0x010101d0, 0x010101d3) AM_WRITENOP
	AM_RANGE(0x010101e0, 0x010101e3) AM_WRITENOP
	AM_RANGE(0x010101f0, 0x010101f3) AM_WRITENOP
	AM_RANGE(0x01010200, 0x01010203) AM_WRITENOP
	AM_RANGE(0x01010210, 0x01010213) AM_WRITENOP
	AM_RANGE(0x01010220, 0x01010223) AM_WRITENOP
	AM_RANGE(0x01010230, 0x01010233) AM_WRITENOP

	AM_RANGE(0x01010280, 0x01010283) AM_WRITENOP

	AM_RANGE(0x010102a0, 0x010102a3) AM_WRITENOP

	AM_RANGE(0x010102c0, 0x010102c3) AM_WRITENOP

	AM_RANGE(0x010102f0, 0x010102f3) AM_WRITENOP

	AM_RANGE(0x01010300, 0x01010303) AM_WRITENOP
	
	AM_RANGE(0x01010330, 0x01010333) AM_WRITENOP
	
	AM_RANGE(0x01010360, 0x01010363) AM_WRITENOP

	AM_RANGE(0x01010380, 0x01010383) AM_WRITENOP
	AM_RANGE(0x01010390, 0x01010393) AM_WRITENOP

	// dev2
	AM_RANGE(0x01020000, 0x01020003) AM_WRITENOP
	AM_RANGE(0x01020010, 0x01020013) AM_WRITENOP
	AM_RANGE(0x01020020, 0x01020023) AM_WRITENOP
	AM_RANGE(0x01020030, 0x01020033) AM_WRITENOP
	AM_RANGE(0x01020040, 0x01020043) AM_WRITENOP
	AM_RANGE(0x01020050, 0x01020053) AM_WRITENOP
	AM_RANGE(0x01020060, 0x01020063) AM_WRITENOP
	AM_RANGE(0x01020070, 0x01020073) AM_WRITENOP
	AM_RANGE(0x01020080, 0x01020083) AM_WRITENOP
	AM_RANGE(0x01020090, 0x01020093) AM_WRITENOP
	AM_RANGE(0x010200a0, 0x010200a3) AM_WRITENOP
	AM_RANGE(0x010200b0, 0x010200b3) AM_WRITENOP
	AM_RANGE(0x010200c0, 0x010200c3) AM_WRITENOP
	AM_RANGE(0x010200d0, 0x010200d3) AM_WRITENOP
	AM_RANGE(0x010200e0, 0x010200e3) AM_WRITENOP
	AM_RANGE(0x010200f0, 0x010200f3) AM_WRITENOP
	AM_RANGE(0x01020100, 0x01020103) AM_WRITENOP
	AM_RANGE(0x01020110, 0x01020113) AM_WRITENOP
	AM_RANGE(0x01020120, 0x01020123) AM_WRITENOP
	AM_RANGE(0x01020130, 0x01020133) AM_WRITENOP
	AM_RANGE(0x01020140, 0x01020143) AM_WRITENOP
	AM_RANGE(0x01020150, 0x01020153) AM_WRITENOP
	AM_RANGE(0x01020160, 0x01020163) AM_WRITENOP
	AM_RANGE(0x01020170, 0x01020173) AM_WRITENOP
	AM_RANGE(0x01020180, 0x01020183) AM_WRITENOP
	AM_RANGE(0x01020190, 0x01020193) AM_WRITENOP
	AM_RANGE(0x010201a0, 0x010201a3) AM_WRITENOP
	AM_RANGE(0x010201b0, 0x010201b3) AM_WRITENOP
	AM_RANGE(0x010201c0, 0x010201c3) AM_WRITENOP
	AM_RANGE(0x010201d0, 0x010201d3) AM_WRITENOP
	AM_RANGE(0x010201e0, 0x010201e3) AM_WRITENOP
	AM_RANGE(0x010201f0, 0x010201f3) AM_WRITENOP
	AM_RANGE(0x01020200, 0x01020203) AM_WRITENOP
	AM_RANGE(0x01020210, 0x01020213) AM_WRITENOP
	AM_RANGE(0x01020220, 0x01020223) AM_WRITENOP
	AM_RANGE(0x01020230, 0x01020233) AM_WRITENOP

	AM_RANGE(0x01020280, 0x01020283) AM_WRITENOP

	AM_RANGE(0x010202a0, 0x010202a3) AM_WRITENOP
	AM_RANGE(0x010202b0, 0x010202b3) AM_WRITENOP
	AM_RANGE(0x010202c0, 0x010202c3) AM_WRITENOP
#endif
	AM_RANGE(0x010202F0, 0x010202F3) AM_READWRITE8(sc5_10202F0_r, sc5_10202F0_w, 0xffffffff)
#if 1	
	AM_RANGE(0x01020330, 0x01020333) AM_WRITENOP

	AM_RANGE(0x01020350, 0x01020353) AM_WRITENOP
	AM_RANGE(0x01020360, 0x01020363) AM_WRITENOP
	AM_RANGE(0x01020370, 0x01020373) AM_WRITENOP

	AM_RANGE(0x01020390, 0x01020393) AM_WRITENOP
#endif
	AM_RANGE(0x02000000, 0x0200001f) AM_WRITE16(sc5_duart_w, 0xffffffff)

	// ram
	AM_RANGE(0x40000000, 0x4000ffff) AM_RAM

	// peripherals
	AM_RANGE(0xffff0000, 0xffff03ff) AM_DEVREADWRITE("maincpu_onboard", mcf5206e_peripheral_device, dev_r, dev_w) // technically this can be moved with MBAR
ADDRESS_MAP_END

INPUT_PORTS_START( bfm_sc5 )
INPUT_PORTS_END

READ8_MEMBER( bfm_sc5_state::sc5_10202F0_r )
{
	switch (offset)
	{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
			printf("%s: sc5_10202F0_r %d\n", machine().describe_context(), offset);
			return 0xff;
	}

	return 0;
}

WRITE8_MEMBER( bfm_sc5_state::sc5_10202F0_w )
{
	switch (offset)
	{
		case 0x0:
			bfm_sc45_write_serial_vfd(machine(), (data &0x4)?1:0, (data &0x1)?1:0, (data&0x2) ? 0:1);
			if (data&0xf8) printf("%s: sc5_10202F0_w %d - %02x\n", machine().describe_context(), offset, data);
			break;
		case 0x1:
		case 0x2:
		case 0x3:
			printf("%s: sc5_10202F0_w %d - %02x\n", machine().describe_context(), offset, data);
			break;
	}
}



WRITE_LINE_MEMBER(bfm_sc5_state::bfm_sc5_ym_irqhandler)
{
	logerror("YMZ280 is generating an interrupt. State=%08x\n",state);
}

static const ymz280b_interface ymz280b_config =
{
	DEVCB_DRIVER_LINE_MEMBER(bfm_sc5_state,bfm_sc5_ym_irqhandler)
};



void bfm_sc5_duart_irq_handler(device_t *device, int state, UINT8 vector)
{
	printf("bfm_sc5_duart_irq_handler\n");
};

void bfm_sc5_duart_tx(device_t *device, int channel, UINT8 data)
{
	logerror("bfm_sc5_duart_tx\n");
};



UINT8 bfm_sc5_duart_input_r(device_t *device)
{
	//bfm_sc5_state *state = device->machine().driver_data<bfm_sc5_state>();
	printf("bfm_sc5_duart_input_r\n");
	return 0xff;
}

void bfm_sc5_duart_output_w(device_t *device, UINT8 data)
{
	logerror("bfm_sc5_duart_output_w\n");
}


static const duart68681_config bfm_sc5_duart68681_config =
{
	bfm_sc5_duart_irq_handler,
	bfm_sc5_duart_tx,
	bfm_sc5_duart_input_r,
	bfm_sc5_duart_output_w,
	// TODO: What are the actual frequencies?
	16000000/2/8,     /* IP2/RxCB clock */
	16000000/2/16,    /* IP3/TxCA clock */
	16000000/2/16,    /* IP4/RxCA clock */
	16000000/2/8,     /* IP5/TxCB clock */
};


MACHINE_CONFIG_START( bfm_sc5, bfm_sc5_state )
	MCFG_CPU_ADD("maincpu", MCF5206E, 40000000) /* MCF5206eFT */
	MCFG_CPU_PROGRAM_MAP(sc5_map)
	MCFG_MCF5206E_PERIPHERAL_ADD("maincpu_onboard")

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_DUART68681_ADD("duart68681", 16000000/4, bfm_sc5_duart68681_config) // ?? Mhz


	MCFG_BFMBDA_ADD("vfd0",0)

	MCFG_DEFAULT_LAYOUT(layout_bfm_sc5)

	MCFG_SOUND_ADD("ymz", YMZ280B, 16000000) // ?? Mhz
	MCFG_SOUND_CONFIG(ymz280b_config)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_CONFIG_END
