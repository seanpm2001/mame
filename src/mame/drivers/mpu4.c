/* Note 19/07/11 DH
 - added lots of sets

   these are mostly unsorted and need to be split into clones
   the original source of these was a mess, assume things to be mislabled, bad, duplicated, or otherwise
   badly organized.  a lot of work is needed to sort them out, especially the Barcrest sets!  Some of this
   stuff MIGHT be in the wrong driver, or missing roms (sound roms especially)
*/

/***********************************************************************************************************
  Barcrest MPU4 highly preliminary driver by J.Wallace, and Anonymous.

  This is the core driver, no video specific stuff should go in here.
  This driver holds all the mechanical games.

     06-2011: Fixed boneheaded interface glitch that was causing samples to not be cancelled correctly.
              Added the ability to read each segment of an LED display separately, this may be necessary for some
              games that use them as surrogate lamp lines.
              New persistence 'hack' to stop light flicker for the small extender.
     05-2011: Add better OKI emulation
     04-2011: More accurate gamball code, fixed ROM banking (Project Amber), added BwB CHR simulator (Amber)
              This is still a hard coded system, but significantly different to Barcrest's version.
              Started adding support for the Crystal Gaming program card, and the link keys for setting parameters.
     03-2011: Lamp timing fixes, support for all known expansion cards added.
     01-2011: Adding the missing 'OKI' sound card, and documented it, but it needs a 6376 rewrite.
     09-2007: Haze: Added Deal 'Em video support.
  03-08-2007: J Wallace: Removed audio filter for now, since sound is more accurate without them.
                         Connect 4 now has the right sound.
  03-07-2007: J Wallace: Several major changes, including input relabelling, and system timer improvements.
     06-2007: Atari Ace, many cleanups and optimizations of I/O routines
  09-06-2007: J Wallace: Fixed 50Hz detection circuit.
  17-02-2007: J Wallace: Added Deal 'Em - still needs some work.
  10-02-2007: J Wallace: Improved input timing.
  30-01-2007: J Wallace: Characteriser rewritten to run the 'extra' data needed by some games.
  24-01-2007: J Wallace: With thanks to Canonman and HIGHWAYMAN/System 80, I was able to confirm a seemingly
              ghastly misuse of a PIA is actually on the real hardware. This fixes the meters.

See http://agemame.mameworld.info/techinfo/mpu4.php for Information.

--- Board Setup ---

The MPU4 BOARD is the driver board, originally designed to run Fruit Machines made by the Barcrest Group, but later
licensed to other firms as a general purpose unit (even some old Photo-Me booths used the unit).

This board uses a ~1.72 Mhz 6809B CPU, and a number of PIA6821 chips for multiplexing inputs and the like.

To some extent, the hardware feels like a revision of the MPU3 design, integrating into the base unit features that were
previously added through expansion ports. However, there is no backwards compatibility, and the entire memory map has been
reworked.

Like MPU3, a 6840PTM is used for internal timing, and other miscellaneous control functions, including as a crude analogue sound device
(a square wave from the PTM being used as the alarm sound generator). However, the main sound functionality is provided by
dedicated hardware (an AY8913).

A MPU4 GAME CARD (cartridge) plugs into the MPU4 board containing the game, and a protection PAL (the 'characteriser').
This PAL, as well as protecting the games, also controlled some of the lamp address matrix for many games, and acted as
an anti-tampering device which helped to prevent the hacking of certain titles in a manner which broke UK gaming laws.

Like MPU3, over the years developers have added more capabilities through the spare inputs and outputs provided. These provided
support for more reels, lamps and LEDs through daughtercards.
Several solutions were released depending on the manufacturer of the machine, all are emulated here.

In later revisions of the main board (MOD4 onwards), the AY8913 was removed entirely, as two official alternatives for sound had been produced.
In one, a YM2413 is built into the gameboard, and in the other an OKI MSM6376 is interfaced with a PIA and PTM to allow sophisticated
sampled sound.

The lamping and input handling side of the machine rely entirely on a column by column 'strobe' system, with lights and LEDs selected in turn.
In the inputs there are two orange connectors (sampled every 8ms) and two black ones (sampled every 16ms), giving 32 multiplexed inputs.

In addition there are two auxiliary ports that can be accessed separately to these and are bidirectional

--- Preliminary MPU4 Memorymap  ---

(NV) indicates an item which is not present on the video version, which has a Comms card instead.

   hex     |r/w| D D D D D D D D |
 location  |   | 7 6 5 4 3 2 1 0 | function
-----------+---+-----------------+--------------------------------------------------------------------------
 0000-07FF |R/W| D D D D D D D D | 2k RAM
-----------+---+-----------------+--------------------------------------------------------------------------
 0800      |R/W|                 | Characteriser (Security PAL) (NV)
-----------+---+-----------------+--------------------------------------------------------------------------
 0850 ?    | W | ??????????????? | page latch (NV)
-----------+---+-----------------+--------------------------------------------------------------------------
 0880      |R/W| D D D D D D D D | PIA6821 on soundboard (Oki MSM6376 clocked by 6840 (8C0))
           |   |                 | port A = ??
           |   |                 | port B (882)
           |   |                 |        b7 = NAR
           |   |                 |        b6 = 0 if OKI busy, 1 if OKI ready
           |   |                 |        b5 = volume control clock
           |   |                 |        b4 = volume control direction (0= up, 1 = down)
           |   |                 |        b3 = ??
           |   |                 |        b2 = ??
           |   |                 |        b1 = 2ch
           |   |                 |        b0 = ST
-----------+---+-----------------+--------------------------------------------------------------------------
 08C0      |   |                 | MC6840 on sound board
-----------+---+-----------------+--------------------------------------------------------------------------
 0900-     |R/W| D D D D D D D D | MC6840 PTM IC2


  Clock1 <--------------------------------------
     |                                          |
     V                                          |
  Output1 ---> Clock2                           |
                                                |
               Output2 --+-> Clock3             |
                         |                      |
                         |   Output3 ---> 'to audio amp' ??
                         |
                         +--------> CA1 IC3 (

IRQ line connected to CPU

-----------+---+-----------------+--------------------------------------------------------------------------
 0A00-0A03 |R/W| D D D D D D D D | PIA6821 IC3 port A Lamp Drives 1,2,3,4,6,7,8,9 (sic)(IC14)
           |   |                 |
           |   |                 |          CA1 <= output2 from PTM6840 (IC2)
           |   |                 |          CA2 => alpha data
           |   |                 |
           |   |                 |          port B Lamp Drives 10,11,12,13,14,15,16,17 (sic)(IC13)
           |   |                 |
           |   |                 |          CB2 => alpha reset (clock on Dutch systems)
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0B00-0B03 |R/W| D D D D D D D D | PIA6821 IC4 port A = data for 7seg leds (pins 10 - 17, via IC32)
           |   |                 |
           |   |                 |             CA1 INPUT, 50 Hz input (used to generate IRQ)
           |   |                 |             CA2 OUTPUT, connected to pin2 74LS138 CE for multiplexer
           |   |                 |                        (B on LED strobe multiplexer)
           |   |                 |             IRQA connected to IRQ of CPU
           |   |                 |             port B
           |   |                 |                    PB7 = INPUT, serial port Receive data (Rx)
           |   |                 |                    PB6 = INPUT, reel A sensor
           |   |                 |                    PB5 = INPUT, reel B sensor
           |   |                 |                    PB4 = INPUT, reel C sensor
           |   |                 |                    PB3 = INPUT, reel D sensor
           |   |                 |                    PB2 = INPUT, Connected to CA1 (50Hz signal)
           |   |                 |                    PB1 = INPUT, undercurrent sense
           |   |                 |                    PB0 = INPUT, overcurrent  sense
           |   |                 |
           |   |                 |             CB1 INPUT,  used to generate IRQ on edge of serial input line
           |   |                 |             CB2 OUTPUT, enable signal for reel optics
           |   |                 |             IRQB connected to IRQ of CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0C00-0C03 |R/W| D D D D D D D D | PIA6821 IC5 port A
           |   |                 |
           |   |                 |                    PA0-PA7, INPUT AUX1 connector
           |   |                 |
           |   |                 |             CA2  OUTPUT, serial port Transmit line
           |   |                 |             CA1  not connected
           |   |                 |             IRQA connected to IRQ of CPU
           |   |                 |
           |   |                 |             port B
           |   |                 |
           |   |                 |                    PB0-PB7 INPUT, AUX2 connector
           |   |                 |
           |   |                 |             CB1  INPUT,  connected to PB7 (Aux2 connector pin 4)
           |   |                 |
           |   |                 |             CB2  OUTPUT, AY8913 chip select line
           |   |                 |             IRQB connected to IRQ of CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0D00-0D03 |R/W| D D D D D D D D | PIA6821 IC6
           |   |                 |
           |   |                 |  port A
           |   |                 |
           |   |                 |        PA0 - PA7 (INPUT/OUTPUT) data port AY8913 sound chip
           |   |                 |
           |   |                 |        CA1 INPUT,  not connected
           |   |                 |        CA2 OUTPUT, BC1 pin AY8913 sound chip
           |   |                 |        IRQA , connected to IRQ CPU
           |   |                 |
           |   |                 |  port B
           |   |                 |
           |   |                 |        PB0-PB3 OUTPUT, reel A
           |   |                 |        PB4-PB7 OUTPUT, reel B
           |   |                 |
           |   |                 |        CB1 INPUT,  not connected
           |   |                 |        CB2 OUTPUT, B01R pin AY8913 sound chip
           |   |                 |        IRQB , connected to IRQ CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0E00-0E03 |R/W| D D D D D D D D | PIA6821 IC7
           |   |                 |
           |   |                 |  port A
           |   |                 |
           |   |                 |        PA0-PA3 OUTPUT, reel C
           |   |                 |        PA4-PA7 OUTPUT, reel D
           |   |                 |        CA1     INPUT,  not connected
           |   |                 |        CA2     OUTPUT, A on LED strobe multiplexer
           |   |                 |        IRQA , connected to IRQ CPU
           |   |                 |
           |   |                 |  port B
           |   |                 |
           |   |                 |        PB0-PB6 OUTPUT mech meter 1-7 or reel E + F
           |   |                 |        PB7     Voltage drop sensor
           |   |                 |        CB1     INPUT, not connected
           |   |                 |        CB2     OUTPUT,mech meter 8
           |   |                 |        IRQB , connected to IRQ CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 0F00-0F03 |R/W| D D D D D D D D | PIA6821 IC8
           |   |                 |
           |   |                 | port A
           |   |                 |
           |   |                 |        PA0-PA7 INPUT  multiplexed inputs data
           |   |                 |
           |   |                 |        CA1     INPUT, not connected
           |   |                 |        CA2    OUTPUT, C on LED strobe multiplexer
           |   |                 |        IRQA           connected to IRQ CPU
           |   |                 |
           |   |                 | port B
           |   |                 |
           |   |                 |        PB0-PB7 OUTPUT  triacs outputs connector PL6
           |   |                 |        used for slides / hoppers
           |   |                 |
           |   |                 |        CB1     INPUT, not connected
           |   |                 |        CB2    OUTPUT, pin1 alpha display PL7 (clock signal)
           |   |                 |        IRQB           connected to IRQ CPU
           |   |                 |
-----------+---+-----------------+--------------------------------------------------------------------------
 1000-FFFF | R | D D D D D D D D | ROM (can be bank switched by 0x850 in 8 banks of 64 k ) (NV)
-----------+---+-----------------+--------------------------------------------------------------------------

TODO: - Distinguish door switches using manual
      - Complete stubs for hoppers (needs slightly better 68681 emulation, and new 'hoppers' device emulation)
      - It seems that the MPU4 core program relies on some degree of persistence when switching strobes and handling
      writes to the various hardware ports. This explains the occasional lamping/LED blackout and switching bugs
      For now, we're ignoring any extra writes to strobes, as the alternative is to assign a timer to *everything* and
      start modelling the individual hysteresis curves of filament lamps.
      - Fix BwB characteriser, need to be able to calculate stabiliser bytes. Anyone fancy reading 6809 source?
      - Strange bug in Andy's Great Escape - Mystery nudge sound effect is not played, mpu4 latches in silence instead (?)
***********************************************************************************************************/
#include "emu.h"
#include "machine/6821pia.h"
#include "machine/6840ptm.h"
#include "machine/nvram.h"

#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"
#include "sound/okim6376.h"
#include "sound/2413intf.h"
#include "sound/upd7759.h"
#include "machine/steppers.h"
#include "machine/roc10937.h"
#include "machine/meters.h"
#include "includes/mpu4.h"


#include "video/awpvid.h"		//Fruit Machines Only
#include "connect4.lh"
#include "gamball.lh"
#include "mpu4.lh"
#include "mpu4ext.lh"


static TIMER_CALLBACK( ic24_timeout );


/*
LED Segments related to pins (5 is not connected):
Unlike the controllers emulated in the layout code, each
segment of an MPU4 LED can be set individually, even
being used as individual lamps. However, we can get away
with settings like this in the majority of cases.
   _9_
  |   |
  3   8
  |   |
   _2_
  |   |
  4   7
  |_ _|
    6  1

8 display enables (pins 10 - 17)
*/

static void lamp_extend_small(mpu4_state *state, int data)
{
	int lamp_ext_data,column,i;
	column = data & 0x07;

	lamp_ext_data = 0x1f - ((data & 0xf8) >> 3);//remove the mux lines from the data

	if ((state->m_lamp_strobe_ext_persistence == 0))
	//One write to reset the drive lines, one with the data, one to clear the lines, so only the 2nd write does anything
	//Once again, lamp persistences would take care of this, but we can't do that
	{
		for (i = 0; i < 5; i++)
		{
			output_set_lamp_value((8*column)+i+128,((lamp_ext_data  & (1 << i)) != 0));
		}
	}
	state->m_lamp_strobe_ext_persistence ++;
	if ((state->m_lamp_strobe_ext_persistence == 3)||(state->m_lamp_strobe_ext!=column))
	{
		state->m_lamp_strobe_ext_persistence = 0;
		state->m_lamp_strobe_ext=column;
	}
}

static void lamp_extend_large(mpu4_state *state, int data,int column,int active)
{
	int lampbase,i,bit7;

	state->m_lamp_sense = 0;
	bit7 = data & 0x80;
	if ( bit7 != state->m_last_b7 )
	{
		state->m_card_live = 1;
		//depending on bit 7, we can access one of two 'blocks' of 64 lamps
		lampbase = bit7 ? 0 : 64;
		if ( data & 0x3f )
		{
			state->m_lamp_sense = 1;
		}
		if ( active )
		{
			if (state->m_lamp_strobe_ext != column)
			{
				for (i = 0; i < 8; i++)
				{//CHECK, this includes bit 7
					output_set_lamp_value((8*column)+i+128+lampbase ,(data  & (1 << i)) != 0);
				}
				state->m_lamp_strobe_ext = column;
			}
		}
	    state->m_last_b7 = bit7;
	}
	else
	{
		state->m_card_live = 0;
	}
}

static void led_write_latch(mpu4_state *state, int latch, int data, int column)
{
	int diff,i,j;

	diff = (latch ^ state->m_last_latch) & latch;
	column = 7 - column; // like main board, these are wired up in reverse
	data = ~data;//inverted drive lines?

	for(i=0; i<5; i++)
	{
		if (diff & (1<<i))
		{
			column += i;
		}
	}
	for(j=0; j<8; j++)
	{
		output_set_indexed_value("mpu4led",(8*column)+j,(data & (1 << j)) !=0);
	}
	output_set_digit_value(column * 8, data);

	state->m_last_latch = diff;
}


static void update_meters(mpu4_state *state)
{
	int meter;
	int data = ((state->m_mmtr_data & 0x7f) | state->m_remote_meter);
	switch (state->m_reel_mux)
	{
		case STANDARD_REEL:
		// Change nothing
		break;
		case FIVE_REEL_5TO8:
		stepper_update(4, ((data >> 4) & 0x0f));
		data = (data & 0x0F); //Strip reel data from meter drives, leaving active elements
		awp_draw_reel(4);
		break;
		case FIVE_REEL_8TO5:
		stepper_update(4, (((data & 0x01) + ((data & 0x08) >> 2) + ((data & 0x20) >> 3) + ((data & 0x80) >> 4)) & 0x0f)) ;
		data = 0x00; //Strip all reel data from meter drives, nothing is connected
		awp_draw_reel(4);
		break;
		case FIVE_REEL_3TO6:
		stepper_update(4, ((data >> 2) & 0x0f));
		data = 0x00; //Strip all reel data from meter drives
		awp_draw_reel(4);
		break;
		case SIX_REEL_1TO8:
		stepper_update(4, (data & 0x0f));
		stepper_update(5, ((data >> 4) & 0x0f));
		data = 0x00; //Strip all reel data from meter drives
		awp_draw_reel(4);
		awp_draw_reel(5);
		break;
		case SIX_REEL_5TO8:
		stepper_update(4, ((data >> 4) & 0x0f));
		data = 0x00; //Strip all reel data from meter drives
		awp_draw_reel(4);
		break;
		case SEVEN_REEL:
		stepper_update(0, (((data & 0x01) + ((data & 0x08) >> 2) + ((data & 0x20) >> 3) + ((data & 0x80) >> 4)) & 0x0f)) ;
		data = 0x00; //Strip all reel data from meter drives
		awp_draw_reel(0);
		break;
		case FLUTTERBOX: //The backbox fan assembly fits in a reel unit sized box, wired to the remote meter pin, so we can handle it here
		output_set_value("flutterbox", data & 0x80);
		data &= ~0x80; //Strip flutterbox data from meter drives
	}

	MechMtr_update(7, (data & 0x80));
	for (meter = 0; meter < 4; meter ++)
	{
		MechMtr_update(meter, (data & (1 << meter)));
	}
	if (state->m_reel_mux == STANDARD_REEL)
	{
		for (meter = 4; meter < 7; meter ++)
		{
			MechMtr_update(meter, (data & (1 << meter)));
		}
	}
}

/* called if board is reset */
void mpu4_stepper_reset(mpu4_state *state)
{
	int pattern = 0,reel;
	for (reel = 0; reel < 6; reel++)
	{
		stepper_reset_position(reel);
		if(!state->m_reel_mux)
		{
			if (stepper_optic_state(reel)) pattern |= 1<<reel;
		}
	}
	state->m_optic_pattern = pattern;
}


static MACHINE_RESET( mpu4 )
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	ROC10937_reset(0);	/* reset display1 */

	mpu4_stepper_reset(state);

	state->m_lamp_strobe    = 0;
	state->m_lamp_strobe2   = 0;
	state->m_led_strobe     = 0;
	state->m_mmtr_data      = 0;
	state->m_remote_meter   = 0;

	state->m_IC23GC    = 0;
	state->m_IC23GB    = 0;
	state->m_IC23GA    = 0;
	state->m_IC23G1    = 1;
	state->m_IC23G2A   = 0;
	state->m_IC23G2B   = 0;

	state->m_prot_col  = 0;
	state->m_chr_counter    = 0;
	state->m_chr_value		= 0;


	/* init rom bank, some games don't set this, and will assume bank 0,set 0 */
	{
		UINT8 *rom = machine.region("maincpu")->base();

		memory_configure_bank(machine, "bank1", 0, 8, &rom[0x01000], 0x10000);

		memory_set_bank(machine, "bank1",0);
		machine.device("maincpu")->reset();
	}
}


/* 6809 IRQ handler */
WRITE_LINE_DEVICE_HANDLER( cpu0_irq )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	pia6821_device *pia3 = device->machine().device<pia6821_device>("pia_ic3");
	pia6821_device *pia4 = device->machine().device<pia6821_device>("pia_ic4");
	pia6821_device *pia5 = device->machine().device<pia6821_device>("pia_ic5");
	pia6821_device *pia6 = device->machine().device<pia6821_device>("pia_ic6");
	pia6821_device *pia7 = device->machine().device<pia6821_device>("pia_ic7");
	pia6821_device *pia8 = device->machine().device<pia6821_device>("pia_ic8");
	ptm6840_device *ptm2 = device->machine().device<ptm6840_device>("ptm_ic2");

	/* The PIA and PTM IRQ lines are all connected to a common PCB track, leading directly to the 6809 IRQ line. */
	int combined_state = pia3->irq_a_state() | pia3->irq_b_state() |
						 pia4->irq_a_state() | pia4->irq_b_state() |
						 pia5->irq_a_state() | pia5->irq_b_state() |
						 pia6->irq_a_state() | pia6->irq_b_state() |
						 pia7->irq_a_state() | pia7->irq_b_state() |
						 pia8->irq_a_state() | pia8->irq_b_state() |
						 ptm2->irq_state();

	if (!drvstate->m_link7a_connected) //7B = IRQ, 7A = FIRQ, both = NMI
	{
		cputag_set_input_line(device->machine(), "maincpu", M6809_IRQ_LINE, combined_state ? ASSERT_LINE : CLEAR_LINE);
		LOG(("6809 int%d \n", combined_state));
	}
	else
	{
		cputag_set_input_line(device->machine(), "maincpu", M6809_FIRQ_LINE, combined_state ? ASSERT_LINE : CLEAR_LINE);
		LOG(("6809 fint%d \n", combined_state));
	}
}

/* Bankswitching
The MOD 4 ROM cards are set up to handle 8 separate ROM pages, arranged as 2 sets of 4.
The bankswitch selects which of the 4 pages in the set is active, while the bankset
switches between the sets.
It appears that the cards were originally intended to be used in a 'half' page setup,
where the two halves of the ROM space could be mixed and matched as appropriate.
However, there is no evidence to suggest this was ever implemented.
The controls for it exist however, in the form of the Soundboard PIA CB2 pin, which is
used in some cabinets instead of the main control.
*/
static WRITE8_HANDLER( bankswitch_w )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
//  printf("bank %02x\n", data);

	state->m_pageval=(data&0x03);
	memory_set_bank(space->machine(), "bank1",state->m_pageval + (state->m_pageset?4:0));
}


static READ8_HANDLER( bankswitch_r )
{
	return memory_get_bank(space->machine(), "bank1");
}


static WRITE8_HANDLER( bankset_w )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	state->m_pageval=(data - 2);//writes 2 and 3, to represent 0 and 1 - a hangover from the half page design?
	memory_set_bank(space->machine(), "bank1",state->m_pageval + (state->m_pageset?4:0));
}


/* IC2 6840 PTM handler */
static WRITE8_DEVICE_HANDLER( ic2_o1_callback )
{
	downcast<ptm6840_device *>(device)->set_c2(data);

	/* copy output value to IC2 c2
    this output is the clock for timer2 */
	/* 1200Hz System interrupt timer */
}


static WRITE8_DEVICE_HANDLER( ic2_o2_callback )
{
	pia6821_device *pia = device->machine().device<pia6821_device>("pia_ic3");
	pia->ca1_w(data); /* copy output value to IC3 ca1 */
	/* the output from timer2 is the input clock for timer3 */
	/* miscellaneous interrupts generated here */
	downcast<ptm6840_device *>(device)->set_c3(data);
}


static WRITE8_DEVICE_HANDLER( ic2_o3_callback )
{
	/* the output from timer3 is used as a square wave for the alarm output
    and as an external clock source for timer 1! */
	/* also runs lamp fade */
	downcast<ptm6840_device *>(device)->set_c1(data);
}


static const ptm6840_interface ptm_ic2_intf =
{
	MPU4_MASTER_CLOCK / 4,
	{ 0, 0, 0 },
	{ DEVCB_HANDLER(ic2_o1_callback),
	  DEVCB_HANDLER(ic2_o2_callback),
	  DEVCB_HANDLER(ic2_o3_callback) },
	DEVCB_LINE(cpu0_irq)
};


/* 6821 PIA handlers */
/* IC3, lamp data lines + alpha numeric display */
static WRITE8_DEVICE_HANDLER( pia_ic3_porta_w )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	int i;
	LOG_IC3(("%s: IC3 PIA Port A Set to %2x (lamp strobes 1 - 9)\n", device->machine().describe_context(),data));

	if(state->m_ic23_active)
	{
		if (state->m_lamp_strobe != state->m_input_strobe)
		{
			// Because of the nature of the lamping circuit, there is an element of persistance
			// As a consequence, the lamp column data can change before the input strobe without
			// causing the relevant lamps to black out.

			for (i = 0; i < 8; i++)
			{
				output_set_lamp_value((8*state->m_input_strobe)+i, ((data  & (1 << i)) !=0));
			}
			state->m_lamp_strobe = state->m_input_strobe;
		}
	}
}

static WRITE8_DEVICE_HANDLER( pia_ic3_portb_w )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	int i;
	LOG_IC3(("%s: IC3 PIA Port B Set to %2x  (lamp strobes 10 - 17)\n", device->machine().describe_context(),data));

	if(state->m_ic23_active)
	{
		if (state->m_lamp_strobe2 != state->m_input_strobe)
		{
			for (i = 0; i < 8; i++)
			{
				output_set_lamp_value((8*state->m_input_strobe)+i+64, ((data  & (1 << i)) !=0));
			}
			state->m_lamp_strobe2 = state->m_input_strobe;
		}

		if (state->m_led_lamp)
		{
			/* Some games (like Connect 4) use 'programmable' LED displays, built from light display lines in section 2. */
			/* These are mostly low-tech machines, where such wiring proved cheaper than an extender card */
			UINT8 pled_segs[2] = {0,0};

			static const int lamps1[8] = { 106, 107, 108, 109, 104, 105, 110, 133 };
			static const int lamps2[8] = { 114, 115, 116, 117, 112, 113, 118, 119 };

			for (i = 0; i < 8; i++)
			{
				if (output_get_lamp_value(lamps1[i])) pled_segs[0] |= (1 << i);
				if (output_get_lamp_value(lamps2[i])) pled_segs[1] |= (1 << i);
			}

			output_set_digit_value(8,pled_segs[0]);
			output_set_digit_value(9,pled_segs[1]);
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( pia_ic3_ca2_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	LOG_IC3(("%s: IC3 PIA Write CA2 (alpha data), %02X\n", device->machine().describe_context(),state));

	drvstate->m_alpha_data_line = state;
}


static WRITE_LINE_DEVICE_HANDLER( pia_ic3_cb2_w )
{
	LOG_IC3(("%s: IC3 PIA Write CB (alpha reset), %02X\n",device->machine().describe_context(),state));
// DM Data pin A
	if ( !state )
	{
		ROC10937_reset(0);
	}
}


static const pia6821_interface pia_ic3_intf =
{
	DEVCB_NULL,		/* port A in */
	DEVCB_NULL,		/* port B in */
	DEVCB_NULL,		/* line CA1 in */
	DEVCB_NULL,		/* line CB1 in */
	DEVCB_NULL,		/* line CA2 in */
	DEVCB_NULL,		/* line CB2 in */
	DEVCB_HANDLER(pia_ic3_porta_w),		/* port A out */
	DEVCB_HANDLER(pia_ic3_portb_w),		/* port B out */
	DEVCB_LINE(pia_ic3_ca2_w),			/* line CA2 out */
	DEVCB_LINE(pia_ic3_cb2_w),			/* port CB2 out */
	DEVCB_LINE(cpu0_irq),				/* IRQA */
	DEVCB_LINE(cpu0_irq)				/* IRQB */
};


/*
IC23 emulation

IC23 is a 74LS138 1-of-8 Decoder

It is used as a multiplexer for the LEDs, lamp selects and inputs.*/

static void ic23_update(mpu4_state *state)
{
	if (!state->m_IC23G2A)
	{
		if (!state->m_IC23G2B)
		{
			if (state->m_IC23G1)
			{
				if ( state->m_IC23GA ) state->m_input_strobe |= 0x01;
				else				 state->m_input_strobe &= ~0x01;

				if ( state->m_IC23GB ) state->m_input_strobe |= 0x02;
				else		         state->m_input_strobe &= ~0x02;

				if ( state->m_IC23GC ) state->m_input_strobe |= 0x04;
				else				 state->m_input_strobe &= ~0x04;
			}
		}
	}
	else
	if ((state->m_IC23G2A)||(state->m_IC23G2B))
	{
		state->m_input_strobe = 0x00;
	}
}


/*
IC24 emulation

IC24 is a 74LS122 pulse generator

CLEAR and B2 are tied high and A1 and A2 tied low, meaning any pulse
on B1 will give a low pulse on the output pin.
*/
static void ic24_output(mpu4_state *state, int data)
{
	state->m_IC23G2A = data;
	ic23_update(state);
}


static void ic24_setup(mpu4_state *state)
{
	if (state->m_IC23GA)
	{
		double duration = TIME_OF_74LS123((220*1000),(0.1*0.000001));
		{
			state->m_ic23_active=1;
			ic24_output(state, 0);
			state->m_ic24_timer->adjust(attotime::from_double(duration));
		}
	}
}


static TIMER_CALLBACK( ic24_timeout )
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_ic23_active=0;
	ic24_output(state, 1);
}


/* IC4, 7 seg leds, 50Hz timer reel sensors, current sensors */
static WRITE8_DEVICE_HANDLER( pia_ic4_porta_w )
{
	int i;
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	if(state->m_ic23_active)
	{
		if (((state->m_lamp_extender == NO_EXTENDER)||(state->m_lamp_extender == SMALL_CARD)||(state->m_lamp_extender == LARGE_CARD_C))&& (state->m_led_extender == NO_EXTENDER))
		{
			if(state->m_led_strobe != state->m_input_strobe)
			{
				for(i=0; i<8; i++)
				{
					output_set_indexed_value("mpu4led",((7 - state->m_input_strobe) * 8) +i,(data & (1 << i)) !=0);
				}
				output_set_digit_value(7 - state->m_input_strobe,data);
			}
			state->m_led_strobe = state->m_input_strobe;
		}
	}
}

static WRITE8_DEVICE_HANDLER( pia_ic4_portb_w )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	if (state->m_reel_mux)
	{
		/* A write here connects one reel (and only one)
        to the optic test circuit. This allows 8 reels
        to be supported instead of 4. */
		if (state->m_reel_mux == SEVEN_REEL)
		{
			state->m_active_reel= reel_mux_table7[(data >> 4) & 0x07];
		}
		else
		state->m_active_reel= reel_mux_table[(data >> 4) & 0x07];
	}
}

static READ8_DEVICE_HANDLER( pia_ic4_portb_r )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	pia6821_device *pia = downcast<pia6821_device *>(device);
	if ( state->m_serial_data )
	{
		state->m_ic4_input_b |=  0x80;
		pia->cb1_w(1);
	}
	else
	{
		state->m_ic4_input_b &= ~0x80;
		pia->cb1_w(0);
	}

	if (!state->m_reel_mux)
	{
		if ( state->m_optic_pattern & 0x01 ) state->m_ic4_input_b |=  0x40; /* reel A tab */
		else							   state->m_ic4_input_b &= ~0x40;

		if ( state->m_optic_pattern & 0x02 ) state->m_ic4_input_b |=  0x20; /* reel B tab */
		else							   state->m_ic4_input_b &= ~0x20;

		if ( state->m_optic_pattern & 0x04 ) state->m_ic4_input_b |=  0x10; /* reel C tab */
		else							   state->m_ic4_input_b &= ~0x10;

		if ( state->m_optic_pattern & 0x08 ) state->m_ic4_input_b |=  0x08; /* reel D tab */
		else							   state->m_ic4_input_b &= ~0x08;

	}
	else
	{
		if (stepper_optic_state(state->m_active_reel))
		{
			state->m_ic4_input_b |=  0x08;
		}
		else
		{
			state->m_ic4_input_b &= ~0x08;
		}
	}
	if ( state->m_signal_50hz )			state->m_ic4_input_b |=  0x04; /* 50 Hz */
	else								state->m_ic4_input_b &= ~0x04;

	if (state->m_ic4_input_b & 0x02)
	{
		state->m_ic4_input_b &= ~0x02;
	}
	else
	{
		state->m_ic4_input_b |= 0x02; //Pulse the overcurrent line with every read to show the CPU each lamp has lit
	}
	#ifdef UNUSED_FUNCTION
	if ( lamp_undercurrent ) state->m_ic4_input_b |= 0x01;
	#endif

	LOG_IC3(("%s: IC4 PIA Read of Port B %x\n",device->machine().describe_context(),state->m_ic4_input_b));
	return state->m_ic4_input_b;
}


static WRITE_LINE_DEVICE_HANDLER( pia_ic4_ca2_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	LOG_IC3(("%s: IC4 PIA Write CA (input MUX strobe /LED B), %02X\n", device->machine().describe_context(),state));

	drvstate->m_IC23GB = state;
	ic23_update(drvstate);
}

static WRITE_LINE_DEVICE_HANDLER( pia_ic4_cb2_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	LOG_IC3(("%s: IC4 PIA Write CA (input MUX strobe /LED B), %02X\n", device->machine().describe_context(),state));
	drvstate->m_reel_flag=state;
}
static const pia6821_interface pia_ic4_intf =
{
	DEVCB_NULL,		/* port A in */
	DEVCB_HANDLER(pia_ic4_portb_r),	/* port B in */
	DEVCB_NULL,		/* line CA1 in */
	DEVCB_NULL,		/* line CB1 in */
	DEVCB_NULL,		/* line CA2 in */
	DEVCB_NULL,		/* line CB2 in */
	DEVCB_HANDLER(pia_ic4_porta_w),		/* port A out */
	DEVCB_HANDLER(pia_ic4_portb_w),		/* port B out */
	DEVCB_LINE(pia_ic4_ca2_w),		/* line CA2 out */
	DEVCB_LINE(pia_ic4_cb2_w),		/* line CB2 out */
	DEVCB_LINE(cpu0_irq),		/* IRQA */
	DEVCB_LINE(cpu0_irq)		/* IRQB */
};

/* IC5, AUX ports, coin lockouts and AY sound chip select (MODs below 4 only) */
static READ8_DEVICE_HANDLER( pia_ic5_porta_r )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	if (state->m_lamp_extender == LARGE_CARD_A)
	{
		if (state->m_lamp_sense && state->m_ic23_active)
		{
			state->m_aux1_input |= 0x40;
		}
		else
		{
			state->m_aux1_input &= ~0x40; //Pulse the overcurrent line with every read to show the CPU each lamp has lit
		}
	}
	if (state->m_hopper == HOPPER_NONDUART_A)
	{
/*      if (hopper1_active)
        {
            state->m_aux1_input |= 0x04;
        }
        else
        {
            state->m_aux1_input &= ~0x04;
        }*/
	}
	LOG(("%s: IC5 PIA Read of Port A (AUX1)\n",device->machine().describe_context()));

	return input_port_read(device->machine(), "AUX1")|state->m_aux1_input;
}

static WRITE8_DEVICE_HANDLER( pia_ic5_porta_w )
{
	int i;
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	pia6821_device *pia_ic4 = device->machine().device<pia6821_device>("pia_ic4");
	if (state->m_hopper == HOPPER_NONDUART_A)
	{
		//hopper1_drive_sensor(data&0x10);
	}
	switch (state->m_lamp_extender)
	{
		case NO_EXTENDER:
		if (state->m_led_extender == CARD_B)
		{
			led_write_latch(state, data & 0x1f, pia_ic4->a_output(),state->m_input_strobe);
		}
		else if ((state->m_led_extender != CARD_A)||(state->m_led_extender != NO_EXTENDER))
		{
			for(i=0; i<8; i++)
			{
				output_set_indexed_value("mpu4led",((state->m_input_strobe + 8) * 8) +i,(data & (1 << i)) !=0);
			}
			output_set_digit_value((state->m_input_strobe+8),data);
		}
		break;
		case SMALL_CARD:
		if(state->m_ic23_active)
		{
			lamp_extend_small(state,data);
		}
		break;
		case LARGE_CARD_A:
		lamp_extend_large(state,data,state->m_input_strobe,state->m_ic23_active);
		break;
		case LARGE_CARD_B:
		lamp_extend_large(state,data,state->m_input_strobe,state->m_ic23_active);
		if ((state->m_ic23_active) && state->m_card_live)
		{
			for(i=0; i<8; i++)
			{
				output_set_indexed_value("mpu4led",(((8*(state->m_last_b7 >>7))+ state->m_input_strobe) * 8) +i,(~data & (1 << i)) !=0);
			}
			output_set_digit_value(((8*(state->m_last_b7 >>7))+state->m_input_strobe),~data);
		}
		break;
		case LARGE_CARD_C:
		lamp_extend_large(state,data,state->m_input_strobe,state->m_ic23_active);
		break;
	}
	if (state->m_reel_mux == SIX_REEL_5TO8)
	{
		stepper_update(4, data&0x0F);
		stepper_update(5, (data >> 4)&0x0F);
		awp_draw_reel(4);
		awp_draw_reel(5);
	}
	else
	if (state->m_reel_mux == SEVEN_REEL)
	{
		stepper_update(1, data&0x0F);
		stepper_update(2, (data >> 4)&0x0F);
		awp_draw_reel(1);
		awp_draw_reel(2);
	}

		if (mame_stricmp(device->machine().system().name, "m4gambal") == 0)
		{
		/* The 'Gamball' device is a unique piece of mechanical equipment, designed to
        provide a truly fair hi-lo gamble for an AWP machine. Functionally, it consists of
        a ping-pong ball or similar enclosed in the machine's backbox, on a platform with 12
        holes. When the low 4 bytes of AUX1 are triggered, this fires the ball out from the
        hole it's currently in, to land in another. Landing in the same hole causes the machine to
        refire the ball. The ball detection is done by the high 4 bytes of AUX1.
        Here we call the MAME RNG, once to pick a row, once to pick from the four pockets within it. We
        then trigger the switches corresponding to the correct number. This appears to be the best way
        of making the game fair, short of simulating the physics of a bouncing ball ;)*/
		if (data & 0x0f)
		{
			switch (device->machine().rand() & 0x2)
			{
				case 0x00: //Top row
				{
					switch (device->machine().rand() & 0x3)
					{
						case 0x00: //7
						state->m_aux1_input = (state->m_aux1_input & 0x0f);
						state->m_aux1_input|= 0xa0;
						break;
						case 0x01://4
						state->m_aux1_input = (state->m_aux1_input & 0x0f);
						state->m_aux1_input|= 0xb0;
						break;
						case 0x02://9
						state->m_aux1_input = (state->m_aux1_input & 0x0f);
						state->m_aux1_input|= 0xc0;
						break;
						case 0x03://8
						state->m_aux1_input = (state->m_aux1_input & 0x0f);
						state->m_aux1_input|= 0xd0;
						break;
					}
				}
				case 0x01: //Middle row - note switches don't match pattern
				{
					switch (device->machine().rand() & 0x3)
					{
						case 0x00://12
						state->m_aux1_input = (state->m_aux1_input & 0x0f);
						state->m_aux1_input|= 0x40;
						break;
						case 0x01://1
						state->m_aux1_input = (state->m_aux1_input & 0x0f);
						state->m_aux1_input|= 0x50;
						break;
						case 0x02://11
						state->m_aux1_input = (state->m_aux1_input & 0x0f);
						state->m_aux1_input|= 0x80;
						break;
						case 0x03://2
						state->m_aux1_input = (state->m_aux1_input & 0x0f);
						state->m_aux1_input|= 0x90;
						break;
					}
				}
				case 0x02: //Bottom row
				{
					switch (device->machine().rand() & 0x3)
					{
						case 0x00://5
						state->m_aux1_input = (state->m_aux1_input & 0x0f);
						state->m_aux1_input|= 0x00;
						break;
						case 0x01://10
						state->m_aux1_input = (state->m_aux1_input & 0x0f);
						state->m_aux1_input|= 0x10;
						break;
						case 0x02://3
						state->m_aux1_input = (state->m_aux1_input & 0x0f);
						state->m_aux1_input|= 0x20;
						break;
						case 0x03://6
						state->m_aux1_input = (state->m_aux1_input & 0x0f);
						state->m_aux1_input|= 0x30;
						break;
					}
				}
			}
		}
	}
}

static WRITE8_DEVICE_HANDLER( pia_ic5_portb_w )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	if (state->m_hopper == HOPPER_NONDUART_B)
	{
		//hopper1_drive_motor(data &0x01)
		//hopper1_drive_sensor(data &0x08)
	}
	if (state->m_led_extender == CARD_A)
	{
		// led_write_latch(state, data & 0x07, pia_get_output_a(pia_ic4),state->m_input_strobe)
	}

}
READ8_DEVICE_HANDLER( pia_ic5_portb_r )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	pia6821_device *pia_ic5 = device->machine().device<pia6821_device>("pia_ic5");
	if (state->m_hopper == HOPPER_NONDUART_B)
	{/*
        if (hopper1_active)
        {
            state->m_aux2_input |= 0x08;
        }
        else
        {
            state->m_aux2_input &= ~0x08;
        }*/
	}

	LOG(("%s: IC5 PIA Read of Port B (coin input AUX2)\n",device->machine().describe_context()));
	coin_lockout_w(device->machine(), 0, (pia_ic5->b_output() & 0x01) );
	coin_lockout_w(device->machine(), 1, (pia_ic5->b_output() & 0x02) );
	coin_lockout_w(device->machine(), 2, (pia_ic5->b_output() & 0x04) );
	coin_lockout_w(device->machine(), 3, (pia_ic5->b_output() & 0x08) );
	return input_port_read(device->machine(), "AUX2") | state->m_aux2_input;
}


WRITE_LINE_DEVICE_HANDLER( pia_ic5_ca2_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	LOG(("%s: IC5 PIA Write CA2 (Serial Tx) %2x\n",device->machine().describe_context(),state));
	drvstate->m_serial_data = state;
}


/* ---------------------------------------
   AY Chip sound function selection -
   ---------------------------------------
The databus of the AY sound chip is connected to IC6 Port A.
Data is read from/written to the AY chip through this port.

If this sounds familiar, Amstrad did something very similar with their home computers.

The PSG function, defined by the BC1,BC2 and BDIR signals, is controlled by CA2 and CB2 of IC6.

PSG function selection:
-----------------------
BDIR = IC6 CB2 and BC1 = IC6 CA2

Pin            | PSG Function
BDIR BC1       |
0    0         | Inactive
0    1         | Read from selected PSG register. When function is set, the PSG will make the register data available to Port A.
1    0         | Write to selected PSG register. When set, the PSG will take the data at Port A and write it into the selected PSG register.
1    1         | Select PSG register. When set, the PSG will take the data at Port A and select a register.
*/

/* PSG function selected */
static void update_ay(device_t *device)
{
	device_t *ay = device->machine().device("ay8913");
	if (!ay) return;

	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	pia6821_device *pia = downcast<pia6821_device *>(device);
	if (!pia->cb2_output())
	{
		switch (state->m_ay8913_address)
		{
			case 0x00:
			{
				/* Inactive */
				break;
			}
			case 0x01:
			{	/* CA2 = 1 CB2 = 0? : Read from selected PSG register and make the register data available to Port A */
				pia6821_device *pia_ic6 = device->machine().device<pia6821_device>("pia_ic6");
				LOG(("AY8913 address = %d \n",pia_ic6->a_output()&0x0f));
				break;
			}
			case 0x02:
			{/* CA2 = 0 CB2 = 1? : Write to selected PSG register and write data to Port A */
				pia6821_device *pia_ic6 = device->machine().device<pia6821_device>("pia_ic6");
				device_t *ay = device->machine().device("ay8913");
				ay8910_data_w(ay, 0, pia_ic6->a_output());
				LOG(("AY Chip Write \n"));
				break;
			}
			case 0x03:
			{/* CA2 = 1 CB2 = 1? : The register will now be selected and the user can read from or write to it.
             The register will remain selected until another is chosen.*/
				pia6821_device *pia_ic6 = device->machine().device<pia6821_device>("pia_ic6");
				device_t *ay = device->machine().device("ay8913");
				ay8910_address_w(ay, 0, pia_ic6->a_output());
				LOG(("AY Chip Select \n"));
				break;
			}
			default:
			{
				LOG(("AY Chip error \n"));
			}
		}
	}
}


WRITE_LINE_DEVICE_HANDLER( pia_ic5_cb2_w )
{
	update_ay(device);
}


static const pia6821_interface pia_ic5_intf =
{
	DEVCB_HANDLER(pia_ic5_porta_r),		/* port A in */
	DEVCB_HANDLER(pia_ic5_portb_r),	/* port B in */
	DEVCB_NULL,		/* line CA1 in */
	DEVCB_NULL,		/* line CB1 in */
	DEVCB_NULL,		/* line CA2 in */
	DEVCB_NULL,		/* line CB2 in */
	DEVCB_HANDLER(pia_ic5_porta_w),		/* port A out */
	DEVCB_HANDLER(pia_ic5_portb_w),		/* port B out */
	DEVCB_LINE(pia_ic5_ca2_w),		/* line CA2 out */
	DEVCB_LINE(pia_ic5_cb2_w),		/* port CB2 out */
	DEVCB_LINE(cpu0_irq),			/* IRQA */
	DEVCB_LINE(cpu0_irq)			/* IRQB */
};


/* IC6, Reel A and B and AY registers (MODs below 4 only) */
static WRITE8_DEVICE_HANDLER( pia_ic6_portb_w )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	LOG(("%s: IC6 PIA Port B Set to %2x (Reel A and B)\n", device->machine().describe_context(),data));

	if (state->m_reel_mux == SEVEN_REEL)
	{
		stepper_update(3, data&0x0F);
		stepper_update(4, (data >> 4)&0x0F);
		awp_draw_reel(3);
		awp_draw_reel(4);
	}
	else if (state->m_reels)
	{
		stepper_update(0, data & 0x0F );
		stepper_update(1, (data>>4) & 0x0F );
		awp_draw_reel(0);
		awp_draw_reel(1);
	}

	if (state->m_reel_flag && (state->m_reel_mux == STANDARD_REEL) && state->m_reels)
	{
		if ( stepper_optic_state(0) ) state->m_optic_pattern |=  0x01;
		else                          state->m_optic_pattern &= ~0x01;

		if ( stepper_optic_state(1) ) state->m_optic_pattern |=  0x02;
		else                          state->m_optic_pattern &= ~0x02;
	}
}


static WRITE8_DEVICE_HANDLER( pia_ic6_porta_w )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	LOG(("%s: IC6 PIA Write A %2x\n", device->machine().describe_context(),data));
	if (state->m_mod_number <4)
	{
		state->m_ay_data = data;
		update_ay(device);
	}
}


static WRITE_LINE_DEVICE_HANDLER( pia_ic6_ca2_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	LOG(("%s: IC6 PIA write CA2 %2x (AY8913 BC1)\n", device->machine().describe_context(),state));
	if (drvstate->m_mod_number <4)
	{
		if ( state ) drvstate->m_ay8913_address |=  0x01;
		else         drvstate->m_ay8913_address &= ~0x01;
		update_ay(device);
	}
}


static WRITE_LINE_DEVICE_HANDLER( pia_ic6_cb2_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	LOG(("%s: IC6 PIA write CB2 %2x (AY8913 BCDIR)\n", device->machine().describe_context(),state));
	if (drvstate->m_mod_number <4)
	{
		if ( state ) drvstate->m_ay8913_address |=  0x02;
		else         drvstate->m_ay8913_address &= ~0x02;
		update_ay(device);
	}
}


static const pia6821_interface pia_ic6_intf =
{
	DEVCB_NULL,		/* port A in */
	DEVCB_NULL,		/* port B in */
	DEVCB_NULL,		/* line CA1 in */
	DEVCB_NULL,		/* line CB1 in */
	DEVCB_NULL,		/* line CA2 in */
	DEVCB_NULL,		/* line CB2 in */
	DEVCB_HANDLER(pia_ic6_porta_w),		/* port A out */
	DEVCB_HANDLER(pia_ic6_portb_w),		/* port B out */
	DEVCB_LINE(pia_ic6_ca2_w),			/* line CA2 out */
	DEVCB_LINE(pia_ic6_cb2_w),			/* port CB2 out */
	DEVCB_LINE(cpu0_irq),				/* IRQA */
	DEVCB_LINE(cpu0_irq)				/* IRQB */
};


/* IC7 Reel C and D, mechanical meters/Reel E and F, input strobe bit A */
static WRITE8_DEVICE_HANDLER( pia_ic7_porta_w )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	LOG(("%s: IC7 PIA Port A Set to %2x (Reel C and D)\n", device->machine().describe_context(),data));
	if (state->m_reel_mux == SEVEN_REEL)
	{
		stepper_update(5, data&0x0F);
		stepper_update(6, (data >> 4)&0x0F);
		awp_draw_reel(5);
		awp_draw_reel(6);
	}
	else if (state->m_reels)
	{
		stepper_update(2, data & 0x0F );
		stepper_update(3, (data>>4) & 0x0F );
		awp_draw_reel(2);
		awp_draw_reel(3);
	}

	if (state->m_reel_flag && (state->m_reel_mux == STANDARD_REEL) && state->m_reels)
	{
		if ( stepper_optic_state(2) ) state->m_optic_pattern |=  0x04;
		else                          state->m_optic_pattern &= ~0x04;
		if ( stepper_optic_state(3) ) state->m_optic_pattern |=  0x08;
		else                          state->m_optic_pattern &= ~0x08;
	}
}

static WRITE8_DEVICE_HANDLER( pia_ic7_portb_w )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	if (state->m_hopper == HOPPER_DUART_A)
	{
		//duart write data
	}
	else if (state->m_hopper == HOPPER_NONDUART_A)
	{
		//hoppr1_drive_motor(data & 0x10);
	}

    state->m_mmtr_data = data;
}

static READ8_DEVICE_HANDLER( pia_ic7_portb_r )
{
/* The meters are connected to a voltage drop sensor, where current
flowing through them also passes through pin B7, meaning that when
any meter is activated, pin B7 goes high.
As for why they connected this to an output port rather than using
CB1, no idea, although it proved of benefit when the reel multiplexer was designed
as it allows a separate meter to be used when the rest of the port is blocked.
This appears to have confounded the schematic drawer, who has assumed that
all eight meters are driven from this port, giving the 8 line driver chip
9 connections in total. */

	//This may be overkill, but the meter sensing is VERY picky

	int combined_meter = MechMtr_GetActivity(0) | MechMtr_GetActivity(1) |
						 MechMtr_GetActivity(2) | MechMtr_GetActivity(3) |
						 MechMtr_GetActivity(4) | MechMtr_GetActivity(5) |
						 MechMtr_GetActivity(6) | MechMtr_GetActivity(7);

	if(combined_meter)
	{
		return 0x80;
	}
	else
	{
		return 0x00;
	}
}







static WRITE_LINE_DEVICE_HANDLER( pia_ic7_ca2_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	LOG(("%s: IC7 PIA write CA2 %2x (input strobe bit 0 / LED A)\n", device->machine().describe_context(),state));

	drvstate->m_IC23GA = state;
	ic24_setup(drvstate);
	ic23_update(drvstate);
}

static WRITE_LINE_DEVICE_HANDLER( pia_ic7_cb2_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	drvstate->m_remote_meter = state?0x80:0x00;
}

static const pia6821_interface pia_ic7_intf =
{
	DEVCB_NULL,		/* port A in */
	DEVCB_HANDLER(pia_ic7_portb_r),		/* port B in */
	DEVCB_NULL,		/* line CA1 in */
	DEVCB_NULL,		/* line CB1 in */
	DEVCB_NULL,		/* line CA2 in */
	DEVCB_NULL,		/* line CB2 in */
	DEVCB_HANDLER(pia_ic7_porta_w),		/* port A out */
	DEVCB_HANDLER(pia_ic7_portb_w),		/* port B out */
	DEVCB_LINE(pia_ic7_ca2_w),			/* line CA2 out */
	DEVCB_LINE(pia_ic7_cb2_w),			/* line CB2 out */
	DEVCB_LINE(cpu0_irq),				/* IRQA */
	DEVCB_LINE(cpu0_irq)				/* IRQB */
};


/* IC8, Inputs, TRIACS, alpha clock */
static READ8_DEVICE_HANDLER( pia_ic8_porta_r )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	static const char *const portnames[] = { "ORANGE1", "ORANGE2", "BLACK1", "BLACK2", "ORANGE1", "ORANGE2", "DIL1", "DIL2" };
	pia6821_device *pia_ic5 = device->machine().device<pia6821_device>("pia_ic5");

	LOG_IC8(("%s: IC8 PIA Read of Port A (MUX input data)\n", device->machine().describe_context()));
/* The orange inputs are polled twice as often as the black ones, for reasons of efficiency.
   This is achieved via connecting every input line to an AND gate, thus allowing two strobes
   to represent each orange input bank (strobes are active low). */
	pia_ic5->cb1_w(input_port_read(device->machine(), "AUX2") & 0x80);
	return input_port_read(device->machine(), portnames[state->m_input_strobe]);
}


static WRITE8_DEVICE_HANDLER( pia_ic8_portb_w )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	if (state->m_hopper == HOPPER_DUART_B)
	{
//      duart.drive_sensor(data & 0x04, data & 0x01, 0, 0);
	}
	else if (state->m_hopper == HOPPER_DUART_C)
	{
//      duart.drive_sensor(data & 0x04, data & 0x01, data & 0x04, data & 0x02);
	}
	int i;
	LOG_IC8(("%s: IC8 PIA Port B Set to %2x (OUTPUT PORT, TRIACS)\n", device->machine().describe_context(),data));
	for (i = 0; i < 8; i++)
	{
		output_set_indexed_value("triac", i, data & (1 << i));
	}
}

static WRITE_LINE_DEVICE_HANDLER( pia_ic8_ca2_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	LOG_IC8(("%s: IC8 PIA write CA2 (input_strobe bit 2 / LED C) %02X\n", device->machine().describe_context(), state & 0xFF));

	drvstate->m_IC23GC = state;
	ic23_update(drvstate);
}


static WRITE_LINE_DEVICE_HANDLER( pia_ic8_cb2_w )
{
	mpu4_state *drvstate = device->machine().driver_data<mpu4_state>();
	LOG_IC8(("%s: IC8 PIA write CB2 (alpha clock) %02X\n", device->machine().describe_context(), state & 0xFF));

	// DM Data pin B
	if (drvstate->m_alpha_clock != state)
	{
		if (!drvstate->m_alpha_clock)//falling edge
		{
			ROC10937_shift_data(0, drvstate->m_alpha_data_line?0:1);
		}
	}
	drvstate->m_alpha_clock = state;
	ROC10937_draw_16seg(0);
}


static const pia6821_interface pia_ic8_intf =
{
	DEVCB_HANDLER(pia_ic8_porta_r),		/* port A in */
	DEVCB_NULL,		/* port B in */
	DEVCB_NULL,		/* line CA1 in */
	DEVCB_NULL,		/* line CB1 in */
	DEVCB_NULL,		/* line CA2 in */
	DEVCB_NULL,		/* line CB2 in */
	DEVCB_NULL,		/* port A out */
	DEVCB_HANDLER(pia_ic8_portb_w),		/* port B out */
	DEVCB_LINE(pia_ic8_ca2_w),			/* line CA2 out */
	DEVCB_LINE(pia_ic8_cb2_w),			/* port CB2 out */
	DEVCB_LINE(cpu0_irq),				/* IRQA */
	DEVCB_LINE(cpu0_irq)				/* IRQB */
};

// universal sampled sound program card PCB 683077
// Sampled sound card, using a PIA and PTM for timing and data handling
static WRITE8_DEVICE_HANDLER( pia_gb_porta_w )
{
	device_t *msm6376 = device->machine().device("msm6376");
	LOG_SS(("%s: GAMEBOARD: PIA Port A Set to %2x\n", device->machine().describe_context(),data));
	okim6376_w(msm6376, 0, data);
}

static WRITE8_DEVICE_HANDLER( pia_gb_portb_w )
{
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	device_t *msm6376 = device->machine().device("msm6376");
	okim6376_device *msm = device->machine().device<okim6376_device>("msm6376");

	int changed = state->m_expansion_latch^data;

	LOG_SS(("%s: GAMEBOARD: PIA Port B Set to %2x\n", device->machine().describe_context(),data));

	if ( changed & 0x20)
	{ // digital volume clock line changed
		if ( !(data & 0x20) )
		{ // changed from high to low,
			if ( !(data & 0x10) )//down
			{
				if ( state->m_global_volume < 32 ) state->m_global_volume++; //steps unknown
			}
			else
			{//up
				if ( state->m_global_volume > 0  ) state->m_global_volume--;
			}

			{
				float percent = (32-state->m_global_volume)/32.0;
				msm->set_output_gain(0, percent);
				msm->set_output_gain(1, percent);
			}
		}
	}
	okim6376_ch2_w(msm6376,data&0x02);
	okim6376_st_w(msm6376,data&0x01);
}
static READ8_DEVICE_HANDLER( pia_gb_portb_r )
{
	device_t *msm6376 = device->machine().device("msm6376");
	mpu4_state *state = device->machine().driver_data<mpu4_state>();
	LOG_SS(("%s: GAMEBOARD: PIA Read of Port B\n",device->machine().describe_context()));
	int data=0;
	// b7 NAR - we can load another address into Channel 1
	// b6, 1 = OKI ready, 0 = OKI busy
	// b5, vol clock
	// b4, 1 = Vol down, 0 = Vol up
	//

	if ( okim6376_nar_r(msm6376) ) data |= 0x80;
	else							data &= ~0x80;

	if ( okim6376_busy_r(msm6376) )	data |= 0x40;
	else							data &= ~0x40;

	return ( data | state->m_expansion_latch );
}

static WRITE_LINE_DEVICE_HANDLER( pia_gb_ca2_w )
{
	LOG_SS(("%s: GAMEBOARD: OKI RESET data = %02X\n", device->machine().describe_context(), state));

//  reset line
}

static WRITE_LINE_DEVICE_HANDLER( pia_gb_cb2_w )
{
	mpu4_state *mstate = device->machine().driver_data<mpu4_state>();
	//Some BWB games use this to drive the bankswitching
	if (mstate->m_bwb_bank)
	{
		mstate->m_pageval=state;

		memory_set_bank(device->machine(), "bank1",mstate->m_pageval + (mstate->m_pageset?4:0));
	}
}

static const pia6821_interface pia_ic4ss_intf =
{
	DEVCB_NULL,		/* port A in */
	DEVCB_HANDLER(pia_gb_portb_r),	/* port B in */
	DEVCB_NULL,		/* line CA1 in */
	DEVCB_NULL,		/* line CB1 in */
	DEVCB_NULL,		/* line CA2 in */
	DEVCB_NULL,		/* line CB2 in */
	DEVCB_HANDLER(pia_gb_porta_w),		/* port A out */
	DEVCB_HANDLER(pia_gb_portb_w),		/* port B out */
	DEVCB_LINE(pia_gb_ca2_w),		/* line CA2 out */
	DEVCB_LINE(pia_gb_cb2_w),		/* line CB2 out */
	DEVCB_NULL,		/* IRQA */
	DEVCB_NULL		/* IRQB */
};

//Sampled sound timer
/*
The MSM6376 sound chip is configured in a slightly strange way, to enable dynamic
sample rate changes (8Khz, 10.6 Khz, 16 KHz) by varying the clock.
According to the BwB programmer's guide, the formula is:
MSM6376 clock frequency:-
freq = (1720000/((t3L+1)(t3H+1)))*[(t3H(T3L+1)+1)/(2(t1+1))]
where [] means rounded up integer,
t3L is the LSB of Clock 3,
t3H is the MSB of Clock 3,
and t1 is the initial value in clock 1.
*/

//O3 -> G1  O1 -> c2 o2 -> c1
static WRITE8_DEVICE_HANDLER( ic3ss_o1_callback )
{
	downcast<ptm6840_device *>(device)->set_c2(data);
}


static WRITE8_DEVICE_HANDLER( ic3ss_o2_callback )//Generates 'beep' tone
{
	downcast<ptm6840_device *>(device)->set_c1(data);//?
}


static WRITE8_DEVICE_HANDLER( ic3ss_o3_callback )
{
	//downcast<ptm6840_device *>(device)->set_g1(data); /* this output is the clock for timer1 */
}

/* This is a bit of a cheat - since we don't clock into the OKI chip directly, we need to
calculate the oscillation frequency in advance. We're running the timer for interrupt
purposes, but the frequency calculation is done by plucking the values out as they are written.*/
WRITE8_HANDLER( ic3ss_w )
{
	device_t *ic3ss = space->machine().device("ptm_ic3ss");
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	downcast<ptm6840_device *>(ic3ss)->write(offset,data);
	device_t *msm6376 = space->machine().device("msm6376");

	if (offset == 3)
	{
		state->m_t1 = data;
	}
	if (offset == 6)
	{
		state->m_t3h = data;
	}
	if (offset == 7)
	{
		state->m_t3l = data;
	}

	float num = (1720000/((state->m_t3l + 1)*(state->m_t3h + 1)));
	float denom1 = ((state->m_t3h *(state->m_t3l + 1)+ 1)/(2*(state->m_t1 + 1)));

	int denom2 = denom1 +0.5;//need to round up, this gives same precision as chip
	int freq=num*denom2;

	if (freq)
	{
		okim6376_set_frequency(msm6376, freq);
	}
}


static const ptm6840_interface ptm_ic3ss_intf =
{
	MPU4_MASTER_CLOCK / 4,
	{ 0, 0, 0 },
	{ DEVCB_HANDLER(ic3ss_o1_callback),
	  DEVCB_HANDLER(ic3ss_o2_callback),
	  DEVCB_HANDLER(ic3ss_o3_callback) },
	DEVCB_NULL//LINE(cpu1_ptm_irq)
};

/* input ports for MPU4 board */
INPUT_PORTS_START( mpu4 )
	PORT_START("ORANGE1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("00")//  20p level
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("01")// 100p level
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("02")// Token 1 level
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("03")// Token 2 level
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("04")
	PORT_CONFNAME( 0xE0, 0x00, "Stake Key" )
	PORT_CONFSETTING(    0x00, "Not fitted / 5p"  )
	PORT_CONFSETTING(    0x20, "10p" )
	PORT_CONFSETTING(    0x40, "20p" )
	PORT_CONFSETTING(    0x60, "25p" )
	PORT_CONFSETTING(    0x80, "30p" )
	PORT_CONFSETTING(    0xA0, "40p" )
	PORT_CONFSETTING(    0xC0, "50p" )
	PORT_CONFSETTING(    0xE0, "1 GBP" )

	PORT_START("ORANGE2")
	PORT_CONFNAME( 0x0F, 0x00, "Jackpot / Prize Key" )
	PORT_CONFSETTING(    0x00, "Not fitted"  )
	PORT_CONFSETTING(    0x01, "3 GBP"  )
	PORT_CONFSETTING(    0x02, "4 GBP"  )
	PORT_CONFSETTING(    0x08, "5 GBP"  )
	PORT_CONFSETTING(    0x03, "6 GBP"  )
	PORT_CONFSETTING(    0x04, "6 GBP Token"  )
	PORT_CONFSETTING(    0x05, "8 GBP"  )
	PORT_CONFSETTING(    0x06, "8 GBP Token"  )
	PORT_CONFSETTING(    0x07, "10 GBP"  )
	PORT_CONFSETTING(    0x09, "15 GBP"  )
	PORT_CONFSETTING(    0x0A, "25 GBP"  )
	PORT_CONFSETTING(    0x0B, "25 GBP (Licensed Betting Office Profile)"  )
	PORT_CONFSETTING(    0x0C, "35 GBP"  )
	PORT_CONFSETTING(    0x0D, "70 GBP"  )
	PORT_CONFSETTING(    0x0E, "Reserved"  )
	PORT_CONFSETTING(    0x0F, "Reserved"  )

	PORT_CONFNAME( 0xF0, 0x00, "Percentage Key" )
	PORT_CONFSETTING(    0x00, "Not fitted / 68% (Invalid for UK Games)"  )
	PORT_CONFSETTING(    0x10, "70" )
	PORT_CONFSETTING(    0x20, "72" )
	PORT_CONFSETTING(    0x30, "74" )
	PORT_CONFSETTING(    0x40, "76" )
	PORT_CONFSETTING(    0x50, "78" )
	PORT_CONFSETTING(    0x60, "80" )
	PORT_CONFSETTING(    0x70, "82" )
	PORT_CONFSETTING(    0x80, "84" )
	PORT_CONFSETTING(    0x90, "86" )
	PORT_CONFSETTING(    0xA0, "88" )
	PORT_CONFSETTING(    0xB0, "90" )
	PORT_CONFSETTING(    0xC0, "92" )
	PORT_CONFSETTING(    0xD0, "94" )
	PORT_CONFSETTING(    0xE0, "96" )
	PORT_CONFSETTING(    0xF0, "98" )

	PORT_START("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Hi")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Lo")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER)   PORT_NAME("18")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER)   PORT_NAME("19")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER)	PORT_NAME("20")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Button") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_INTERLOCK) PORT_NAME("Cashbox (Back) Door")  PORT_CODE(KEYCODE_Q) PORT_TOGGLE

	PORT_START("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("24")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("25")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Cancel")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Hold 1")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("Hold 2")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME("Hold 3")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_BUTTON7) PORT_NAME("Hold 4")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_START1)

	PORT_START("DIL1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("DIL1:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("DIL1:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("DIL1:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("DIL1:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0xF0, 0x00, "Target Percentage (if key not fitted)" )PORT_DIPLOCATION("DIL1:05,06,07,08")
	PORT_DIPSETTING(    0x00, "Unset (Program Optimum)"  )
	PORT_DIPSETTING(    0x10, "70" )
	PORT_DIPSETTING(    0x20, "72" )
	PORT_DIPSETTING(    0x30, "74" )
	PORT_DIPSETTING(    0x40, "76" )
	PORT_DIPSETTING(    0x50, "78" )
	PORT_DIPSETTING(    0x60, "80" )
	PORT_DIPSETTING(    0x70, "82" )
	PORT_DIPSETTING(    0x80, "84" )
	PORT_DIPSETTING(    0x90, "86" )
	PORT_DIPSETTING(    0xA0, "88" )
	PORT_DIPSETTING(    0xB0, "90" )
	PORT_DIPSETTING(    0xC0, "92" )
	PORT_DIPSETTING(    0xD0, "94" )
	PORT_DIPSETTING(    0xE0, "96" )
	PORT_DIPSETTING(    0xF0, "98" )

	PORT_START("DIL2")
	PORT_DIPNAME( 0x01, 0x00, "Token Lockout when full" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused )) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "Scottish Coin Handling" ) PORT_DIPLOCATION("DIL2:03")//20p payout
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x08, "Out of Credit Display Inhibit" ) PORT_DIPLOCATION("DIL2:04")  // many games need this on to boot
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "OCD Audio Enable" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "Coin Alarm Inhibit" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "Token Refill Level Inhibit" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "Single Credit Entry" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("0")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("2")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("3")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("4")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("5")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("6")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("7")

	PORT_START("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")PORT_IMPULSE(5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")PORT_IMPULSE(5)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")PORT_IMPULSE(5)
INPUT_PORTS_END

INPUT_PORTS_START( mpu4jackpot8tkn )
	PORT_INCLUDE( mpu4 )

	PORT_MODIFY("ORANGE2")
	PORT_CONFNAME( 0x0F, 0x06, "Jackpot / Prize Key" )
	PORT_CONFSETTING(    0x00, "Not fitted"  )
	PORT_CONFSETTING(    0x01, "3 GBP"  )
	PORT_CONFSETTING(    0x02, "4 GBP"  )
	PORT_CONFSETTING(    0x08, "5 GBP"  )
	PORT_CONFSETTING(    0x03, "6 GBP"  )
	PORT_CONFSETTING(    0x04, "6 GBP Token"  )
	PORT_CONFSETTING(    0x05, "8 GBP"  )
	PORT_CONFSETTING(    0x06, "8 GBP Token"  )
	PORT_CONFSETTING(    0x07, "10 GBP"  )
	PORT_CONFSETTING(    0x09, "15 GBP"  )
	PORT_CONFSETTING(    0x0A, "25 GBP"  )
	PORT_CONFSETTING(    0x0B, "25 GBP (Licensed Betting Office Profile)"  )
	PORT_CONFSETTING(    0x0C, "35 GBP"  )
	PORT_CONFSETTING(    0x0D, "70 GBP"  )
	PORT_CONFSETTING(    0x0E, "Reserved"  )
	PORT_CONFSETTING(    0x0F, "Reserved"  )
INPUT_PORTS_END

INPUT_PORTS_START( mpu4jackpot8per )
	PORT_INCLUDE( mpu4 )

	PORT_MODIFY("ORANGE2")
	PORT_CONFNAME( 0x0F, 0x06, "Jackpot / Prize Key" )
	PORT_CONFSETTING(    0x00, "Not fitted"  )
	PORT_CONFSETTING(    0x01, "3 GBP"  )
	PORT_CONFSETTING(    0x02, "4 GBP"  )
	PORT_CONFSETTING(    0x08, "5 GBP"  )
	PORT_CONFSETTING(    0x03, "6 GBP"  )
	PORT_CONFSETTING(    0x04, "6 GBP Token"  )
	PORT_CONFSETTING(    0x05, "8 GBP"  )
	PORT_CONFSETTING(    0x06, "8 GBP Token"  )
	PORT_CONFSETTING(    0x07, "10 GBP"  )
	PORT_CONFSETTING(    0x09, "15 GBP"  )
	PORT_CONFSETTING(    0x0A, "25 GBP"  )
	PORT_CONFSETTING(    0x0B, "25 GBP (Licensed Betting Office Profile)"  )
	PORT_CONFSETTING(    0x0C, "35 GBP"  )
	PORT_CONFSETTING(    0x0D, "70 GBP"  )
	PORT_CONFSETTING(    0x0E, "Reserved"  )
	PORT_CONFSETTING(    0x0F, "Reserved"  )

	PORT_CONFNAME( 0xF0, 0x10, "Percentage Key" )
	PORT_CONFSETTING(    0x00, "Not fitted / 68% (Invalid for UK Games)"  )
	PORT_CONFSETTING(    0x10, "70" )
	PORT_CONFSETTING(    0x20, "72" )
	PORT_CONFSETTING(    0x30, "74" )
	PORT_CONFSETTING(    0x40, "76" )
	PORT_CONFSETTING(    0x50, "78" )
	PORT_CONFSETTING(    0x60, "80" )
	PORT_CONFSETTING(    0x70, "82" )
	PORT_CONFSETTING(    0x80, "84" )
	PORT_CONFSETTING(    0x90, "86" )
	PORT_CONFSETTING(    0xA0, "88" )
	PORT_CONFSETTING(    0xB0, "90" )
	PORT_CONFSETTING(    0xC0, "92" )
	PORT_CONFSETTING(    0xD0, "94" )
	PORT_CONFSETTING(    0xE0, "96" )
	PORT_CONFSETTING(    0xF0, "98" )
INPUT_PORTS_END


static INPUT_PORTS_START( connect4 )
	PORT_START("ORANGE1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("00")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("01")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("02")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("03")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("04")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("05")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("06")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("07")

	PORT_START("ORANGE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("08")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("09")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("10")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("11")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("12")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("13")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("14")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("15")

	PORT_START("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("16")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("17")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("18")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("19")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("20")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Switch")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Door Switch?") PORT_TOGGLE

	PORT_START("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Select")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("25")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_START2) PORT_NAME("Pass")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Play")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("28")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("29")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("30")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Drop")

	PORT_START("DIL1")
	PORT_DIPNAME( 0x80, 0x00, "DIL101" ) PORT_DIPLOCATION("DIL1:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL102" ) PORT_DIPLOCATION("DIL1:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL103" ) PORT_DIPLOCATION("DIL1:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL104" ) PORT_DIPLOCATION("DIL1:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL105" ) PORT_DIPLOCATION("DIL1:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL106" ) PORT_DIPLOCATION("DIL1:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL107" ) PORT_DIPLOCATION("DIL1:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x01, 0x00, "DIL108" ) PORT_DIPLOCATION("DIL1:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )

	PORT_START("DIL2")
	PORT_DIPNAME( 0x80, 0x00, "DIL201" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL202" ) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL203" ) PORT_DIPLOCATION("DIL2:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL204" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL205" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL206" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL207" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x01, 0x00, "DIL208" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )

	PORT_START("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("0")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("2")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("3")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("4")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("5")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("6")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("7")

	PORT_START("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")PORT_IMPULSE(5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")PORT_IMPULSE(5)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")PORT_IMPULSE(5)
INPUT_PORTS_END

static INPUT_PORTS_START( gamball )
	PORT_START("ORANGE1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("00")//  20p level
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("01")// 100p level
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("02")// Token 1 level
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("03")// Token 2 level
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("04")
	PORT_CONFNAME( 0xE0, 0x00, "Stake Key" )
	PORT_CONFSETTING(    0x00, "Not fitted / 5p"  )
	PORT_CONFSETTING(    0x20, "10p" )
	PORT_CONFSETTING(    0x40, "20p" )
	PORT_CONFSETTING(    0x60, "25p" )
	PORT_CONFSETTING(    0x80, "30p" )
	PORT_CONFSETTING(    0xA0, "40p" )
	PORT_CONFSETTING(    0xC0, "50p" )
	PORT_CONFSETTING(    0xE0, "1 GBP" )

	PORT_START("ORANGE2")
	PORT_CONFNAME( 0x0F, 0x00, "Jackpot / Prize Key" )
	PORT_CONFSETTING(    0x00, "Not fitted"  )
	PORT_CONFSETTING(    0x01, "3 GBP"  )
	PORT_CONFSETTING(    0x02, "4 GBP"  )
	PORT_CONFSETTING(    0x08, "5 GBP"  )
	PORT_CONFSETTING(    0x03, "6 GBP"  )
	PORT_CONFSETTING(    0x04, "6 GBP Token"  )
	PORT_CONFSETTING(    0x05, "8 GBP"  )
	PORT_CONFSETTING(    0x06, "8 GBP Token"  )
	PORT_CONFSETTING(    0x07, "10 GBP"  )
	PORT_CONFSETTING(    0x09, "15 GBP"  )
	PORT_CONFSETTING(    0x0A, "25 GBP"  )
	PORT_CONFSETTING(    0x0B, "25 GBP (Licensed Betting Office Profile)"  )
	PORT_CONFSETTING(    0x0C, "35 GBP"  )
	PORT_CONFSETTING(    0x0D, "70 GBP"  )
	PORT_CONFSETTING(    0x0E, "Reserved"  )
	PORT_CONFSETTING(    0x0F, "Reserved"  )

	PORT_CONFNAME( 0xF0, 0x00, "Percentage Key" )
	PORT_CONFSETTING(    0x00, "As Option Switches"  )
	PORT_CONFSETTING(    0x10, "70" )
	PORT_CONFSETTING(    0x20, "72" )
	PORT_CONFSETTING(    0x30, "74" )
	PORT_CONFSETTING(    0x40, "76" )
	PORT_CONFSETTING(    0x50, "78" )
	PORT_CONFSETTING(    0x60, "80" )
	PORT_CONFSETTING(    0x70, "82" )
	PORT_CONFSETTING(    0x80, "84" )
	PORT_CONFSETTING(    0x90, "86" )
	PORT_CONFSETTING(    0xA0, "88" )
	PORT_CONFSETTING(    0xB0, "90" )
	PORT_CONFSETTING(    0xC0, "92" )
	PORT_CONFSETTING(    0xD0, "94" )
	PORT_CONFSETTING(    0xE0, "96" )
	PORT_CONFSETTING(    0xF0, "98" )

	PORT_START("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Hi")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Lo")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("18")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("19")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("20")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Button") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_INTERLOCK) PORT_NAME("Cashbox Door")  PORT_CODE(KEYCODE_Q) PORT_TOGGLE

	PORT_START("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("24")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("25")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Cancel/Collect")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Hold/Nudge 1")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("Hold/Nudge 2")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME("Hold/Nudge 3")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_BUTTON7) PORT_NAME("Hold/Nudge 4")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_START1)

	PORT_START("DIL1")
	PORT_DIPNAME( 0x80, 0x00, "DIL101" ) PORT_DIPLOCATION("DIL1:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL102" ) PORT_DIPLOCATION("DIL1:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL103" ) PORT_DIPLOCATION("DIL1:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL104" ) PORT_DIPLOCATION("DIL1:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL105" ) PORT_DIPLOCATION("DIL1:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL106" ) PORT_DIPLOCATION("DIL1:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL107" ) PORT_DIPLOCATION("DIL1:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x01, 0x00, "DIL108" ) PORT_DIPLOCATION("DIL1:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )

	PORT_START("DIL2")
	PORT_DIPNAME( 0x80, 0x00, "DIL201" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "DIL202" ) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "DIL203" ) PORT_DIPLOCATION("DIL2:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "DIL204" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "DIL205" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "DIL206" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, "DIL207" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x01, 0x00, "DIL208" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )

	PORT_START("AUX1")
	PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_SPECIAL)//Handled by Gamball unit

	PORT_START("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")PORT_IMPULSE(5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")PORT_IMPULSE(5)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")PORT_IMPULSE(5)
INPUT_PORTS_END

static INPUT_PORTS_START( grtecp )
	PORT_START("ORANGE1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("00")//  20p level
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("01")// 100p level
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("02")// Token 1 level
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("03")// Token 2 level
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("04")
	PORT_CONFNAME( 0xE0, 0x00, "Stake Key" )
	PORT_CONFSETTING(    0x00, "Not fitted / 5p"  )
	PORT_CONFSETTING(    0x20, "10p" )
	PORT_CONFSETTING(    0x40, "20p" )
	PORT_CONFSETTING(    0x60, "25p" )
	PORT_CONFSETTING(    0x80, "30p" )
	PORT_CONFSETTING(    0xA0, "40p" )
	PORT_CONFSETTING(    0xC0, "50p" )
	PORT_CONFSETTING(    0xE0, "1 GBP" )

	PORT_START("ORANGE2")
	PORT_CONFNAME( 0x0F, 0x00, "Jackpot / Prize Key" )
	PORT_CONFSETTING(    0x00, "Not fitted"  )
	PORT_CONFSETTING(    0x01, "3 GBP"  )
	PORT_CONFSETTING(    0x02, "4 GBP"  )
	PORT_CONFSETTING(    0x08, "5 GBP"  )
	PORT_CONFSETTING(    0x03, "6 GBP"  )
	PORT_CONFSETTING(    0x04, "6 GBP Token"  )
	PORT_CONFSETTING(    0x05, "8 GBP"  )
	PORT_CONFSETTING(    0x06, "8 GBP Token"  )
	PORT_CONFSETTING(    0x07, "10 GBP"  )
	PORT_CONFSETTING(    0x09, "15 GBP"  )
	PORT_CONFSETTING(    0x0A, "25 GBP"  )
	PORT_CONFSETTING(    0x0B, "25 GBP (Licensed Betting Office Profile)"  )
	PORT_CONFSETTING(    0x0C, "35 GBP"  )
	PORT_CONFSETTING(    0x0D, "70 GBP"  )
	PORT_CONFSETTING(    0x0E, "Reserved"  )
	PORT_CONFSETTING(    0x0F, "Reserved"  )

	PORT_CONFNAME( 0xF0, 0x00, "Percentage Key" )
	PORT_CONFSETTING(    0x00, "As Option Switches"  )
	PORT_CONFSETTING(    0x10, "70" )
	PORT_CONFSETTING(    0x20, "72" )
	PORT_CONFSETTING(    0x30, "74" )
	PORT_CONFSETTING(    0x40, "76" )
	PORT_CONFSETTING(    0x50, "78" )
	PORT_CONFSETTING(    0x60, "80" )
	PORT_CONFSETTING(    0x70, "82" )
	PORT_CONFSETTING(    0x80, "84" )
	PORT_CONFSETTING(    0x90, "86" )
	PORT_CONFSETTING(    0xA0, "88" )
	PORT_CONFSETTING(    0xB0, "90" )
	PORT_CONFSETTING(    0xC0, "92" )
	PORT_CONFSETTING(    0xD0, "94" )
	PORT_CONFSETTING(    0xE0, "96" )
	PORT_CONFSETTING(    0xF0, "98" )

	PORT_START("BLACK1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Test Button") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_INTERLOCK) PORT_NAME("Cashbox (Back) Door") PORT_CODE(KEYCODE_Q) PORT_TOGGLE

	PORT_START("BLACK2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Collect/Cancel")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Hold 1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Hold 2")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("Hold 3")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("Hi")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME("Lo")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_BUTTON7) PORT_NAME("Exchange")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_START1)

	PORT_START("DIL1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("DIL1:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("DIL1:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("DIL1:03")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("DIL1:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0xF0, 0x00, "Target Percentage (if key not fitted)" )PORT_DIPLOCATION("DIL1:05,06,07,08")
	PORT_DIPSETTING(    0x00, "Unset (Program Optimum)"  )
	PORT_DIPSETTING(    0x10, "70" )
	PORT_DIPSETTING(    0x20, "72" )
	PORT_DIPSETTING(    0x30, "74" )
	PORT_DIPSETTING(    0x40, "76" )
	PORT_DIPSETTING(    0x50, "78" )
	PORT_DIPSETTING(    0x60, "80" )
	PORT_DIPSETTING(    0x70, "82" )
	PORT_DIPSETTING(    0x80, "84" )
	PORT_DIPSETTING(    0x90, "86" )
	PORT_DIPSETTING(    0xA0, "88" )
	PORT_DIPSETTING(    0xB0, "90" )
	PORT_DIPSETTING(    0xC0, "92" )
	PORT_DIPSETTING(    0xD0, "94" )
	PORT_DIPSETTING(    0xE0, "96" )
	PORT_DIPSETTING(    0xF0, "98" )

	PORT_START("DIL2")
	PORT_DIPNAME( 0x01, 0x00, "Token Lockout when full" ) PORT_DIPLOCATION("DIL2:01")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On  ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused )) PORT_DIPLOCATION("DIL2:02")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On  ) )
	PORT_DIPNAME( 0x04, 0x00, "Scottish Coin Handling" ) PORT_DIPLOCATION("DIL2:03")//20p payout
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On  ) )
	PORT_DIPNAME( 0x08, 0x00, "Out of Credit Display Inhibit" ) PORT_DIPLOCATION("DIL2:04")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On  ) )
	PORT_DIPNAME( 0x10, 0x00, "OCD Audio Enable" ) PORT_DIPLOCATION("DIL2:05")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On  ) )
	PORT_DIPNAME( 0x20, 0x00, "Coin Alarm Inhibit" ) PORT_DIPLOCATION("DIL2:06")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On  ) )
	PORT_DIPNAME( 0x40, 0x00, "Token Refill Level Inhibit" ) PORT_DIPLOCATION("DIL2:07")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On  ) )
	PORT_DIPNAME( 0x80, 0x00, "Single Credit Entry" ) PORT_DIPLOCATION("DIL2:08")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On  ) )

	PORT_START("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("0")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("2")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("3")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("4")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("5")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("6")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("7")

	PORT_START("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("10p")PORT_IMPULSE(5)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("20p")PORT_IMPULSE(5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("50p")PORT_IMPULSE(5)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("100p")PORT_IMPULSE(5)
INPUT_PORTS_END

static const stepper_interface barcrest_reel_interface =
{
	BARCREST_48STEP_REEL,
	92,
	4,
	0x00
};

static const stepper_interface barcrest_reelrev_interface =
{
	BARCREST_48STEP_REEL,
	92,
	4,
	0x00,
	1
};

static const stepper_interface barcrest_opto1_interface =
{
	BARCREST_48STEP_REEL,
	4,
	12,
	0x00
};

static const stepper_interface barcrest_opto2_interface =
{
	BARCREST_48STEP_REEL,
	92,
	3,
	0x00
};

static const stepper_interface barcrest_opto3_interface =
{
	BARCREST_48STEP_REEL,
	0,
	5,
	0x00
};

static const stepper_interface bwb_opto1_interface =
{
	BARCREST_48STEP_REEL,
	96,
	3,
	0x00
};

/*
Characteriser (CHR)

As built, the CHR is a PAL which can perform basic bit manipulation according to
an as yet unknown unique key. However, the programmers decided to best use this protection device in read/write/compare
cycles, storing almost the entire 'hidden' data table in the ROMs in plain sight. Only later rebuilds by BwB
avoided this 'feature' of the development kit, and will need a different setup.

This information has been used to generate the CHR tables loaded by the programs, until a key can be determined.

For most Barcrest games, the following method was used:

The initial 'PALTEST' routine as found in the Barcrest programs simply writes the first 'call' to the CHR space,
to read back the 'response'. There is no attempt to alter the order or anything else, just
a simple runthrough of the entire data table. The only 'catch' in this is to note that the CHR chip always scans
through the table starting at the last accessed data value, unless 00 is used to reset to the beginning. This is obviously
a simplification, in fact the PAL does bit manipulation with some latching.

However, a final 8 byte row, that controls the lamp matrix is not tested - to date, no-one outside of Barcrest knows
how this is generated, and currently trial and error is the only sensible method. It is noted that the default,
of all 00, is sometimes the correct answer, particularly in non-Barcrest use of the CHR chip, though when used normally,
there are again fixed call values.
*/


static WRITE8_HANDLER( characteriser_w )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	int x;
	int call=data;
	LOG_CHR_FULL(("%04x Characteriser write offset %02X data %02X", cpu_get_previouspc(&space->device()),offset,data));
	if (!state->m_current_chr_table)
	{
		logerror("No Characteriser Table @ %04x\n", cpu_get_previouspc(&space->device()));
		return;
	}



	if (offset == 0)
	{
		{
			if (call == 0)
			{
				state->m_prot_col = 0;
			}
			else
			{
				for (x = state->m_prot_col; x < 64; x++)
				{
					if	(state->m_current_chr_table[(x)].call == call)
					{
						state->m_prot_col = x;
						LOG_CHR(("Characteriser find column %02X\n",state->m_prot_col));
						break;
					}
				}
			}
		}
	}
	else if (offset == 2)
	{
		LOG_CHR(("Characteriser write 2 data %02X\n",data));
		switch (call)
		// Rather than the search strategy, we can map the calls directly here. Note that they are hex versions of the square number series
		{
			case 0x00:
			state->m_lamp_col = 0;
			break;
			case 0x01:
			state->m_lamp_col = 1;
			break;
			case 0x04:
			state->m_lamp_col = 2;
			break;
			case 0x09:
			state->m_lamp_col = 3;
			break;
			case 0x10:
			state->m_lamp_col = 4;
			break;
			case 0x19:
			state->m_lamp_col = 5;
			break;
			case 0x24:
			state->m_lamp_col = 6;
			break;
			case 0x31:
			state->m_lamp_col = 7;
			break;
		}
		LOG_CHR(("Characteriser find 2 column %02X\n",state->m_lamp_col));
	}
}


static READ8_HANDLER( characteriser_r )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	if (!state->m_current_chr_table)
	{
		logerror("No Characteriser Table @ %04x", cpu_get_previouspc(&space->device()));

		/* a cheat ... many early games use a standard check */
		int addr = cpu_get_reg(&space->device(), M6809_X);
		UINT8 ret = space->read_byte(addr);
		logerror(" (returning %02x)",ret);

		logerror("\n");

		return ret;
	}

	LOG_CHR(("Characteriser read offset %02X \n",offset));
	if (offset == 0)
	{
		LOG_CHR(("Characteriser read data %02X \n",state->m_current_chr_table[state->m_prot_col].response));
		return state->m_current_chr_table[state->m_prot_col].response;
	}
	if (offset == 3)
	{
		LOG_CHR(("Characteriser read data off 3 %02X \n",state->m_current_chr_table[state->m_lamp_col+64].response));
		return state->m_current_chr_table[state->m_lamp_col+64].response;
	}
	return 0;
}

/*
BwB Characteriser (CHR)

The BwB method of protection is considerably different to the Barcrest one, with any
incorrect behaviour manifesting in ridiculously large payouts. The hardware is the
same, however the main weakness of the software has been eliminated.

In fact, the software seems deliberately designed to mislead, but is (fortunately for
us) prone to similar weaknesses that allow a per game solution.

Project Amber performed a source analysis (available on request) which appears to make things work.
Said weaknesses (A Cheats Guide according to Project Amber)

The common initialisation sequence is "00 04 04 0C 0C 1C 14 2C 5C 2C"
                                        0  1  2  3  4  5  6  7  8
Using debug search for the first read from said string (best to find it first).

At this point, the X index on the CPU is at the magic number address.

The subsequent calls for each can be found based on the magic address

           (0) = ( (BWBMagicAddress))
           (1) = ( (BWBMagicAddress + 1))
           (2) = ( (BWBMagicAddress + 2))
           (3) = ( (BWBMagicAddress + 4))
           (4) = ( (BWBMagicAddress - 5))
           (5) = ( (BWBMagicAddress - 4))
           (6) = ( (BWBMagicAddress - 3))
           (7) = ( (BWBMagicAddress - 2))
           (8) = ( (BWBMagicAddress - 1))

These return the standard init sequence as above.

For ease of understanding, we use three tables, one holding the common responses
and two holding the appropriate call and response pairs for the two stages of operation
*/


static WRITE8_HANDLER( bwb_characteriser_w )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();
	int x;
	int call=data;
	LOG_CHR_FULL(("%04x Characteriser write offset %02X data %02X \n", cpu_get_previouspc(&space->device()),offset,data));
	if (!state->m_current_chr_table)
		fatalerror("No Characteriser Table @ %04x\n", cpu_get_previouspc(&space->device()));

	if ((offset & 0x3f)== 0)//initialisation is always at 0x800
	{
		if (!state->m_chr_state)
		{
			state->m_chr_state=1;
			state->m_chr_counter=0;
		}
		if (call == 0)
		{
			state->m_init_col ++;
		}
		else
		{
			state->m_init_col =0;
		}
	}

	state->m_chr_value = space->machine().rand();
	for (x = 0; x < 4; x++)
	{
		if	(state->m_current_chr_table[(x)].call == call)
		{
			if (x == 0) // reinit
			{
				state->m_bwb_return = 0;
			}
			state->m_chr_value = bwb_chr_table_common[(state->m_bwb_return)];
			state->m_bwb_return++;
			break;
		}
	}
}

static READ8_HANDLER( bwb_characteriser_r )
{
	mpu4_state *state = space->machine().driver_data<mpu4_state>();

	LOG_CHR(("Characteriser read offset %02X \n",offset));


	if (offset ==0)
	{
		switch (state->m_chr_counter)
		{
			case 6:
			case 13:
			case 20:
			case 27:
			case 34:
			{
				return state->m_bwb_chr_table1[(((state->m_chr_counter + 1) / 7) - 1)].response;
				break;
			}
			default:
			{
				if (state->m_chr_counter > 34)
				{
					state->m_chr_counter = 35;
					state->m_chr_state = 2;
				}
				state->m_chr_counter ++;
				return state->m_chr_value;
			}
		}
	}
	else
	{
		return state->m_chr_value;
	}
}

/* Common configurations */

static WRITE8_HANDLER( mpu4_ym2413_w )
{
	device_t *ym = space->machine().device("ym2413");
	if (ym) ym2413_w(ym,offset,data);
}

static READ8_HANDLER( mpu4_ym2413_r )
{
//  device_t *ym = space->machine().device("ym2413");
//  return ym2413_read(ym,offset);
	return 0xff;
}


void mpu4_install_mod4yam_space(address_space *space)
{
	space->install_legacy_read_handler(0x0880, 0x0882, FUNC(mpu4_ym2413_r));
	space->install_legacy_write_handler(0x0880, 0x0881, FUNC(mpu4_ym2413_w));
}

void mpu4_install_mod4oki_space(address_space *space)
{
	pia6821_device *pia_ic4ss = space->machine().device<pia6821_device>("pia_ic4ss");
	ptm6840_device *ptm_ic3ss = space->machine().device<ptm6840_device>("ptm_ic3ss");

	space->install_readwrite_handler(0x0880, 0x0883, 0, 0, read8_delegate(FUNC(pia6821_device::read), pia_ic4ss), write8_delegate(FUNC(pia6821_device::write), pia_ic4ss));
	space->install_read_handler(0x08c0, 0x08c7, 0, 0, read8_delegate(FUNC(ptm6840_device::read), ptm_ic3ss));
	space->install_legacy_write_handler(0x08c0, 0x08c7, 0, 0, FUNC(ic3ss_w));
}

void mpu4_install_mod4bwb_space(address_space *space)
{
	space->install_legacy_readwrite_handler(0x0810, 0x0810, 0, 0, FUNC(bwb_characteriser_r),FUNC(bwb_characteriser_w));
	mpu4_install_mod4oki_space(space);
}


void mpu4_config_common(running_machine &machine)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_ic24_timer = machine.scheduler().timer_alloc(FUNC(ic24_timeout));
	state->m_lamp_strobe_ext_persistence = 0;
	/* setup 8 mechanical meters */
	MechMtr_config(machine,8);

	/* setup the standard oki MSC1937 display */
	ROC10937_init(0, MSC1937,0);

}

static void mpu4_config_common_reels(running_machine &machine,int reels)
{
	int n;
	/* setup n default 96 half step reels, using the standard optic flag */
	for ( n = 0; n < reels; n++ )
	{
		stepper_config(machine, n, &barcrest_reel_interface);
	}
	awp_reel_setup();
}

MACHINE_START( mod2     )
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	mpu4_config_common(machine);

	state->m_link7a_connected=0;
	state->m_mod_number=2;
}

static MACHINE_START( mpu4yam )
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	mpu4_config_common(machine);

	state->m_link7a_connected=0;
	state->m_mod_number=4;
	mpu4_install_mod4yam_space(space);
}

static MACHINE_START( mpu4oki )
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	mpu4_config_common(machine);

	state->m_link7a_connected=0;
	state->m_mod_number=4;
	mpu4_install_mod4oki_space(space);
}

static MACHINE_START( mpu4bwb )
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	mpu4_config_common(machine);

	state->m_link7a_connected=0;
	state->m_mod_number=4;
	mpu4_install_mod4bwb_space(space);
}

static MACHINE_START( mpu4cry )
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	mpu4_config_common(machine);

	state->m_link7a_connected=0;
	state->m_mod_number=4;
}

/* CHR Tables */

static mpu4_chr_table ccelbr_data[72] = {
{0x00, 0x00},{0x1a, 0x84},{0x04, 0x8c},{0x10, 0xb8},{0x18, 0x74},{0x0f, 0x80},{0x13, 0x1c},{0x1b, 0xb4},
{0x03, 0xd8},{0x07, 0x74},{0x17, 0x00},{0x1d, 0xd4},{0x36, 0xc8},{0x35, 0x78},{0x2b, 0xa4},{0x28, 0x4c},
{0x39, 0xe0},{0x21, 0xdc},{0x22, 0xf4},{0x25, 0x88},{0x2c, 0x78},{0x29, 0x24},{0x31, 0x84},{0x34, 0xcc},
{0x0a, 0xb8},{0x1f, 0x74},{0x06, 0x90},{0x0e, 0x48},{0x1c, 0xa0},{0x12, 0x1c},{0x1e, 0x24},{0x0d, 0x94},
{0x14, 0xc8},{0x0a, 0xb8},{0x19, 0x74},{0x15, 0x00},{0x06, 0x94},{0x0f, 0x48},{0x08, 0x30},{0x1b, 0x90},
{0x1e, 0x08},{0x04, 0x60},{0x01, 0xd4},{0x0c, 0x58},{0x18, 0xf4},{0x1a, 0x18},{0x11, 0x74},{0x0b, 0x80},
{0x03, 0xdc},{0x17, 0x74},{0x10, 0xd0},{0x1d, 0x58},{0x0e, 0x24},{0x07, 0x94},{0x12, 0xd8},{0x09, 0x34},
{0x0d, 0x90},{0x1f, 0x58},{0x16, 0xf4},{0x05, 0x88},{0x13, 0x38},{0x1c, 0x24},{0x02, 0xd4},{0x00, 0x00},
{0x00, 0x00},{0x01, 0x50},{0x04, 0x00},{0x09, 0x50},{0x10, 0x10},{0x19, 0x40},{0x24, 0x04},{0x31, 0x00}
};

static mpu4_chr_table gmball_data[72] = {
{0x00, 0x00},{0x1a, 0x0c},{0x04, 0x50},{0x10, 0x90},{0x18, 0xb0},{0x0f, 0x38},{0x13, 0xd4},{0x1b, 0xa0},
{0x03, 0xbc},{0x07, 0xd4},{0x17, 0x30},{0x1d, 0x90},{0x36, 0x38},{0x35, 0xc4},{0x2b, 0xac},{0x28, 0x70},
{0x39, 0x98},{0x21, 0xdc},{0x22, 0xdc},{0x25, 0x54},{0x2c, 0x80},{0x29, 0xb4},{0x31, 0x38},{0x34, 0xcc},
{0x0a, 0xe8},{0x1f, 0xf8},{0x06, 0xd4},{0x0e, 0x30},{0x1c, 0x00},{0x12, 0x84},{0x1e, 0x2c},{0x0d, 0xc8},
{0x14, 0xf8},{0x0a, 0x4c},{0x19, 0x58},{0x15, 0xd4},{0x06, 0xa8},{0x0f, 0x78},{0x08, 0x44},{0x1b, 0x0c},
{0x1e, 0x48},{0x04, 0x50},{0x01, 0x98},{0x0c, 0xd4},{0x18, 0xb0},{0x1a, 0xa0},{0x11, 0xa4},{0x0b, 0x3c},
{0x03, 0xdc},{0x17, 0xd4},{0x10, 0xb8},{0x1d, 0xd4},{0x0e, 0x30},{0x07, 0x88},{0x12, 0xe0},{0x09, 0x24},
{0x0d, 0x8c},{0x1f, 0xf8},{0x16, 0xcc},{0x05, 0x70},{0x13, 0x90},{0x1c, 0x20},{0x02, 0x9c},{0x00, 0x00},
{0x00, 0x00},{0x01, 0x18},{0x04, 0x08},{0x09, 0x10},{0x10, 0x00},{0x19, 0x18},{0x24, 0x08},{0x31, 0x00}
};

static mpu4_chr_table grtecp_data[72] = {
{0x00, 0x00},{0x1a, 0x84},{0x04, 0xa4},{0x10, 0xac},{0x18, 0x70},{0x0f, 0x80},{0x13, 0x2c},{0x1b, 0xc0},
{0x03, 0xbc},{0x07, 0x5c},{0x17, 0x5c},{0x1d, 0x5c},{0x36, 0xdc},{0x35, 0x5c},{0x2b, 0xcc},{0x28, 0x68},
{0x39, 0xd0},{0x21, 0xb8},{0x22, 0xdc},{0x25, 0x54},{0x2c, 0x08},{0x29, 0x58},{0x31, 0x54},{0x34, 0x90},
{0x0a, 0xb8},{0x1f, 0x5c},{0x06, 0x5c},{0x0e, 0x44},{0x1c, 0x84},{0x12, 0xac},{0x1e, 0xe0},{0x0d, 0xbc},
{0x14, 0xcc},{0x0a, 0xe8},{0x19, 0x70},{0x15, 0x00},{0x06, 0x8c},{0x0f, 0x70},{0x08, 0x00},{0x1b, 0x84},
{0x1e, 0xa4},{0x04, 0xa4},{0x01, 0xbc},{0x0c, 0xdc},{0x18, 0x5c},{0x1a, 0xcc},{0x11, 0xe8},{0x0b, 0xe0},
{0x03, 0xbc},{0x17, 0x4c},{0x10, 0xc8},{0x1d, 0xf8},{0x0e, 0xd4},{0x07, 0xa8},{0x12, 0x68},{0x09, 0x40},
{0x0d, 0x0c},{0x1f, 0xd8},{0x16, 0xdc},{0x05, 0x54},{0x13, 0x98},{0x1c, 0x44},{0x02, 0x9c},{0x00, 0x00},
{0x00, 0x00},{0x01, 0x18},{0x04, 0x00},{0x09, 0x18},{0x10, 0x08},{0x19, 0x10},{0x24, 0x00},{0x31, 0x00}
};

static mpu4_chr_table oldtmr_data[72] = {
{0x00, 0x00},{0x1a, 0x90},{0x04, 0xc0},{0x10, 0x54},{0x18, 0xa4},{0x0f, 0xf0},{0x13, 0x64},{0x1b, 0x90},
{0x03, 0xe4},{0x07, 0xd4},{0x17, 0x60},{0x1d, 0xb4},{0x36, 0xc0},{0x35, 0x70},{0x2b, 0x80},{0x28, 0x74},
{0x39, 0xa4},{0x21, 0xf4},{0x22, 0xe4},{0x25, 0xd0},{0x2c, 0x64},{0x29, 0x10},{0x31, 0x20},{0x34, 0x90},
{0x0a, 0xe4},{0x1f, 0xf4},{0x06, 0xc4},{0x0e, 0x70},{0x1c, 0x00},{0x12, 0x14},{0x1e, 0x00},{0x0d, 0x14},
{0x14, 0xa0},{0x0a, 0xf0},{0x19, 0x64},{0x15, 0x10},{0x06, 0x84},{0x0f, 0x70},{0x08, 0x00},{0x1b, 0x90},
{0x1e, 0x40},{0x04, 0x90},{0x01, 0xe4},{0x0c, 0xf4},{0x18, 0x64},{0x1a, 0x90},{0x11, 0x64},{0x0b, 0x90},
{0x03, 0xe4},{0x17, 0x50},{0x10, 0x24},{0x1d, 0xb4},{0x0e, 0xe0},{0x07, 0xd4},{0x12, 0xe4},{0x09, 0x50},
{0x0d, 0x04},{0x1f, 0xb4},{0x16, 0xc0},{0x05, 0xd0},{0x13, 0x64},{0x1c, 0x90},{0x02, 0xe4},{0x00, 0x00},
{0x00, 0x00},{0x01, 0x00},{0x04, 0x00},{0x09, 0x00},{0x10, 0x00},{0x19, 0x10},{0x24, 0x00},{0x31, 0x00}
};

static const bwb_chr_table blsbys_data1[5] = {
//Magic number 724A

// PAL Codes
// 0   1   2  3  4  5  6  7  8
// ??  ?? 20 0F 24 3C 36 27 09

	{0x67},{0x17},{0x0f},{0x24},{0x3c},
};

static mpu4_chr_table blsbys_data[8] = {
{0xEF, 0x02},{0x81, 0x00},{0xCE, 0x00},{0x00, 0x2e},
{0x06, 0x20},{0xC6, 0x0f},{0xF8, 0x24},{0x8E, 0x3c},
};

// set percentage and other options. 2e 20 0f
// PAL Codes
// 0   1   2  3  4  5  6  7  8
// 42  2E 20 0F 24 3C 36 27 09
   //      6  0  7  0  8  0  7  0  0  8
//request 36 42 27 42 09 42 27 42 42 09
//verify  00 04 04 0C 0C 1C 14 2C 5C 2C

static DRIVER_INIT (m_oldtmr)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reel_mux=SIX_REEL_1TO8;
	state->m_reels = 6;

	stepper_config(machine, 0, &barcrest_opto1_interface);
	stepper_config(machine, 1, &barcrest_opto1_interface);
	stepper_config(machine, 2, &barcrest_opto1_interface);
	stepper_config(machine, 3, &barcrest_opto1_interface);
	stepper_config(machine, 4, &barcrest_opto1_interface);
	stepper_config(machine, 5, &barcrest_opto1_interface);

	awp_reel_setup();
	state->m_current_chr_table = oldtmr_data;
}

static DRIVER_INIT (m_ccelbr)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reel_mux=STANDARD_REEL;
	state->m_reels = 4;
	// setup 4 default 96 half step reels ///////////////////////////////////
	mpu4_config_common_reels(machine,4);

	state->m_current_chr_table = ccelbr_data;
}

static DRIVER_INIT (m_gmball)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reel_mux=STANDARD_REEL;
	state->m_reels = 4;
	// setup 4 default 96 half step reels ///////////////////////////////////
	mpu4_config_common_reels(machine,4);

	state->m_current_chr_table = gmball_data;
}

static DRIVER_INIT (m_grtecp)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reel_mux=FIVE_REEL_5TO8;
	state->m_reels = 5;
	state->m_lamp_extender=SMALL_CARD;
	// setup 4 default 96 half step reels with the mux board
	mpu4_config_common_reels(machine,4);
	stepper_config(machine, 4, &barcrest_reelrev_interface);
	state->m_current_chr_table = grtecp_data;
}

static DRIVER_INIT (m_blsbys)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_bwb_bank=1;
	state->m_reel_mux=FIVE_REEL_5TO8;
	state->m_reels = 5;
	stepper_config(machine, 0, &bwb_opto1_interface);
	stepper_config(machine, 1, &bwb_opto1_interface);
	stepper_config(machine, 2, &bwb_opto1_interface);
	stepper_config(machine, 3, &bwb_opto1_interface);
	stepper_config(machine, 4, &bwb_opto1_interface);
	state->m_bwb_chr_table1 = blsbys_data1;
	state->m_current_chr_table = blsbys_data;
}

static DRIVER_INIT (m4tst2)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reel_mux=STANDARD_REEL;
	state->m_reels = 4;
	mpu4_config_common_reels(machine,4);
}

static DRIVER_INIT (m4tst)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reel_mux=STANDARD_REEL;
	state->m_reels = 4;
	mpu4_config_common_reels(machine,4);
}

static DRIVER_INIT (connect4)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reels = 0; //reel-free game
	state->m_led_lamp=1;
}



static DRIVER_INIT (m4default)
{
	mpu4_state *state = machine.driver_data<mpu4_state>();
	state->m_reel_mux=STANDARD_REEL;
	state->m_reels = 4;
	mpu4_config_common_reels(machine,4);
	state->m_bwb_bank=0;
}

static DRIVER_INIT( m4default_bigbank )
{
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	mpu4_state *state = machine.driver_data<mpu4_state>();
	DRIVER_INIT_CALL(m4default);
	state->m_bwb_bank=1;
	space->install_legacy_write_handler(0x0858, 0x0858, 0, 0, FUNC(bankswitch_w));
	space->install_legacy_write_handler(0x0878, 0x0878, 0, 0, FUNC(bankset_w));
}

static READ8_HANDLER( crystal_sound_r )
{
	return space->machine().rand();
}

static WRITE8_HANDLER( crystal_sound_w )
{
	printf("crystal_sound_w %02x\n",data);
}

static DRIVER_INIT (m_frkstn)
{
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	DRIVER_INIT_CALL(m4default_bigbank);
	space->install_legacy_read_handler(0x0880, 0x0880, 0, 0, FUNC(crystal_sound_r));
	space->install_legacy_write_handler(0x0881, 0x0881, 0, 0, FUNC(crystal_sound_w));
}

/* generate a 50 Hz signal (based on an RC time) */
TIMER_DEVICE_CALLBACK( gen_50hz )
{
	mpu4_state *state = timer.machine().driver_data<mpu4_state>();
	/* Although reported as a '50Hz' signal, the fact that both rising and
    falling edges of the pulse are used means the timer actually gives a 100Hz
    oscillating signal.*/
	state->m_signal_50hz = state->m_signal_50hz?0:1;
	timer.machine().device<pia6821_device>("pia_ic4")->ca1_w(state->m_signal_50hz);	/* signal is connected to IC4 CA1 */

	update_meters(state);//run at 100Hz to sync with PIAs
}

static ADDRESS_MAP_START( mpu4_memmap, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_SHARE("nvram")
	AM_RANGE(0x0800, 0x0810) AM_READWRITE(characteriser_r,characteriser_w)
	AM_RANGE(0x0850, 0x0850) AM_READWRITE(bankswitch_r,bankswitch_w)	/* write bank (rom page select) */
/*  AM_RANGE(0x08e0, 0x08e7) AM_READWRITE(68681_duart_r,68681_duart_w) */ //Runs hoppers
	AM_RANGE(0x0900, 0x0907) AM_DEVREADWRITE_MODERN("ptm_ic2", ptm6840_device, read, write)/* PTM6840 IC2 */
	AM_RANGE(0x0a00, 0x0a03) AM_DEVREADWRITE_MODERN("pia_ic3", pia6821_device, read, write)		/* PIA6821 IC3 */
	AM_RANGE(0x0b00, 0x0b03) AM_DEVREADWRITE_MODERN("pia_ic4", pia6821_device, read, write)		/* PIA6821 IC4 */
	AM_RANGE(0x0c00, 0x0c03) AM_DEVREADWRITE_MODERN("pia_ic5", pia6821_device, read, write)		/* PIA6821 IC5 */
	AM_RANGE(0x0d00, 0x0d03) AM_DEVREADWRITE_MODERN("pia_ic6", pia6821_device, read, write)		/* PIA6821 IC6 */
	AM_RANGE(0x0e00, 0x0e03) AM_DEVREADWRITE_MODERN("pia_ic7", pia6821_device, read, write)		/* PIA6821 IC7 */
	AM_RANGE(0x0f00, 0x0f03) AM_DEVREADWRITE_MODERN("pia_ic8", pia6821_device, read, write)		/* PIA6821 IC8 */
	AM_RANGE(0x1000, 0xffff) AM_ROMBANK("bank1")	/* 64k  paged ROM (4 pages)  */
ADDRESS_MAP_END

const ay8910_interface ay8910_config =
{
	AY8910_SINGLE_OUTPUT,
	{820,0,0},
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

MACHINE_CONFIG_FRAGMENT( mpu4_common )
	MCFG_TIMER_ADD_PERIODIC("50hz",gen_50hz, attotime::from_hz(100))

	/* 6840 PTM */
	MCFG_PTM6840_ADD("ptm_ic2", ptm_ic2_intf)

	MCFG_PIA6821_ADD("pia_ic3", pia_ic3_intf)
	MCFG_PIA6821_ADD("pia_ic4", pia_ic4_intf)
	MCFG_PIA6821_ADD("pia_ic5", pia_ic5_intf)
	MCFG_PIA6821_ADD("pia_ic6", pia_ic6_intf)
	MCFG_PIA6821_ADD("pia_ic7", pia_ic7_intf)
	MCFG_PIA6821_ADD("pia_ic8", pia_ic8_intf)
MACHINE_CONFIG_END

MACHINE_CONFIG_FRAGMENT( mpu4_common2 )
	MCFG_PTM6840_ADD("ptm_ic3ss", ptm_ic3ss_intf)
	MCFG_PIA6821_ADD("pia_ic4ss", pia_ic4ss_intf)
MACHINE_CONFIG_END

/* machine driver for MOD 2 board */
MACHINE_CONFIG_START( mpu4base, mpu4_state )

	MCFG_MACHINE_START(mod2    )
	MCFG_MACHINE_RESET(mpu4)
	MCFG_CPU_ADD("maincpu", M6809, MPU4_MASTER_CLOCK/4)
	MCFG_CPU_PROGRAM_MAP(mpu4_memmap)


	MCFG_FRAGMENT_ADD(mpu4_common)


	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MCFG_NVRAM_ADD_0FILL("nvram")

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(64*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(8*8, 48*8-1, 2*8, 30*8-1)
	MCFG_PALETTE_LENGTH(0x200)


	MCFG_DEFAULT_LAYOUT(layout_mpu4)
MACHINE_CONFIG_END


 MACHINE_CONFIG_DERIVED( mod2    , mpu4base )
	MCFG_SOUND_ADD("ay8913",AY8913, MPU4_MASTER_CLOCK/4)
	MCFG_SOUND_CONFIG(ay8910_config)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 1.0)

MACHINE_CONFIG_END




static MACHINE_CONFIG_DERIVED( mod4yam, mpu4base )
	MCFG_MACHINE_START(mpu4yam)

	MCFG_SOUND_ADD("ym2413", YM2413, MPU4_MASTER_CLOCK/4)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 1.0)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( mod4oki, mpu4base )
	MCFG_MACHINE_START(mpu4oki)

	MCFG_FRAGMENT_ADD(mpu4_common2)

	MCFG_SOUND_ADD("msm6376", OKIM6376, 128000)		//16KHz sample Can also be 85430 at 10.5KHz and 64000 at 8KHz
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 1.0)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( bwboki, mod4oki )
	MCFG_MACHINE_START(mpu4bwb)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED(mpu4crys, mod2     )
	MCFG_MACHINE_START(mpu4cry)

	MCFG_SOUND_ADD("upd", UPD7759, UPD7759_STANDARD_CLOCK)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 1.0)
MACHINE_CONFIG_END







ROM_START( m4conn4 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00  )
	ROM_LOAD( "connect4.p2",  0x8000, 0x4000,  CRC(6090633c) SHA1(0cd2725a235bf93cfe94f2ca648d5fccb87b8e5c) )
	ROM_LOAD( "connect4.p1",  0xC000, 0x4000,  CRC(b1af50c0) SHA1(7c9645ea378f0857b849ca24a239d9114f62da7f) )
ROM_END

ROM_START( m4tst )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00  )
	ROM_LOAD( "ut4.p1",  0xC000, 0x4000,  CRC(086dc325) SHA1(923caeb61347ac9d3e6bcec45998ddf04b2c8ffd))
ROM_END

ROM_START( m4tst2 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00  )
	ROM_LOAD( "ut2.p1",  0xE000, 0x2000,  CRC(f7fb6575) SHA1(f7961cbd0801b9561d8cd2d23081043d733e1902))
ROM_END

ROM_START( m4clr )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00  )
	ROM_LOAD( "meter-zero.p1",  0x8000, 0x8000,  CRC(e74297e5) SHA1(49a2cc85eda14199975ec37a794b685c839d3ab9))
ROM_END



ROM_START( m4blsbys )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD("bbprog.bin",  0x00000, 0x20000,  CRC(c262cfda) SHA1(f004895e0dd3f8420683927915554e19e41bd20b))
	ROM_RELOAD(0x40000,0x20000)

	ROM_REGION( 0x20000, "altrevs", 0 )
	ROM_LOAD( "bf_20a__.3_1", 0x0000, 0x020000, CRC(fca7764f) SHA1(a88378247b6710d6122c515c31c39c5cd9678ce2) )
	ROM_LOAD( "bf_20a__.6_1", 0x0000, 0x020000, CRC(0822931a) SHA1(8d53321832ee56ed5ad851928ad7705e1ad059ee) )
	ROM_LOAD( "bf_20s__.3_1", 0x0000, 0x020000, CRC(029a3a0b) SHA1(25952cafbc351ec6d5fc65dd2acddadeb48fb649) )
	ROM_LOAD( "bf_20s__.6_1", 0x0000, 0x020000, CRC(af7462cb) SHA1(188935ebb574e0b09f9e0e5f094eb99ed7df5075) )
	ROM_LOAD( "bf_20sb_.3_1", 0x0000, 0x020000, CRC(d2ff1a12) SHA1(d425985a8d109c9e3729618a995eeac1f3bf643c) )
	ROM_LOAD( "bf_20sb_.6_1", 0x0000, 0x020000, CRC(7548bdf1) SHA1(a366c10f35d85b1f3c103787dd5b2e6600ecf6c8) )
	ROM_LOAD( "bluesboys8tok.bin", 0x0000, 0x020000, CRC(d03ef955) SHA1(03fa4b3b37b71fb61439200d5dd65dab846abc2c) )
	ROM_LOAD( "bs_05___.3v1", 0x0000, 0x020000, CRC(26e8eb95) SHA1(7d8dbca127e1867714cbeb9d699b2173de724eb2) )
	ROM_LOAD( "bs_05__c.3v1", 0x0000, 0x020000, CRC(12e51237) SHA1(68235cf5f36862a26d5d44464041dabb01b9f95c) )
	ROM_LOAD( "bs_05_b_.3v1", 0x0000, 0x020000, CRC(a8ec1731) SHA1(5acd86b1018301df5c3caa388bbbee0c0daacb40) )
	ROM_LOAD( "bs_05_d_.3v1", 0x0000, 0x020000, CRC(5def4142) SHA1(e0019979f65e240002455d5aa41e1492dc517740) )
	ROM_LOAD( "bs_05_k_.3v1", 0x0000, 0x020000, CRC(d023bad9) SHA1(54c995c5166f018c41dd7007163f07f054f564e9) )
	ROM_LOAD( "bs_05_kc.3v1", 0x0000, 0x020000, CRC(0b8911da) SHA1(1426944c01edbf6edb664f961fe75befd3e81e65) )
	ROM_LOAD( "bs_05a__.2_1", 0x0000, 0x020000, CRC(88af47c9) SHA1(e5dfd572b47f2cff6e6ef68bd9016a3e5b2273b2) )
	ROM_LOAD( "bs_05a__.3v1", 0x0000, 0x00e561, CRC(a06db3b3) SHA1(ebdc56a2810612b495d35225b640d0cd42cdd9f3) )
	ROM_LOAD( "bs_05a_c.3v1", 0x0000, 0x020000, CRC(737999c4) SHA1(957fd06f7937b85436ef7011d47c559729d17d51) )
	ROM_LOAD( "bs_05b__.3v1", 0x0000, 0x020000, CRC(ea1b09e7) SHA1(9310e3d63b77c4bc2d0d428a630c8aafb500de74) )
	ROM_LOAD( "bs_05b_c.3v1", 0x0000, 0x020000, CRC(3ec0ce9f) SHA1(09472d392350da1593f084a2591125b0d71aca29) )
	ROM_LOAD( "bs_20__c.1_1", 0x0000, 0x020000, CRC(328dc0a6) SHA1(7bea0bac121f2c521d891267fb891479549350aa) )
	ROM_LOAD( "bs_20a__.6_1", 0x0000, 0x020000, CRC(9968e21f) SHA1(8615c8b7f0ae55f0b77f30c84a74d1fba0450a73) )
	ROM_LOAD( "bs_20a__.7_1", 0x0000, 0x020000, CRC(c48bc5d6) SHA1(5aa44d85c1a33ea1aa27e2eaa987ce4fe570b713) )
	ROM_LOAD( "bs_20a_c.1_1", 0x0000, 0x020000, CRC(b9483173) SHA1(a81ef9cb42cd090861b2e0a28a63906cf61c7534) )
	ROM_LOAD( "bs_20a_p.4_1", 0x0000, 0x020000, CRC(fb7ec0aa) SHA1(a0b681a8eacde06825c7e4fcf5b7ef8f64723d96) )
	ROM_LOAD( "bs_20a_s.6_1", 0x0000, 0x020000, CRC(72c5c16e) SHA1(18bdb0f9aff13587d95d871b4124d5a1cc07af04) )
	ROM_LOAD( "bs_20s__.6_1", 0x0000, 0x020000, CRC(1862df89) SHA1(b18f15f2098dfc488a6bdb9a7adff4446268f0d3) )
	ROM_LOAD( "bs_20s_s.6_1", 0x0000, 0x020000, CRC(f3cffcf8) SHA1(bda99c269e85baf64f66788f868879d21a896c5a) )
	ROM_LOAD( "bs_20sb_.6_1", 0x0000, 0x020000, CRC(3278bdcb) SHA1(c694d485c79be983924410ce00d364d233a054c0) )
	ROM_LOAD( "bs_20sbc.1_1", 0x0000, 0x020000, CRC(3a55b728) SHA1(948b353ec92aae1cb801e5e2d2385cd2316f0838) )
	ROM_LOAD( "bs_20sbs.6_1", 0x0000, 0x020000, CRC(d9d59eba) SHA1(91f608dc33541297797f937546fa5759b26af511) )
	ROM_LOAD( "bs_20sd_.6_1", 0x0000, 0x020000, CRC(df087fd9) SHA1(35c85ffe0bef5847a3a72bb33f196f4708f03b28) )
	ROM_LOAD( "bs_20sdc.1_1", 0x0000, 0x020000, CRC(1439c63e) SHA1(fdac1199e98bbecd944431338d2215d434a0c004) )
	ROM_LOAD( "bs_20sds.6_1", 0x0000, 0x020000, CRC(34a55ca8) SHA1(393d0164699a91fcb4eb68ca5246a0744e245873) )
	ROM_LOAD( "bs_25__c.2_1", 0x0000, 0x020000, CRC(d41de6c6) SHA1(43c059ef673de9cc1ef800f3da68b3a6fd54e8f7) )
	ROM_LOAD( "bs_25_bc.2_1", 0x0000, 0x020000, CRC(b692305d) SHA1(4ed96655a4fdfd97fa5783648f8cdea5af0c565e) )
	ROM_LOAD( "bs_25_dc.2_1", 0x0000, 0x020000, CRC(bbe844a5) SHA1(bd1d5d8601b0c36a2f4abaae04a49626cbef23b6) )
	ROM_LOAD( "bs_25a_c.2_1", 0x0000, 0x020000, CRC(0b64cc29) SHA1(43b958321ad5a04aae4629929844643dfcf17819) )
	ROM_LOAD( "bs_x3a__.2v1", 0x0000, 0x020000, CRC(99471e88) SHA1(e566cd1368e7234ec546b05528f2fcf345e03697) )
	ROM_LOAD( "bs_x3s__.2v1", 0x0000, 0x020000, CRC(84249d95) SHA1(3962ee9fde49b25c3485c7bfaecd63c400d8502d) )
	ROM_LOAD( "bs_x6a__.2v1", 0x0000, 0x020000, CRC(d03ef955) SHA1(03fa4b3b37b71fb61439200d5dd65dab846abc2c) )
	ROM_LOAD( "bs_x6s__.2v1", 0x0000, 0x020000, CRC(61d782b5) SHA1(70ca3875ff023fc091a6fe6e002fad662dbd639f) )
	ROM_LOAD( "bsix3___.2v1", 0x0000, 0x020000, CRC(4e7451fa) SHA1(b1417f948c7e80f506b90d6608f6dd79739389bf) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "bbsnd.p1",  0x000000, 0x080000,  CRC(715c9e95) SHA1(6a0c9c63e56cfc21bf77cf29c1b844b8e0844c1e) )
	ROM_LOAD( "bbsnd.p2",  0x080000, 0x080000,  CRC(594a87f8) SHA1(edfef7d08fab41fb5814c92930f08a565371eae1) )
ROM_END




ROM_START( m4tenten )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "t2002s.p1", 0x0000, 0x010000, CRC(6cd9fa10) SHA1(8efe36e3fc5b709fa4363194634686d62b5d6609) )

	ROM_REGION( 0x10000, "altrevs", 0 )
	ROM_LOAD( "n2503ad.p1", 0x0000, 0x010000, CRC(c84150e6) SHA1(8f143c26c6026a413bdd65ca148d78dead1d2474) )
	ROM_LOAD( "n2503b.p1", 0x0000, 0x010000, CRC(dd74fb57) SHA1(402f632f48cf1153cb8c22879a7482c82c8fecfe) )
	ROM_LOAD( "n2503bd.p1", 0x0000, 0x010000, CRC(62542d80) SHA1(90ebfb92891a7aaa4814b733c0e0df06bb292a4f) )
	ROM_LOAD( "n2503d.p1", 0x0000, 0x010000, CRC(b8c64fa4) SHA1(f53d9bdf97cc021399c8598a051ad5bcb7a611b5) )
	ROM_LOAD( "n2503dk.p1", 0x0000, 0x010000, CRC(28715d57) SHA1(1ab648a7f5dde5575e5ab5653823e4b88580677f) )
	ROM_LOAD( "n2503dr.p1", 0x0000, 0x010000, CRC(59932d11) SHA1(9895392ce48569e23816646124933192b6f720e3) )
	ROM_LOAD( "n2503dy.p1", 0x0000, 0x010000, CRC(287b8ee7) SHA1(10e428c73911e01ba53dd4321eb2c5a58e35441e) )
	ROM_LOAD( "n2503k.p1", 0x0000, 0x010000, CRC(f240b5ba) SHA1(fca353213447e50e29f1cd2c0d3895c437ae9336) )
	ROM_LOAD( "n2503r.p1", 0x0000, 0x010000, CRC(afaacb9d) SHA1(1bf3d648b82a8160ff1a38d92d2bb3ce1b41125e) )
	ROM_LOAD( "n2503s.p1", 0x0000, 0x010000, CRC(0d9e912c) SHA1(67d888fde242e81a1808f36e1e81c0c5724e99a7) )
	ROM_LOAD( "n2503y.p1", 0x0000, 0x010000, CRC(b365cff0) SHA1(65ab9d624bec1efc0590cec739541887ca20fc6f) )
	ROM_LOAD( "t2002ad.p1", 0x0000, 0x010000, CRC(f7903f0d) SHA1(8a10ff31ddaad817a31d39ea9a66d4453d8767bb) )
	ROM_LOAD( "t2002b.p1", 0x0000, 0x010000, CRC(90e150ea) SHA1(abc8456de42cef605e1f0e80a97b25cd51d90707) )
	ROM_LOAD( "t2002bd.p1", 0x0000, 0x010000, CRC(110c0d5c) SHA1(a263c678ebd58c95f33b198be544b726ac506449) )
	ROM_LOAD( "t2002d.p1", 0x0000, 0x010000, CRC(65a9bb19) SHA1(9a472eaded63ecab260cda03fb9e96bc6db5a99e) )
	ROM_LOAD( "t2002dk.p1", 0x0000, 0x010000, CRC(7cbbca26) SHA1(238717c37fbfab737805f46225ac740fed9a9501) )
	ROM_LOAD( "t2002dr.p1", 0x0000, 0x010000, CRC(b43b946d) SHA1(9239adb301eef818fe46ce7f3ce8b01a47031165) )
	ROM_LOAD( "t2002dy.p1", 0x0000, 0x010000, CRC(883e6777) SHA1(d5a5f0624ccf373922ef4b41519c6c95a2367b2a) )
	ROM_LOAD( "t2002k.p1", 0x0000, 0x010000, CRC(34628c9d) SHA1(3547dac8fd73abcba0f088c3f2db02ef8a40cf4e) )
	ROM_LOAD( "t2002r.p1", 0x0000, 0x010000, CRC(9c639380) SHA1(c044b7fc53daa01d93c701a3d3c786ab6a883e76) )
	ROM_LOAD( "t2002y.p1", 0x0000, 0x010000, CRC(dcf2ac8c) SHA1(e745534f45148ff33e6b1e503108a39e27d8bdae) )
	ROM_LOAD( "t2504ad.p1", 0x0000, 0x010000, CRC(f964cf74) SHA1(a72bd32f3785506e8f57107f2b8e42d64cbe267b) )
	ROM_LOAD( "t2504b.p1", 0x0000, 0x010000, CRC(f6cc485b) SHA1(6a95bf4c0cf35ccb79cb2a9e6bef208d6b61f4c7) )
	ROM_LOAD( "t2504bd.p1", 0x0000, 0x010000, CRC(f4ce2ee9) SHA1(8d0dc72f288810118702d00dc08f4e748c4da9e6) )
	ROM_LOAD( "t2504d.p1", 0x0000, 0x010000, CRC(44975553) SHA1(29116ab2d8bcbd7e73c2fa63381e93df7f9bdad9) )
	ROM_LOAD( "t2504dk.p1", 0x0000, 0x010000, CRC(3794f312) SHA1(1c15c9dd9c2631ea7fce070b45fd960cad053fcb) )
	ROM_LOAD( "t2504dr.p1", 0x0000, 0x010000, CRC(96c76a75) SHA1(28cf5e111b98e8f78ba5a5ea1ad59fc92657cd08) )
	ROM_LOAD( "t2504dy.p1", 0x0000, 0x010000, CRC(af52e603) SHA1(8c234f0964018eb21efff7a9e820c355d4465401) )
	ROM_LOAD( "t2504k.p1", 0x0000, 0x010000, CRC(c489942d) SHA1(047509ede2cb5a155274698423a1caa32ac0baeb) )
	ROM_LOAD( "t2504r.p1", 0x0000, 0x010000, CRC(da847c46) SHA1(b5897fe0266ee3eb5f364570f29ffca3de8c4c7f) )
	ROM_LOAD( "t2504s.p1", 0x0000, 0x010000, CRC(a7618248) SHA1(e28d34a7a4b2eb3f6192a04cc8849475f021d392) )
	ROM_LOAD( "t2504y.p1", 0x0000, 0x010000, CRC(e311f030) SHA1(ce21932013f1fdab1b9bbe8eb61d4fe067ccc62f) )
	ROM_LOAD( "t2t01ad.p1", 0x0000, 0x010000, CRC(debb50d9) SHA1(18b099111d81cf847155b81faba8be06cdfb3d54) )
	ROM_LOAD( "t2t01b.p1", 0x0000, 0x010000, CRC(4d8b3369) SHA1(e2221c712e8b031f39531fed61895af2214c23bc) )
	ROM_LOAD( "t2t01bd.p1", 0x0000, 0x010000, CRC(e7f852d2) SHA1(7aff9dc05e8a5db91abea61860f85793a73ad8db) )
	ROM_LOAD( "t2t01d.p1", 0x0000, 0x010000, CRC(6aba0e0d) SHA1(7feeb665974663109dc3a2afacd2b7b8c1c048d6) )
	ROM_LOAD( "t2t01dk.p1", 0x0000, 0x010000, CRC(1bd1d573) SHA1(95b0b6eab8b71d7b43e1c50f77fb2d7e4c00d0ac) )
	ROM_LOAD( "t2t01dr.p1", 0x0000, 0x010000, CRC(45978b39) SHA1(c40ea3a013a2593064cafb3933f7f3b15e1034f3) )
	ROM_LOAD( "t2t01dy.p1", 0x0000, 0x010000, CRC(205c3c2e) SHA1(53d6829ac650fc7637f8743ac6cdb3b02dd0cc86) )
	ROM_LOAD( "t2t01k.p1", 0x0000, 0x010000, CRC(488b2ce4) SHA1(3b47ea15f318b49744e869d0485e00fbd2cd74b2) )
	ROM_LOAD( "t2t01r.p1", 0x0000, 0x010000, CRC(a8f71c79) SHA1(0b9d468819bfd2f7e2ec7b5f11f052531a13d703) )
	ROM_LOAD( "t2t01s.p1", 0x0000, 0x010000, CRC(75b421e3) SHA1(d5de7485180baf9d8458a895edbfd65310fed2cc) )
	ROM_LOAD( "t2t01y.p1", 0x0000, 0x010000, CRC(0c3e29c2) SHA1(a0163587193145a0a173d1571ad9076c9f03d3ad) )
	ROM_LOAD( "t3t01ad.p1", 0x0000, 0x010000, CRC(7075b69b) SHA1(3ac28d0542c287de9c2cabfed2da0ac0cc4a24cb) )
	ROM_LOAD( "t3t01b.p1", 0x0000, 0x010000, CRC(7f7d4170) SHA1(3925a0e1f620777356ed6d6c67d6d432daeb9ade) )
	ROM_LOAD( "t3t01bd.p1", 0x0000, 0x010000, CRC(90d5dbcb) SHA1(2c348f81fe070d0825c1824f05bc4c640da9ce24) )
	ROM_LOAD( "t3t01d.p1", 0x0000, 0x010000, CRC(8a35aa83) SHA1(4156abc1eb649baf875e9b628f9b865974959c96) )
	ROM_LOAD( "t3t01dk.p1", 0x0000, 0x010000, CRC(b7d70c87) SHA1(0d0c3d73b37641151ca8e5aea02cbd90a6c93560) )
	ROM_LOAD( "t3t01dr.p1", 0x0000, 0x010000, CRC(f2dc9f57) SHA1(ba6a3d48c832d833df543f21d8aad98099894cff) )
	ROM_LOAD( "t3t01dy.p1", 0x0000, 0x010000, CRC(94bb082d) SHA1(236b81d7e10afd4dc3facfad30d9ab0a72ea21e3) )
	ROM_LOAD( "t3t01k.p1", 0x0000, 0x010000, CRC(b7bd2547) SHA1(6154b30a587d52dadd1fa137e805762e096f02cb) )
	ROM_LOAD( "t3t01r.p1", 0x0000, 0x010000, CRC(d4401ee1) SHA1(c270a0f191870136278ff8e47d529458bbc049d0) )
	ROM_LOAD( "t3t01s.p1", 0x0000, 0x010000, CRC(eae20667) SHA1(3ea054516e36ac5ca521a68bba16e299a1926c90) )
	ROM_LOAD( "t3t01y.p1", 0x0000, 0x010000, CRC(d2a6c8cd) SHA1(b30064145cebc2a39ae63009529cd8422dc98373) )
	ROM_LOAD( "tst01ad.p1", 0x0000, 0x010000, CRC(ca4be612) SHA1(70673e29a99ea0c70ff386c6c5fed49eabeea0e4) )
	ROM_LOAD( "tst01b.p1", 0x0000, 0x010000, CRC(7dd06165) SHA1(6853436a1b0c60283707932961bbf5cce7e185c0) )
	ROM_LOAD( "tst01bd.p1", 0x0000, 0x010000, CRC(38f4c0a2) SHA1(2cc62837c37618198dae7195e848c06ad1b96b06) )
	ROM_LOAD( "tst01d.p1", 0x0000, 0x010000, CRC(346adbf7) SHA1(5500822ea6838435ce28e084e2b13533e9467e85) )
	ROM_LOAD( "tst01dk.p1", 0x0000, 0x010000, CRC(2e8f8b14) SHA1(932b7c32d623dc8c2127959a4a0642d10ec22e71) )
	ROM_LOAD( "tst01dr.p1", 0x0000, 0x010000, CRC(191a1272) SHA1(8baae132505c469082af538b233f3fa3e7332d50) )
	ROM_LOAD( "tst01dy.p1", 0x0000, 0x010000, CRC(ef87da08) SHA1(9358e4a02def10c98fddbb21d81b62f83500aa69) )
	ROM_LOAD( "tst01k.p1", 0x0000, 0x010000, CRC(1f097f64) SHA1(d752b688b4e0520393bc4bef0c618a7c68c2e323) )
	ROM_LOAD( "tst01r.p1", 0x0000, 0x010000, CRC(ccc4ecb5) SHA1(0629c7951b56f44b8cb48d9aa66fb7c71ae275ec) )
	ROM_LOAD( "tst01s.p1", 0x0000, 0x010000, CRC(c4bb2a12) SHA1(1d8c134facfa72d8438676c96e530f93c41f1266) )
	ROM_LOAD( "tst01y.p1", 0x0000, 0x010000, CRC(e3ba4b94) SHA1(a7b13c172e5177711ddb81ef1ea77e27e14bf470) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "tttsnd01.p1", 0x0000, 0x080000, CRC(5518474c) SHA1(0b7e98e33f62d80882f2b0b4af0c9056f1ffb78d) )
ROM_END

ROM_START( m421club )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dtw27.bin", 0x0000, 0x010000, CRC(8e37977e) SHA1(8e996e50b2a87b97f999bfd00166c32240b74690) )
ROM_END



ROM_START( m4actbnk )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "acts.p1", 0x0000, 0x010000, CRC(49a9007c) SHA1(b205270e53264c3d8cb009a5780cacba1ce2e2a8) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "actb.p1", 0x0000, 0x010000, CRC(1429708e) SHA1(8b3ecb443e5920ccec80695a142cb1eb9596b1c1) )
	ROM_LOAD( "actbd.p1", 0x0000, 0x010000, CRC(727d7bb6) SHA1(765a9944ee27b175ba1f45bf82dcf7ef0defd076) )
	ROM_LOAD( "actc.p1", 0x0000, 0x010000, CRC(0a2498e5) SHA1(97f1e35426156c8eece6f76f3ecffa85714ade5b) )
	ROM_LOAD( "actd.p1", 0x0000, 0x010000, CRC(c89f6957) SHA1(3389e933e1ba7c1e32d43de572f24e2612e0ee99) )
	ROM_LOAD( "actdk.p1", 0x0000, 0x010000, CRC(1342ac16) SHA1(659618dc7c0c2d370e6c418620d2f399c1725aaf) )
	ROM_LOAD( "actdy.p1", 0x0000, 0x010000, CRC(045b2c9b) SHA1(4136dbb417e41b601394575a0f13ecb3bc89aecb) )
	ROM_LOAD( "actk.p1", 0x0000, 0x010000, CRC(fa555503) SHA1(8bf2b0b28453eedf6541f3471e8c6ffaba04de9d) )
	ROM_LOAD( "abank.hex", 0x0000, 0x00a000, CRC(2cd1a269) SHA1(5ce22b2736844a2de6cda04abdd0fe435391e033) )
	ROM_LOAD( "acty.p1", 0x0000, 0x010000, CRC(0ab8ff54) SHA1(7dc16ef1bbed2a5b2da3bf9eb4bbaf87b176954f) )
	ROM_LOAD( "actad.p1", 0x0000, 0x010000, CRC(a8dfdf77) SHA1(92e9f0f3837e466c0c6d98b890234d80318ef236) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "actsnd.p1", 0x000000, 0x080000, CRC(34777fea) SHA1(be784e73586719219ae5c1a3841f0e44edb6b497) )
	ROM_LOAD( "actsnd.p2", 0x080000, 0x080000, CRC(2e832d40) SHA1(622b2c9694714446dbf67beb67d03af97d14ece7) )
ROM_END

ROM_START( m4actclb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "abcs.p1", 0x0000, 0x010000, CRC(cf730606) SHA1(35b95b924a24b306428c6c81136c14d7732e2356) )

	ROM_REGION( 0x10000, "altrevs", 0 )// also contains what looks like 68k code?
	ROM_LOAD( "a2c1-1mkii.bin", 0x0000, 0x010000, CRC(4c8ee662) SHA1(17e710c2bda21db609b619dfc0c9280a211da151) )
ROM_END

ROM_START( m4actnot )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "an12.bin", 0x0000, 0x010000, CRC(54c6a33b) SHA1(91870c46b538abf56c356c96290cfedcf41db21f) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "an22c.bin", 0x0000, 0x010000, CRC(54c6a33b) SHA1(91870c46b538abf56c356c96290cfedcf41db21f) )
ROM_END

ROM_START( m4actpak )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "action.hex", 0x0000, 0x010000, CRC(c5808b5d) SHA1(577950166c91e7f1ca390ebcf34be2da945c0a5f) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ap04_p1.bin", 0x0000, 0x008000, CRC(16a16ec0) SHA1(ff3b9413864572b3a2fadfe13d73f60928d9ae73) )
	ROM_LOAD( "ap04_p2.bin", 0x0000, 0x004000, CRC(75b77ba0) SHA1(cac8350a9a3c2bd6770d0e2dfb133ff06f5b3db3) )
ROM_END

ROM_START( m4addrd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dal12.bin", 0x0000, 0x010000, CRC(4affa79a) SHA1(68bceab42b3616641a34a64a83306175ffc1ce32) )
ROM_END

ROM_START( m4addr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "a6ls.p1", 0x0000, 0x010000, CRC(9f97f57b) SHA1(402d1518bb78fdc489b06c2aabc771e5ce151847) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "5p4addersladders.bin", 0x0000, 0x010000, CRC(03fc43da) SHA1(cf2fdb0d1ad702331ba004fd39072484b05e2b97) )
	ROM_LOAD( "a6lbdy.p1", 0x0000, 0x010000, CRC(28064099) SHA1(c916f73911974440d4c79ecb51b343aad78f115b) )
	ROM_LOAD( "a6lc.p1", 0x0000, 0x010000, CRC(1e75fe67) SHA1(4497b19d4c512c934d445b4acf607dc2dc080d44) )
	ROM_LOAD( "a6ld.p1", 0x0000, 0x010000, CRC(de555e12) SHA1(2233160f1c734c889c1c00dee202a928f18ad763) )
	ROM_LOAD( "a6ldy.p1", 0x0000, 0x010000, CRC(82f060a5) SHA1(2e8474e6c17def07e35448b5bf8d453cce0f292c) )
	ROM_LOAD( "a6lk.p1", 0x0000, 0x010000, CRC(af5ae5c4) SHA1(20e40cf996c2c3b7b18ec104a374be1da193b94e) )
	ROM_LOAD( "a6pcx.p1", 0x0000, 0x010000, CRC(517d8c9a) SHA1(487cecfb10b24eff1582ca6bc97a2dc004e65b0f) )
	ROM_LOAD( "a6ps.p1", 0x0000, 0x010000, CRC(41e375c7) SHA1(93556a90227cde6814123c8a7f29f734884e182c) )
	ROM_LOAD( "ad05.6c", 0x0000, 0x010000, CRC(0940e4aa) SHA1(e8e7f7249a18386af990999a4c06f001db7003c5) )
	ROM_LOAD( "adders ladders 20p 6.bin", 0x0000, 0x010000, CRC(62abeb34) SHA1(8069e6fde0673fdbc124a1a172dc988bb3205ff6) )
	ROM_LOAD( "adl5pv2", 0x0000, 0x010000, CRC(09c39527) SHA1(16af3e552a7d6c6b802d2b1923523e9aa9de766a) )

	// these appear to be MPU4 VIDEO (incomplete / bad?)
	//ROM_LOAD( "a6ppl.p0", 0x0000, 0x020000, CRC(350da2df) SHA1(a390e0c7e1e624c17f0e254e0b99ef9dbf56269d) )
	//ROM_LOAD( "a6ppl.p1", 0x0000, 0x020000, CRC(63038aba) SHA1(8ec4e02109e872460a9598e469b59919cc5450dd) )
	//ROM_LOAD( "al.p2", 0x0000, 0x010000, CRC(d5909619) SHA1(eb8d978ed28cd3afab6b3c1274d79bc6b188d240) )
	//ROM_LOAD( "al.p3", 0x0000, 0x010000, CRC(5dc65d9f) SHA1(52dffb162a0e5649e056817e9db567cc7653538c) )
	//ROM_LOAD( "al.p4", 0x0000, 0x010000, CRC(17c77264) SHA1(321372c1c0d25cfc5a1302ccfd5680d9bf62d31b) )
	//ROM_LOAD( "ald.p1", 0x0000, 0x010000, CRC(ecc7c79c) SHA1(e03f470d0b83ed81af737a1d16a02528df733149) )
	//ROM_LOAD( "als.p1", 0x0000, 0x010000, CRC(c2c4ceea) SHA1(0c5ea4d02ba053191855547b714da9a7f61f3713) )
	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ad_05___.1o3", 0x0000, 0x010000, CRC(8d9e0f5d) SHA1(fecc844908876e161d0134ce3cc098e79e74e0b1) )
	ROM_LOAD( "ad_05___.3q3", 0x0000, 0x010000, CRC(ec6ed7ce) SHA1(dfad04b5f6c4ff0fd784ad20471f1cf84586f2cd) )
	ROM_LOAD( "ad_05___.4s3", 0x0000, 0x010000, CRC(6d1a3c51) SHA1(0e4b985173c7c3bd5804573d99913d66a05d54fb) )
	ROM_LOAD( "ad_05___.5a3", 0x0000, 0x010000, CRC(9821a988) SHA1(2be85a0b68e5e31401a5c753b40f3cf803589444) )
	ROM_LOAD( "ad_05___.5n3", 0x0000, 0x010000, CRC(86ac3564) SHA1(1dd9cf39d2aee11a3e1bbc68460c12f10e62aeaf) )
	ROM_LOAD( "ad_05_b_.1o3", 0x0000, 0x010000, CRC(b10b194a) SHA1(4dc3f14ff3b903c49829f4a91136f9b03a5cb1ae) )
	ROM_LOAD( "ad_05_b_.3q3", 0x0000, 0x010000, CRC(d4c06db1) SHA1(dacb66b98f9d1d51eddc48b6946d517c277e588e) )
	ROM_LOAD( "ad_05_b_.4s3", 0x0000, 0x010000, CRC(6bd6fdb6) SHA1(7ee1e80da5833b3eaf4b23035690a09379781584) )
	ROM_LOAD( "ad_05_b_.5a3", 0x0000, 0x010000, CRC(592cb1ae) SHA1(5696ecb3e9e6419f73087120b6a832fde606bacc) )
	ROM_LOAD( "ad_05_b_.5n3", 0x0000, 0x010000, CRC(cdc8ca39) SHA1(33fdeef8ab8908f6908120aedf501ec3e9d7d23e) )
	ROM_LOAD( "ad_05_d_.1o3", 0x0000, 0x010000, CRC(2d29040f) SHA1(ee2bdd5da1a7e4146419ffd8bad521a9c1b49aa2) )
	ROM_LOAD( "ad_05_d_.3q3", 0x0000, 0x010000, CRC(8d05fba9) SHA1(9c69d7eec7ce0d647d4f8b8b0a6b7e54daa7a79f) )
	ROM_LOAD( "ad_05_d_.4s3", 0x0000, 0x010000, CRC(e672baf0) SHA1(bae2e2fe9f51b3b8da20fcefb145f6d35fa2d604) )
	ROM_LOAD( "ad_05_d_.5a3", 0x0000, 0x010000, CRC(b5be8114) SHA1(28dfe1d1cc1d9fc2bcc13fd6437602a6e8c90de2) )
	ROM_LOAD( "ad_05_d_.5n3", 0x0000, 0x010000, CRC(ca2653d5) SHA1(30cd35627be8fb4fff2f0d61a6ab43cf3e4c1742) )
	ROM_LOAD( "ad_10___.1o3", 0x0000, 0x010000, CRC(d587cb00) SHA1(6574c42402f13e5f9cb8f951e0f59b499b2d025d) )
	ROM_LOAD( "ad_10___.4a3", 0x0000, 0x010000, CRC(9151dac3) SHA1(bf1c065a62e84a8073f8f9854981bedad60805be) )
	ROM_LOAD( "ad_10_b_.1o3", 0x0000, 0x010000, CRC(e2b5c0db) SHA1(9e1716186bb049c61dddaef2465fb1e55d2d93fd) )
	ROM_LOAD( "ad_10_d_.1o3", 0x0000, 0x010000, CRC(d7670d32) SHA1(09dfe2a7fe267f485efed234411efc92d9cce414) )
	ROM_LOAD( "ad_20___.3a3", 0x0000, 0x010000, CRC(c2431657) SHA1(b2b7541207eb3c898f9cf3df520bff396213b78a) )
	ROM_LOAD( "ad_20___.3n3", 0x0000, 0x010000, CRC(883ff001) SHA1(50540270dba31820ad99a4a4034c69d4a58d87c5) )
	ROM_LOAD( "ad_20___.3s3", 0x0000, 0x010000, CRC(b1d54cb6) SHA1(35205975ccdaccd5bf3c1b7bf9a26c5ef30050b3) )
	ROM_LOAD( "ad_20_b_.3a3", 0x0000, 0x010000, CRC(19990a19) SHA1(ab1031513fb1e499da4a3001b5b26ff1e86cc628) )
	ROM_LOAD( "ad_20_b_.3n3", 0x0000, 0x010000, CRC(65f9946f) SHA1(6bf6f315ed2dc6f603381d36dd408e951ace76bc) )
	ROM_LOAD( "ad_20_b_.3s3", 0x0000, 0x010000, CRC(86982248) SHA1(a6d876333777a29eb0504fa3636727ebcc104f0a) )
	ROM_LOAD( "ad_20_d_.3a3", 0x0000, 0x010000, CRC(62304025) SHA1(59b7815bf1b5337f46083cef186fedd078a4ad37) )
	ROM_LOAD( "ad_20_d_.3n3", 0x0000, 0x010000, CRC(cf254a00) SHA1(1e430b652e4023e28b5648b8bea63e778c6dafc9) )
	ROM_LOAD( "ad_20_d_.3s3", 0x0000, 0x010000, CRC(89d2301b) SHA1(62ad1a9e008063eb16442b50af806f061669dba7) )
	ROM_LOAD( "adi05___.1o3", 0x0000, 0x010000, CRC(050764b1) SHA1(364c50e4887c9fdd7ff62e63a6be4513336b4814) )
	ROM_LOAD( "adi05___.4s3", 0x0000, 0x010000, CRC(a4343a89) SHA1(cef67bbe03e6f535b530fc099f1b9a8bc7a2f864) )
	ROM_LOAD( "adi05___.5a3", 0x0000, 0x010000, CRC(03777f8c) SHA1(9e3fddc2130600f343df0531bf3e636b82c2f108) )
	ROM_LOAD( "adi05___.5n3", 0x0000, 0x010000, CRC(13003560) SHA1(aabad24748f9b1b09f1820bf1af932160e64fe3e) )
	ROM_LOAD( "adi10___.1o3", 0x0000, 0x010000, CRC(005caaa1) SHA1(b4b421c045012b5fbeaca95fa09d087a9c5e6b5b) )
	ROM_LOAD( "adi10___.4a3", 0x0000, 0x010000, CRC(2d2aa3cc) SHA1(21a7690c3fb7d158f4b4e6da63663778246ac902) )
	ROM_LOAD( "adi10___.4n3", 0x0000, 0x010000, CRC(af9aad00) SHA1(09729e73f27d9ac5d6ac7171191ed76aeaac3e3d) )

ROM_END

ROM_START( m4addrc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "add05_101", 0x0000, 0x010000, CRC(4b3fb104) SHA1(9dba619019a476ce317122a3553965b279c684ba) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "add10_101", 0x0000, 0x010000, CRC(af8f8b4e) SHA1(712c33ed0f425dc10b79780b0cfce0ac5768e2d5) )
	ROM_LOAD( "add20_101", 0x0000, 0x010000, CRC(361b7173) SHA1(dea2b1b0f5910e2fd3f45d220554f0e712dedada) )
	ROM_LOAD( "add55", 0x0000, 0x010000, CRC(48c5bc73) SHA1(18c9f70bad6141cca95b6bbcb4fc621e71f87700) )
	ROM_LOAD( "alddr20", 0x0000, 0x010000, CRC(19cf4437) SHA1(b528823c476bebd1a9a6c720a4144294743693d2) )
	ROM_LOAD( "classic adders & ladders (labelled - nudge w 5p 10p 1p10).bin", 0x0000, 0x010000, CRC(ac948903) SHA1(e07023efd7722a661a2bbf93c0a168af70ad6c20) )
	ROM_LOAD( "classic adders & ladders (labelled connect 4 v1-9 (27512) 15-3-90)", 0x0000, 0x010000, CRC(843ed53d) SHA1(b1dff249df37800744e3fc9c32be20a62bd130a1) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ad_05s___183.bin", 0x0000, 0x010000, CRC(8d9e0f5d) SHA1(fecc844908876e161d0134ce3cc098e79e74e0b1) )
	ROM_LOAD( "adders classic.bin", 0x0000, 0x010000, CRC(6bc1d2aa) SHA1(cf17e697ff0cfba999f6511f24051dbc3d0384ef) )
	ROM_LOAD( "addl_10_.4", 0x0000, 0x010000, CRC(c2d11126) SHA1(0eafe9dc30013ed5817ac303a4eea5ea82d62715) )
	ROM_LOAD( "addl_10_.8", 0x0000, 0x010000, CRC(9fc82c47) SHA1(0f56afc33f09fe22afc5ec74aeb496c32f9e623c) )
	ROM_LOAD( "addl_20_.8", 0x0000, 0x010000, CRC(43c98f46) SHA1(0ca4a093b38fc04639e3f4bb742a8923b90d2ed1) )
	ROM_LOAD( "al10", 0x0000, 0x010000, CRC(3c3c82b6) SHA1(cc5ffdd0837c9af31d5737a70430a01d1989cdcc) )
	ROM_LOAD( "alad58c", 0x0000, 0x010000, CRC(df9c46b8) SHA1(439ea1ce17aa89e19cedb78465b4388b72c8c5ed) )
	ROM_LOAD( "nik56c", 0x0000, 0x010000, CRC(05fa11d1) SHA1(01d3d0c504489f1513a0c3aa26e910c9604f5366) )

ROM_END

ROM_START( m4addrcc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "adcd.p1", 0x0000, 0x010000, CRC(47e41c9a) SHA1(546aaaa5765b3bc91eeb9bf5a979ed68a2e72da8) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "adcf.p1", 0x0000, 0x010000, CRC(1dbbc990) SHA1(fb9439b43089e3135a719ab94b24dd65561d17cf) )
	ROM_LOAD( "adcl.p1", 0x0000, 0x010000, CRC(89299196) SHA1(9a92b250b47b11536f8708429d69c95111ecdb98) )
	ROM_LOAD( "adcs.p1", 0x0000, 0x010000, CRC(7247de78) SHA1(e390b4e912d7bc8c1ca5e42bf2e2753d4c2b4d17) )
	ROM_LOAD( "adrscfm", 0x0000, 0x010000, CRC(6c95881a) SHA1(db658bd722c54fc84734105f1a9b0028b23179fb) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "aal.chr", 0x0000, 0x000048, CRC(bb48409f) SHA1(adefde520104b8c3815260ee136460ddf3e9e4b2) )
ROM_END

ROM_START( m4addrcb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "adcd.p1", 0x0000, 0x010000, CRC(47e41c9a) SHA1(546aaaa5765b3bc91eeb9bf5a979ed68a2e72da8) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "adcf.p1", 0x0000, 0x010000, CRC(1dbbc990) SHA1(fb9439b43089e3135a719ab94b24dd65561d17cf) )
	ROM_LOAD( "adcl.p1", 0x0000, 0x010000, CRC(89299196) SHA1(9a92b250b47b11536f8708429d69c95111ecdb98) )
	ROM_LOAD( "adcs.p1", 0x0000, 0x010000, CRC(7247de78) SHA1(e390b4e912d7bc8c1ca5e42bf2e2753d4c2b4d17) )
	ROM_LOAD( "adrscfm", 0x0000, 0x010000, CRC(6c95881a) SHA1(db658bd722c54fc84734105f1a9b0028b23179fb) )
	ROM_LOAD( "alad1", 0x0000, 0x002000, CRC(e10949e8) SHA1(9fd189d40a32caf0866f77242573777b3d44e316) ) // bad dump? (it's just the fisrt 8kb of the rom)

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "aal.chr", 0x0000, 0x000048, CRC(bb48409f) SHA1(adefde520104b8c3815260ee136460ddf3e9e4b2) )
ROM_END

ROM_START( m4alladv )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "c2a60p1.bin", 0x8000, 0x008000, CRC(6630b4a4) SHA1(e4b35fde196d544b41e1dc9cee94442cc5c7223f) )
	ROM_LOAD( "c2a60p2.bin", 0x4000, 0x004000, CRC(0f0c9c2d) SHA1(7dc154048aeaf2842e9eb748a9d6d197cc429c69) )
ROM_END

ROM_START( m4alpha )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "alphabet.hex", 0x6000, 0x00a000, CRC(2bf0d7fd) SHA1(143543f45bfae379233a8c21959618e5ad8034e4) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "m435.chr", 0x0000, 0x000048, CRC(1e8df0e3) SHA1(02ecde48b9ce49884062eeea4a399c8d52bbe323) )
ROM_END

ROM_START( m4ambass )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ambassador.bin", 0x0000, 0x010000, CRC(310313ac) SHA1(8e11515615754090d716b428adc4e2718ee1211d) )
ROM_END

ROM_START( m4amhiwy )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dah20", 0x0000, 0x010000, CRC(e3f92f00) SHA1(122c8a429a1f75dac80b90c4f218bd311813daf5) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sdr6_1.snd", 0x000000, 0x080000, CRC(63ad952d) SHA1(acc0ac3898fcc281e2d7ba19ada52d727885fe06) )
	ROM_LOAD( "sdr6_2.snd", 0x080000, 0x080000, CRC(48d2ace5) SHA1(ada0180cc60266c0a6d981a019d66bbedbced21a) )
ROM_END

ROM_START( m4andycpd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dac13.bin", 0x0000, 0x010000, CRC(a0cdd5b3) SHA1(7b7bc40a9a9aed3569f491acad15c606fe243e9b) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "sdac_1.snd", 0x000000, 0x080000, CRC(5ce93532) SHA1(547f98740889e6fbafc5a0c517ff75de41f2acc7) )
	ROM_LOAD( "sdac_2.snd", 0x080000, 0x080000, CRC(22dacd4b) SHA1(ad2dc943d4e3ec54937acacb963da938da809614) )
	ROM_LOAD( "sjcv2.snd", 0x080000, 0x080000, CRC(f247ba83) SHA1(9b173503e63a4a861d1380b2ab1fe14af1a189bd) )
ROM_END

ROM_START( m4andycp )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ac10.hex", 0x0000, 0x010000, CRC(0e250923) SHA1(9557315cca7a47c307e811d437ff424fe77a2843) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ac056c", 0x0000, 0x010000, CRC(cdeaeb06) SHA1(5bfcfba614477f4df9f4b2e56e8448eb357c554a) )
	ROM_LOAD( "ac058c", 0x0000, 0x010000, CRC(15204ccc) SHA1(ade376193bc2d53dd4c824ee35fbcc16da31330a) )
	ROM_LOAD( "ac_05_d4.2_1", 0x0000, 0x010000, CRC(f672182a) SHA1(55a6691fa9878bc2becf1f080915c0cd939240dd) )
	ROM_LOAD( "acap05_11", 0x0000, 0x010000, CRC(fb1533a0) SHA1(814e5dd9c4fe3baf4ea3b22c7e02e30b07bd27a1) )
	ROM_LOAD( "acap10_11", 0x0000, 0x010000, CRC(c3a866e7) SHA1(4c18e5a26ad2885eb012fd3dd61aaf9cc7d3519a) )
	ROM_LOAD( "acap2010", 0x0000, 0x010000, CRC(1b8e712b) SHA1(6770869966290fe6e61b7bf1971ab7a15e601d69) )
	ROM_LOAD( "acap20_11", 0x0000, 0x010000, CRC(799fd89e) SHA1(679016fad8b012bf6b6c617b99fd0dbe71eff562) )
	ROM_LOAD( "acap20p", 0x0000, 0x010000, CRC(f0a9a4a4) SHA1(3c9a2e3d90ea91f92ae500856ad97c376edc1548) )
	ROM_LOAD( "acap55", 0x0000, 0x010000, CRC(8007c459) SHA1(b3b6213d89eb0d2cc2f7dab81e0f0f2fdd0f8776) )
	ROM_LOAD( "acapp10p5.bin", 0x0000, 0x010000, CRC(de650e19) SHA1(c1b9cbad23a1eac9b3718f4f2457c97317f96be6) )
	ROM_LOAD( "acp8ac", 0x0000, 0x010000, CRC(d51997b5) SHA1(fe08b5a3832eeaa80f674893342c3baea1608a91) )
	ROM_LOAD( "an8ad.p1", 0x0000, 0x010000, CRC(d0f9da00) SHA1(fb380897fffc33d238b8fe7d47ff4d9d97960283) )
	ROM_LOAD( "an8b.p1", 0x0000, 0x010000, CRC(fc4001ae) SHA1(b0cd795235e6f500f0150097b8f760165c17ca27) )
	ROM_LOAD( "an8c.p1", 0x0000, 0x010000, CRC(35a4403e) SHA1(33d3ca4e7bad25d064e0780c2104c395259c2a94) )
	ROM_LOAD( "an8d.p1", 0x0000, 0x010000, CRC(ae01af1c) SHA1(7b2305480a318648a3cc6c3bc66f21ac327e25aa) )
	ROM_LOAD( "an8dk.p1", 0x0000, 0x010000, CRC(d43ad86d) SHA1(a71f1eb26e5f688db675b5c6bddda713e709a7af) )
	ROM_LOAD( "an8dy.p1", 0x0000, 0x010000, CRC(6730e476) SHA1(d19f7d173ec18085ef904c8621e81305bd54a143) )
	ROM_LOAD( "an8k.p1", 0x0000, 0x010000, CRC(296b4453) SHA1(060a6cea9a0be923e359dd69e34a6c25d631e4e5) )
	ROM_LOAD( "an8s.p1", 0x0000, 0x010000, CRC(14ac28da) SHA1(0b4a3f997e10573f2c4c44daac344f4be52363a0) )
	ROM_LOAD( "an8y.p1", 0x0000, 0x010000, CRC(44da57c9) SHA1(0f2776214068400a0e30b5642f42d72f58bbc29b) )
	ROM_LOAD( "andc.p1", 0x0000, 0x010000, CRC(31735e79) SHA1(7247efbfe41dce04dd494f07a8871f34d76eaacd) )
	ROM_LOAD( "andd.p1", 0x0000, 0x010000, CRC(d48a42fb) SHA1(94e3b994b9425af9a7744d511ad3413a79e24f21) )
	ROM_LOAD( "anddy.p1", 0x0000, 0x010000, CRC(7f24b95d) SHA1(0aa97ad653b24265d73577db61200e44abf11c50) )
	ROM_LOAD( "andk.p1", 0x0000, 0x010000, CRC(08e6d20f) SHA1(f66207f69bf417e9380ecc8bd2ba73c6f3d55150) )
	ROM_LOAD( "ands.p1", 0x0000, 0x010000, CRC(120967eb) SHA1(f47846e5f1c6300518104341740e66610b9a9ab3) )
	ROM_LOAD( "andy.p1", 0x0000, 0x010000, CRC(b1124803) SHA1(0f3422e5f048d1748d2c912f2ea56f206fd101bb) )
	ROM_LOAD( "c2t02ad.p1", 0x0000, 0x010000, CRC(38e36fe3) SHA1(01c007e21a6ac1a77bf314402d727c41b7a222ca) )
	ROM_LOAD( "c2t02b.p1", 0x0000, 0x010000, CRC(f059a9dc) SHA1(0c5d5a4b108c85215b9d5f8c2263b66559cfa90a) )
	ROM_LOAD( "c2t02bd.p1", 0x0000, 0x010000, CRC(0eff02a9) SHA1(f460f93098630ac2757a560deb2e741ae9631a54) )
	ROM_LOAD( "c2t02d.p1", 0x0000, 0x010000, CRC(ce5bbf2e) SHA1(dab2a1015713ceb8dce8b766fc2660207fcbb9f2) )
	ROM_LOAD( "c2t02dk.p1", 0x0000, 0x010000, CRC(dc8c078e) SHA1(9dcde48d17a39dbe10333632eacc1f0860e165de) )
	ROM_LOAD( "c2t02dr.p1", 0x0000, 0x010000, CRC(7daee156) SHA1(2ae03c39ca5704c112c9ec6acba46022f4dd9805) )
	ROM_LOAD( "c2t02dy.p1", 0x0000, 0x010000, CRC(e0c5c9b8) SHA1(d067d4786ded041d8031808078eb2c0383937931) )
	ROM_LOAD( "c2t02k.p1", 0x0000, 0x010000, CRC(077024e0) SHA1(80597f28891caa25506bb6bbc77a005623096ff9) )
	ROM_LOAD( "c2t02r.p1", 0x0000, 0x010000, CRC(6ccaf958) SHA1(8878e16d2c01131d36f211b3a73e987409f54ef9) )
	ROM_LOAD( "c2t02s.p1", 0x0000, 0x010000, CRC(d004f962) SHA1(1f211fd62438cb7c5d5f4ce9ced29a0a7e64e80b) )
	ROM_LOAD( "c2t02y.p1", 0x0000, 0x010000, CRC(f1a1d1b6) SHA1(d9ceedee3b833be8de5b065e45a72ca180283528) )
	ROM_LOAD( "c5tad.p1", 0x0000, 0x010000, CRC(dab92a37) SHA1(30297a7e1a995b76d8f955fd8a40efc914874e29) )
	ROM_LOAD( "c5tb.p1", 0x0000, 0x010000, CRC(1a747871) SHA1(61eb026c2d35feade5cfecf609e99cd0c6d0693e) )
	ROM_LOAD( "c5tbd.p1", 0x0000, 0x010000, CRC(b0fb7c1c) SHA1(f5edf7685cc7015ac9791d35dde3fd284180660f) )
	ROM_LOAD( "c5td.p1", 0x0000, 0x010000, CRC(ab359cae) SHA1(f8ab817709e0eeb91a059cdef19df99c6286bf3f) )
	ROM_LOAD( "c5tdk.p1", 0x0000, 0x010000, CRC(295976d6) SHA1(a506097e94d290f5b66f61c9979b0ae4f211bb0c) )
	ROM_LOAD( "c5tdy.p1", 0x0000, 0x010000, CRC(d9b4dc81) SHA1(e7b7a5f9b1ad348444d5403df2bf16b829364d33) )
	ROM_LOAD( "c5tk.p1", 0x0000, 0x010000, CRC(26a1d1f6) SHA1(c64763188dd0520c3f802863d36c84a476efef40) )
	ROM_LOAD( "c5ts.bin", 0x0000, 0x010000, CRC(3ade4b1b) SHA1(c65d05e2493a0e2d6a4be58a42aac6cb7f9c01b5) )
	ROM_LOAD( "c5ts.p1", 0x0000, 0x010000, CRC(3ade4b1b) SHA1(c65d05e2493a0e2d6a4be58a42aac6cb7f9c01b5) )
	ROM_LOAD( "c5ty.p1", 0x0000, 0x010000, CRC(52953040) SHA1(65102c88e8766e07d268fe0267bc6731d8b3eeb3) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ac_05a__.1_1", 0x0000, 0x010000, CRC(880c2532) SHA1(a6a3c996c7507f0e2b8ae8e9fdfb7473263bd5cf) )
	ROM_LOAD( "ac_05s__.1_1", 0x0000, 0x010000, CRC(eab8aaca) SHA1(ccec86cf44f97a894192b2a6f900a93d26e84bf9) )
	ROM_LOAD( "ac_05sb_.1_1", 0x0000, 0x010000, CRC(dfd2571b) SHA1(98d93e30f4684fcbbc5ce4f356b8c9eeb20cbbdb) )
	ROM_LOAD( "ac_05sd_.1_1", 0x0000, 0x010000, CRC(4c815831) SHA1(66c6a4fed60ecc5ff5c9202528797d044fde3e76) )
	ROM_LOAD( "ac_10a__.1_1", 0x0000, 0x010000, CRC(c8a1150b) SHA1(99ba283aeacd1c415d261e10b5b7fd43d3c25af8) )
	ROM_LOAD( "ac_10sb_.1_1", 0x0000, 0x010000, CRC(f68f8f48) SHA1(a156d942e7ab7446290dcd8def6236e7436126b9) )
	ROM_LOAD( "ac_10sd_.1_1", 0x0000, 0x010000, CRC(ec800208) SHA1(47734ae5a3184e4805a7620287fb5da7fe823929) )
	ROM_LOAD( "acap_05_.4", 0x0000, 0x010000, CRC(ca00ee84) SHA1(f1fef3db3db5ca7f0eb72ccc1daba8446db02924) )
	ROM_LOAD( "acap_05_.8", 0x0000, 0x010000, CRC(a17dd8de) SHA1(963d39fdca7c7b54f5ecf723c982eb30a426ebae) )
	ROM_LOAD( "acap_10_.4", 0x0000, 0x010000, CRC(fffe742d) SHA1(f2ca45391690dc31662e2d97a3ee34473effa258) )
	ROM_LOAD( "acap_10_.8", 0x0000, 0x010000, CRC(614403a7) SHA1(b627c7c3c6f9a43a0cd9e064715aeee8834c717c) )
	ROM_LOAD( "acap_20_.4", 0x0000, 0x010000, CRC(29848eed) SHA1(4096ab2f58b3293c559ff69c6f0f4d6c5dee2fd2) )
	ROM_LOAD( "acap_20_.8", 0x0000, 0x010000, CRC(3981ec67) SHA1(ad040a4c8690d4348bfe306309df5374251f2b3e) )
	ROM_LOAD( "aci05___.1_1", 0x0000, 0x010000, CRC(e06174e8) SHA1(e984e45b99d4aef9b46c83590efadbdec9888b2d) )
	ROM_LOAD( "aci10___.1_1", 0x0000, 0x010000, CRC(afa29daa) SHA1(33d161977b1e3512b550980aed48954ba7f0c5a2) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "ac.chr", 0x0000, 0x000048, CRC(87808826) SHA1(df0915a6f89295efcd10e6a06bfa3d3fe8fef160) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "acapp1.bin",  0x000000, 0x080000, CRC(bda61fc6) SHA1(b94f39fa92d3d0cb580eaafa0f58bd5cde947e3a) )
	ROM_LOAD( "andsnd.bin",  0x000000, 0x080000, CRC(7d568671) SHA1(3a0a6af3dc980f2ccff0b6ef85833eb2e352031a) )
	ROM_LOAD( "andsnd2.bin", 0x080000, 0x080000, CRC(98a586ee) SHA1(94b94d198725e8174e14873b99afa19217a1d4fa) )
ROM_END

ROM_START( m4andyfl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "andy loves flo 05a 4  2-1(27512)", 0x0000, 0x010000, CRC(773d2c6f) SHA1(944be6fff70439077a9c0d858e76806e0317585c) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "al3ad.p1", 0x0000, 0x010000, CRC(6d057dd3) SHA1(3febe5aea14852559de554c2e034c328393ae0fa) )
	ROM_LOAD( "al3b.p1", 0x0000, 0x010000, CRC(4b967a4f) SHA1(1a6e24ecaa907a5bb6fa589dd0de473c7e4c6f6c) )
	ROM_LOAD( "al3bd.p1", 0x0000, 0x010000, CRC(5b191099) SHA1(9049ff924123ee9309155730d53cb168bd8237bf) )
	ROM_LOAD( "al3d.p1", 0x0000, 0x010000, CRC(621b5831) SHA1(589e5a94324a56704b1a05bafe16bf6d838dea6c) )
	ROM_LOAD( "al3dk.p1", 0x0000, 0x010000, CRC(5a77dcf2) SHA1(63e67ca1e112b56ea99b3c91952fa9b04518d6ae) )
	ROM_LOAD( "al3dy.p1", 0x0000, 0x010000, CRC(c7bdd13e) SHA1(674cad23b7d6299918951de5dbbb33acf01dac66) )
	ROM_LOAD( "al3k.p1", 0x0000, 0x010000, CRC(f036b844) SHA1(62269e3ed0c6fa5df592883294efc74da856d897) )
	ROM_LOAD( "al3s.p1", 0x0000, 0x010000, CRC(07d4d6c3) SHA1(d013cf49ed4b84e6149065c95d1cd00eca0d62b8) )
	ROM_LOAD( "al3y.p1", 0x0000, 0x010000, CRC(1cce9f53) SHA1(aaa8492ea28cc0134ae7d070e182a3f98e769c40) )
	ROM_LOAD( "al8ad.p1", 0x0000, 0x010000, CRC(90a72618) SHA1(2c11e98b446500da9b618c8a7a9d441cff916851) )
	ROM_LOAD( "al8b.p1", 0x0000, 0x010000, CRC(3c0324e9) SHA1(5ddf33b06728de62d995cdbfc6bdc9e711661e38) )
	ROM_LOAD( "al8bd.p1", 0x0000, 0x010000, CRC(a6bb4b52) SHA1(0735c45c3f02a3f17dfbe1f744a8685de97fdd8f) )
	ROM_LOAD( "al8c.p1", 0x0000, 0x010000, CRC(154b0f79) SHA1(e178404674ace57c639c90a44e5f03803ec812d0) )
	ROM_LOAD( "al8d.p1", 0x0000, 0x010000, CRC(c1cb8f01) SHA1(c267c208c23bb7816f5475b0c0db2d69c6b98970) )
	ROM_LOAD( "al8dk.p1", 0x0000, 0x010000, CRC(bf346ace) SHA1(fdf0e5550caaae9e63ac5ea571e290fec4c768af) )
	ROM_LOAD( "al8k.p1", 0x0000, 0x010000, CRC(77d8f8b4) SHA1(c91fe5a543ba83b68fe3285da55d77f7b93131db) )
	ROM_LOAD( "al8s.p1", 0x0000, 0x010000, CRC(37e211f9) SHA1(8614e8081fdd370d6c3dd537ee6058a2247d4ae0) )
	ROM_LOAD( "al8y.p1", 0x0000, 0x010000, CRC(c77ee4c2) SHA1(fc5cb6aff5e5aeaf577cb0b9ed2e1ac06359089e) )
	ROM_LOAD( "alfb.p1", 0x0000, 0x010000, CRC(3133c954) SHA1(49bedc54c7d39b3cf40c19a0e56a8bea798aeba7) )
	ROM_LOAD( "alfc.p1", 0x0000, 0x010000, CRC(c0fc9244) SHA1(30c7929a95e67b6a10877087a337b34a726b0ec9) )
	ROM_LOAD( "alfk.p1", 0x0000, 0x010000, CRC(f9691e32) SHA1(9b72a9c78de8979568a720e5e1986734063defac) )
	ROM_LOAD( "alfr.p1", 0x0000, 0x010000, CRC(acc4860b) SHA1(cbae236c5e1bdbb294f99cd749067d41a24e8973) )
	ROM_LOAD( "alfs.p1", 0x0000, 0x010000, CRC(5c0e14f6) SHA1(ebce737afb71b27829d69ff203ff86a828df946a) )
	ROM_LOAD( "alt04ad.p1", 0x0000, 0x010000, CRC(b0a332c5) SHA1(b0bbd193d44543c0f8cfe8c51b7956de84b9af10) )
	ROM_LOAD( "alt04b.p1", 0x0000, 0x010000, CRC(9da0465d) SHA1(2f02f739bf9ef8fa4bcb163f1a881052fc3d483f) )
	ROM_LOAD( "alt04bd.p1", 0x0000, 0x010000, CRC(86bf5f8f) SHA1(c310ac44f85e5883a8d4ed369c4b68c0aebe2820) )
	ROM_LOAD( "alt04d.p1", 0x0000, 0x010000, CRC(c6b36e95) SHA1(5e4e8fd1a2f0411be1ab5c0bbae1f9cd8062f234) )
	ROM_LOAD( "alt04dk.p1", 0x0000, 0x010000, CRC(596abc65) SHA1(c534852c6dd8e6574529cd1da665dc60147b71de) )
	ROM_LOAD( "alt04dr.p1", 0x0000, 0x010000, CRC(9237bdfc) SHA1(630bed3c4e17774f30a7fc26aa69c69054bffdd9) )
	ROM_LOAD( "alt04dy.p1", 0x0000, 0x010000, CRC(f6750d3e) SHA1(1c87a7f574e9db45cbfe3e9bf4600a68cb6d5bd4) )
	ROM_LOAD( "alt04k.p1", 0x0000, 0x010000, CRC(a5818f6d) SHA1(bd69e9e4e05cedfc044ae91a12e84d73e19a50ac) )
	ROM_LOAD( "alt04r.p1", 0x0000, 0x010000, CRC(a1d4caf8) SHA1(ced673abb0a3f6f72c441f26eabb473e0a1b2fd7) )
	ROM_LOAD( "alt04s.p1", 0x0000, 0x010000, CRC(81cf27b3) SHA1(b04970a20a297032cf33dbe97fa22fb723587228) )
	ROM_LOAD( "alt04y.p1", 0x0000, 0x010000, CRC(c63a5a57) SHA1(90bb47cb87dcdc875546be64d9cf9e8cf9e15f97) )
	ROM_LOAD( "alu0.3s", 0x0000, 0x010000, CRC(87704898) SHA1(47fe7b835619e770a58c71796197a0d2810a8e9b) )
	ROM_LOAD( "alu03ad.p1", 0x0000, 0x010000, CRC(3e6f03b5) SHA1(0bc85442b614091a25529b7ae2fc7e907d8d82e8) )
	ROM_LOAD( "alu03b.p1", 0x0000, 0x010000, CRC(92baccd8) SHA1(08e4544575a62991a6ae19ec4430a459074a1984) )
	ROM_LOAD( "alu03bd.p1", 0x0000, 0x010000, CRC(08736eff) SHA1(bb0c3c2a54b8828ed7ce5576abcec7800d033c9b) )
	ROM_LOAD( "alu03d.p1", 0x0000, 0x010000, CRC(478e09e6) SHA1(8a6221ceb841c41b839a8d478736144343d565a1) )
	ROM_LOAD( "alu03dk.p1", 0x0000, 0x010000, CRC(fe3efc18) SHA1(3f08c19581672748f9bc34a7d85ff946320f89ae) )
	ROM_LOAD( "alu03dr.p1", 0x0000, 0x010000, CRC(5486a30c) SHA1(8417afd7a9a07d4e9ac65880906320c49c0bf230) )
	ROM_LOAD( "alu03dy.p1", 0x0000, 0x010000, CRC(686e4818) SHA1(cf2af851ff7f4ce8edb82b78c3841b8b8c09bd17) )
	ROM_LOAD( "alu03k.p1", 0x0000, 0x010000, CRC(1e5fcd28) SHA1(0e520661a47d2e6c9f2f14a8c5cbf17bc15ffc9b) )
	ROM_LOAD( "alu03r.p1", 0x0000, 0x010000, CRC(123c111c) SHA1(641fb7a49e2160956178e3dc635ec33a377bfa7a) )
	ROM_LOAD( "alu03s.p1", 0x0000, 0x010000, CRC(87704898) SHA1(47fe7b835619e770a58c71796197a0d2810a8e9b) )
	ROM_LOAD( "alu03y.p1", 0x0000, 0x010000, CRC(254e43c4) SHA1(963b4e46d88b64f8ebc0c42dee2bbcb0ae1d3bec) )
	ROM_LOAD( "andylovesflostdarc.bin", 0x0000, 0x010000, CRC(9c98b37b) SHA1(43c5aa478ec2518b34de9258aab3e2507815a46a) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "al_05a__.2_1", 0x0000, 0x010000, CRC(d28849c8) SHA1(17e79f92cb3667de0be54fd4bae7f4c3a3a80aa5) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "alf.chr", 0x0000, 0x000048, CRC(22f09b0d) SHA1(5a612e54e0bb5ea5c35f1a7b1d7bc3cdc34e3bdd) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "andyflosound.bin", 0x0000, 0x080000, CRC(19934ff5) SHA1(c723859d05b210a3ff419f1346d673fb4c3ed835) ) // bad?
	ROM_LOAD( "alfsnd0.1", 0x0000, 0x080000, CRC(6691bc25) SHA1(4dd67b8bbdc5d707814b756005075fcb4f0c8be4) )
ROM_END

ROM_START( m4andybt )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "abt18d.p1", 0x0000, 0x020000, CRC(77874578) SHA1(455964614b67af14f5baa5883e1076e986de9e9c) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "abt18f.p1", 0x0000, 0x020000, CRC(cdd756af) SHA1(b1bb851ad2a2ba631e13509a476fe60cb8a24e69) )
	ROM_LOAD( "abt18s.p1", 0x0000, 0x020000, CRC(625263e4) SHA1(23fa0547164cc1f9b7c6cd26e06b0d779bf0329d) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "abt18s.chr", 0x0000, 0x000048, CRC(68007536) SHA1(72f7a76a1ba1c8ac94de425892780ffe78269513) )


	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "abtsnd.p1", 0x000000, 0x080000, CRC(0ba1e73a) SHA1(dde70b1bf973b023c45afb8d3191325514b96e47) )
	ROM_LOAD( "abtsnd.p2", 0x080000, 0x080000, CRC(dcfa85f2) SHA1(30e8467841309a4840824ec89f82044489c94ac5) )
ROM_END

ROM_START( m4andyfh )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "afhs.p1", 0x0000, 0x010000, CRC(722660ef) SHA1(e1700f4dc6d14da8e8d8402466057cfd126e067b) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "af3ad.p1", 0x0000, 0x010000, CRC(ef141eca) SHA1(1ba03db9c05f5d60c5e1e0729eb124f6c5c3acf5) )
	ROM_LOAD( "af3b.p1", 0x0000, 0x010000, CRC(78889d06) SHA1(5ea4c8010b7fd3e2e41d378b69a7cfda27aba99f) )
	ROM_LOAD( "af3bd.p1", 0x0000, 0x010000, CRC(d9087380) SHA1(1a7f203b722583927eb6f99a493e564100321fe6) )
	ROM_LOAD( "af3d.p1", 0x0000, 0x010000, CRC(ffe8a5f9) SHA1(b8632489ff015aa50e2c062f39096bb49e39ffe5) )
	ROM_LOAD( "af3dk.p1", 0x0000, 0x010000, CRC(4fc4a031) SHA1(c5c68027231988a88610931f395ae08d8e60f962) )
	ROM_LOAD( "af3dy.p1", 0x0000, 0x010000, CRC(f59c1a50) SHA1(55054a49b7bbf4a27ec808727cfbf3ce9bdfce40) )
	ROM_LOAD( "af3k.p1", 0x0000, 0x010000, CRC(ddf5edfb) SHA1(ce69c70b1cdcebfa29e1613cb619617a961a649b) )
	ROM_LOAD( "af3s.p1", 0x0000, 0x010000, CRC(e9860d9a) SHA1(f1d1323e2329613748602559b6458a19963c091a) )
	ROM_LOAD( "af3y.p1", 0x0000, 0x010000, CRC(1895bbe1) SHA1(c084f77004c9086bd75add665b25b0b3e114a91f) )
	ROM_LOAD( "af8b.p1", 0x0000, 0x010000, CRC(fa8d002e) SHA1(cad754268706a1c942ce3751aa5a51720a104899) )
	ROM_LOAD( "af8bd.p1", 0x0000, 0x010000, CRC(f64ce609) SHA1(d36a868647a954fd7974613510aabc6fc18035ee) )
	ROM_LOAD( "af8c.p1", 0x0000, 0x010000, CRC(6dff4569) SHA1(10809f81924a72b21129158043c023ad6809cced) )
	ROM_LOAD( "af8k.p1", 0x0000, 0x010000, CRC(0f6cb2a4) SHA1(320954e216e48ef2882f6d4feb7e29c106d49b79) )
	ROM_LOAD( "af8s.p1", 0x0000, 0x010000, CRC(1b06be8e) SHA1(c1b67b23c6e2abca68fb242e24b61333bde688fa) )
	ROM_LOAD( "afhb.p1", 0x0000, 0x010000, CRC(899945a4) SHA1(ed4a8c9b35e3aa08ea762740a713352560490443) )
	ROM_LOAD( "afhc.p1", 0x0000, 0x010000, CRC(eff4016e) SHA1(4497ae5033aa4c3b3af8e2f6821dadb3f0683c82) )
	ROM_LOAD( "afhd.p1", 0x0000, 0x010000, CRC(9f673d80) SHA1(ae2658d817d4d07f2d9f7948f0660f51626d07ac) )
	ROM_LOAD( "afhr.p1", 0x0000, 0x010000, CRC(232bc900) SHA1(831368184be51b13db30468d519e395a9af7570e) )
	ROM_LOAD( "aftad.p1", 0x0000, 0x010000, CRC(72b75e4a) SHA1(fb25a4a455589c51ec7bf1e77faee7f9809eea2c) )
	ROM_LOAD( "aftb.p1", 0x0000, 0x010000, CRC(be3cd9ec) SHA1(135f5b575ff1921d08985251b6cd326db4de4f3e) )
	ROM_LOAD( "aftbd.p1", 0x0000, 0x010000, CRC(d7fd4c6d) SHA1(3eea5f025f25194fd1d4b4cf0643445b11694c7b) )
	ROM_LOAD( "aftd.p1", 0x0000, 0x010000, CRC(4e5294c9) SHA1(94ef873a01e0635d4249f9c24b806ed3c08575fb) )
	ROM_LOAD( "aftdk.p1", 0x0000, 0x010000, CRC(51e6eb8a) SHA1(32291d8f8ddd4002a0c149b97ac120f128f8347b) )
	ROM_LOAD( "aftdr.p1", 0x0000, 0x010000, CRC(b8b922d2) SHA1(ec90c7c5fb72a912b0918dc87f8feea49077f7ef) )
	ROM_LOAD( "aftdy.p1", 0x0000, 0x010000, CRC(3e9fc7a6) SHA1(65d181398b3e574b26b060001e9477d5ee40bcc0) )
	ROM_LOAD( "aftk.p1", 0x0000, 0x010000, CRC(8d87a910) SHA1(a315210a5c9d880621937412ff6d8d42ac658db2) )
	ROM_LOAD( "aftr.p1", 0x0000, 0x010000, CRC(1be08c33) SHA1(21cd201cae159a1a3ee17f9661bc6db5e5a0ad48) )
	ROM_LOAD( "afts.p1", 0x0000, 0x010000, CRC(7f059eec) SHA1(06de497bbae7391bbb09241204dfdd59ecc36569) )
	ROM_LOAD( "afty.p1", 0x0000, 0x010000, CRC(d14f2670) SHA1(a6d21e855fbb90e80c8b8c4af02280343edcb3e8) )
	ROM_LOAD( "afuad.p1", 0x0000, 0x010000, CRC(0f14e261) SHA1(080a5667127e14b6959ff1508f028fd849c27c24) )
	ROM_LOAD( "afub.p1", 0x0000, 0x010000, CRC(99c6a4cc) SHA1(36fe83a32aeab413c19bc253edeadf4bc0f73615) )
	ROM_LOAD( "afubd.p1", 0x0000, 0x010000, CRC(c38d376a) SHA1(843d0dcc0909ea7cd93f6ba707e784b160cb4984) )
	ROM_LOAD( "afud.p1", 0x0000, 0x010000, CRC(640e0f24) SHA1(47879027c98557e7087b4fdf40161dbecc0f5c45) )
	ROM_LOAD( "afudk.p1", 0x0000, 0x010000, CRC(eeebf560) SHA1(518931322d1ba7d7fe51140a21b38dd3a90f308a) )
	ROM_LOAD( "afudr.p1", 0x0000, 0x010000, CRC(5f1e67ee) SHA1(80f13720256e00f924cdb2ad8f9101d131addcfb) )
	ROM_LOAD( "afudy.p1", 0x0000, 0x010000, CRC(c2754f00) SHA1(4012231cb4a2eb0e0010f90d173295aa3c1fd6a5) )
	ROM_LOAD( "afuk.p1", 0x0000, 0x010000, CRC(f58fcf3c) SHA1(3731eab62e447a833b7decde842eda6a36cfadef) )
	ROM_LOAD( "afur.p1", 0x0000, 0x010000, CRC(0369ab49) SHA1(53acb382fada789c976b1dd124014778cfe518bc) )
	ROM_LOAD( "afus.p1", 0x0000, 0x010000, CRC(efbde76c) SHA1(abad98f2affb46e449a50f5a43729160b275294b) )
	ROM_LOAD( "afuy.p1", 0x0000, 0x010000, CRC(9e0283a7) SHA1(63c0e3f26132a6bd6d8d3a8a3d0ab46e52fb2c09) )
	ROM_LOAD( "ca4ad.p1", 0x0000, 0x010000, CRC(bb311861) SHA1(9606b536c2775997935049caddb79170a98211b4) )
	ROM_LOAD( "ca4b.p1", 0x0000, 0x010000, CRC(71e8f0f9) SHA1(663f536f4b3de20c2dcae52d22f3be9be19b0a4d) )
	ROM_LOAD( "ca4bd.p1", 0x0000, 0x010000, CRC(808b93e6) SHA1(08667db3f43f8550d7b96b53b92a53029a8d5d29) )
	ROM_LOAD( "ca4d.p1", 0x0000, 0x010000, CRC(9e3f10f1) SHA1(2386b960c8de6ca97bc96b69c11f8eb01188e27a) )
	ROM_LOAD( "ca4dk.p1", 0x0000, 0x010000, CRC(23cabc1a) SHA1(a72e14d7616da0b7cc394e466fe6df4c85eea986) )
	ROM_LOAD( "ca4dy.p1", 0x0000, 0x010000, CRC(caa2e461) SHA1(5dd3c609d1cc4bc43a6d00e98a71e927287c41fd) )
	ROM_LOAD( "ca4k.p1", 0x0000, 0x010000, CRC(78b0a533) SHA1(fa0fc59562be59d0aadba923281086a9de8e8934) )
	ROM_LOAD( "ca4s.p1", 0x0000, 0x010000, CRC(ece1bca7) SHA1(84a168e0d36f7c4f56fc3a7579fe335cc1e5a5ba) )
	ROM_LOAD( "ca4y.p1", 0x0000, 0x010000, CRC(5c1886f2) SHA1(4d6131989a04db993b7ade74d1950077d52cbc23) )
	ROM_LOAD( "catad.p1", 0x0000, 0x010000, CRC(b2c5a227) SHA1(0c4253dddef07476778adf10b7afc8415ac2b170) )
	ROM_LOAD( "catb.p1", 0x0000, 0x010000, CRC(34007275) SHA1(102a30e4a83eec9ed144158cc4896c91f4eadd1b) )
	ROM_LOAD( "catbd.p1", 0x0000, 0x010000, CRC(9c38229a) SHA1(1c24eff59e22d354e07f9a0655b35029a89a60ef) )
	ROM_LOAD( "catd.p1", 0x0000, 0x010000, CRC(ce909947) SHA1(a62cf731cc1309ff9f41777463e9ccc90313c401) )
	ROM_LOAD( "catdk.p1", 0x0000, 0x010000, CRC(c85a53ea) SHA1(1b7245b08bc0ad7ddd7ed4498ba4ac910f1df1d1) )
	ROM_LOAD( "catdy.p1", 0x0000, 0x010000, CRC(08aa7a9c) SHA1(b8552ad4d9f1ea3c536ba7313ded56f9d47930d3) )
	ROM_LOAD( "catk.p1", 0x0000, 0x010000, CRC(843a09b2) SHA1(56e845bcf940d80277b53df8fe847e5e862a05c9) )
	ROM_LOAD( "cats.p1", 0x0000, 0x010000, CRC(e4cb7300) SHA1(fe9daaa587f1796227ad9ccb49869f2288b6d708) )
	ROM_LOAD( "caty.p1", 0x0000, 0x010000, CRC(1a5c2413) SHA1(096695d99cc5dcf9d677b7821af5018751e21a89) )
	ROM_LOAD( "cauad.p1", 0x0000, 0x010000, CRC(a530d8ce) SHA1(9c919dfe85d947545e35c52b070dcb19ad3660ea) )
	ROM_LOAD( "caub.p1", 0x0000, 0x010000, CRC(4c6f35b7) SHA1(3300d3a8cdeda7183d066ff8fec2bdfbcd816f9b) )
	ROM_LOAD( "caubd.p1", 0x0000, 0x010000, CRC(932cb584) SHA1(28dc617a242f01030d3bfdd855b2237b99dfb080) )
	ROM_LOAD( "caud.p1", 0x0000, 0x010000, CRC(3b74131e) SHA1(569478d5e6f9c5db38db347d5c60be6aab8b1689) )
	ROM_LOAD( "caudk.p1", 0x0000, 0x010000, CRC(9d2ca094) SHA1(343304a09ae0aa36039b4b90f90ac7206f6b020b) )
	ROM_LOAD( "caudy.p1", 0x0000, 0x010000, CRC(b8e09c8e) SHA1(b9fc39f754dadfaf359f7df6e51a18e948eda574) )
	ROM_LOAD( "cauk.p1", 0x0000, 0x010000, CRC(38c7b3b0) SHA1(d5ee172e37e65911a4010abe7baae3e32131208d) )
	ROM_LOAD( "caus.p1", 0x0000, 0x010000, CRC(88e263a4) SHA1(2b8bc3d9aab344ca756b4829c4593db74200779e) )
	ROM_LOAD( "cauy.p1", 0x0000, 0x010000, CRC(b04ab546) SHA1(5f9d3a24fb0091406e45cdad7f22fad4bda27bff) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "afhsnd1.bin", 0x000000, 0x080000, CRC(ce0b1890) SHA1(224d05f936a1b6f84ad682c282c557e87ad8931f) )
	ROM_LOAD( "afhsnd2.bin", 0x080000, 0x080000, CRC(8a4dda7b) SHA1(ee77295609ff646212faa207e56acb2440d859b8) )
ROM_END

ROM_START( m4andyge )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "an2s.p1", 0x0000, 0x010000, CRC(65399fa0) SHA1(ecefdf63e7aa477001fa530ed340e90e85252c3c) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "a28ad.p1", 0x0000, 0x010000, CRC(ecb0b180) SHA1(23d68e34e7a58fc6574e6c8524ce2e4e4cd25582) )
	ROM_LOAD( "a28b.p1", 0x0000, 0x010000, CRC(481c6c1c) SHA1(d8133d87e481f9c01c60324e918f706da6486c1b) )
	ROM_LOAD( "a28bd.p1", 0x0000, 0x010000, CRC(a59430b1) SHA1(000a00ba115408ab35fea74faa745220a9fcad68) )
	ROM_LOAD( "a28c.p1", 0x0000, 0x010000, CRC(e74533db) SHA1(f6f77dc61c08cdced0dca9133dfeeb5fdd4076f0) )
	ROM_LOAD( "a28d.p1", 0x0000, 0x010000, CRC(e8eee34e) SHA1(c223a8c1fd2c609376bab9e780020523c4e76b08) )
	ROM_LOAD( "a28dk.p1", 0x0000, 0x010000, CRC(115a2bc1) SHA1(31736f9583b4f110a6c838cecbd47acb7baa58c9) )
	ROM_LOAD( "a28dy.p1", 0x0000, 0x010000, CRC(05ef8b21) SHA1(762aaad6892511ba1f3266c1ed0a09850339cc63) )
	ROM_LOAD( "a28k.p1", 0x0000, 0x010000, CRC(c83b94fa) SHA1(8194b25bfcb8ba0323c63ee2f2b45f030aa1caeb) )
	ROM_LOAD( "a28s.p1", 0x0000, 0x010000, CRC(40529bad) SHA1(d22b0e8a8f4acec78dc05cde01d68b625008f3b0) )
	ROM_LOAD( "a28y.p1", 0x0000, 0x010000, CRC(fb1c83b7) SHA1(76b40e1ea47732ae0f6e9557c2d0445421122ac8) )
	ROM_LOAD( "a2tad.p1", 0x0000, 0x010000, CRC(0e3971d7) SHA1(f8de4a932937923d585f816fc9bffbe9887011c1) )
	ROM_LOAD( "a2tb.p1", 0x0000, 0x010000, CRC(d8c4bf4d) SHA1(06e082db39576f2da39866bdb8daab49e2b4108d) )
	ROM_LOAD( "a2tbd.p1", 0x0000, 0x010000, CRC(ed048ad0) SHA1(a2ffae901171363ccb827c7bf6299f29b0347e3c) )
	ROM_LOAD( "a2td.p1", 0x0000, 0x010000, CRC(ad17a652) SHA1(86006c706768a9227a21eb8da25817f4efacaa39) )
	ROM_LOAD( "a2tdk.p1", 0x0000, 0x010000, CRC(f11bd420) SHA1(0904ecf296474ee5283da26d8c728af438aac595) )
	ROM_LOAD( "a2tdy.p1", 0x0000, 0x010000, CRC(0ffcb8d7) SHA1(b1d591eed982d2bc2e02b96e2561bbb372242480) )
	ROM_LOAD( "a2tk.p1", 0x0000, 0x010000, CRC(8ca6ce3d) SHA1(6c869eceea88109b23a2b850deda6c5a46ca5a48) )
	ROM_LOAD( "a2ts.p1", 0x0000, 0x010000, CRC(d47c9c42) SHA1(5374cb5739a5c2ab2be32166c4819682f3266320) )
	ROM_LOAD( "a2ty.p1", 0x0000, 0x010000, CRC(30c22b5d) SHA1(be87fcbfb13c34c3d0ee1f586e887c80ffa01245) )
	ROM_LOAD( "a5tad.p1", 0x0000, 0x010000, CRC(df767538) SHA1(17ca5ea5b217fda448f61412cae82ae61447c5ad) )
	ROM_LOAD( "a5tb.p1", 0x0000, 0x010000, CRC(e6f22d3f) SHA1(f6da8edc0b058ce316ccca306f930469ef6d016c) )
	ROM_LOAD( "a5tbd.p1", 0x0000, 0x010000, CRC(24aa63c8) SHA1(838f1fff46c65dd56f25fd491f8aab3be826a845) )
	ROM_LOAD( "a5td.p1", 0x0000, 0x010000, CRC(b3ebc357) SHA1(6d0718474f83f71151189c3175b687564c1d49b0) )
	ROM_LOAD( "a5tdk.p1", 0x0000, 0x010000, CRC(67472634) SHA1(aae14b9ea4125b94dd1a7325c000629258573499) )
	ROM_LOAD( "a5tdy.p1", 0x0000, 0x010000, CRC(9f9c15c2) SHA1(0e6471c62450bd8468adde1a2d69c5b24c472bfc) )
	ROM_LOAD( "a5tk.p1", 0x0000, 0x010000, CRC(c63209f8) SHA1(71968dd94431610ddef35bb4cf8dcba749470a26) )
	ROM_LOAD( "a5ts.p1", 0x0000, 0x010000, CRC(9ab99a1e) SHA1(605c5ee71aa0583f02e9ced604692814e33b741a) )
	ROM_LOAD( "a5ty.p1", 0x0000, 0x010000, CRC(86ef0bd8) SHA1(870b8165e206f84e59a3badfba441a567626f297) )
	ROM_LOAD( "acappgreatescape5p4.bin", 0x0000, 0x010000, CRC(87733a0d) SHA1(6e2fc0f43eb48740b120af77302f1322a27e8a5a) )
	ROM_LOAD( "ag_05_d4.2_1", 0x0000, 0x010000, CRC(29953aa1) SHA1(c1346ab7e651c35d704e5127c4d44d2086fd48e3) )
	ROM_LOAD( "age5p8p.bin", 0x0000, 0x010000, CRC(c3b40981) SHA1(da56e468ae67f1a231fea721235036c75c5efac3) )
	ROM_LOAD( "age_20_.8", 0x0000, 0x010000, CRC(b1f91b2a) SHA1(9340f87d6d186b3af0384ab546c3d3f487e797d4) )
	ROM_LOAD( "ages58c", 0x0000, 0x010000, CRC(af479dc9) SHA1(7e0e3b36289d689bbd0c022730d7aee62192f49f) )
	ROM_LOAD( "agesc20p", 0x0000, 0x010000, CRC(94fec0f3) SHA1(7678e01a4e0fcc4136f6d4a668c4d1dd9a8f1246) )
	ROM_LOAD( "agesc5p", 0x0000, 0x010000, CRC(9de05e25) SHA1(b4d6aea5cffb14babd89cfa76575a68277bfaa4b) )
	ROM_LOAD( "an2c.p1", 0x0000, 0x010000, CRC(3e233c24) SHA1(4e8f0cb45851db509020afd47821893ab49448d7) )
	ROM_LOAD( "an2d.p1", 0x0000, 0x010000, CRC(5651ed3d) SHA1(6a1fbff252bf266b03c4cb64294053f686a523d6) )
	ROM_LOAD("an2k.p1",  0x00000, 0x10000,  CRC(c0886dff) SHA1(ef2b509fde05ef4ef055a09275afc9e153f50efc))
	ROM_LOAD( "an2y.p1", 0x0000, 0x010000, CRC(a9cd1ed2) SHA1(052fc711efe633a2ece6bf24fabdc0b69b9355fd) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ag_05__c.2_1", 0x0000, 0x010000, CRC(c38c11a3) SHA1(c2d81d99a842eac8dff3e0be57f37af9eb534ad1) )
	ROM_LOAD( "ag_05a__.3_1", 0x0000, 0x010000, CRC(89f4281e) SHA1(3ada70d7c5ef523f1a4eddfc8f1967e4a6de190d) )
	ROM_LOAD( "ag_05s__.3_1", 0x0000, 0x010000, CRC(c0e45872) SHA1(936ca3230cd36dd4ad2c74ea33ea469c482e5688) )
	ROM_LOAD( "ag_05sb_.3_1", 0x0000, 0x010000, CRC(f5055b62) SHA1(b12a7d2a1143ce47e6a327831d5df21483d78b03) )
	ROM_LOAD( "ag_05sd_.3_1", 0x0000, 0x010000, CRC(b7fced5c) SHA1(6b359b29019bf22b2ebdd96a69f919b18935a98c) )
	ROM_LOAD( "ag_10a__.2_1", 0x0000, 0x010000, CRC(ca80d891) SHA1(17bf51fecc3cecbb1e0ef0550296c8bf81d3d879) )
	ROM_LOAD( "ag_10s__.2_1", 0x0000, 0x010000, CRC(0dfeda46) SHA1(27e7548845f116537043e26002d8a5458275389d) )
	ROM_LOAD( "ag_10sb_.2_1", 0x0000, 0x010000, CRC(6f025416) SHA1(bb0167ba0a67dd1a03ec3e69e2050e2bf1d35244) )
	ROM_LOAD( "ag_10sd_.2_1", 0x0000, 0x010000, CRC(03ab435f) SHA1(3b04324c1ae839529d99255008874df3744769a4) )
	ROM_LOAD( "age05_101", 0x0000, 0x010000, CRC(70c1d1ab) SHA1(478891cadaeba76666af5c4f25531456ebbe789a) )
	ROM_LOAD( "age10_101", 0x0000, 0x010000, CRC(55e3a27e) SHA1(209166d052cc296f135225c77bb57abbef1a86ae) )
	ROM_LOAD( "age20_101", 0x0000, 0x010000, CRC(7e3674f0) SHA1(351e353da24b63d2ef7cb09690b770b26505569a) )
	ROM_LOAD( "age55", 0x0000, 0x010000, CRC(481e942d) SHA1(23ac3c4f624ae73940baf515002a178d39ba32b0) )
	ROM_LOAD( "age58c", 0x0000, 0x010000, CRC(0b1e4a0e) SHA1(e2bcd590a358e48b26b056f83c7180da0e036024) )
	ROM_LOAD( "agi05___.3_1", 0x0000, 0x010000, CRC(b061a468) SHA1(a1f1a8bd55eb7a684de270bace9464812172ed92) )
	ROM_LOAD( "agi10___.2_1", 0x0000, 0x010000, CRC(7c56a6ca) SHA1(adb567b8e1b6cc727bcfa694ade947f8c695f44a) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "char.chr", 0x0000, 0x000048, CRC(053a5846) SHA1(c92de79e568c9f253bb71dbda2ca32b7b3b6661a) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "an2snd.p1",  0x000000, 0x080000,  CRC(5394e9ae) SHA1(86ccd8531fc87f34d3c5482ba7e5a2c06ea69491) )
	ROM_LOAD( "an2snd.p2",  0x080000, 0x080000,  CRC(109ace1f) SHA1(9f0e8065186beb61ed50fea834de2d91e68db953) )
ROM_END

ROM_START( m4apach )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "a6ps.p1", 0x0000, 0x010000, CRC(41e375c7) SHA1(93556a90227cde6814123c8a7f29f734884e182c) )
ROM_END

ROM_START( m4atlan )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dat14.bin", 0x0000, 0x010000, CRC(d91fb9b2) SHA1(a06a868a17f84e2a012b0fe28025458e4f899c1d) )
ROM_END

ROM_START( m4bagtel )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bgt05s.p1", 0x0000, 0x010000, CRC(ddf1c7dc) SHA1(a786e5e04538ce498493795fc4054bb5de57ffd2) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "bg201c.p1", 0x0000, 0x010000, CRC(ee9bf501) SHA1(5c6ee55cfac5bb92695b412fe56f4c843dcae424) )
	ROM_LOAD( "bg201dy.p1", 0x0000, 0x010000, CRC(c4916bc0) SHA1(7600a5be6ff235d19f7c99b44b86054555b43638) )
	ROM_LOAD( "bg201s.p1", 0x0000, 0x010000, CRC(639b078b) SHA1(0c5d270457b2ae88c3885838f96ce29824996e77) )
	ROM_LOAD( "bgt05dk.p1", 0x0000, 0x010000, CRC(4acaf68d) SHA1(fb7e04c8201829c252add05599218fb2b32c8533) )
	ROM_LOAD( "bgt05k.p1", 0x0000, 0x010000, CRC(72eb14ad) SHA1(18f9dbc5fd85e14d507b4c69d03d01f24aabb325) )
	ROM_LOAD( "bgt05r.p1", 0x0000, 0x010000, CRC(e92ad743) SHA1(649496429572a339dea50e262b7eb2ef22273bea) )
	ROM_LOAD( "bgt05y.p1", 0x0000, 0x010000, CRC(f2508bfa) SHA1(936fb79d5d953d1e2138a55754cbd364d3c307ed) )
	ROM_LOAD( "el101ad.p1", 0x0000, 0x010000, CRC(fcb39192) SHA1(a604122e40c313ed240f722a48f56d1478754ed3) )
	ROM_LOAD( "el101b.p1", 0x0000, 0x010000, CRC(947548b4) SHA1(dc74fa15843ec4c34f5bd7269b041ed4406832c2) )
	ROM_LOAD( "el101bd.p1", 0x0000, 0x010000, CRC(338664f4) SHA1(074261acbf0611d7d54f2718eed04ef6eda81b50) )
	ROM_LOAD( "el101c.p1", 0x0000, 0x010000, CRC(053b52f2) SHA1(3abc3b63b0050ec7b4b04ad097643852d662d848) )
	ROM_LOAD( "el101d.p1", 0x0000, 0x010000, CRC(e4362ec7) SHA1(e2689ea6ec97329499625f0912016d7fac882fca) )
	ROM_LOAD( "el101dk.p1", 0x0000, 0x010000, CRC(fbdcb392) SHA1(f97474ab225bb9f694d601bc04eb5b0b54826a06) )
	ROM_LOAD( "el101dr.p1", 0x0000, 0x010000, CRC(1c89bc6a) SHA1(0c5d23fc0d928df5c73d0e24bfa10ec443bf306e) )
	ROM_LOAD( "el101dy.p1", 0x0000, 0x010000, CRC(81e29484) SHA1(3ac48cef176df5d4ab3b00dc9366f7c9192c8c77) )
	ROM_LOAD( "el101k.p1", 0x0000, 0x010000, CRC(0dd7427e) SHA1(83373dca6ec50a03506bda2c220949b2d2f0a7db) )
	ROM_LOAD( "el101r.p1", 0x0000, 0x010000, CRC(6299ff71) SHA1(137842a886fb4790571b94d94199b362cd86bc3c) )
	ROM_LOAD( "el101s.p1", 0x0000, 0x010000, CRC(2035faf2) SHA1(1b640fee2f0ace25dfaa702ab2602cdec5ab6018) )
	ROM_LOAD( "el101y.p1", 0x0000, 0x010000, CRC(fff2d79f) SHA1(5d1142d8d96803c8b4ddba43283e21bab3a0b598) )
	ROM_LOAD( "el201ad.p1", 0x0000, 0x010000, CRC(7ebf37ba) SHA1(d6d09d707458aa8a17507e3a1a396569b1eaef4d) )
	ROM_LOAD( "el201b.p1", 0x0000, 0x010000, CRC(ed9c0546) SHA1(8884420caa7bd9347d882f79f05288c2581026b1) )
	ROM_LOAD( "el201bd.p1", 0x0000, 0x010000, CRC(48a35af0) SHA1(17a8f4c178a744dd4b7ab16388a9622f335e3a79) )
	ROM_LOAD( "el201c.p1", 0x0000, 0x010000, CRC(b4821bd6) SHA1(bf806708fcfae5f23781efaa73cb3cf13c8009ed) )
	ROM_LOAD( "el201d.p1", 0x0000, 0x010000, CRC(8692aeaf) SHA1(6ffe1f7d088b8b3fc6fcb922f689a592a19f48e3) )
	ROM_LOAD( "el201dk.p1", 0x0000, 0x010000, CRC(ea866ad7) SHA1(4bec2f195681c6c4f6207aa4da66950019465344) )
	ROM_LOAD( "el201dr.p1", 0x0000, 0x010000, CRC(8bc21178) SHA1(413a700e61709d7a138552c5987a2b3ef353c429) )
	ROM_LOAD( "el201dy.p1", 0x0000, 0x010000, CRC(16a93996) SHA1(b151e4f16d0d78cd6651976d7108d3e8e8a17696) )
	ROM_LOAD( "el201k.p1", 0x0000, 0x010000, CRC(4fb93561) SHA1(ec4575ff6243a6402db7286826197262821d52e4) )
	ROM_LOAD( "el201r.p1", 0x0000, 0x010000, CRC(118e1494) SHA1(dc5d4a06d99c2855fd737178e0df19a5b6eb422b) )
	ROM_LOAD( "el201s.p1", 0x0000, 0x010000, CRC(87280546) SHA1(f7af53fc1c5e98897c36eaec013f13b1da283c53) )
	ROM_LOAD( "el201y.p1", 0x0000, 0x010000, CRC(8ce53c7a) SHA1(cf6f863be222eec894da34a414c0a6dd0c8601d7) )
	ROM_LOAD( "el310ad.p1", 0x0000, 0x010000, CRC(7029e664) SHA1(a4e0996710dc6c5cd2b6a79f83e08406a153a01d) )
	ROM_LOAD( "el310b.p1", 0x0000, 0x010000, CRC(aa10ed40) SHA1(0722be3c2c582b1179f3dafd4ed6c38f503ee17a) )
	ROM_LOAD( "el310bd.p1", 0x0000, 0x010000, CRC(b9204c03) SHA1(ecd3fcc301f5a7ce63b06dc4153b18602c405289) )
	ROM_LOAD( "el310c.p1", 0x0000, 0x010000, CRC(b2215a2a) SHA1(af388fcdb0f23b0d764ee023bb95a582e585ae8e) )
	ROM_LOAD( "el310d.p1", 0x0000, 0x010000, CRC(650dfa3f) SHA1(0b8aa1f51084351b4ed176a244cd746d63d312d3) )
	ROM_LOAD( "el310dk.p1", 0x0000, 0x010000, CRC(ea1a7da9) SHA1(19251b3b46eaf2db7077b9b901f306e2942c095b) )
	ROM_LOAD( "el310dr.p1", 0x0000, 0x010000, CRC(261f3cea) SHA1(c80cbd85aca73f09e6a52ee3385588fff11155e9) )
	ROM_LOAD( "el310dy.p1", 0x0000, 0x010000, CRC(41f1ac45) SHA1(b3fe09704de422ecc0f7632ec8b8bad646498cd3) )
	ROM_LOAD( "el310k.p1", 0x0000, 0x010000, CRC(8bb4d65c) SHA1(f05b448e7ba9808fb3a1c1f25f4e50fc27549031) )
	ROM_LOAD( "el310r.p1", 0x0000, 0x010000, CRC(0ea8a744) SHA1(2839dd86b54ed073765d97a82b056e20eb05f32f) )
	ROM_LOAD( "el310s.p1", 0x0000, 0x010000, CRC(5e1cace4) SHA1(b78d8021ef91127f8a60cdcb458723de8925fba5) )
	ROM_LOAD( "el310y.p1", 0x0000, 0x010000, CRC(9653f0c6) SHA1(188056b8b704f9b06f93144ce358ec47cc026902) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bgtsnd.p1", 0x000000, 0x080000, CRC(40a68dd1) SHA1(d70cf436dca242d49cd3bd39d3f6484a30968d0d) )
	ROM_LOAD( "bgtsnd.p2", 0x080000, 0x080000, CRC(90961429) SHA1(6390e575d030f6d2953ee8460876c50fe48026f8) )
ROM_END

ROM_START( m4bnknot )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bankanote.bin", 0x0000, 0x010000, CRC(73021b17) SHA1(71040af105ef832247b90ac9fac246068e75193b) )
ROM_END

ROM_START( m4bnkrol )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cbr05s.p1", 0x0000, 0x020000, CRC(a8b53a0d) SHA1(661ab61aa8f427b92fdee02539f19e5dd2243da7) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "br301d.p1", 0x0000, 0x020000, CRC(b9334e2d) SHA1(263808eb5ea3f9987eb7579b43329cb27e109921) )
	ROM_LOAD( "br301f.p1", 0x0000, 0x020000, CRC(c4be5b69) SHA1(9b08d5c0c5aebeef9f0767f5bd456cc6b05ea317) )
	ROM_LOAD( "br301s.p1", 0x0000, 0x020000, CRC(1e117651) SHA1(c06d3f14e55be83c89c8132cf219d46acc42991c) )
	ROM_LOAD( "cbr05d.p1", 0x0000, 0x020000, CRC(44cefec0) SHA1(7034c5acd44ccd3cd985ba4945c004c070a599a4) )
	ROM_LOAD( "cbr05f.p1", 0x0000, 0x020000, CRC(3943eb84) SHA1(76a00db6a0c6655c3a7942550c788822bacd73e5) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cbrsnd.p1", 0x000000, 0x080000, CRC(3524418a) SHA1(85cf286d9cf97cc9009c0283d632fef2a19f5de2) )
	ROM_LOAD( "cbrsnd.p2", 0x080000, 0x080000, CRC(a53796a3) SHA1(f094f40cc93ea445922a9c5412aa355b7d21b1f4) )
ROM_END

ROM_START( m4btclok )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "beattheclock.hex", 0x6000, 0x00a000, CRC(a0d4e463) SHA1(45d1df08bfd70caf63b14d2ccc56038ed85e23d0) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "btc.chr", 0x0000, 0x000048, CRC(c77e5215) SHA1(c9e26ed593840cbc47dad893ea4df476f1d69ecd) )
ROM_END

ROM_START( m4berser )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bess.p1", 0x0000, 0x010000, CRC(b95bafbe) SHA1(034c80ef5fd0a12fad918c9b01bafb9a99c2e991) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "be3ad.p1", 0x0000, 0x010000, CRC(db4d77e9) SHA1(80e9ecf0a5d213e23fe8d328fbe8af52d49e2897) )
	ROM_LOAD( "be3b.p1", 0x0000, 0x010000, CRC(b25e9adb) SHA1(cc72c7a02868d56371f6d1bbaf78a017147b1a5a) )
	ROM_LOAD( "be3bd.p1", 0x0000, 0x010000, CRC(ed511aa3) SHA1(e6efe14490fa62ec9e5565e92216e371ab98b78a) )
	ROM_LOAD( "be3d.p1", 0x0000, 0x010000, CRC(9bd3b8a5) SHA1(9432e985d6fc0158a325b75cfdec38ab10d0e991) )
	ROM_LOAD( "be3dk.p1", 0x0000, 0x010000, CRC(82b4513e) SHA1(a36eebef35dbc2fbc25d39fde811e180a3682b67) )
	ROM_LOAD( "be3dy.p1", 0x0000, 0x010000, CRC(6f2869f2) SHA1(b006f467463038b5987764f9eedec1d7357ade65) )
	ROM_LOAD( "be3k.p1", 0x0000, 0x010000, CRC(12b3954a) SHA1(cce68a720a0bb1ce64cbf6ab2902a712403516cd) )
	ROM_LOAD( "be3s.p1", 0x0000, 0x010000, CRC(1a66772e) SHA1(e604315cea3db5f3859f1756e84b37b805f1f995) )
	ROM_LOAD( "be3y.p1", 0x0000, 0x010000, CRC(5c6451ca) SHA1(b63b9289fdef3be6add1e1ee22f9e316f296cc97) )
	ROM_LOAD( "be8ad.p1", 0x0000, 0x010000, CRC(0c196fd2) SHA1(830917f4c9bb7df35ac7d1e4fdcbb3eaac65ec5f) )
	ROM_LOAD( "be8b.p1", 0x0000, 0x010000, CRC(8f25875d) SHA1(e53ca0322838891274e0c5c61882a6690df3f1a0) )
	ROM_LOAD( "be8bcd.p1", 0x0000, 0x010000, CRC(40d40ecd) SHA1(a728a2ea63dc7d520e2f189e18b4884fb5f292bc) )
	ROM_LOAD( "be8bd.p1", 0x0000, 0x010000, CRC(81af1323) SHA1(49c2c086079080f3ac5f44c9a3a051b7170ad777) )
	ROM_LOAD( "be8c.p1", 0x0000, 0x010000, CRC(8ff5ddc0) SHA1(2a9a1d6f74981c14a6b6c3077c8ba3b3437fbb42) )
	ROM_LOAD( "be8d.p1", 0x0000, 0x010000, CRC(480e0983) SHA1(bfce722e5371b30c119a11149f02d139b699903a) )
	ROM_LOAD( "be8dk.p1", 0x0000, 0x010000, CRC(39a3f145) SHA1(00242636e7519e37ed4f5e65ecf41c315c2607ad) )
	ROM_LOAD( "be8dy.p1", 0x0000, 0x010000, CRC(5f885b13) SHA1(c398bb40fbac39530ba7b96bf4a4fde575c9fa87) )
	ROM_LOAD( "be8k.p1", 0x0000, 0x010000, CRC(b66051b6) SHA1(910bd0a4112a81df0160efd86bed1a8a59b2acb8) )
	ROM_LOAD( "be8s.p1", 0x0000, 0x010000, CRC(12d0fb4f) SHA1(103a468a0712dfc44b140cad01cd49b6f159b621) )
	ROM_LOAD( "be8y.p1", 0x0000, 0x010000, CRC(80d73997) SHA1(99e76616e9a73f111bdd560f2691b99905e4e454) )
	ROM_LOAD( "besb.p1", 0x0000, 0x010000, CRC(a0aa05f9) SHA1(3831c07e9e33c83b2f7148a34037023433b49cd0) )
	ROM_LOAD( "besc.p1", 0x0000, 0x010000, CRC(3a7ea673) SHA1(469e104ca1008c274f2a58c3ec6e96b40e1b4fb6) )
	ROM_LOAD( "besd.p1", 0x0000, 0x010000, CRC(ac7daf9c) SHA1(9951cf3194bc5acc17044dfb4b854edb0cc2c090) )
	ROM_LOAD( "besdk.p1", 0x0000, 0x010000, CRC(f9d20012) SHA1(d3942406af0573a58e49a24f98b4e3c0a9ff508e) )
	ROM_LOAD( "besdy.p1", 0x0000, 0x010000, CRC(461ac51f) SHA1(217f169bd2bc4108145231e9b974d2f890a4f25e) )
	ROM_LOAD( "besk.p1", 0x0000, 0x010000, CRC(03eb2a05) SHA1(375f47bd1d0f21fde5ea0fcf7b79c02db9f8c9c6) )
	ROM_LOAD( "besy.p1", 0x0000, 0x010000, CRC(64a49f88) SHA1(6bd1275e9172e311ead36566432729530c1b6c21) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "be_05a_4.1_1", 0x0000, 0x010000, CRC(e4ec1624) SHA1(e6241edb729796dd248abca6bf67281379c39af2) )


	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "bessnd.p1", 0x0000, 0x080000, CRC(4eb15200) SHA1(1997a304df5219153418369bd8cc4fd169fb4bd4) )
ROM_END

ROM_START( m4bigbn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dbb12.bin", 0x0000, 0x010000, CRC(7acec20d) SHA1(5f3a21227329608c0afdb5facac977dee94ab9f5) )
ROM_END

ROM_START( m4bigchfd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bch15.bin", 0x0000, 0x010000, CRC(b745b19f) SHA1(b654af35200c69604a3c30e3df1252f8bedc2000) )
ROM_END

ROM_START( m4bigchf )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "b6cc.p1", 0x0000, 0x010000, CRC(8d3916b4) SHA1(1818137da9d53000053a8023c4994c6539459df0) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "b6cs.p1", 0x0000, 0x010000, CRC(4f45086b) SHA1(e1d639b068951df8f25b9c77d4fb86336ad19933) )
	ROM_LOAD( "bchf20-6", 0x0000, 0x010000, CRC(7940eb01) SHA1(b23537e91842a0d9b25b9c76b245d2be3d9af57f) )
	ROM_LOAD( "big chief 6.bin", 0x0000, 0x010000, CRC(edee08b7) SHA1(8de6160a4a4e5cd57f64c49d913f763aa87dc69a) )

	// 68k.. doesn't seem to be mpu4 video, so what is it?
	ROM_LOAD( "b6cpl.p0", 0x0000, 0x020000, CRC(7fbb2efb) SHA1(c21136bf10407f1685f3933d426ef53925aca8d8) )
	ROM_LOAD( "b6cpl.p1", 0x0000, 0x020000, CRC(a9f67f3e) SHA1(1309cc2dc8565ee79ac8cdc754187c8db6ddb3ea) )
ROM_END

ROM_START( m4blkwhd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dbw11.bin", 0x0000, 0x010000, CRC(337aaa2c) SHA1(26b12ea3ada9668293c6b44d62458590e5b4ac8f) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bwsnd.bin", 0x0000, 0x080000, CRC(f247ba83) SHA1(9b173503e63a4a861d1380b2ab1fe14af1a189bd) )
ROM_END

ROM_START( m4blkwht )
	ROM_REGION( 0x10000, "maincpu", 0 )
    ROM_LOAD( "b&wrom.bin",  0x00000, 0x10000,  CRC(da095666) SHA1(bc7654dc9da1f830a43f925db8079f27e18bb61e) ) // == oldtimer

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "b&wchrt.chr", 0x0000, 0x000048, CRC(10d302d4) SHA1(5858e550470a25dcd64efe004c79e6e9783bce07) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "b&wsound.bin", 0x0000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END

ROM_START( m4blkbuld )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dgu16", 0x0000, 0x010000, CRC(6aa23345) SHA1(45e129ec95b1a796f334bedd08469f2ab47a18f8) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "dbbsnd.p1", 0x000000, 0x080000, CRC(a913ad0d) SHA1(5f39b661912da903ce8d6658b7848081b191ea56) )
	ROM_LOAD( "dbbsnd.p2", 0x080000, 0x080000, CRC(6a22b39f) SHA1(0e0dbeac4310e03490b665fff514392481ad265f) )
ROM_END

ROM_START( m4blkbul )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cbb08.epr", 0x0000, 0x010000, CRC(09376df6) SHA1(ba3b101accb6bbfbf75b9d22621dbda4efcb7769) )
ROM_END

ROM_START( m4blkcat )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dbl14.bin", 0x0000, 0x010000, CRC(c5db9532) SHA1(309b5122b4a1cb33bbccfb97faf4fa996d29432e) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "dblcsnd.bin", 0x0000, 0x080000, CRC(c90fa8ad) SHA1(a98f03d4b6f5892333279bff7537d4d6d887da62) )

	ROM_REGION( 0x200000, "msm6376_alt", 0 ) // bad dump of some sound rom?
	ROM_LOAD( "sdbl_1.snd", 0x0000, 0x18008e, CRC(e36f71ae) SHA1(ebb643cfa02d28550f2bef135ceefc902baf0df6) )
ROM_END

ROM_START( m4bj )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dbj16p1.dat", 0x8000, 0x008000, CRC(025b2ad7) SHA1(1cd043ce3c550b0dcec8b3a7ed137c0032c4e4dc) )
	ROM_LOAD( "dbj16p2.dat", 0x4000, 0x004000, CRC(b961e9cb) SHA1(ffad698986b5ab9fe76f1addcb2bfa64f5b06fe4) )
ROM_END

ROM_START( m4bjc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dbc11.bin", 0x0000, 0x010000, CRC(ce28b677) SHA1(81006768e937b42f051e580f093b7182ad59236a) )
ROM_END

ROM_START( m4bja )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bla20s.p1", 0x0000, 0x010000, CRC(675e6b72) SHA1(c8e5e5cb108fdf521e1f4edc7fb02b0075b0ab99) )
ROM_END

ROM_START( m4bjac )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bjak1.8", 0x0000, 0x010000, CRC(e6a3c263) SHA1(fb28657cb43a0f24354382518d5d5be9cfdfa1d1) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "c2js.hex", 0x0000, 0x010000, CRC(3e6dd1f3) SHA1(dbd87368124244931b8868eb740b02a1775ed734) )
	ROM_LOAD( "c2js.p1", 0x0000, 0x010000, CRC(3e6dd1f3) SHA1(dbd87368124244931b8868eb740b02a1775ed734) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "bj.chr", 0x0000, 0x000048, CRC(b7851f82) SHA1(a6b2ae283b8e251169508ab6a2cba2fb70f706bf) )
ROM_END

ROM_START( m4bjack )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "b2j2-2.bin", 0x0000, 0x010000, CRC(b6a6df89) SHA1(193cb2eab57d783a28c7811081b076ec87274658) )
ROM_END


ROM_START( m4bjsm )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bjsmh.p1", 0x0000, 0x010000, CRC(9bb07e46) SHA1(989e343eab79c5382ca1d30d9923e7284b04a4b9) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "blackjacksupermulti.p1", 0x0000, 0x010000, CRC(6265cd77) SHA1(6dcc77f17079c36a7eb2b75f042ce4c204dba0ca) )
ROM_END

ROM_START( m4blstbk )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bab.p1", 0x8000, 0x008000, CRC(c38d33f4) SHA1(1337d802d609dbf972e3e99ae8344e97054726e5) )
	ROM_LOAD( "bab.p2", 0x6000, 0x002000, CRC(1a8a6fd7) SHA1(9e31f2c33aa1d9982f1493a842fa637a65922d23) )
ROM_END

ROM_START( m4bluedm )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dbd10.bin", 0x0000, 0x010000, CRC(b75e319d) SHA1(8b81e852e318cfde1f5ff2123e1ef7076b208253) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bdsnd.bin", 0x0000, 0x080000, CRC(8ac4aae6) SHA1(70dba43b398010a8bd0d82cf91553d3f5e0921f0) )
ROM_END

ROM_START( m4bluemn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "blm20p-6", 0x0000, 0x010000, CRC(4b9f83cf) SHA1(7014da9f7fc20443251dd5b2817f06a4ef862afd) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "blu23.bin", 0x0000, 0x010000, CRC(499d41c7) SHA1(1a741cf2c6ed6910717324ca2b0a2630338479e0) )
	ROM_LOAD( "bmoon.hex", 0x0000, 0x010000, CRC(109aa258) SHA1(88820b1090ce6b6538b4ca0428c02979535895c3) )
ROM_END

ROM_START( m4bdash )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "bls01s.p1", 0x0000, 0x020000, CRC(4e4f403b) SHA1(f040568af530cf0ff060199f98b00e476191da22) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "bdvarg.bin", 0x0000, 0x020000, CRC(99d579e7) SHA1(afc47144e0a8d464d8547b1ad14b0a3a1c15c027) )
	ROM_LOAD( "bld.hex", 0x0000, 0x020000, CRC(b8e435d5) SHA1(500c30d687d3e029f22de2bf132c12349c1575b4) )
	ROM_LOAD( "bld06s", 0x0000, 0x020000, CRC(0bc580b8) SHA1(432ac5aec08bd9d36cc4a0b257c17d6e22015bae) )
	ROM_LOAD( "bld07ad.p1", 0x0000, 0x020000, CRC(56438185) SHA1(f78789042a1ac61b7dd333120b9fef76a2805cc7) )
	ROM_LOAD( "bld07b.p1", 0x0000, 0x020000, CRC(4b24ec01) SHA1(80763e2832d9ef9c49f8729fbc93843865422d47) )
	ROM_LOAD( "bld07bd.p1", 0x0000, 0x020000, CRC(db592f40) SHA1(bca6b78ea13ccab1f49d6b6078071739cb418778) )
	ROM_LOAD( "bld07c.p1", 0x0000, 0x020000, CRC(7c6e5113) SHA1(814fb61aa64eecfa8b6d8e9c39a7b3b3287247dd) )
	ROM_LOAD( "bld07d.p1", 0x0000, 0x020000, CRC(363fe777) SHA1(181b10e828b1308725cbe185c655c7deb7899cbe) )
	ROM_LOAD( "bld07dh.p1", 0x0000, 0x020000, CRC(b299b28c) SHA1(3784cc7e33cd31c6ef5fd7fbc336b1b024a13993) )
	ROM_LOAD( "bld07dk.p1", 0x0000, 0x020000, CRC(919e8716) SHA1(4be8b30a3db436fab8dfc9a131f2ca2b16ba6f7d) )
	ROM_LOAD( "bld07dr.p1", 0x0000, 0x020000, CRC(22913c27) SHA1(33ed5a70b30d16fd607ec56a5ab085b55778c483) )
	ROM_LOAD( "bld07dy.p1", 0x0000, 0x020000, CRC(167da7e8) SHA1(1cb81ad595ad5c7b70aed4f48ce9f6ae34d92089) )
	ROM_LOAD( "bld07h.p1", 0x0000, 0x020000, CRC(22e471cd) SHA1(3e6e0a052761e1ed108475687dead185eef10119) )
	ROM_LOAD( "bld07k.p1", 0x0000, 0x020000, CRC(01e34457) SHA1(5e9d8cb558222340df42904365ad90288ca5cdf2) )
	ROM_LOAD( "bld07r.p1", 0x0000, 0x020000, CRC(b2ecff66) SHA1(9d8bca3e137a654d786b9257ce1206c7118ac6e0) )
	ROM_LOAD( "bld07s.p1", 0x0000, 0x020000, CRC(b9c61540) SHA1(d6752d90a431cde17c7915746f645ef3157eeffe) )
	ROM_LOAD( "bld07y.p1", 0x0000, 0x020000, CRC(860064a9) SHA1(8e13df769bde73bc5af3fa8010b39502e269f63f) )
	ROM_LOAD( "bld10ad.p1", 0x0000, 0x020000, CRC(04b2781e) SHA1(828426d6191974050e3ccbfbc826d5474dc18312) )
	ROM_LOAD( "bld10b.p1", 0x0000, 0x020000, CRC(160a6c83) SHA1(e2421fbc166b9e64a2b10afbfd12ebc724077248) )
	ROM_LOAD( "bld10bd.p1", 0x0000, 0x020000, CRC(89a8d6db) SHA1(c93b8d7c57a970649204078f3428fc766ada32f7) )
	ROM_LOAD( "bld10c.p1", 0x0000, 0x020000, CRC(2140d191) SHA1(151aba51cf0909f8bc3d252ef49a2ae2e96adf32) )
	ROM_LOAD( "bld10d.p1", 0x0000, 0x020000, CRC(6b1167f5) SHA1(2aeb0fa0964867d90412bfd895da664d9be8a339) )
	ROM_LOAD( "bld10dh.p1", 0x0000, 0x020000, CRC(e0684b17) SHA1(eb2832a3344aa9dfdc10c845faf3ae67171c40e9) )
	ROM_LOAD( "bld10dk.p1", 0x0000, 0x020000, CRC(c36f7e8d) SHA1(1ee7bcca0cfdd27cd23328f60aa5230325db6366) )
	ROM_LOAD( "bld10dr.p1", 0x0000, 0x020000, CRC(7060c5bc) SHA1(e409684b5f2494c1e580e4c9db001e89ef63ea1a) )
	ROM_LOAD( "bld10dy.p1", 0x0000, 0x020000, CRC(448c5e73) SHA1(38193dbe23266d29344439d75e020b8236d34037) )
	ROM_LOAD( "bld10h.p1", 0x0000, 0x020000, CRC(7fcaf14f) SHA1(e093bbaea83f7b2683b968b70d821ec42addab92) )
	ROM_LOAD( "bld10k.p1", 0x0000, 0x020000, CRC(5ccdc4d5) SHA1(b7be6f027092106ef5e33ff988a153050f48943b) )
	ROM_LOAD( "bld10r.p1", 0x0000, 0x020000, CRC(efc27fe4) SHA1(1ed3c5c92505b7fdf4993a9c8b119eff5e9a6f94) )
	ROM_LOAD( "bld10s.p1", 0x0000, 0x020000, CRC(c59c186b) SHA1(83f16e15a215fe1cf3c07fac7268b00c55e0ff5b) )
	ROM_LOAD( "bld10y.p1", 0x0000, 0x020000, CRC(db2ee42b) SHA1(b6a4bb4f78c14428a7bd2286b8fda51acb0c9e10) )
	ROM_LOAD( "bls01ad.p1", 0x0000, 0x020000, CRC(2425cab4) SHA1(9df08f9dffc0ac5fe5994ad086e6b8eb8d03baa9) )
	ROM_LOAD( "bls01b.p1", 0x0000, 0x020000, CRC(64d1e31d) SHA1(e30d199bd1d60ceabef27cfa81605eb3b307f68e) )
	ROM_LOAD( "bls01bd.p1", 0x0000, 0x020000, CRC(a93f6471) SHA1(d895c7825c713626be57dd9eef2dbfc5f591825b) )
	ROM_LOAD( "bls01c.p1", 0x0000, 0x020000, CRC(539b5e0f) SHA1(46abc7e8dd286fbb313233b6d2fabb73b1c4c519) )
	ROM_LOAD( "bls01d.p1", 0x0000, 0x020000, CRC(19cae86b) SHA1(de811aed861bf4d834051d6048fb0028f6c68a7c) )
	ROM_LOAD( "bls01dh.p1", 0x0000, 0x020000, CRC(c0fff9bd) SHA1(5b10547157a96f6221a41118ee29639ffc1bcdab) )
	ROM_LOAD( "bls01dk.p1", 0x0000, 0x020000, CRC(e3f8cc27) SHA1(4ffb6ebb792b7edbf88463ed0fb80743aab5c517) )
	ROM_LOAD( "bls01dr.p1", 0x0000, 0x020000, CRC(50f77716) SHA1(a20b709f65e2722e4fb622e851b4fb9c7d5820ca) )
	ROM_LOAD( "bls01dy.p1", 0x0000, 0x020000, CRC(641becd9) SHA1(419486ed63bd8d657796f786e73d44bd213b2916) )
	ROM_LOAD( "bls01h.p1", 0x0000, 0x020000, CRC(0d117ed1) SHA1(8b0a4f24b067907377072e5b5645ac8b44bf1d2e) )
	ROM_LOAD( "bls01k.p1", 0x0000, 0x020000, CRC(2e164b4b) SHA1(37d1c45db0002c7e8f16ede87cfe62cfbdbf39e8) )
	ROM_LOAD( "bls01r.p1", 0x0000, 0x020000, CRC(9d19f07a) SHA1(bb1e2d8f6e1fd75d6c9a15448fc29b21d8f14bf7) )
	ROM_LOAD( "bls01y.p1", 0x0000, 0x020000, CRC(a9f56bb5) SHA1(771ea854bdc71af0ce09952be53671629babfa9b) )
	ROM_LOAD( "bls02ad.p1", 0x0000, 0x020000, CRC(f4b6828b) SHA1(8ca39a9dc29b40a097489e34ababaf70eb58c326) )
	ROM_LOAD( "bls02b.p1", 0x0000, 0x020000, CRC(d75f9bdb) SHA1(2018e1ebe4f00782be649544bc8d56d923d6c198) )
	ROM_LOAD( "bls02bd.p1", 0x0000, 0x020000, CRC(79ac2c4e) SHA1(83d6828272438ae0d687b40368701cbccac32d9f) )
	ROM_LOAD( "bls02c.p1", 0x0000, 0x020000, CRC(e01526c9) SHA1(1bc17c22e0741f6dbf1c3ad2e154ce8acc1e1788) )
	ROM_LOAD( "bls02d.p1", 0x0000, 0x020000, CRC(aa4490ad) SHA1(963482e4977309babf35732be0b7046c543808d3) )
	ROM_LOAD( "bls02dh.p1", 0x0000, 0x020000, CRC(106cb182) SHA1(1556392744646d8852cc82975dd94df250d54bfa) )
	ROM_LOAD( "bls02dk.p1", 0x0000, 0x020000, CRC(336b8418) SHA1(c42a29c7599c2d8025b8e25fcda0034100a43835) )
	ROM_LOAD( "bls02dr.p1", 0x0000, 0x020000, CRC(80643f29) SHA1(e9128f78e7051274dbaab28051a1f23ed2426c3c) )
	ROM_LOAD( "bls02dy.p1", 0x0000, 0x020000, CRC(b488a4e6) SHA1(7a9083cfa9032cb0e6e28c22dde8da9a1bebadcf) )
	ROM_LOAD( "bls02h.p1", 0x0000, 0x020000, CRC(be9f0617) SHA1(fcc491ae5cb5312f47726c4b9ffda99317171bab) )
	ROM_LOAD( "bls02k.p1", 0x0000, 0x020000, CRC(9d98338d) SHA1(0e43896ae8361894c9060ec7a74dd23c6e2bed56) )
	ROM_LOAD( "bls02r.p1", 0x0000, 0x020000, CRC(2e9788bc) SHA1(586a30b3485e0ceb8b9b389e103fdbab78115446) )
	ROM_LOAD( "bls02s.p1", 0x0000, 0x020000, CRC(b8e435d5) SHA1(500c30d687d3e029f22de2bf132c12349c1575b4) )
	ROM_LOAD( "bls02y.p1", 0x0000, 0x020000, CRC(1a7b1373) SHA1(dde4754d92f0fde495ab826294a650ac81fd586e) )
	ROM_LOAD( "bold15g", 0x0000, 0x020000, CRC(fa400d34) SHA1(2faeb9b880fb4980aa0d96b4b962c879498445f2) )
	ROM_LOAD( "bold15t", 0x0000, 0x020000, CRC(f3f331ae) SHA1(d999c8571549d8d26b7b861299d77c7282aef700) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "bo__x__x.2_0", 0x0000, 0x020000, CRC(7e54982f) SHA1(c5187d2f6a5b202af5fd6326d52451d3b3f48f33) )
	ROM_LOAD( "bo__x__x.2_1", 0x0000, 0x020000, CRC(3e48d8ad) SHA1(73d69712993819d012c2ab2a8a36b7ebad419144) )
	ROM_LOAD( "bo__x_dx.2_0", 0x0000, 0x020000, CRC(d0d9e7b1) SHA1(31e858991fc1dfe9c1a8bd7955096617ebe0a4ce) )
	ROM_LOAD( "bo__x_dx.2_1", 0x0000, 0x020000, CRC(b6e146c4) SHA1(8bda363f16bd258d5c6ba1b20cecc0a76e0965f7) )
	ROM_LOAD( "bo__xa_x.2_0", 0x0000, 0x020000, CRC(e7054491) SHA1(7d102b1071d90ff29ea4a9418478b17b93c08059) )
	ROM_LOAD( "bo__xa_x.2_1", 0x0000, 0x020000, CRC(813de5e4) SHA1(498923261e49b20666a930593fcf25ccfc9a9d79) )
	ROM_LOAD( "bo__xb_x.2_0", 0x0000, 0x020000, CRC(adc2ecc7) SHA1(75e4216ff022c1ae0642913c9aaa7e241b806fcd) )
	ROM_LOAD( "bo__xb_x.2_1", 0x0000, 0x020000, CRC(cbfa4db2) SHA1(d1ed60f876b4f056f478cfc23b08a7789379e143) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "blds1.hex", 0x000000, 0x080000, CRC(9cc07f5f) SHA1(e25295eb304624ed77d98d7e974363214c2c2cd1) )
	ROM_LOAD( "blds2.hex", 0x080000, 0x080000, CRC(949bee73) SHA1(9ea2001a4d91236708dc948b4e1cac9978095945) )

ROM_END

ROM_START( m4brktak )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "b-t v1-0 p1", 0xc000, 0x004000, CRC(a3457409) SHA1(ceaca37f20a055b18a24ee99e43991df95e9b520) )
	ROM_LOAD( "b-t v1-0 p2", 0x8000, 0x004000, CRC(7465cc6f) SHA1(f984e41c310bc58d7a668ec9f31c238fbf5de9c6) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "breakntake p1.bin", 0xc000, 0x004000, CRC(a3457409) SHA1(ceaca37f20a055b18a24ee99e43991df95e9b520) )
	ROM_LOAD( "breakntake p2.bin", 0x8000, 0x004000, CRC(7465cc6f) SHA1(f984e41c310bc58d7a668ec9f31c238fbf5de9c6) )
ROM_END

ROM_START( m4brdway )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dbr11.bin", 0x0000, 0x010000, CRC(5cbb8a0f) SHA1(bee8b2b7d70c24f98b7626caa278cb84136941a4) )
ROM_END

ROM_START( m4brook )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "brkl10.epr", 0x0000, 0x010000, CRC(857255b3) SHA1(cfd77918a19b2532a02b8bb3fa8e2716db31fb0e) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "brkl_snd.epr", 0x0000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END


ROM_START( m4buc )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "buccaneer5-15sw.bin", 0x000000, 0x020000, CRC(9b92d1f6) SHA1(d374fe966a1b039c971f278ab1113640e7629233) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "bucc15g", 0x000000, 0x020000, CRC(63dd1180) SHA1(a557af6927744b4ce2773c70db5ce1a7708ceb2c) )
	ROM_LOAD( "bucc15t", 0x000000, 0x020000, CRC(66104749) SHA1(4b5a9a3f1409e207cad42ea29a205a18facf57ab) )
	ROM_LOAD( "bug04ad.p1", 0x000000, 0x020000, CRC(c6171b29) SHA1(a66aa4b05f974aa9cea9e05e95d14a0e746374be) )
	ROM_LOAD( "bug04b.p1", 0x000000, 0x020000, CRC(4358fe51) SHA1(6e61397f71018d3f9d369a0ac8fefacafbada2d5) )
	ROM_LOAD( "bug04bd.p1", 0x000000, 0x020000, CRC(282b7832) SHA1(1ae3e45606bb875dd178beab231bdaa472687d46) )
	ROM_LOAD( "bug04d.p1", 0x000000, 0x020000, CRC(68e74bf0) SHA1(7808ff977e61b4c21fec83a8fe1cbabfc1bed7c8) )
	ROM_LOAD( "bug04dh.p1", 0x000000, 0x020000, CRC(689e47a7) SHA1(c3b7cd2cb6a397528b2a525b85df2bbff67450c1) )
	ROM_LOAD( "bug04dk.p1", 0x000000, 0x020000, CRC(01ca1dba) SHA1(83386f8a9bdf4e50b31c25cd4502a09a94ee3a1d) )
	ROM_LOAD( "bug04dr.p1", 0x000000, 0x020000, CRC(b2c5a68b) SHA1(42a0cee8cc2ba5b36adde1ab024799792982e5db) )
	ROM_LOAD( "bug04dy.p1", 0x000000, 0x020000, CRC(4b0db5ec) SHA1(f13ce614105f317bbc9318b7f512f8550b737e1d) )
	ROM_LOAD( "bug04h.p1", 0x000000, 0x020000, CRC(03edc1c4) SHA1(52a3040ec602008dc9143900d149251235282dca) )
	ROM_LOAD( "bug04k.p1", 0x000000, 0x020000, CRC(6ab99bd9) SHA1(d199e88dd22f6c2d31c23413c4c3f262834f5751) )
	ROM_LOAD( "bug04r.p1", 0x000000, 0x020000, CRC(d9b620e8) SHA1(01e7232f62dc33d3a9a26ee5456c2bb47dd4fce4) )
	ROM_LOAD( "bug04s.p1", 0x000000, 0x020000, CRC(0f76cf1d) SHA1(e0081f88e23958564a87346082629c4fdc0cc147) )
	ROM_LOAD( "bug04y.p1", 0x000000, 0x020000, CRC(207e338f) SHA1(3a95d0029e3e3a8f839f335ed1a981e8d6124dcc) )
	ROM_LOAD( "bug05ad.p1", 0x000000, 0x020000, CRC(515539dd) SHA1(6f8eb199f4738edb6f405f3d5df1ba0256dfa0bf) )
	ROM_LOAD( "bug05b.p1", 0x000000, 0x020000, CRC(a4e57b87) SHA1(64a76762b028349da9fb14141d27423785bdb9c8) )
	ROM_LOAD( "bug05bd.p1", 0x000000, 0x020000, CRC(bf695ac6) SHA1(24f5c4f46ed5d269426357ef578774ac456be1d0) )
	ROM_LOAD( "bug05d.p1", 0x000000, 0x020000, CRC(8f5ace26) SHA1(32e3d2b8cca2176ac141b4f746f400e0d4d6f534) )
	ROM_LOAD( "bug05dh.p1", 0x000000, 0x020000, CRC(ffdc6553) SHA1(cb31535424d67fa326cbe87912023fa528d2f0c0) )
	ROM_LOAD( "bug05dk.p1", 0x000000, 0x020000, CRC(96883f4e) SHA1(6cdc9b47bb170118e09648f5481bd56459df6acf) )
	ROM_LOAD( "bug05dr.p1", 0x000000, 0x020000, CRC(2587847f) SHA1(d564e4fedb7b6304f34fe4d7b6428922ecf509c1) )
	ROM_LOAD( "bug05dy.p1", 0x000000, 0x020000, CRC(dc4f9718) SHA1(035530dced28772b29974a4adcb648fdeff44cb4) )
	ROM_LOAD( "bug05h.p1", 0x000000, 0x020000, CRC(e4504412) SHA1(84596d14c1474f7965956ed707261dbe272d9c14) )
	ROM_LOAD( "bug05k.p1", 0x000000, 0x020000, CRC(8d041e0f) SHA1(94c4fa84f6c978c725593c6086c61521cd791c74) )
	ROM_LOAD( "bug05r.p1", 0x000000, 0x020000, CRC(3e0ba53e) SHA1(041dbf7f086dce0182f855249d832b68942e2c33) )
	ROM_LOAD( "bug05s.p1", 0x000000, 0x020000, CRC(99ce7ada) SHA1(6cdb17d8dfd759ceb2d7acd5f6b15952106f3178) )
	ROM_LOAD( "bug05y.p1", 0x000000, 0x020000, CRC(c7c3b659) SHA1(66aa9481b69ee282ecfa8f7614b7d476919e35b3) )
	ROM_LOAD( "bus01ad.p1", 0x000000, 0x020000, CRC(0f5920ee) SHA1(5152c24fd3e3642a5324e68465403a6ce199db5a) )
	ROM_LOAD( "bus01b.p1", 0x000000, 0x020000, CRC(656d2609) SHA1(525fc8e2dc1d6bfe17c24e28ccde0f0a580e4330) )
	ROM_LOAD( "bus01bd.p1", 0x000000, 0x020000, CRC(e16543f5) SHA1(98dc6a098ad13c1f7c3e0e1079eed92931ce279c) )
	ROM_LOAD( "bus01d.p1", 0x000000, 0x020000, CRC(4ed293a8) SHA1(577ca646df4d607c04a3c55853df847e1403bbfd) )
	ROM_LOAD( "bus01dh.p1", 0x000000, 0x020000, CRC(a1d07c60) SHA1(830998adf3a4fc8d6ae9e08039719cbacff79fac) )
	ROM_LOAD( "bus01dk.p1", 0x000000, 0x020000, CRC(c884267d) SHA1(60b1bba2fb0c471c4808496d08f094f3989d1000) )
	ROM_LOAD( "bus01dr.p1", 0x000000, 0x020000, CRC(7b8b9d4c) SHA1(67b6f54153cf312c0f4bcc229763e5abcc61e4f4) )
	ROM_LOAD( "bus01dy.p1", 0x000000, 0x020000, CRC(82438e2b) SHA1(1daa8094c8c33d88be1a2f9e833559d4386af0bc) )
	ROM_LOAD( "bus01h.p1", 0x000000, 0x020000, CRC(25d8199c) SHA1(84be66148fe14d85bd69fb4c9b5263b7c208e690) )
	ROM_LOAD( "bus01k.p1", 0x000000, 0x020000, CRC(4c8c4381) SHA1(0e6204e6f937ca8b9dc31927d31ec11db18068c0) )
	ROM_LOAD( "bus01r.p1", 0x000000, 0x020000, CRC(ff83f8b0) SHA1(bbd56730eccb4df1815b98921522beec5c74a9bd) )
	ROM_LOAD( "bus01s.p1", 0x000000, 0x020000, CRC(d5a35734) SHA1(7b905ac16eb50d462e9edc5bb50fe660b6f7c81b) )
	ROM_LOAD( "bus01y.p1", 0x000000, 0x020000, CRC(064bebd7) SHA1(8b19edae49c919ddf20d2ebff43ffec79809d90c) )
	ROM_LOAD( "bus02ad.p1", 0x000000, 0x020000, CRC(a1ad9f3d) SHA1(bfb61b1f2a449293d23d6c385a0aab67ccdcc8fe) )
	ROM_LOAD( "bus02b.p1", 0x000000, 0x020000, CRC(f6d1181b) SHA1(f4606b4f9522293ad73936f9dd80e54b9ee58f33) )
	ROM_LOAD( "bus02bd.p1", 0x000000, 0x020000, CRC(4f91fc26) SHA1(66e2aec28ec474a8d9c11dd7375de0c2050db963) )
	ROM_LOAD( "bus02d.p1", 0x000000, 0x020000, CRC(dd6eadba) SHA1(9240f96cc1366652855ac321f622004923ede8da) )
	ROM_LOAD( "bus02dh.p1", 0x000000, 0x020000, CRC(0f24c3b3) SHA1(66d5a28b1497b4f31518f427408c2e6f9f70d034) )
	ROM_LOAD( "bus02dk.p1", 0x000000, 0x020000, CRC(667099ae) SHA1(e0362b80bbc5525f94e1109780a085b87e17cb80) )
	ROM_LOAD( "bus02dr.p1", 0x000000, 0x020000, CRC(d57f229f) SHA1(3b2cfdeeab5c910d405bad44de85324088919415) )
	ROM_LOAD( "bus02dy.p1", 0x000000, 0x020000, CRC(2cb731f8) SHA1(9926b782298099a60680bfa46ab3514ea6653bf3) )
	ROM_LOAD( "bus02h.p1", 0x000000, 0x020000, CRC(b664278e) SHA1(ab009d09f1e3a8aa3c425db689553c3ac63f17ce) )
	ROM_LOAD( "bus02k.p1", 0x000000, 0x020000, CRC(df307d93) SHA1(1dda940868273f81c501dcee27c9d6bc91f411e1) )
	ROM_LOAD( "bus02r.p1", 0x000000, 0x020000, CRC(6c3fc6a2) SHA1(f65cab3fcc5a7176dded8ebf3de8ff90479686c6) )
	ROM_LOAD( "bus02s.p1", 0x000000, 0x020000, CRC(c43f9f09) SHA1(83501473bf8fc17748fa42ab446d4bc54eeb2a80) )
	ROM_LOAD( "bus02y.p1", 0x000000, 0x020000, CRC(95f7d5c5) SHA1(301949ad27963041a3cef000ed9ffd16c119b18d) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "br_sj___.1_1", 0x0000, 0x020000, CRC(02c30d48) SHA1(8e5d09d721bf6e1876d672b6c84f46666cf42b90) )
	ROM_LOAD( "br_sj_b_.1_1", 0x0000, 0x020000, CRC(490ec8a7) SHA1(faf9f450d48382aeb7b8e01750fc226c30e761d3) )
	ROM_LOAD( "br_sj_d_.1_1", 0x0000, 0x020000, CRC(ac4e72d6) SHA1(303f77e536b8da79a926dc5b30441ae9071f683b) )
	ROM_LOAD( "br_sj_k_.1_1", 0x0000, 0x020000, CRC(1c71f108) SHA1(10f4e99b0af4a102ed23098123d82da2a8f1c5be) )
	ROM_LOAD( "br_sjb__.1_1", 0x0000, 0x020000, CRC(d15579a0) SHA1(577c7cd11da15083327dba385a6769b346be2b71) )
	ROM_LOAD( "br_sjbg_.1_1", 0x0000, 0x020000, CRC(5f8ec0ae) SHA1(8eacfd43e3f875af862b77f044b7a9f1487af4a1) )
	ROM_LOAD( "br_sjbt_.1_1", 0x0000, 0x020000, CRC(00f9581e) SHA1(1461539f501250a08bf66e4a94e4b84113dc0dc5) )
	ROM_LOAD( "br_sjwb_.1_1", 0x0000, 0x020000, CRC(d15cb680) SHA1(4ab485eb2d1d57c690926e430e0c8b2af045381d) )


	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "buccsnd1.bin", 0x000000, 0x080000, CRC(b671fd7b) SHA1(8123d1ef9d5e2cc8783a78137540e6f13e5e2304) )
	ROM_LOAD( "buccsnd2.bin", 0x080000, 0x080000, CRC(66966b41) SHA1(87e2058f39ef1b19c35e63d55e62e2034fd24c0d) )
ROM_END

ROM_START( m4bucks )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bufd.p1", 0x0000, 0x010000, CRC(02c575d3) SHA1(92dc7a0c298e4d2d19bf754a5c82cc15e4e6456c) )
	ROM_LOAD( "bufs.p1", 0x0000, 0x010000, CRC(e394ae40) SHA1(911077053c47cebba1bed9d359cd38bd676a46f1) )
ROM_END

ROM_START( m4calamab )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "bc302d.p1", 0x0000, 0x020000, CRC(36b87f8e) SHA1(6e3cbfa52d9ec52fe009d3331dda3781f7f7783a) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "bc302f.p1", 0x0000, 0x020000, CRC(4b356aca) SHA1(81ce1585f529f1717ec56ace0a4902ae901593ae) )
	ROM_LOAD( "bc302s.p1", 0x0000, 0x020000, CRC(b349bd2d) SHA1(9b026bece40584c4f53c30f3dacc91942c871a9f) )
	ROM_LOAD( "calamari.cl", 0x0000, 0x020000, CRC(bb5e81ac) SHA1(b27f71321978712d2950d58715d18fd5523d6b06) )
ROM_END

ROM_START( m4calama )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cala.hex", 0x0000, 0x020000, CRC(edc97795) SHA1(58fb91809c7f475fbceacfc1c3bda41b86dff54b) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ca301d.p1", 0x0000, 0x020000, CRC(9a220126) SHA1(d5b12955bb336f8233ed3f892e23a14ba755a511) )
	ROM_LOAD( "ca301f.p1", 0x0000, 0x020000, CRC(e7af1462) SHA1(72659ef85c3b7916e10b4dbc09ad62638e7ab7e1) )
	ROM_LOAD( "ca301s.p1", 0x0000, 0x020000, CRC(95beecf1) SHA1(70f72abc0d4280618033b61f9dbe5b90b455c2b1) )
	ROM_LOAD( "cac03d.p1", 0x0000, 0x020000, CRC(14436ec7) SHA1(eb654ef5cef94e24296512acb6134440a5f8d17e) )
	ROM_LOAD( "cac03f.p1", 0x0000, 0x020000, CRC(69ce7b83) SHA1(c1f2dea6fe7983f5cefbf58ad63bce5ae8d7f7a5) )
	ROM_LOAD( "cac03s.p1", 0x0000, 0x020000, CRC(edc97795) SHA1(58fb91809c7f475fbceacfc1c3bda41b86dff54b) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "bca04.p1", 0x0000, 0x020000, CRC(3f97fe65) SHA1(6bc2c7e60658f39701974426ab652e8dd96b1913) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "m407.chr", 0x0000, 0x000048, CRC(fa693a0d) SHA1(601afba4a6efe8334ecc2cadfee99273a9818c1c) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cala1.hex", 0x0000, 0x080000, CRC(c9768f65) SHA1(a8f2946fdba640033da0e21d4e18293b3fc004bf) )
	ROM_LOAD( "cala2.hex", 0x0000, 0x080000, CRC(56bd2950) SHA1(b109c726514c3ee04c1bbdf5f518f60dfd0375a8) )
ROM_END

ROM_START( m4calicl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ca2s.p1", 0x0000, 0x010000, CRC(fad153fd) SHA1(bd1f1a5c73624df45d01cb4853d87e998e434d7a) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ca2d.p1", 0x0000, 0x010000, CRC(75eb8c6f) SHA1(1bb923d06dcfa24eaf9533c083f68f4bd840834f) )
	ROM_LOAD( "ca2f.p1", 0x0000, 0x010000, CRC(6c53cf29) SHA1(2e58453891ab4faa17ef58a81c5f3c0618d046a5) )
	ROM_LOAD( "cald.p1", 0x0000, 0x010000, CRC(296fdeeb) SHA1(7782c0c7d8f44e2c0d48cc24c13015241e47b9ec) )
	ROM_LOAD( "cals.p1", 0x0000, 0x010000, CRC(28a1c5fe) SHA1(e8474df609ea7f3517780b54d6f493987aad3650) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "ca2s.chr", 0x0000, 0x000048, CRC(97618d38) SHA1(7958e99684d50b9bdb56c97f7fcfe161f0824578) )
ROM_END

ROM_START( m4cardcs )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cshcd19l.bin", 0x0000, 0x010000, CRC(539040f6) SHA1(f3a1a876c4f17b8d9a0cf2cd54ebf52f4b31dbd1) )
ROM_END

ROM_START( m4cojok )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cojx.p1", 0x0000, 0x010000, CRC(a9c0aefb) SHA1(c5b367a01ddee2cb90e266f1e62459b9b96eb3e3) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cojxb.p1", 0x0000, 0x010000, CRC(2680c84a) SHA1(6cf9bb72df41ea1389334597a772fd197aba4fc4) )
	ROM_LOAD( "cojxc.p1", 0x0000, 0x010000, CRC(a67db981) SHA1(08ac65baf774c63705c3a4db36248777375404f6) )
	ROM_LOAD( "cojxcd.p1", 0x0000, 0x010000, CRC(33d31701) SHA1(a7ccaa5a3b1c97cc84cdca2f77381ea4a8d743a3) )
	ROM_LOAD( "cojxd.p1", 0x0000, 0x010000, CRC(97c12c95) SHA1(282dfc5bc66fd4ad57f442c3ae75f6645919352d) )
	ROM_LOAD( "cojxdy.p1", 0x0000, 0x010000, CRC(4f0be63b) SHA1(d701b5c2d2c71942f8574598a4ba687f532c16a8) )
	ROM_LOAD( "cojxy.p1", 0x0000, 0x010000, CRC(88f1b57a) SHA1(cfc98d6ec90e7c186741d62d3ec68bd350196878) )
ROM_END

ROM_START( m4cashat )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "csa12y.p1", 0x0000, 0x020000, CRC(0374584a) SHA1(446e1d122d5b38e4ee11d98a4235d7198d98b541) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "caa22ad.p1", 0x0000, 0x020000, CRC(b6274874) SHA1(7c2dc0f3e8e7bb76f3b90300141b320fa0ca39ac) )
	ROM_LOAD( "caa22b.p1", 0x0000, 0x020000, CRC(e7f6f5e5) SHA1(fc16b50ae00525a3c84c0cbf7b418898cc5db1bc) )
	ROM_LOAD( "caa22bd.p1", 0x0000, 0x020000, CRC(581b2b6f) SHA1(55f910c7646d5e7d3be6ffd5b4ec0f04fb98b82e) )
	ROM_LOAD( "caa22d.p1", 0x0000, 0x020000, CRC(cc494044) SHA1(13ff215f41833aa133fe9d120792c834d1e0752b) )
	ROM_LOAD( "caa22dh.p1", 0x0000, 0x020000, CRC(18ae14fa) SHA1(20a8f197075ec153ac116b9a85e3591d9d4d045d) )
	ROM_LOAD( "caa22dk.p1", 0x0000, 0x020000, CRC(71fa4ee7) SHA1(ddf2cee47f93cc5794d64922658d5892993c8d2f) )
	ROM_LOAD( "caa22dr.p1", 0x0000, 0x020000, CRC(c2f5f5d6) SHA1(aebedb84ae388a1f0c558d36893d1341c1959594) )
	ROM_LOAD( "caa22dy.p1", 0x0000, 0x020000, CRC(3b3de6b1) SHA1(d72ce7851969466063c6d7952787691a7c44c9dd) )
	ROM_LOAD( "caa22h.p1", 0x0000, 0x020000, CRC(a743ca70) SHA1(e4b5ee02524873c2ccb66b4bfca39464c23eb43e) )
	ROM_LOAD( "caa22k.p1", 0x0000, 0x020000, CRC(ce17906d) SHA1(18a302132e683b00509982c09c6e3b00ae1201a0) )
	ROM_LOAD( "caa22r.p1", 0x0000, 0x020000, CRC(7d182b5c) SHA1(801d1b032e94cc45302a9f84ba7f9ce2b74f6449) )
	ROM_LOAD( "caa22s.p1", 0x0000, 0x020000, CRC(e7edf653) SHA1(f2bdf45cc18ad4b45b47d2b2b4641460fcdfa963) )
	ROM_LOAD( "caa22y.p1", 0x0000, 0x020000, CRC(84d0383b) SHA1(791666ce17fd65067df446a3320efd22bce23925) )
	ROM_LOAD( "caa23ad.p1", 0x0000, 0x020000, CRC(a8641c35) SHA1(18dad4634e27e4f0b791c331b9efcf5e1d56d3bb) )
	ROM_LOAD( "caa23b.p1", 0x0000, 0x020000, CRC(a867c129) SHA1(9b0b577938ae0500a8b80211710ed5c0b2a597fa) )
	ROM_LOAD( "caa23bd.p1", 0x0000, 0x020000, CRC(46587f2e) SHA1(b14ed6b810ba3039824a0d13c5b75fedd40803b3) )
	ROM_LOAD( "caa23d.p1", 0x0000, 0x020000, CRC(83d87488) SHA1(1e13a47de4837e42650c6a4a13a838eb68d0beae) )
	ROM_LOAD( "caa23dh.p1", 0x0000, 0x020000, CRC(06ed40bb) SHA1(68f2923c4ecd91231cc66a4be7c797d7b2a46ae0) )
	ROM_LOAD( "caa23dk.p1", 0x0000, 0x020000, CRC(6fb91aa6) SHA1(5966be8aa9d5348bbdcb85b21acceaabc3c02602) )
	ROM_LOAD( "caa23dr.p1", 0x0000, 0x020000, CRC(dcb6a197) SHA1(a18af78ba604b53a2af1e9b8dfdc6858964f631d) )
	ROM_LOAD( "caa23dy.p1", 0x0000, 0x020000, CRC(257eb2f0) SHA1(0a5f9743afb5dd7392425951580532ea5f8f17f1) )
	ROM_LOAD( "caa23h.p1", 0x0000, 0x020000, CRC(e8d2febc) SHA1(14fe5e1699fef74145f2f6fff61e75fe3e3a0b3b) )
	ROM_LOAD( "caa23k.p1", 0x0000, 0x020000, CRC(8186a4a1) SHA1(0d8f59df0fb5a1044f6fb7d81f50f9c9b94add9b) )
	ROM_LOAD( "caa23r.p1", 0x0000, 0x020000, CRC(32891f90) SHA1(c832c2610606bc5a3beeff8f85c31af496b14427) )
	ROM_LOAD( "caa23s.p1", 0x0000, 0x020000, CRC(26a49cdd) SHA1(ee28a22eeb8c4e8ddf041122505f9846d6b6d7d6) )
	ROM_LOAD( "caa23y.p1", 0x0000, 0x020000, CRC(cb410cf7) SHA1(31d34a766939a9b2a23be00c2ffd658d854b3ab4) )
	ROM_LOAD( "casattack8.bin", 0x0000, 0x020000, CRC(e29ea247) SHA1(ad00ea3bfd2eab51b20fd786cb1ce84de0d98173) )
	ROM_LOAD( "catt15g", 0x0000, 0x020000, CRC(3f7a8863) SHA1(df8ed393aeb3a5ec3fd5bdc01c9dbbb630e6d254) )
	ROM_LOAD( "catt15t", 0x0000, 0x020000, CRC(c6760c3a) SHA1(b7f4a3af52faf7e430e5b4ec75e2dc97e3f07dc0) )
	ROM_LOAD( "csa11ad.p1", 0x0000, 0x020000, CRC(7c1daa59) SHA1(9c0479094ba2f985803e58360b738b0baa2e410a) )
	ROM_LOAD( "csa11b.p1", 0x0000, 0x020000, CRC(c740daba) SHA1(afa5bdf9f6aacb3a5126aa828e4d0d2518efe663) )
	ROM_LOAD( "csa11bd.p1", 0x0000, 0x020000, CRC(55fccfd1) SHA1(b7c748573e5fb32a6be5e069e7f165a11c62b7d5) )
	ROM_LOAD( "csa11d.p1", 0x0000, 0x020000, CRC(ecff6f1b) SHA1(3f37d8e20d5663e376c9dc5876a910a320edcf7d) )
	ROM_LOAD( "csa11dh.p1", 0x0000, 0x020000, CRC(bbc0acca) SHA1(80d95505041fd4c869f9d835d0527070f2f582d9) )
	ROM_LOAD( "csa11dk.p1", 0x0000, 0x020000, CRC(1549f044) SHA1(22bc130106a23d0e9c354b4aa97d7b7fd8776082) )
	ROM_LOAD( "csa11dr.p1", 0x0000, 0x020000, CRC(cf121168) SHA1(aa52b528ac565684399dc58aeb56691457727035) )
	ROM_LOAD( "csa11dy.p1", 0x0000, 0x020000, CRC(36da020f) SHA1(ca202c7127450d905e4717776e1f1d32fa89279b) )
	ROM_LOAD( "csa11h.p1", 0x0000, 0x020000, CRC(297cb9a1) SHA1(63460eed75242fc7c27ee1fc9da28221e7bb21b1) )
	ROM_LOAD( "csa11k.p1", 0x0000, 0x020000, CRC(87f5e52f) SHA1(30b7f8c17198045bba30aaabbe74b3c1dc7d0320) )
	ROM_LOAD( "csa11r.p1", 0x0000, 0x020000, CRC(5dae0403) SHA1(6f2238f0fe0797bf0926044bb251fed6f97dbed6) )
	ROM_LOAD( "csa11s.p1", 0x0000, 0x020000, CRC(bef7a119) SHA1(88fc2003a7adda928e2e0fb78db32c7ffcbda924) )
	ROM_LOAD( "csa11y.p1", 0x0000, 0x020000, CRC(a4661764) SHA1(740be82275358b8e3dcec5982b18a083d043d99d) )
	ROM_LOAD( "csa12ad.p1", 0x0000, 0x020000, CRC(b15c5c64) SHA1(7a8c7b929ecaf0e14d9a5d6cdea303f5e3fc1dec) )
	ROM_LOAD( "csa12b.p1", 0x0000, 0x020000, CRC(60529594) SHA1(a5e70b55b8df6a94c963b970c3a4398b64b0286b) )
	ROM_LOAD( "csa12bd.p1", 0x0000, 0x020000, CRC(98bd39ec) SHA1(ebc5a2690f1453adae0f8faee0159a01df91dd6e) )
	ROM_LOAD( "csa12d.p1", 0x0000, 0x020000, CRC(4bed2035) SHA1(d5438d372222c4258ffb6487ba64eed9ce190133) )
	ROM_LOAD( "csa12dh.p1", 0x0000, 0x020000, CRC(76815af7) SHA1(6edd4a866a1b038a51cfcc9ed8cef48886b393fb) )
	ROM_LOAD( "csa12dk.p1", 0x0000, 0x020000, CRC(d8080679) SHA1(92babec65fbcee37ff8136a5c4b1e5f4ecd2f5a6) )
	ROM_LOAD( "csa12dr.p1", 0x0000, 0x020000, CRC(0253e755) SHA1(742174137549147ff23fbc9ba1b835cbeaffa602) )
	ROM_LOAD( "csa12dy.p1", 0x0000, 0x020000, CRC(fb9bf432) SHA1(5f519871cc50cf9f49ec652d620267cd11ab155b) )
	ROM_LOAD( "csa12h.p1", 0x0000, 0x020000, CRC(8e6ef68f) SHA1(b6ac0993938bb065f02498a71628cf532085b347) )
	ROM_LOAD( "csa12k.p1", 0x0000, 0x020000, CRC(20e7aa01) SHA1(093786b0992c1d9ce5e2d2cfad1eaf1d8e6dc733) )
	ROM_LOAD( "csa12r.p1", 0x0000, 0x020000, CRC(fabc4b2d) SHA1(3710b7b4bf56e46c60a60fcae82342bf201e38dc) )
	ROM_LOAD( "csa12s.p1", 0x0000, 0x020000, CRC(61c8af36) SHA1(d81a4056b573194a8627a3618f805d379140ff6a) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cas1.hex", 0x000000, 0x080000, CRC(4711c483) SHA1(af1ceb317b7bb1c2d0c3f7a99049679c356e1860) )
	ROM_LOAD( "cas2.hex", 0x080000, 0x080000, CRC(26ec235c) SHA1(51de955e5def47b82ac8891d09dc0b0e5e19c01d) )
ROM_END

ROM_START( m4cashcn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cash connect 70.bin", 0x0000, 0x010000, CRC(8b4f8056) SHA1(a9e7927ac9aa2ee1dccf207f4b249ae90c4c713d) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cc032h2.bin", 0x0000, 0x010000, CRC(f29c01e0) SHA1(6e302ce02e4568d582c6f55741334b3fb6ea0746) )
ROM_END

ROM_START( m4cashco )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "c324.bin", 0x0000, 0x010000, CRC(d559bde5) SHA1(aed48c699118258c5dc915d5982d3de75c0213a1) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "c318d.bin", 0x0000, 0x010000, CRC(980df80c) SHA1(bf513d72a3e56c7f8c2a1c50d8ebf82496cd7ee3) )
	ROM_LOAD( "cash counter v0-5", 0x0000, 0x010000, CRC(69012475) SHA1(e850de5b9c3ff83f758bee0e74f0f44108997305) )
	ROM_LOAD( "cashcounter-1-2.bin", 0x0000, 0x010000, CRC(e4fa4e0f) SHA1(e78fbf0fc550aca3dc9139cdd5791875206d80bf) )
ROM_END

ROM_START( m4cashln )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cls04s.p1", 0x0000, 0x020000, CRC(c8b7f355) SHA1(437324bf499ba49ecbb3854f5f787da5f575f7f5) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cl15g", 0x0000, 0x020000, CRC(fdd5765d) SHA1(fee8ddc9b93934a5582d6730cfa26246191c22ff) )
	ROM_LOAD( "cl15t", 0x0000, 0x020000, CRC(56bb9f21) SHA1(2876ac79283ea5cbee45e9ac6d5d140ea7db8e95) )
	ROM_LOAD( "cli10s", 0x0000, 0x020000, CRC(9aca737d) SHA1(6669c8b7a192b1c67caad62aad528b08737f7e73) )
	ROM_LOAD( "cli11ad.p1", 0x0000, 0x020000, CRC(ab47d9b3) SHA1(e501bb61de76ffe6618be03a59614b36e71e031b) )
	ROM_LOAD( "cli11b.p1", 0x0000, 0x020000, CRC(4adcaa58) SHA1(027dda275f32e49e6c0f9ff4e7d42472bdd3174e) )
	ROM_LOAD( "cli11bd.p1", 0x0000, 0x020000, CRC(e18071e5) SHA1(e219be249b5992808745a4c668686dfad77e2837) )
	ROM_LOAD( "cli11d.p1", 0x0000, 0x020000, CRC(c7c6049d) SHA1(6ad1caed1272ff4e6902843bf2420020ca2d0cb4) )
	ROM_LOAD( "cli11dh.p1", 0x0000, 0x020000, CRC(c21383ae) SHA1(01a840f160aacbb2b253ab6ba0a915f7055cc381) )
	ROM_LOAD( "cli11dk.p1", 0x0000, 0x020000, CRC(18486282) SHA1(5910a68539f5343661f8e55105c767067633ef1c) )
	ROM_LOAD( "cli11dr.p1", 0x0000, 0x020000, CRC(82a6bc3b) SHA1(c12b46234bebe408374a7fc3e18bf79942434d95) )
	ROM_LOAD( "cli11dy.p1", 0x0000, 0x020000, CRC(2ca4f94d) SHA1(4d0345e3c819d8dd48c559f01c118430d5ccdea2) )
	ROM_LOAD( "cli11h.p1", 0x0000, 0x020000, CRC(694f5813) SHA1(3acf84d6e5060e7054097aa2601c9c2833a2d524) )
	ROM_LOAD( "cli11k.p1", 0x0000, 0x020000, CRC(b314b93f) SHA1(e94472e1d1e345be84b277088dc852ed75117344) )
	ROM_LOAD( "cli11r.p1", 0x0000, 0x020000, CRC(29fa6786) SHA1(8bd7a7b0cf84615a126ebed494954ed2b7bc0ec4) )
	ROM_LOAD( "cli11s.p1", 0x0000, 0x020000, CRC(e5ad1734) SHA1(69fb5c81ae04c98920d84f829be14983168196e5) )
	ROM_LOAD( "cli11y.p1", 0x0000, 0x020000, CRC(87f822f0) SHA1(701a8c88be972b8363490e92e98f37acd493ef07) )
	ROM_LOAD( "cli12ad.p1", 0x0000, 0x020000, CRC(d6c03a42) SHA1(203ea61def95c2ea0a9ea1f808056122d87993ff) )
	ROM_LOAD( "cli12b.p1", 0x0000, 0x020000, CRC(9d4245da) SHA1(604b1e68d271784b982cde81e3675298662df1bc) )
	ROM_LOAD( "cli12bd.p1", 0x0000, 0x020000, CRC(9c079214) SHA1(1ee6362d876dd5aaa1699186ee905b48755a80de) )
	ROM_LOAD( "cli12d.p1", 0x0000, 0x020000, CRC(1058eb1f) SHA1(2c4305679c829cbf6c34d2627a1e8e366c5be5c1) )
	ROM_LOAD( "cli12dh.p1", 0x0000, 0x020000, CRC(bf94605f) SHA1(e2ae87041791ba92d8db69c7a3b40b62c97cf941) )
	ROM_LOAD( "cli12dk.p1", 0x0000, 0x020000, CRC(65cf8173) SHA1(f7ed6227b3e57d50e327f3ff72b9aafafa51da63) )
	ROM_LOAD( "cli12dr.p1", 0x0000, 0x020000, CRC(ff215fca) SHA1(40cab82f6a7f5229b365236b4758e73b2a3dce1e) )
	ROM_LOAD( "cli12dy.p1", 0x0000, 0x020000, CRC(51231abc) SHA1(ae80491fb1496f6fccbd46f424fe1d45640da8e2) )
	ROM_LOAD( "cli12h.p1", 0x0000, 0x020000, CRC(bed1b791) SHA1(d017f62a099475786fa15c7a185574301b80dbdd) )
	ROM_LOAD( "cli12k.p1", 0x0000, 0x020000, CRC(648a56bd) SHA1(25ad92789b12512b8c65cc912080f62abfe101a6) )
	ROM_LOAD( "cli12r.p1", 0x0000, 0x020000, CRC(fe648804) SHA1(27e74aea209b90dc3d8cf46474e271d68d9af7e2) )
	ROM_LOAD( "cli12s.p1", 0x0000, 0x020000, CRC(ef5aa578) SHA1(c4c288a297f5f6cd0712c396237aa3bf1363e188) )
	ROM_LOAD( "cli12y.p1", 0x0000, 0x020000, CRC(5066cd72) SHA1(03cef55b4fff8fb6edd804fdbc4076db6b234614) )
	ROM_LOAD( "cls03ad.p1", 0x0000, 0x020000, CRC(c68249cf) SHA1(d2d16ce76a5b144827a11f7fa471c7ea558c1ce0) )
	ROM_LOAD( "cls03b.p1", 0x0000, 0x020000, CRC(db667c56) SHA1(8a8f2374d0d02307206071718376400d5a52dc6c) )
	ROM_LOAD( "cls03bd.p1", 0x0000, 0x020000, CRC(4b98e70a) SHA1(04e0732dd8c0283dd928da9caafd78f509a4d479) )
	ROM_LOAD( "cls03c.p1", 0x0000, 0x020000, CRC(ec2cc144) SHA1(bc2879e4487ad5638e2818f4c8c5b23ab660cecc) )
	ROM_LOAD( "cls03d.p1", 0x0000, 0x020000, CRC(a67d7720) SHA1(556128a8c464a5d1de0ebb0de8ce87a7ea0813d3) )
	ROM_LOAD( "cls03dh.p1", 0x0000, 0x020000, CRC(22587ac6) SHA1(04cacccfebdc01046e048b2c98a05f3f97fcd6f3) )
	ROM_LOAD( "cls03dk.p1", 0x0000, 0x020000, CRC(015f4f5c) SHA1(fd0e622217d42f52cfba78989e8cf843cb08a17d) )
	ROM_LOAD( "cls03dr.p1", 0x0000, 0x020000, CRC(b250f46d) SHA1(02f04778ce6fb674ba9760b0fa9828b76a58239a) )
	ROM_LOAD( "cls03dy.p1", 0x0000, 0x020000, CRC(86bc6fa2) SHA1(40a2bb1148989b5895b0f58417f404f7b035b472) )
	ROM_LOAD( "cls03h.p1", 0x0000, 0x020000, CRC(b2a6e19a) SHA1(668527a7939ab70deb03b1db8a4a2629fb332815) )
	ROM_LOAD( "cls03k.p1", 0x0000, 0x020000, CRC(91a1d400) SHA1(9e4ccd4f4119471d66a22b14a22312149faf28c0) )
	ROM_LOAD( "cls03r.p1", 0x0000, 0x020000, CRC(22ae6f31) SHA1(b08ec5eb5bae377b829a14065b54ec1dbbc55677) )
	ROM_LOAD( "cls03s.p1", 0x0000, 0x020000, CRC(cb9a86b2) SHA1(2b4aee61c0070d295ba81ffa5739ceb8e05dc0e8) )
	ROM_LOAD( "cls03y.p1", 0x0000, 0x020000, CRC(1642f4fe) SHA1(b4e3fa360fdc1908cafe61d833c5097cee965404) )
	ROM_LOAD( "cls04ad.p1", 0x0000, 0x020000, CRC(6eb174ed) SHA1(dcf4e91ca16e3d5644429289470329102bc96f83) )
	ROM_LOAD( "cls04b.p1", 0x0000, 0x020000, CRC(73554174) SHA1(f74ba0a6212c58306d3e4f2e467e860bd4b2b294) )
	ROM_LOAD( "cls04bd.p1", 0x0000, 0x020000, CRC(e3abda28) SHA1(65b7883b52a8105f3df12659f4d9467f503ac1c6) )
	ROM_LOAD( "cls04c.p1", 0x0000, 0x020000, CRC(441ffc66) SHA1(d56f8e47a17de84d63c580bd1100dd53cc90071c) )
	ROM_LOAD( "cls04d.p1", 0x0000, 0x020000, CRC(0e4e4a02) SHA1(94cf99d072731bd82902d6237d226e7872f1aa69) )
	ROM_LOAD( "cls04dh.p1", 0x0000, 0x020000, CRC(8a6b47e4) SHA1(ba7b0810375b650fa4c503dc9d3504a6297ca2b5) )
	ROM_LOAD( "cls04dk.p1", 0x0000, 0x020000, CRC(a96c727e) SHA1(4c9dfc007f69231cd31f166ba0eea87ecbfe4abc) )
	ROM_LOAD( "cls04dr.p1", 0x0000, 0x020000, CRC(1a63c94f) SHA1(83ea53bfae911bbc1a2659b0a20ba2218849fdf0) )
	ROM_LOAD( "cls04dy.p1", 0x0000, 0x020000, CRC(2e8f5280) SHA1(453a9012174bd6715abd74c7be25e882f8209ee3) )
	ROM_LOAD( "cls04h.p1", 0x0000, 0x020000, CRC(1a95dcb8) SHA1(0926a6381e3426a18587d05430ad3fbe3ef9a0be) )
	ROM_LOAD( "cls04k.p1", 0x0000, 0x020000, CRC(3992e922) SHA1(9c39d2740ef51004ea59f859ec3e008c6586c00d) )
	ROM_LOAD( "cls04r.p1", 0x0000, 0x020000, CRC(8a9d5213) SHA1(dce36cdf790415bb37735a09226d73e322f7510a) )
	ROM_LOAD( "cls04y.p1", 0x0000, 0x020000, CRC(be71c9dc) SHA1(d2c2374685e953a028726ba48329196aa0a9f098) )
	ROM_LOAD( "ncc10ad.p1", 0x0000, 0x020000, CRC(d4be9280) SHA1(cbae1aad2dc8d88df9869755063a7a1097995417) )
	ROM_LOAD( "ncc10b.p1", 0x0000, 0x020000, CRC(42abdedf) SHA1(4c428288ba9bb426ae011127896634f5995f8a6c) )
	ROM_LOAD( "ncc10bd.p1", 0x0000, 0x020000, CRC(9e793ad6) SHA1(7c26c6cf2b23e2b73887c2eea1e5f7566127ca95) )
	ROM_LOAD( "ncc10d.p1", 0x0000, 0x020000, CRC(cfb1701a) SHA1(20c47f0d30c4e4da37ce38fc097e841fb3ee89ed) )
	ROM_LOAD( "ncc10dh.p1", 0x0000, 0x020000, CRC(bdeac89d) SHA1(5ddde5cee5c95714e66e10a061e76603a993f446) )
	ROM_LOAD( "ncc10dk.p1", 0x0000, 0x020000, CRC(67b129b1) SHA1(d7c0a107295ae3d11abe4138a1c76fd779777bec) )
	ROM_LOAD( "ncc10dr.p1", 0x0000, 0x020000, CRC(fd5ff708) SHA1(1bc5b96a46130370fc16b43804721eb392ec585f) )
	ROM_LOAD( "ncc10dy.p1", 0x0000, 0x020000, CRC(535db27e) SHA1(16b8d87fc22e040952988823d2c677130c17e137) )
	ROM_LOAD( "ncc10h.p1", 0x0000, 0x020000, CRC(61382c94) SHA1(98e4fcfedbe16773be0abbd6691ccd9f19a33684) )
	ROM_LOAD( "ncc10k.p1", 0x0000, 0x020000, CRC(bb63cdb8) SHA1(d4737ab4492bcdc0ffa3eb1cb0456f4f32e4482a) )
	ROM_LOAD( "ncc10r.p1", 0x0000, 0x020000, CRC(218d1301) SHA1(4bbdb69c2ac6ee13b50b1de8a17b019ce46004ba) )
	ROM_LOAD( "ncc10s.p1", 0x0000, 0x020000, CRC(2a18dc72) SHA1(f2434805212719db22ce163f2b338b25ca275c94) )
	ROM_LOAD( "ncc10y.p1", 0x0000, 0x020000, CRC(8f8f5677) SHA1(188d5d9c274147367bde644e33e162d0541cdca2) )
	ROM_LOAD( "ncl11ad.p1", 0x0000, 0x020000, CRC(73ba0558) SHA1(d86b2469d7cbe5f9271f8e8f3c6ee1659d72b3c6) )
	ROM_LOAD( "ncl11b.p1", 0x0000, 0x020000, CRC(f6e8ee46) SHA1(6d0da96b6cba4482c254fef6ba44a4e80fc0fda4) )
	ROM_LOAD( "ncl11bd.p1", 0x0000, 0x020000, CRC(397dad0e) SHA1(ad764f376b34f01578fc2ce78351f89642601252) )
	ROM_LOAD( "ncl11d.p1", 0x0000, 0x020000, CRC(7bf24083) SHA1(eec5d8b279b751a432ed8a2635ba6ecb519ef985) )
	ROM_LOAD( "ncl11dh.p1", 0x0000, 0x020000, CRC(1aee5f45) SHA1(1de8bc30925f7bc8c3363d680a07434875aee1d1) )
	ROM_LOAD( "ncl11dk.p1", 0x0000, 0x020000, CRC(c0b5be69) SHA1(b7c808da29f021a056eb268b1910f60cb570a0a7) )
	ROM_LOAD( "ncl11dr.p1", 0x0000, 0x020000, CRC(5a5b60d0) SHA1(634533386174cc77825c4def1c4524c20d665b30) )
	ROM_LOAD( "ncl11dy.p1", 0x0000, 0x020000, CRC(f45925a6) SHA1(49b7b949898d886431529f76a25cf4d2af3130f6) )
	ROM_LOAD( "ncl11h.p1", 0x0000, 0x020000, CRC(d57b1c0d) SHA1(cfb0e78ecba28a5ebe6e272190c473023544452b) )
	ROM_LOAD( "ncl11k.p1", 0x0000, 0x020000, CRC(0f20fd21) SHA1(25c3a7fe77819613de7bf4218759b096a05e5605) )
	ROM_LOAD( "ncl11r.p1", 0x0000, 0x020000, CRC(95ce2398) SHA1(72aa7ec767a81c568b442f63c3cf916925b22a4a) )
	ROM_LOAD( "ncl11s.p1", 0x0000, 0x020000, CRC(06ae30c0) SHA1(eb7fde45e0a0aa08f3c788f581b48adc8ee86a79) )
	ROM_LOAD( "ncl11y.p1", 0x0000, 0x020000, CRC(3bcc66ee) SHA1(795ecf1e34ae44d7aea70512124b66b0bed3e875) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cash lines 0.1 snd 1 9c3f.bin", 0x0000, 0x080000, CRC(1746f091) SHA1(d57fcaec3e3b0344671f2c984974bfdac50ec3d7) )

	ROM_LOAD( "cls1.hex", 0x000000, 0x080000, CRC(d6b5d862) SHA1(eab2ef2999229db7182896267cd83742b2390237) )
	ROM_LOAD( "cls2.hex", 0x080000, 0x080000, CRC(e42e674b) SHA1(1cda06425f3d4797ee0c4ff7426970150e5af4b6) )
ROM_END

ROM_START( m4cashmn )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cma07s.p1", 0x0000, 0x020000, CRC(e9c1d9f2) SHA1(f2df4ae650ec2b62d15bbaa562d638476bf926e7) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "camc2010", 0x0000, 0x020000, CRC(82e459ab) SHA1(62e1906007f6bba99e3e8badc3472070e8ae84f8) )
	ROM_LOAD( "cma07ad.p1", 0x0000, 0x020000, CRC(411889fd) SHA1(5855b584315867ecc5df6d37f4a664b8331ecde8) )
	ROM_LOAD( "cma07b.p1", 0x0000, 0x020000, CRC(ab889a33) SHA1(0f3ed0e4b8131585bcb4af47674fb1b65c37503d) )
	ROM_LOAD( "cma07bd.p1", 0x0000, 0x020000, CRC(cc022738) SHA1(5968d1b6db55008cbd3c83651214c61c28fd4c5c) )
	ROM_LOAD( "cma07c.p1", 0x0000, 0x020000, CRC(9cc22721) SHA1(ee4e9860641c8bf7db024a5bf9469265a6383e0a) )
	ROM_LOAD( "cma07d.p1", 0x0000, 0x020000, CRC(d6939145) SHA1(45b6f7f80c7a2f4377d9bf8e184fb791f4ed0a2d) )
	ROM_LOAD( "cma07dk.p1", 0x0000, 0x020000, CRC(86c58f6e) SHA1(fce50f86a641d27d0f5e5ecbac84822ccc9c177b) )
	ROM_LOAD( "cma07dr.p1", 0x0000, 0x020000, CRC(35ca345f) SHA1(ddbb926988028bef13ebaa949d3ee92599770003) )
	ROM_LOAD( "cma07dy.p1", 0x0000, 0x020000, CRC(0126af90) SHA1(0f303451fd8ca8c0cc50a31297f0d2729cfc2d7b) )
	ROM_LOAD( "cma07k.p1", 0x0000, 0x020000, CRC(e14f3265) SHA1(7b5dc581fe8679559356fdca9644985da7d299cb) )
	ROM_LOAD( "cma07r.p1", 0x0000, 0x020000, CRC(52408954) SHA1(623f840d94cc3cf2d2d648eb2be644d48350b169) )
	ROM_LOAD( "cma07y.p1", 0x0000, 0x020000, CRC(66ac129b) SHA1(97f8c0c1f46444d4a492bc3dd3689df038000640) )
	ROM_LOAD( "cma08ad.p1", 0x0000, 0x020000, CRC(fce2f785) SHA1(fc508e3d1036319894985600cb0142f13536078c) )
	ROM_LOAD( "cma08b.p1", 0x0000, 0x020000, CRC(df7526de) SHA1(71456496fc31ae11ffa7c543b6444adba735aeb9) )
	ROM_LOAD( "cma08bd.p1", 0x0000, 0x020000, CRC(71f85940) SHA1(439c54f35f4f6161a683d2c3d2bb6ce81b4190bf) )
	ROM_LOAD( "cma08c.p1", 0x0000, 0x020000, CRC(e83f9bcc) SHA1(e20297ba5238b59c3872776b01e6a89a51a7aea7) )
	ROM_LOAD( "cma08d.p1", 0x0000, 0x020000, CRC(a26e2da8) SHA1(928dfe399a7ae278dadd1e930bd370022f5113c4) )
	ROM_LOAD( "cma08dk.p1", 0x0000, 0x020000, CRC(3b3ff116) SHA1(f60f0f9d996398a0f1c5b7d2a411613c42149e65) )
	ROM_LOAD( "cma08dr.p1", 0x0000, 0x020000, CRC(88304a27) SHA1(9b86a49edca078dd68abab4c3e8655d3b4e79d47) )
	ROM_LOAD( "cma08dy.p1", 0x0000, 0x020000, CRC(bcdcd1e8) SHA1(a7a4ab2313198c3bc0536526bd83179fd9170e66) )
	ROM_LOAD( "cma08k.p1", 0x0000, 0x020000, CRC(95b28e88) SHA1(282a782900a0ddf60c66aa6a69e6871bb42c647a) )
	ROM_LOAD( "cma08r.p1", 0x0000, 0x020000, CRC(26bd35b9) SHA1(74d07da26932bf48fe4b79b39ff76956b0993f3b) )
	ROM_LOAD( "cma08s.p1", 0x0000, 0x020000, CRC(d0154d3c) SHA1(773f211092c51fb4ca1ef6a5a0cbdb15f842aca8) )
	ROM_LOAD( "cma08y.p1", 0x0000, 0x020000, CRC(1251ae76) SHA1(600ce195be615796b887bb56bebb6c4322709632) )
	ROM_LOAD( "cmh06ad.p1", 0x0000, 0x020000, CRC(ea2f6866) SHA1(afae312a488d7d83576c17eb2627a84637d88f18) )
	ROM_LOAD( "cmh06b.p1", 0x0000, 0x020000, CRC(2d4d9667) SHA1(896ed70962c8904646df7159c3717399d0ceb022) )
	ROM_LOAD( "cmh06bd.p1", 0x0000, 0x020000, CRC(6735c6a3) SHA1(4bce480c57473a9b0787a87a462c76e146a10157) )
	ROM_LOAD( "cmh06c.p1", 0x0000, 0x020000, CRC(1a072b75) SHA1(89d4aed011391b2f12b48c0344136d83175ff2f0) )
	ROM_LOAD( "cmh06d.p1", 0x0000, 0x020000, CRC(50569d11) SHA1(bdf7e984766bbe90bafbf0b367690ca65a8612d2) )
	ROM_LOAD( "cmh06dk.p1", 0x0000, 0x020000, CRC(2df26ef5) SHA1(c716b73396d0af1f69f5812bace06341d368859f) )
	ROM_LOAD( "cmh06dr.p1", 0x0000, 0x020000, CRC(9efdd5c4) SHA1(b9e02fe91e766aff41ca19879ab29e53bdee537e) )
	ROM_LOAD( "cmh06dy.p1", 0x0000, 0x020000, CRC(aa114e0b) SHA1(8bc9b94e488a98b8a8008f9a35b6c078cc5c8f3f) )
	ROM_LOAD( "cmh06k.p1", 0x0000, 0x020000, CRC(678a3e31) SHA1(2351b5167eec2a0d23c9938014de6f6ee07f13ff) )
	ROM_LOAD( "cmh06r.p1", 0x0000, 0x020000, CRC(d4858500) SHA1(489fd55ac6c93b94bfb9297fd71b5d74bf95a97f) )
	ROM_LOAD( "cmh06s.p1", 0x0000, 0x020000, CRC(9d3b4260) SHA1(7c4740585d17be3da3a0ea6e7fc68f89538013fb) )
	ROM_LOAD( "cmh06y.p1", 0x0000, 0x020000, CRC(e0691ecf) SHA1(978fa00736967dd09d48ce5c847698b39a058ab5) )
	ROM_LOAD( "cmh07ad.p1", 0x0000, 0x020000, CRC(4f354391) SHA1(687eccc312cd69f8bb70e35837f0b7ce74392936) )
	ROM_LOAD( "cmh07b.p1", 0x0000, 0x020000, CRC(27fb6e7b) SHA1(c1558e4a0e2c28a825c2c5bb4089143cf919b67c) )
	ROM_LOAD( "cmh07bd.p1", 0x0000, 0x020000, CRC(c22fed54) SHA1(5b6df1ed8518f9ba3e02b17c189c01ad1d0acbbb) )
	ROM_LOAD( "cmh07c.p1", 0x0000, 0x020000, CRC(10b1d369) SHA1(9933a2a7933df941ee93e16682e91dcc90abb627) )
	ROM_LOAD( "cmh07d.p1", 0x0000, 0x020000, CRC(5ae0650d) SHA1(da6917aa186daf59f35124c7cdc9d039d365c4c2) )
	ROM_LOAD( "cmh07dk.p1", 0x0000, 0x020000, CRC(88e84502) SHA1(2ab86be51b3dde0b2cb05e3af5f43aad3d8a76df) )
	ROM_LOAD( "cmh07dr.p1", 0x0000, 0x020000, CRC(3be7fe33) SHA1(074243cdfd37ba36e18e00610f45473e46ddc728) )
	ROM_LOAD( "cmh07dy.p1", 0x0000, 0x020000, CRC(0f0b65fc) SHA1(68d775bb4af9595ac87c33c2663b272640eea69e) )
	ROM_LOAD( "cmh07k.p1", 0x0000, 0x020000, CRC(6d3cc62d) SHA1(85f76fd8513c20683d486de7a1509cadfb6ecaa9) )
	ROM_LOAD( "cmh07r.p1", 0x0000, 0x020000, CRC(de337d1c) SHA1(dd07727fb183833eced5c0c2dc284d571baacd25) )
	ROM_LOAD( "cmh07s.p1", 0x0000, 0x020000, CRC(0367f4cf) SHA1(8b24a9009ff17d517b34e078ebbdc17465df139d) )
	ROM_LOAD( "cmh07y.p1", 0x0000, 0x020000, CRC(eadfe6d3) SHA1(80541aba612b8ebba7ab159c61e6492b9c06feda) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cmasnd.p1", 0x000000, 0x080000, CRC(1e7e13b8) SHA1(2db5c3789ad1b9bdb59e058562bd8be181ba0259) )
	ROM_LOAD( "cmasnd.p2", 0x080000, 0x080000, CRC(cce703a8) SHA1(97487f3df0724d3ee01f6f4deae126aec6d2dd68) )
ROM_END

ROM_START( m4cashmx )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cash_matrix_20p.bin", 0x0000, 0x010000, CRC(3f2ebfeb) SHA1(1dbabe81204f4b149c125aca3413d8e521a690ca) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cashmat1.hex", 0x0000, 0x010000, CRC(36f1a4bb) SHA1(7eefcbb1be539fcc302d226fa567e8691e85c360) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "cm.chr", 0x0000, 0x000048, CRC(3de2e6a6) SHA1(04e86e90561783f93d5e9d7a8b7f6dd3ea4f78f6) )
ROM_END

ROM_START( m4cashzn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cashzone.bin", 0x0000, 0x010000, CRC(78383d52) SHA1(cd69d4e600273e3da7aa61e51266e8f1765ad5dc) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "caz_1_2.p1", 0x0000, 0x010000, CRC(78383d52) SHA1(cd69d4e600273e3da7aa61e51266e8f1765ad5dc) )
	ROM_LOAD( "caz_1_5.p1", 0x0000, 0x010000, CRC(c0d80d62) SHA1(e062be12159a67b5da883345565f3a52a1dd2ebe) )
ROM_END

ROM_START( m4casmul )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "casinomultiplay.bin", 0x0000, 0x010000, CRC(2ebd1800) SHA1(d15e2593d17d8db9c6946af3366cf429ad291f76) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "casinomultiplaysnd.bin", 0x0000, 0x080000, CRC(be293e95) SHA1(bf0d419c898920a7546b542d8b205e25004ef04f) )
ROM_END

ROM_START( m4casot )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "casrom.bin",  0x00000, 0x10000,  CRC(da095666) SHA1(bc7654dc9da1f830a43f925db8079f27e18bb61e) ) // == old timer

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "caschar.chr", 0x0000, 0x000048, CRC(10d302d4) SHA1(5858e550470a25dcd64efe004c79e6e9783bce07) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cassound.bin", 0x0000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END

ROM_START( m4celclb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cels.p1",  0x00000, 0x10000,  CRC(19d2162f) SHA1(24fe435809352725e7614c32e2184142f355298e))

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "cels.chr", 0x0000, 0x000048, CRC(fe250f3a) SHA1(8b1d569a667921ba1768e7eebba81182466cfabf) )
ROM_END

ROM_START( m4centpt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "centrepoint v1.3.bin", 0x0000, 0x010000, CRC(24d117a5) SHA1(bd48a1687d11e32ea8cda19318e8936d1ffd9fd7) )
ROM_END

ROM_START( m4ceptr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dce10.bin", 0x0000, 0x010000, CRC(c94d41ef) SHA1(58fdff2de8dd3ead3980f6f34362183d084ce917) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cepsnd.p1", 0x000000, 0x080000, CRC(3a91784a) SHA1(7297ccec3264aa9f1e7b3a2841f5f8a1e4ca6c54) )
	ROM_LOAD( "cepsnd.p2", 0x080000, 0x080000, CRC(a82f0096) SHA1(45b6b5a2ae06b45add9cdbb9f5e6f834687b4902) )
ROM_END


ROM_START( m4chasei )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ci2c.p1", 0x0000, 0x010000, CRC(fc49a2e1) SHA1(f4f02e168cd9bf0245c2b7340fe151da66f09c5c) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ch20p8pn.rom", 0x0000, 0x010000, CRC(712bd2e7) SHA1(0e83fa077f42a051aaa07a7e13196955b0ac840d) )
	ROM_LOAD( "chin2010l", 0x0000, 0x010000, CRC(7fe97181) SHA1(1ccf65ff108bdaa46efcb3f831fccc953297b9ac) )
	ROM_LOAD( "ci2k.p1", 0x0000, 0x010000, CRC(8d715b8a) SHA1(5dd6f8d3d6710b0741df37af8792d942f41062d2) )
	ROM_LOAD( "ci2s.p1", 0x0000, 0x010000, CRC(8175e1e3) SHA1(9a4b0a0288508e7900ceac8bc3b245ac1f898b19) )
	ROM_LOAD( "ci2y.p1", 0x0000, 0x010000, CRC(80410946) SHA1(60a4f73eb9a35e5c246d8ef7b25bcf25b28bf8ed) )
	ROM_LOAD( "circ-71a.p1", 0x0000, 0x010000, CRC(a4b3b276) SHA1(fe88345ce04a8e60dae92cd112e11a822775a2ac) )

	ROM_LOAD( "chinv", 0x0000, 0x020000, CRC(c4b83744) SHA1(673d55c01c0f82cd89a3740f87750bb6c36d2a79) ) // bad dump of sound rom (part of cha.s1)
	ROM_LOAD( "chase invaders 8.bin", 0x0000, 0x010000, CRC(0bf6a8a0) SHA1(cea5ea40d71484a455615e14f6708b1bc06bbbe8) ) // bad prg (no vectors?)


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "chaseinvaders.chr", 0x0000, 0x000048, CRC(d7703dcd) SHA1(16fd998d1b44f35c10e5486882aa7f2d018dc82b) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cha.s1", 0x000000, 0x080000, CRC(8200b6bc) SHA1(bcc4ffbddcdcc1dd994fe29e9b24e83272f59442) )
	ROM_LOAD( "cha.s2", 0x080000, 0x080000, CRC(542863fa) SHA1(501d66b2badb5036bb5dd8bac3cdb681f630a982) )
ROM_END

ROM_START( m4cheryo )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dch14.bin", 0x0000, 0x010000, CRC(47333745) SHA1(479bec721ccaa2c4b11f3022d3d1eb12de92ac81) )
ROM_END

ROM_START( m4click )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "clcr.p1", 0x0000, 0x010000, CRC(b0dd4b66) SHA1(fbbd2e5e6a9c498225b219da4f11dd2d3a6c3545) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cc8ad.p1", 0x0000, 0x010000, CRC(80c64bbb) SHA1(ff004106b8be204fe38af19ec751926b1f7ad8df) )
	ROM_LOAD( "cc8b.p1", 0x0000, 0x010000, CRC(ebc8e052) SHA1(5ac2df221a2d8d374511e0973253509bed4414d2) )
	ROM_LOAD( "cc8bcd.p1", 0x0000, 0x010000, CRC(8e4950bf) SHA1(7165acd61263c7a1cada3379811d5c4e109a1a3e) )
	ROM_LOAD( "cc8bd.p1", 0x0000, 0x010000, CRC(122b6277) SHA1(686053964aa5bd1d3323e0b3a0adf1212ac5a992) )
	ROM_LOAD( "cc8c.p1", 0x0000, 0x010000, CRC(bf70180f) SHA1(473f0bc56a3a1e940f448ce758021a51fcb29344) )
	ROM_LOAD( "cc8d.p1", 0x0000, 0x010000, CRC(80c64bbb) SHA1(ff004106b8be204fe38af19ec751926b1f7ad8df) )
	ROM_LOAD( "cc8dy.p1", 0x0000, 0x010000, CRC(bd3e009c) SHA1(fb07860d7e6bca5b5b12c12aa8624ebfaf223877) )
	ROM_LOAD( "cc8s.p1", 0x0000, 0x010000, CRC(2c04f991) SHA1(c54ddd571dd9484c23c9f36a87b52fa357f4fdf2) )
	ROM_LOAD( "cc8y.p1", 0x0000, 0x010000, CRC(5352b635) SHA1(76d789dd1a912fbe35b6410cee60178854be3d3a) )
	ROM_LOAD( "cl3ad.p1", 0x0000, 0x010000, CRC(086ed5d9) SHA1(bbf8acf7660c365fecdf51943625ef63a4990b67) )
	ROM_LOAD( "cl3b.p1", 0x0000, 0x010000, CRC(1e90adfd) SHA1(a0c9dc92f6d851a99f011f032f16255bc9c7216d) )
	ROM_LOAD( "cl3bd.p1", 0x0000, 0x010000, CRC(5592b909) SHA1(3d36b5ae986306f2aea37f176b71f4b915f17620) )
	ROM_LOAD( "cl3d.p1", 0x0000, 0x010000, CRC(086ed5d9) SHA1(bbf8acf7660c365fecdf51943625ef63a4990b67) )
	ROM_LOAD( "cl3dy.p1", 0x0000, 0x010000, CRC(96c463a2) SHA1(ee97ee3db452f1e568eb92dc6627b61348e15b4b) )
	ROM_LOAD( "cl3r.p1", 0x0000, 0x010000, CRC(5cb8d2f8) SHA1(16b2643248c424bbdea984bf83f67c4300ccd85a) )
	ROM_LOAD( "cl3s.p1", 0x0000, 0x010000, CRC(d300d6e4) SHA1(c256d550250e270ed913b362c61921210598eb0e) )
	ROM_LOAD( "cl3xrd.p1", 0x0000, 0x010000, CRC(a1b6317a) SHA1(9b8ed6596b70a4197c759a1b1a0aad4bb2a4d5d7) )
	ROM_LOAD( "cl3y.p1", 0x0000, 0x010000, CRC(3a325d8c) SHA1(9a46f59463601206509d0e394d59fbab736f7850) )
	ROM_LOAD( "clcb.p1", 0x0000, 0x010000, CRC(64333462) SHA1(b9bcd0ecb6eac828b268b59ac174acacaa74e363) )
	ROM_LOAD( "clcdg.p1", 0x0000, 0x010000, CRC(76633c65) SHA1(65a27e5e4bf2ebb09284072f611c4176ce8d0157) )
	ROM_LOAD( "clcdy.p1", 0x0000, 0x010000, CRC(8c4752ff) SHA1(fb6942722242daf180abee7a16d7359b51ee09ca) )
	ROM_LOAD( "clcpl.p0", 0x0000, 0x010000, CRC(b774c1f1) SHA1(2708a1e4c539d72d9ada8c37d3372b64f6edc4a0) )
	ROM_LOAD( "clcpl.p1", 0x0000, 0x010000, CRC(51cf5f53) SHA1(f7b14acb0fd831aa19fdaa1bb36272ac9910f0b3) )
	ROM_LOAD( "clcs.p1", 0x0000, 0x010000, CRC(99dcea3e) SHA1(2ec9842f7d920d449cfeac43f7fe79f8c62ecec9) )
ROM_END

// at the very least some of these (such as the parent) don't use the oki.. but instead the basic AY
ROM_START( m4c999 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "c999 2p unprotected.bin", 0x0000, 0x010000, CRC(7637e074) SHA1(3b9e724cc1e657ab2a6cf6fe237f0ca43990aa53) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "c910.hex", 0x0000, 0x010000, CRC(80c1d5bb) SHA1(5928f58f7963710e4ec9043aae4f656d98888e5b) )
	ROM_LOAD( "c915.hex", 0x0000, 0x010000, CRC(dabfa3f3) SHA1(f507c78e61cba74e9b776bebaf0cc4fa40b6de95) )
	ROM_LOAD( "c9210dk.p1", 0x0000, 0x010000, CRC(169a3ce4) SHA1(74d5d533c145908d17bb3e6ac6fea6e3c826ef1e) )
	ROM_LOAD( "c9211.p1", 0x0000, 0x010000, CRC(44e5cc87) SHA1(36fca9493d36ee6988d02da1b4c575278c43748c) )
	ROM_LOAD( "c9510ad.p1", 0x0000, 0x010000, CRC(e1a6a573) SHA1(d653d8dce8d8df4151e2fcd8b93964e326bfbe7f) )
	ROM_LOAD( "c9510b.p1", 0x0000, 0x010000, CRC(80c1d5bb) SHA1(5928f58f7963710e4ec9043aae4f656d98888e5b) )
	ROM_LOAD( "c9510bd.p1", 0x0000, 0x010000, CRC(0aadc7d5) SHA1(143d937ef7b17d86d2e41065bb8f851b548ac8a3) )
	ROM_LOAD( "c9510d.p1", 0x0000, 0x010000, CRC(e669989f) SHA1(a9ee5e1d309585f21882681a06f064f6ed03951f) )
	ROM_LOAD( "c9510dk.p1", 0x0000, 0x010000, CRC(43be243e) SHA1(3974051fe47a192c135eceb2a7966e6a41b01a3d) )
	ROM_LOAD( "c9510dr.p1", 0x0000, 0x010000, CRC(8edf7aa6) SHA1(ac15a8c1d0e24cc99452b560b68a664e16e8d82f) )
	ROM_LOAD( "c9510dy.p1", 0x0000, 0x010000, CRC(b0ffae04) SHA1(81921a45a06c38a5391ed3edec57da74b220a181) )
	ROM_LOAD( "c9510k.p1", 0x0000, 0x010000, CRC(665b330a) SHA1(75fe5fbe6f3b11a21092f6d18f7f50980c92febe) )
	ROM_LOAD( "c9510r.p1", 0x0000, 0x010000, CRC(a9f25224) SHA1(3fe4091b27a2d789a8c5d00cb4fc00289639588f) )
	ROM_LOAD( "c9510s.p1", 0x0000, 0x010000, CRC(dc70433e) SHA1(86f158909fea49baf4239821ccf092d8ef1027b7) )
	ROM_LOAD( "c9510y.p1", 0x0000, 0x010000, CRC(3a93bc6a) SHA1(2832b48b6391746dbcea3484715dd6a169c081af) )
	ROM_LOAD( "c99910p6", 0x0000, 0x010000, CRC(e3f6710a) SHA1(d527541ec6e799c8bc12e1e31519415eaf11fbe5) )
	ROM_LOAD( "c99920p2", 0x0000, 0x010000, CRC(94f8f03e) SHA1(a99c3c60f2e9c15d5dd6265cfa73fad1058ce7fa) )
	ROM_LOAD( "c99920p6", 0x0000, 0x010000, CRC(f88f3bfc) SHA1(8dd1bd13645b8c3e38d45a8a6941e56d6268c21d) )
	ROM_LOAD( "clnv.p1", 0x0000, 0x010000, CRC(486097d8) SHA1(33e9eab0fb1c750160a8cb2b75eca73145d6956e) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "c9s.hex", 0x0000, 0x080000, CRC(ae952e15) SHA1(a9eed61c3d65ded5e1faa67362f181393cb6339a) )
ROM_END

/* some of these roms are also in the above set, probably not sorted properly */
ROM_START( m4c9 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "c910.hex", 0x0000, 0x010000, CRC(80c1d5bb) SHA1(5928f58f7963710e4ec9043aae4f656d98888e5b) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "c915.hex", 0x0000, 0x010000, CRC(dabfa3f3) SHA1(f507c78e61cba74e9b776bebaf0cc4fa40b6de95) )
	ROM_LOAD( "c9211ad.p1", 0x0000, 0x010000, CRC(dcabab11) SHA1(d73f33da37decfc403975a844916b49d527ee8f8) )
	ROM_LOAD( "c9211b.p1", 0x0000, 0x010000, CRC(2f10f98b) SHA1(4add53d98f31f4a8bedb621906e91e92622d2c95) )
	ROM_LOAD( "c9211bd.p1", 0x0000, 0x010000, CRC(6dc2add7) SHA1(26a2b9cd629132d7ba48c9ea3476c574006ad4af) )
	ROM_LOAD( "c9211c.p1", 0x0000, 0x010000, CRC(760ee71b) SHA1(ed124fc56a59c06b6ba8d250af5dbfd6154e55c3) )
	ROM_LOAD( "c9211d.p1", 0x0000, 0x010000, CRC(1dd0166f) SHA1(d8f11fc2cd2efe0f6436ffbb31dd6b5c16bbe3ec) )
	ROM_LOAD( "c9211dk.p1", 0x0000, 0x010000, CRC(0ee51f5c) SHA1(c773bf537f92a25b6d2f362d5ea1307eec8f1663) )
	ROM_LOAD( "c9211dr.p1", 0x0000, 0x010000, CRC(35c30093) SHA1(c3b56e468cad9ef0f80983b9c05daa3f38c80a2c) )
	ROM_LOAD( "c9211dy.p1", 0x0000, 0x010000, CRC(a8a8287d) SHA1(5e0de3b864251491d243984c5499650dafd8bb56) )
	ROM_LOAD( "c9211k.p1", 0x0000, 0x010000, CRC(4f9b6b6d) SHA1(5722c0698c3915eb380b24468539dccad6978218) )
	ROM_LOAD( "c9211r.p1", 0x0000, 0x010000, CRC(43f8b759) SHA1(cb0f731f1584e4d23602d276c085b31be6966bb1) )
	ROM_LOAD( "c9211s.p1", 0x0000, 0x010000, CRC(44e5cc87) SHA1(36fca9493d36ee6988d02da1b4c575278c43748c) )
	ROM_LOAD( "c9211y.p1", 0x0000, 0x010000, CRC(de939fb7) SHA1(a305bdf247f498f86cd681fba7d0593a668067c7) )
	ROM_LOAD( "ct202ad.p1", 0x0000, 0x010000, CRC(c8484dfd) SHA1(778fc30597b942fd75f5230ef3193b9f599abd03) )
	ROM_LOAD( "ct202b.p1", 0x0000, 0x010000, CRC(b7c611aa) SHA1(d7d4e7d4d06e7198424206b8259ca66cc06062bb) )
	ROM_LOAD( "ct202bd.p1", 0x0000, 0x010000, CRC(fe5420b7) SHA1(f443f1669b4f263b678526e2890671ad4e5848be) )
	ROM_LOAD( "ct202c.p1", 0x0000, 0x010000, CRC(a0997fbb) SHA1(52d6172d6b737a65d24d6750847ccf2797eb54d4) )
	ROM_LOAD( "ct202d.p1", 0x0000, 0x010000, CRC(5811f1a2) SHA1(87614b915aa697869739026bf45f53574123c6f2) )
	ROM_LOAD( "ct202dk.p1", 0x0000, 0x010000, CRC(58857027) SHA1(bcb37032237c7542bfde915de815eb93b5def43e) )
	ROM_LOAD( "ct202dr.p1", 0x0000, 0x010000, CRC(b2769912) SHA1(b3030c07a07774462e956201b5843e366df39c47) )
	ROM_LOAD( "ct202dy.p1", 0x0000, 0x010000, CRC(47aa690b) SHA1(a3fd71dae7a94402641048b5e986f13347bc28ac) )
	ROM_LOAD( "ct202k.p1", 0x0000, 0x010000, CRC(990cf3cd) SHA1(13d29f3111d193e8cca45d8319f8657066b2ac8a) )
	ROM_LOAD( "ct202r.p1", 0x0000, 0x010000, CRC(0da3e958) SHA1(37760de8134e9298212ddebaebe79a08016da7e9) )
	ROM_LOAD( "ct202s.p1", 0x0000, 0x010000, CRC(19214c6e) SHA1(93c8c40fd7b3a8873715e7bee88a09a995b44b28) )
	ROM_LOAD( "ct202y.p1", 0x0000, 0x010000, CRC(79362dcc) SHA1(80782ddb98f896101fa89f77ce76aa6f63391645) )
	ROM_LOAD( "ct302ad.p1", 0x0000, 0x010000, CRC(2f29a7e9) SHA1(059f73b6a9c2a1d8f9b8bbef9050c61c2d4f13bb) )
	ROM_LOAD( "ct302b.p1", 0x0000, 0x010000, CRC(1e677623) SHA1(c6ee2686f853626e390f28a611e8861cc8f935b0) )
	ROM_LOAD( "ct302bd.p1", 0x0000, 0x010000, CRC(70e60d8f) SHA1(139d92aa978df03af7b7913b6d4e56b211e9ddba) )
	ROM_LOAD( "ct302c.p1", 0x0000, 0x010000, CRC(6a157909) SHA1(cdb8a9a61bcb5e817305cfcde8ad5c3bd74b1cee) )
	ROM_LOAD( "ct302d.p1", 0x0000, 0x010000, CRC(2ca799c7) SHA1(675647f6e1811f2ef1c79a1a49cbb1aaace66444) )
	ROM_LOAD( "ct302dk.p1", 0x0000, 0x010000, CRC(603baa69) SHA1(da00850aa2439a203f6d903d43c8657a3ce3327b) )
	ROM_LOAD( "ct302dr.p1", 0x0000, 0x010000, CRC(9b7b28d1) SHA1(6c365a508a87977aeccb13f0e842d882af5a8192) )
	ROM_LOAD( "ct302dy.p1", 0x0000, 0x010000, CRC(8da792a5) SHA1(38e4aff15b5de00090bf93834f7e215c450f26aa) )
	ROM_LOAD( "ct302k.p1", 0x0000, 0x010000, CRC(cfb85369) SHA1(c5726477aeea5a70e8eef74e57732fe85abea737) )
	ROM_LOAD( "ct302r.p1", 0x0000, 0x010000, CRC(10c64611) SHA1(d85df4ca0fc13ddab219a5602019e54471b83aaf) )
	ROM_LOAD( "ct302y.p1", 0x0000, 0x010000, CRC(46514a44) SHA1(71e698c88488a67e94c322cb393f637c7e35d633) )
	ROM_LOAD( "ct502ad.p1", 0x0000, 0x010000, CRC(ff0ec7a7) SHA1(80ddc21a0df33aaa1c76ed5f57598494a1c36c5a) )
	ROM_LOAD( "ct502b.p1", 0x0000, 0x010000, CRC(2585dc82) SHA1(10ee12ecc6dfc09f9f9993b2fce837b0989c19ee) )
	ROM_LOAD( "ct502bd.p1", 0x0000, 0x010000, CRC(0d80572d) SHA1(6dfb48438accef039e2de12962ad826eaa3caee4) )
	ROM_LOAD( "ct502c.p1", 0x0000, 0x010000, CRC(713d24df) SHA1(bac23bdaecbfceeaffe67b5eb6a84210e43d6a90) )
	ROM_LOAD( "ct502d.p1", 0x0000, 0x010000, CRC(e3ca4d87) SHA1(ab6854e0f546b4100690cda2584f162b27b9ba86) )
	ROM_LOAD( "ct502dk.p1", 0x0000, 0x010000, CRC(1c39ef10) SHA1(1d5e027c171757072801f9078bd829d4d732c21e) )
	ROM_LOAD( "ct502dr.p1", 0x0000, 0x010000, CRC(0e8781d2) SHA1(656b737a79885447ca7f30aca7a1123846408cd1) )
	ROM_LOAD( "ct502dy.p1", 0x0000, 0x010000, CRC(54d27491) SHA1(ba4474f98474da828ebc7bf9db52ead05df0cdfc) )
	ROM_LOAD( "ct502k.p1", 0x0000, 0x010000, CRC(f53ee613) SHA1(678f59b923054e6d91ea1bd91515b6522f192a8c) )
	ROM_LOAD( "ct502r.p1", 0x0000, 0x010000, CRC(b678557d) SHA1(fbf3c367d40d2f914906eb7cd7e95713bfe7fc30) )
	ROM_LOAD( "ct502s.p1", 0x0000, 0x010000, CRC(cb02b9e7) SHA1(786c64abd0b9c5dc23b1508a2527e87e385acfa9) )
	ROM_LOAD( "ct502y.p1", 0x0000, 0x010000, CRC(f4cc4dc9) SHA1(d23757467830dfbdeed2a52a0c7e31276124d24d) )
	ROM_LOAD( "du91.0", 0x0000, 0x010000, CRC(6207753d) SHA1(b19bcb60707b73f37e9bd8177d0b15847af0213f) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "c9o02__1.1", 0x0000, 0x010000, CRC(109f7040) SHA1(3fe9da13d9746e1cdaf6dcd539e4af624d2cec71) )
	ROM_LOAD( "c9o05__1.1", 0x0000, 0x010000, CRC(2c821aa8) SHA1(33fba7dea0f66e7b0251971864d5a2923f96f8cd) )
	ROM_LOAD( "c9o10__1.1", 0x0000, 0x010000, CRC(c5063185) SHA1(ca98038ccd85ebc370cacce8583ddbc1f759558d) )
	ROM_LOAD( "c9o10d_1.1", 0x0000, 0x010000, CRC(6b20b16d) SHA1(15079fc5f14f545c291d357a795e6b41ca1d5a47) )
	ROM_LOAD( "c9o20__1.1", 0x0000, 0x010000, CRC(e05fa532) SHA1(63d070416a4e6979302901bb33e20c994cb3723e) )
	ROM_LOAD( "c9o20d_1.1", 0x0000, 0x010000, CRC(047b2d83) SHA1(b83f8fe6477226ef3e75f406020ea4f8b3d55c32) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "du91.chr", 0x0000, 0x000048, CRC(9724122d) SHA1(a41687eec84cad453c1a2a89317078f48ca0895f) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "c9s.hex", 0x0000, 0x080000, CRC(ae952e15) SHA1(a9eed61c3d65ded5e1faa67362f181393cb6339a) )
ROM_END

ROM_START( m4c9c )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cncs.p1", 0x0000, 0x010000, CRC(10f15e2a) SHA1(c17ab13764d74302246984245485cb7692913b44) )
ROM_END


ROM_START( m4clbcls )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cl11s.bin", 0x0000, 0x010000, CRC(d0064e6e) SHA1(17235d69ef56989a3f05458423a6b101bd635095) )
ROM_END

ROM_START( m4clbclm )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "c1cs.p1", 0x8000, 0x008000, CRC(d646bb63) SHA1(98194ae09e07229c8a44b01813290d1b9c89641c) )
	ROM_LOAD( "c1c.p2", 0x4000, 0x004000, CRC(9726be3a) SHA1(cfe0e3aa448ddce23f9a0591634930204f8c3fe6) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "clubclimber.hex", 0x0000, 0x00c000, CRC(d40180a9) SHA1(30edc2dd1d27bf63ad81456bda43fdaba6f4d1fe) )
ROM_END

ROM_START( m4clbcnt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "con12.bin", 0x0000, 0x010000, CRC(e577d7bc) SHA1(be4d6d75f33782c503f91659b5f69d1fb4c220da) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "con1_0.bin", 0x0000, 0x010000, CRC(2a758619) SHA1(77f993090b7d01901635c56fd9256f57d2371c6d) )
	ROM_LOAD( "con1-1.bin", 0x0000, 0x010000, CRC(9fc1fefc) SHA1(5638c978687526858cbcb105bdb499dce2d234d3) )
	ROM_LOAD( "conf.p1", 0x0000, 0x010000, CRC(7900cee0) SHA1(afcd77e5bc05a21b5eb7b26c66d94ce21f2ce501) )
	ROM_LOAD( "cons.p1", 0x0000, 0x010000, CRC(afe71a40) SHA1(b0065e131eb8fcf7a2fd420f60e2174d927db450) )
ROM_END

ROM_START( m4clbdbl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cd16p1.bin", 0x8000, 0x008000, CRC(368b182e) SHA1(ca09510970e0aaab3897381415b142c1b4dbb56a) )
	ROM_LOAD( "cd16p2.bin", 0x4000, 0x004000, CRC(61d64f04) SHA1(2d7f0d5d98da3259cdd20aa5f201c2748056dec5) )
ROM_END

ROM_START( m4clbrpl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "clrpy1p1.bin", 0xc000, 0x004000, CRC(ee145d39) SHA1(02b9b882ec1b0662f5770181de3482592afd8621) )
	ROM_LOAD( "clrpy1p2.bin", 0x8000, 0x004000, CRC(0d5ca26e) SHA1(a582feb09d4bb6c5ee4c555a1741b0594b110c34) )
ROM_END

ROM_START( m4clbshf )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "csss.p1", 0x0000, 0x010000, CRC(32dd9b96) SHA1(93831858b2f0ada8e4a0aa2fae59d12c53287df1) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "club_shuffle.chr", 0x0000, 0x000048, CRC(97618d38) SHA1(7958e99684d50b9bdb56c97f7fcfe161f0824578) )
ROM_END

ROM_START( m4clbtro )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tr2f.p1", 0x0000, 0x010000, CRC(fbdcd06f) SHA1(27ccdc83e60a62227d33d8cf3d516fc43908ab99) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "tr2d.p1", 0x0000, 0x010000, CRC(0cc23f89) SHA1(a66c8c28073f53381c43e3e597f15f81c5c61479) )
	ROM_LOAD( "tr2s.p1", 0x0000, 0x010000, CRC(6d43375c) SHA1(5be1dc85374c6a1235e0b137b46ebd7a2d7d922a) )
	ROM_LOAD( "tro20.bin", 0x0000, 0x010000, CRC(5e86c3fc) SHA1(ce2419991559839a8875060c1afe0f030190010a) )
	ROM_LOAD( "trod.p1", 0x0000, 0x010000, CRC(60c84612) SHA1(84dc8b34e41436331832c1a32ddac0fce269488a) )
	ROM_LOAD( "tros.p1", 0x0000, 0x010000, CRC(5e86c3fc) SHA1(ce2419991559839a8875060c1afe0f030190010a) )
ROM_END

ROM_START( m4clbveg )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "clad.p1", 0x0000, 0x010000, CRC(4fa45cce) SHA1(58a5d6cc8608eb1aa453429e26eacea589afa524) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "claf.p1", 0x0000, 0x010000, CRC(79b83184) SHA1(7319a405b2b0b274e03f5cd1465436f8548065e4) )
	ROM_LOAD( "clal.p1", 0x0000, 0x010000, CRC(db0bb5a2) SHA1(2735e02642fb92bb824e3b1f415a1a3ef13a856d) )
	ROM_LOAD( "clas.p1", 0x0000, 0x010000, CRC(6aad03f0) SHA1(2f611cc6f020e334dc4b87d2d907727ba15ff7ff) )
	ROM_LOAD( "cvegas.hex", 0x0000, 0x010000, CRC(6aad03f0) SHA1(2f611cc6f020e334dc4b87d2d907727ba15ff7ff) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "cvegas.chr", 0x0000, 0x000048, CRC(a6c341b0) SHA1(c8c838c9bb1ced52889504b9cea8d88f1e7fa79f) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cvegass1.hex", 0x0000, 0x080000, CRC(13a8c857) SHA1(c66e10bca1ad54f467b9c5eacd502c54397c09b2) )
	ROM_LOAD( "cvegass2.hex", 0x0000, 0x080000, CRC(88b37145) SHA1(1c6c9ad2010e1688d3370d1f2a5ae83dc683b500) )
ROM_END

ROM_START( m4clbx )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "clx12s.p1", 0x0000, 0x020000, CRC(6798c153) SHA1(e621e341a0fed1cb35637edb0769ae1cca72a663) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "clx12d.p1", 0x0000, 0x020000, CRC(43e797ba) SHA1(fb2fc843176fe50c1039214d48815d6e9871ae27) )
	ROM_LOAD( "clx12f.p1", 0x0000, 0x020000, CRC(3e6a82fe) SHA1(01ef9a15a3cf9b1191c573b36fb5758e79c3adc1) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cxs1.hex", 0x000000, 0x080000, CRC(4ce005f1) SHA1(ee0f59a9c7e0222dd63fa63ccff8f194abd01ddb) )
	ROM_LOAD( "cxs2.hex", 0x080000, 0x080000, CRC(495e0730) SHA1(7ba8150fbcf974ac494a82fd373ff02185543e35) )
ROM_END


ROM_START( m4copcsh )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "copcash2.bin", 0x0000, 0x010000, CRC(7ba83a9f) SHA1(ce294b7978b4edc64ee35795f017c2e066d1cb61) )
ROM_END

ROM_START( m4coscas )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cc30s.p1", 0x0000, 0x020000, CRC(e308100a) SHA1(14cb07895d17237768877dd62ba7c3fc8e5b2630) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cosm15g", 0x0000, 0x020000, CRC(edd01d55) SHA1(49246fa1e12ceb3297f35616cdc1cf62472a379f) )
	ROM_LOAD( "cosmiccasinos15.bin", 0x0000, 0x020000, CRC(ddba1241) SHA1(7ca2928ae2ab4e323b60bb661b60681f89cc5663) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cc_sj___.3s1", 0x0000, 0x020000, CRC(52c312b0) SHA1(bd5381d58b1acb7adf6857c142eae4a253081fbd) )
	ROM_LOAD( "cc_sj__c.3r1", 0x0000, 0x020000, CRC(44b940a6) SHA1(7e621873fcf6460f654e35cc74552e86b6253ddb) )
	ROM_LOAD( "cc_sj__c.7_1", 0x0000, 0x020000, CRC(ee9e6126) SHA1(fab6fd04004acebf291544720ba06cea79d5a054) )
	ROM_LOAD( "cc_sj_b_.3s1", 0x0000, 0x020000, CRC(019f0a71) SHA1(7a97f4e89c16e25f8e7502bba37f49c8496fbb47) )
	ROM_LOAD( "cc_sj_bc.3r1", 0x0000, 0x020000, CRC(de9bb8e1) SHA1(7974b03974531eb4b5ed865b8eeb9649c1346df4) )
	ROM_LOAD( "cc_sj_bc.7_1", 0x0000, 0x020000, CRC(afe1aac6) SHA1(fc9c69e45db6a85c45ef8d32d048e5726d7da655) )
	ROM_LOAD( "cc_sj_d_.3s1", 0x0000, 0x020000, CRC(215e12f3) SHA1(68ed9923c6fd51e9305afac9d271c7b3ce38b12f) )
	ROM_LOAD( "cc_sj_dc.3r1", 0x0000, 0x020000, CRC(00e357c3) SHA1(02bf7427899d2e536442b87d41c140ebd787a580) )
	ROM_LOAD( "cc_sj_dc.7_1", 0x0000, 0x020000, CRC(330d68a2) SHA1(12410af5f37b26f29f5cd23606ab0e128675095a) )
	ROM_LOAD( "cc_sj_k_.3s1", 0x0000, 0x020000, CRC(9161912d) SHA1(d11109f4bdc1c60f4cf477e1f26556800a83abdb) )
	ROM_LOAD( "cc_sj_kc.3r1", 0x0000, 0x020000, CRC(b0dcd41d) SHA1(6b50a5e401bf854186331673dcc0c3fc5de2991b) )
	ROM_LOAD( "cc_sja__.3s1", 0x0000, 0x020000, CRC(1682b1d3) SHA1(24baaf789eca150f0f6fd9c510e245aa7b88cc4c) )
	ROM_LOAD( "cc_sja_c.3r1", 0x0000, 0x020000, CRC(373ff4e3) SHA1(55b7ab247863eb3c025e84782c8cab7734343077) )
	ROM_LOAD( "cc_sja_c.7_1", 0x0000, 0x020000, CRC(e956898e) SHA1(f51682651520551d481360bf86eba510cd758441) )
	ROM_LOAD( "cc_sjb__.3s1", 0x0000, 0x020000, CRC(5c451985) SHA1(517f634d31f7190ca6685c1037fb66a8b87effba) )
	ROM_LOAD( "cc_sjb_c.7_1", 0x0000, 0x020000, CRC(109e9ae9) SHA1(00f381beb33cae58fc3429d3501efa4a9d9f0035) )
	ROM_LOAD( "cc_sjbgc.3r1", 0x0000, 0x020000, CRC(2de82f88) SHA1(5c8029d43282a014e82b4f975616ed2bbc0e5641) )
	ROM_LOAD( "cc_sjbtc.3r1", 0x0000, 0x020000, CRC(976c2858) SHA1(a70a8fe51d1b9d903d099e89a40481ea6af13683) )
	ROM_LOAD( "cc_sjwb_.3s1", 0x0000, 0x020000, CRC(e2df8167) SHA1(c312b30402dd93c6d4a32932677430c9c996fd36) )
	ROM_LOAD( "cc_sjwbc.3r1", 0x0000, 0x020000, CRC(a33a59a6) SHA1(a74ffd647e8390d89df475cc3f5205462c9d93d7) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cc___snd.1_1", 0x000000, 0x080000, CRC(d858f238) SHA1(92a3dfacde8bfa8705e91fab5bb627f9b34ad2dc) )
	ROM_LOAD( "cc___snd.1_2", 0x080000, 0x080000, CRC(bab1bd8e) SHA1(c703d0e24c0a522ebf79895049e85f5471f7d7e9) )
ROM_END

ROM_START( m4crkpot )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cp1_2.p1", 0xc000, 0x004000, CRC(fae352c7) SHA1(a0a15e6e4af58376d871fb5e719526d71727b098) )
	ROM_LOAD( "cp1_2.p2", 0x8000, 0x004000, CRC(96e095bb) SHA1(329b6e146a3021fcd79e03fab5e7067aeb65324b) )
	ROM_LOAD( "cp1_2.p3", 0x6000, 0x002000, CRC(ab648a6e) SHA1(b2d2a27bc2e1cee05f442817eb243aef04ed2894) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cp38jp1.bin", 0x0000, 0x004000, CRC(0b0794f3) SHA1(7a039c26dd7e3c0fe22c1da5f07167378a3eb2ca) )
	ROM_LOAD( "cp38jp2.bin", 0x0000, 0x004000, CRC(5e70192e) SHA1(840418f2de02876700fe4865745a66a45694114e) )
	ROM_LOAD( "cp38jp3.bin", 0x0000, 0x002000, CRC(54b85efc) SHA1(7750ef653afd44ce5fbd67f0622688b92fcf22e5) )
	ROM_LOAD( "cp_3_1.p1", 0x0000, 0x004000, CRC(f9e0d95a) SHA1(ba69ae5b77e58deb288e13152f89b65cdc44a80d) )
	ROM_LOAD( "cp_3_1.p2", 0x0000, 0x004000, CRC(e013dadc) SHA1(f57c0732cc258cbac1b5605f0cdbcbe6c7464041) )
	ROM_LOAD( "cp_3_1.p3", 0x0000, 0x002000, CRC(e2935a4e) SHA1(8af34d1dd156cf9c529ad0cf8496c8fc4365711c) )
ROM_END

ROM_START( m4crzjk )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "crjok2.04.bin", 0x0000, 0x010000, CRC(838336d6) SHA1(6f36de20c930cbbff479af2667c11152c6adb43e) )
ROM_END

ROM_START( m4crzjwl )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cj11bin", 0x0000, 0x020000, CRC(208fda73) SHA1(8b15c197693ea7749bc961fe4e5e36b317f9f6f8) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cjew_v1-0k", 0x0000, 0x020000, CRC(4434902d) SHA1(31c7be1235cdfd00099d1e09644a0f76fc7a26f7) )
	ROM_LOAD( "cjexlow", 0x0000, 0x020000, CRC(07c227c1) SHA1(286341ed44ef7cd08ca411f2b3e6936b5e83a5f3) )
	ROM_LOAD( "cjgerman", 0x0000, 0x020000, CRC(b090e690) SHA1(bdbe4041085c995761306280c15f782ea3bdc110) )
	ROM_LOAD( "cjj54.bin", 0x0000, 0x020000, CRC(16dc92e7) SHA1(b791535054d5864c7053243408a54accfa014bd1) )
	ROM_LOAD( "gcn11", 0x0000, 0x020000, CRC(51493500) SHA1(901e60c1a7e9e628d723e199579fc82cf2e433e6) )
	ROM_LOAD( "gcn111", 0x0000, 0x020000, CRC(b1152ce6) SHA1(1d236bad57ad38b11215efe44008bb8e4014939e) )
	ROM_LOAD( "gjv4", 0x0000, 0x020000, CRC(df63105d) SHA1(56e28adef9ec8921da7ab8045859e834731196c5) )
	ROM_LOAD( "gjv5", 0x0000, 0x020000, CRC(e4f0bab2) SHA1(1a13d97ff2c4fbae39327f2a5a8b110f2617857e) )
	// Crown Jewels Deluxe?
	ROM_LOAD( "cjg.p1", 0x0000, 0x020000, CRC(1f4743bf) SHA1(f9a0da2ed9cad5e6685c8a6d1d09e5d4bbcfacec) )


	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "cjsound1.bin", 0x000000, 0x080000, CRC(b023f6b9) SHA1(04c362c6511442d3ab775a5ff2051bfe26d5e624) )
	ROM_LOAD( "cjsound2.bin", 0x080000, 0x080000, CRC(02563a43) SHA1(dfcee4e0fdf81c726c8e13278e7950459bcaab18) )
	ROM_LOAD( "cjsound3.bin", 0x100000, 0x080000, CRC(e722e438) SHA1(070f3772920fa64d5214843c313b27a5b2a4c105) )

	ROM_LOAD( "cjew_sound1", 0x000000, 0x080000, CRC(a2f20c95) SHA1(874b22850732514a26448cee8e0b68f8d042a7c7) )
	ROM_LOAD( "cjew_sound2", 0x080000, 0x080000, CRC(3dcb7c38) SHA1(3c0e91f4d2ea9e6b25a01702c6f6fdc7cc2e0b65) )
ROM_END

ROM_START( m4crjwl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cjcf.p1", 0x0000, 0x010000, CRC(7feccc74) SHA1(4d1c7c6d2085492ee4205a7383ad7dc1de4e8d60) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cjcd.p1", 0x0000, 0x010000, CRC(cb83f226) SHA1(f09996436b3db3c8f0fe237884d9125be2b7855e) )
	ROM_LOAD( "cjcs.p1", 0x0000, 0x010000, CRC(1054e02d) SHA1(067705f20862f6cfc4334c74e0fab1a1016d427c) )
	ROM_LOAD( "cjn02.p1", 0x0000, 0x010000, CRC(a3d50e20) SHA1(15698e74a37d5f95a5634d48ae2a9a5d19faa2b6) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* Missing? or in above set? */
ROM_END

ROM_START( m4crjwl2 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cj214f.p1", 0x0000, 0x010000, CRC(7ee4d30c) SHA1(2bf702bc925c473f7e9eaeb5b3ae0b00e124161a) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cj214d.p1", 0x0000, 0x010000, CRC(359e2a73) SHA1(c85eeebafca14e6f975953f5daf2772a62693051) )
	ROM_LOAD( "cj214s.hex", 0x0000, 0x010000, CRC(296aa885) SHA1(045b02848b37e8a04d950d54301dc6888d6178ad) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "chr.chr", 0x0000, 0x000048, CRC(c5812913) SHA1(d167b1f512c183cf01a1f4e1c1588ea0ae21331b) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cjcs1.hex", 0x000000, 0x080000, CRC(2ac3ba9f) SHA1(3332f29f81918c34aeec3da6f7d001dc9922840d) )
	ROM_LOAD( "cjcs2.hex", 0x080000, 0x080000, CRC(89838a9d) SHA1(502243cc0a14e63882b537f05c4cc0eb852e4a0c) )
ROM_END

ROM_START( m4crdome )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cd212k.p1", 0x0000, 0x020000, CRC(673b10a1) SHA1(996ade8193f448970beea2c5b81d9f27c05f162f) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cd212c.p1", 0x0000, 0x020000, CRC(1ab605e5) SHA1(03327b2fac9d3d2891dc5950aa89ac4947c7b444) )
	ROM_LOAD( "cd212ad.p1", 0x0000, 0x020000, CRC(c76cab39) SHA1(abbe5d629929ff89b499cd4d0e15e9fa13fc33de) )
	ROM_LOAD( "cd212b.p1", 0x0000, 0x020000, CRC(2dfcb8f7) SHA1(ba711fb20556c447f4bb3a11fc1cc6a3599bfd6d) )
	ROM_LOAD( "cd212bd.p1", 0x0000, 0x020000, CRC(4a7605fc) SHA1(2a8a327d6dce8f7be1938d5a3854594adf5c092c) )
	ROM_LOAD( "cd212d.p1", 0x0000, 0x020000, CRC(50e7b381) SHA1(58141910fbbc2624da737a2ba273b79a6da695d3) )
	ROM_LOAD( "cd212dk.p1", 0x0000, 0x020000, CRC(00b1adaa) SHA1(ba089bf86fcc8817ae756ed5609aaf876d1ee5a5) )
	ROM_LOAD( "cd212dr.p1", 0x0000, 0x020000, CRC(b3be169b) SHA1(eb4699fdce371d94feec410c640bd49bfdccba98) )
	ROM_LOAD( "cd212dy.p1", 0x0000, 0x020000, CRC(87528d54) SHA1(974fa2c29af43c903add28dca0ea3b04f612d2f7) )
	ROM_LOAD( "cd212r.p1", 0x0000, 0x020000, CRC(d434ab90) SHA1(d42258bd965e8a028a418681a1307234c9b1c450) )
	ROM_LOAD( "cd212s.p1", 0x0000, 0x020000, CRC(f7d9d5e3) SHA1(1378e28c0a2c59a42a440502f20cc011625f43b5) )
	ROM_LOAD( "cd212y.p1", 0x0000, 0x020000, CRC(e0d8305f) SHA1(ddf1125eba0e470f6ae811fe050d4000300cfd0c) )
	ROM_LOAD( "cdom15", 0x0000, 0x020000, CRC(28f9ee8e) SHA1(e3484933dd0b8ddc2eeefc4dc95ce5379565e750) )
	ROM_LOAD( "cdom15r", 0x0000, 0x020000, CRC(28f9ee8e) SHA1(e3484933dd0b8ddc2eeefc4dc95ce5379565e750) )
	ROM_LOAD( "cdome10", 0x0000, 0x020000, CRC(945c9277) SHA1(6afee54b332152f6767781a040799d865999b292) )
	ROM_LOAD( "cdome8ac", 0x0000, 0x020000, CRC(0553bfe6) SHA1(77abfa556f04dca1be52fbed357807e6ada10458) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cd2snd.p1", 0x000000, 0x080000, CRC(65a2dc92) SHA1(2c55a858ab17325189bed1974daf708c380541de) )
	ROM_LOAD( "cd2snd.p2", 0x080000, 0x080000, CRC(b1bb4678) SHA1(8e8ab0a8d1b3e70dcb56d071193fdb5f34af7d14) )
ROM_END

ROM_START( m4crmaze )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "crms.p1", 0x0000, 0x020000, CRC(b289c54b) SHA1(eb74bb559e2be2737fc311d044b9ce87014616f3) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cma15g", 0x0000, 0x020000, CRC(f30b3ef2) SHA1(c8fb4d883d12a477a703d8cb0842994675aaf879) )
	ROM_LOAD( "cma15t", 0x0000, 0x020000, CRC(a4ed66a4) SHA1(0e98859c4d7dbccdea9396c3fea9f345b2f08db6) )
	ROM_LOAD( "cmaz510", 0x0000, 0x010000, CRC(0a1d39ac) SHA1(37888bbea427e115c29253deb85ed851ff6bdfd4) )
	ROM_LOAD( "cmaz510l", 0x0000, 0x010000, CRC(0a1d39ac) SHA1(37888bbea427e115c29253deb85ed851ff6bdfd4) )
	ROM_LOAD( "cmaz55", 0x0000, 0x010000, CRC(2c2540ce) SHA1(12163109e05fe8675bc2dbcad95f598bebec8ba3) )
	ROM_LOAD( "cmaz55v2", 0x0000, 0x010000, CRC(9a3515d6) SHA1(5edd2c67152d353a48ad2f28b685fae1e1e7fff7) )
	ROM_LOAD( "cmaz58t", 0x0000, 0x010000, CRC(81a2c48a) SHA1(3ea25a2863f1350054f41cb169282c592565dbcd) )
	ROM_LOAD( "cmaze8", 0x0000, 0x020000, CRC(f2f81306) SHA1(725bfbdc53cf66c08b440c2b8d45547aa426d9c7) )
	ROM_LOAD( "crmad.p1", 0x0000, 0x020000, CRC(ed30e66e) SHA1(25c09637f6efaf8e24f758405fb55d6cfc7f4782) )
	ROM_LOAD( "crmb.p1", 0x0000, 0x020000, CRC(6f29a37f) SHA1(598541e2dbf05b3f2a70279276407cd93734731e) )
	ROM_LOAD( "crmbd.p1", 0x0000, 0x020000, CRC(602a48ab) SHA1(3f1bf2b3294d15013e89d906865f065476202e54) )
	ROM_LOAD( "crmc.p1", 0x0000, 0x020000, CRC(58631e6d) SHA1(cffecd4c4ca46aa0ccfbaf7592d58da0428cf143) )
	ROM_LOAD( "crmd.p1", 0x0000, 0x020000, CRC(1232a809) SHA1(483b96b3b3ea50cbf5c3823c3ba20369b88bd459) )
	ROM_LOAD( "crmdk.p1", 0x0000, 0x020000, CRC(2aede0fd) SHA1(1731c901149c196d8f6a8bf3c2eec4f9a42126ad) )
	ROM_LOAD( "crmdy.p1", 0x0000, 0x020000, CRC(ad0ec003) SHA1(2d8a7467c3a79d60100f1290abe06410aaefaa49) )
	ROM_LOAD( "crmk.p1", 0x0000, 0x020000, CRC(25ee0b29) SHA1(addadf351a26e235a7fca573145a501aa6c0b53c) )
	ROM_LOAD( "crmy.p1", 0x0000, 0x020000, CRC(a20d2bd7) SHA1(b05a0e2ab2b90a86873976c26a8299cb703fd6eb) )
	ROM_LOAD( "crystalmaze15.bin", 0x0000, 0x020000, CRC(492440a4) SHA1(2d5fe812f1d815620f7e72333d44946b66f5c867) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cmaz5.10", 0x0000, 0x010000, CRC(13a64c64) SHA1(3a7c4173f99fdf1a4b5d5b627022b18eb66837ce) )
	ROM_LOAD( "cmaz5.5", 0x0000, 0x010000, CRC(1f110757) SHA1(a60bac78176dab70d68bfb2b6a44debf499c96e3) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cmazep1.bin", 0x000000, 0x080000, CRC(3d94a320) SHA1(a9b4e89ce36dbc2ef584b3adffffa00b7ae7e245) )
	ROM_LOAD( "crmsnd.p1", 0x000000, 0x080000, CRC(e05cdf96) SHA1(c85c7b31b775e3cc2d7f943eb02ff5ebae6c6080) )
	ROM_LOAD( "crmsnd.p2", 0x080000, 0x080000, CRC(11da0781) SHA1(cd63834bf5d5034c2473372bfcc4930c300333f7) )
ROM_END


ROM_START( m4denmen )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "den20.10", 0x0000, 0x010000, CRC(e002932d) SHA1(0a9b31c138a79695e1c1c29eee40c5a741275da6) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "denb.p1", 0x0000, 0x010000, CRC(b0164796) SHA1(61ff7e7ea2c27742177d851a4eb9a041d95b37d7) )
	ROM_LOAD( "denc.p1", 0x0000, 0x010000, CRC(549e17bc) SHA1(78271e11d4c8e742acce9087f194a1db8fc8c3eb) )
	ROM_LOAD( "dend.p1", 0x0000, 0x010000, CRC(176cd283) SHA1(f72c69b346f926a6e11b685ab9a6a2783b836450) )
	ROM_LOAD( "denk.p1", 0x0000, 0x010000, CRC(8983cbe0) SHA1(159dcbc3f5d24b6be03ae9c3c2af58993bebd38c) )
	ROM_LOAD( "dennis the menace 8 10m.bin", 0x0000, 0x010000, CRC(f5bd6c61) SHA1(ec443a284dae480c944f437426c28481a61c8ebb) )
	ROM_LOAD( "dens.p1", 0x0000, 0x010000, CRC(d3687138) SHA1(611985a9116ea14992b34a84ed31693f88d99797) )
	ROM_LOAD( "deny.p1", 0x0000, 0x010000, CRC(83ebd9f6) SHA1(f59e9d34295df8200f85a51d725437954acf9bdc) )
	ROM_LOAD( "dm5ad.p1", 0x0000, 0x010000, CRC(f01125cc) SHA1(faa80bfb107db127b2f9c4c7d23ec495775d2162) )
	ROM_LOAD( "dm5b.p1", 0x0000, 0x010000, CRC(2c6dae4c) SHA1(281e4ba31a60fb5600790f21095e697db80736b7) )
	ROM_LOAD( "dm5bd.p1", 0x0000, 0x010000, CRC(a65c534d) SHA1(e5c38a9a06e20878cb820e5a12545405d699ff9d) )
	ROM_LOAD( "dm5d.p1", 0x0000, 0x010000, CRC(0c6250d5) SHA1(56b316df56d6448137332044bfe1081401eef3e8) )
	ROM_LOAD( "dm5dk.p1", 0x0000, 0x010000, CRC(848412a1) SHA1(bb385e2abdc2651b4a7ea9d30108dfa8adab0aea) )
	ROM_LOAD( "dm5dy.p1", 0x0000, 0x010000, CRC(0c091457) SHA1(930b87211b8df5846fa857744aafae2f2985e578) )
	ROM_LOAD( "dm5k.p1", 0x0000, 0x010000, CRC(581572d6) SHA1(ac7303ea828846e770f8f1c7c818369d4b006495) )
	ROM_LOAD( "dm5s.hex", 0x0000, 0x010000, CRC(49672daa) SHA1(92e327b59b532e58b8c2a4e507f56c2ae069420c) )
	ROM_LOAD( "dm5s.p1", 0x0000, 0x010000, CRC(49672daa) SHA1(92e327b59b532e58b8c2a4e507f56c2ae069420c) )
	ROM_LOAD( "dm5y.p1", 0x0000, 0x010000, CRC(e6b9a800) SHA1(543ef65352a98676d66f6a5d3d7f568e10aac084) )
	ROM_LOAD( "dm8c.p1", 0x0000, 0x010000, CRC(f5bd6c61) SHA1(ec443a284dae480c944f437426c28481a61c8ebb) )
	ROM_LOAD( "dm8d.p1", 0x0000, 0x010000, CRC(23258932) SHA1(03b929bd86c429a7806f75639569534bfe7634a8) )
	ROM_LOAD( "dm8dy.p1", 0x0000, 0x010000, CRC(3c5ef7c8) SHA1(ac102525900f34c53082d37fb1bd14db9ce928fe) )
	ROM_LOAD( "dm8k.p1", 0x0000, 0x010000, CRC(9b3c3827) SHA1(2f584cfbbf38435377785dd654fe7b97c78e731a) )
	ROM_LOAD( "dm8s.p1", 0x0000, 0x010000, CRC(27484793) SHA1(872ad9bdbad793aa3bb4b8d227627f901a04d70e) )
	ROM_LOAD( "dm8y.p1", 0x0000, 0x010000, CRC(ebfcb926) SHA1(c6a623de9163e3f49ee7e5dbb8df867a90d0d0a9) )
	ROM_LOAD( "dmtad.p1", 0x0000, 0x010000, CRC(2edab31e) SHA1(c1cb258aba42e6ae33df731504d23162118054be) )
	ROM_LOAD( "dmtb.p1", 0x0000, 0x010000, CRC(c40fe8a4) SHA1(e182b0b1b975947da3b0a94afd17cdf166d7a8ac) )
	ROM_LOAD( "dmtbd.p1", 0x0000, 0x010000, CRC(d9140665) SHA1(cba8fc1c285c9192a6ea80b3f0c958781a818489) )
	ROM_LOAD( "dmtd.p1", 0x0000, 0x010000, CRC(9b38fa46) SHA1(ce6509349c82a651336753a3062c1cf2390d0b9a) )
	ROM_LOAD( "dmtdk.p1", 0x0000, 0x010000, CRC(b6211765) SHA1(3a2c5b1ef27113221ce7b61562f06589bcfa9072) )
	ROM_LOAD( "dmtdy.p1", 0x0000, 0x010000, CRC(66064a45) SHA1(3f64212b85320fba66afd40c0bb0cd58a5a616bf) )
	ROM_LOAD( "dmtk.p1", 0x0000, 0x010000, CRC(b64b6b3f) SHA1(f39b2143b811375564ec82030a7d34057f79b3f7) )
	ROM_LOAD( "dmts.p1", 0x0000, 0x010000, CRC(1a2776e3) SHA1(4d5029a5abafb3945d533ca5ca23b32c036fbb31) )
	ROM_LOAD( "dmty.p1", 0x0000, 0x010000, CRC(dbfa78a5) SHA1(edd9a1f286f3aa56a919e9e0c0013e9940d139ac) )
	ROM_LOAD( "dtm205", 0x0000, 0x010000, CRC(af76a460) SHA1(325021a92042c87e804bc17d6a7ccfda8bf865b8) )
	ROM_LOAD( "denm2010", 0x0000, 0x010000, CRC(dbed5e48) SHA1(f374f01aeefca7cc19fc46c93e2ca7a10606b183) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "densnd1.hex", 0x000000, 0x080000, CRC(468a8ec7) SHA1(ec450cd86fda09bc94caf913e9ee7900cfeaa0f2) )
	ROM_LOAD( "densnd2.hex", 0x080000, 0x080000, CRC(1c20a490) SHA1(62eddc469e4b93ea1f82070600fce628dc526f54) )

ROM_END

ROM_START( m4dbl9 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "d9s_6.bin", 0x0000, 0x010000, CRC(6029d46a) SHA1(0823f29f17562675a6f250429e46655c0b2e8f2c) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "du91.0", 0x0000, 0x010000, CRC(6207753d) SHA1(b19bcb60707b73f37e9bd8177d0b15847af0213f) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "du91.chr", 0x0000, 0x000048, CRC(9724122d) SHA1(a41687eec84cad453c1a2a89317078f48ca0895f) )
ROM_END

ROM_START( m4dbldmn )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cdd01.p1", 0x0000, 0x020000, CRC(e35dffde) SHA1(0bfc977f25f25785f20b510c44d2d3d79e23af8b) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cdd05d.p1", 0x0000, 0x020000, CRC(fc1c5e90) SHA1(c756d2ac725168af5396c8ef7550db9087a50937) )
	ROM_LOAD( "cdd05f.p1", 0x0000, 0x020000, CRC(81914bd4) SHA1(cf286810ad6732ca1d706e70f4c2958d28cc979c) )
	ROM_LOAD( "cdd05s.p1", 0x0000, 0x020000, CRC(fc14771f) SHA1(f418af9fed331560195a694f20ef2fea27ed04b0) )
	//ROM_LOAD( "cdd0.1", 0x0000, 0x00c85c, CRC(1e3f2822) SHA1(8cf62748b7a01aed575c35b1504dbc2589d68dd2) ) // cdd01.p1 compressed!

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cddsnd.p1", 0x000000, 0x080000, CRC(e1833e31) SHA1(1486e5afab347d6dee1543a55d1193b7db3c89d7) )
	ROM_LOAD( "cddsnd.p2", 0x080000, 0x080000, CRC(fd33ed2a) SHA1(f68ffadde40f88e7954d4a98bcd7ff023841b55b) )
ROM_END

ROM_START( m4dblup )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "du15.bin", 0x0000, 0x010000, CRC(76d4f30d) SHA1(91b131bec6d38d09ffafbde985d03ba4d8fcb307) )
ROM_END

ROM_START( m4drac )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "dra21.bin", 0x0000, 0x020000, CRC(23be387e) SHA1(08a78f4b8ddef46069d1c75113300b21e52338c1) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "dra24.bin", 0x0000, 0x020000, CRC(3db112ae) SHA1(b5303e2a65476931d4769327ca62afd0f6a9eda7) )
	ROM_LOAD( "dra27.bin", 0x0000, 0x020000, CRC(8a095175) SHA1(41006e298f1688499ce6820ec28196c7578684b9) )
	ROM_LOAD( "dra27.p1", 0x0000, 0x020000, CRC(8a095175) SHA1(41006e298f1688499ce6820ec28196c7578684b9) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "drasnd.p1", 0x000000, 0x080000, CRC(54c3821c) SHA1(1fcc62e2b127dd7f1d5d27a3afdf56dc27f122f8) )
	ROM_LOAD( "drasnd.p2", 0x080000, 0x080000, CRC(9096d2bc) SHA1(1b4c530b7b0fde869980d519255e2585c5461e13) )
	ROM_LOAD( "drasnd.p3", 0x100000, 0x080000, CRC(a07f412b) SHA1(cca8f5cfe620ece45ca40bf801f0643cd76547e9) )
	ROM_LOAD( "drasnd.p4", 0x180000, 0x080000, CRC(018ed789) SHA1(64202da2c542f5ef208faeb04945eb1a758d4746) )
ROM_END

ROM_START( m4dtyfre )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "duts.p1", 0x0000, 0x010000, CRC(8c7d6567) SHA1(8e82c4168d4d455c7cb95a895c04f7ad327894ec) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "df503ad.p1", 0x0000, 0x010000, CRC(348e375f) SHA1(f9a7e84afb33ec8fad14521eb2ea5d5cdfa48005) )
	ROM_LOAD( "df503b.p1", 0x0000, 0x010000, CRC(5eef10a2) SHA1(938e9a04fe54ac24dd93e9a1388c1dcf485ac212) )
	ROM_LOAD( "df503bd.p1", 0x0000, 0x010000, CRC(94840089) SHA1(a48668cdc1d7edae425cc80f2ce0f884f8619242) )
	ROM_LOAD( "df503d.p1", 0x0000, 0x010000, CRC(3eab581a) SHA1(e1f358081953feccf1f03d733f29e839d5f51fcb) )
	ROM_LOAD( "df503dk.p1", 0x0000, 0x010000, CRC(85ede229) SHA1(6799567df8078b69f897c0c5d8a315c6e3ef79b5) )
	ROM_LOAD( "df503dr.p1", 0x0000, 0x010000, CRC(42721aa6) SHA1(8a29a4433d641ea37bbe3bf99f9222e8261dd63f) )
	ROM_LOAD( "df503dy.p1", 0x0000, 0x010000, CRC(cfce461e) SHA1(5bbbe878e89b1d775048945e259b711ef60de9a1) )
	ROM_LOAD( "df503k.p1", 0x0000, 0x010000, CRC(bc51cc39) SHA1(0bb977c14e66ec48cd64b01a509d8f0cecdc7880) )
	ROM_LOAD( "df503r.p1", 0x0000, 0x010000, CRC(6b1940e0) SHA1(e8d3683d1ef65d2e7e035e9aab98ab9136f89464) )
	ROM_LOAD( "df503s.p1", 0x0000, 0x010000, CRC(d5e80ed5) SHA1(b2d601b2a0020f4adf80b1256d31c8cce432ecee) )
	ROM_LOAD( "df503y.p1", 0x0000, 0x010000, CRC(118642d4) SHA1(af2c86f0120f38652dc3d1141c5339a32bf73e11) )
	ROM_LOAD( "df8c.p1", 0x0000, 0x010000, CRC(07a82d24) SHA1(548576ce7c8d661777122e0d86d8273933beff11) )
	ROM_LOAD( "df8d.p1", 0x0000, 0x010000, CRC(df3a0ed7) SHA1(97569499f65e768a059fc86bdbcbde31e1977c23) )
	ROM_LOAD( "df8dy.p1", 0x0000, 0x010000, CRC(0f24e42d) SHA1(1049f50bc8e0a2f7b77d8e3cdc8883b6879e5cd9) )
	ROM_LOAD( "df8k.p1", 0x0000, 0x010000, CRC(056ac122) SHA1(9a993c0a7322323512a26b147963591212a226ab) )
	ROM_LOAD( "df8s.p1", 0x0000, 0x010000, CRC(00571ce4) SHA1(39f5ecec8ccdefb68a8b9d2ab1cd0be6acb0c1c7) )
	ROM_LOAD( "df8y.p1", 0x0000, 0x010000, CRC(cb902ef4) SHA1(efd7cb0a002aa54131725759cb73387f281f15a9) )
	ROM_LOAD( "dfre55", 0x0000, 0x010000, CRC(01e7d367) SHA1(638b709e4bb997998ccc7c4ea8adc33cabf2fe36) )
	ROM_LOAD( "dfree510l", 0x0000, 0x010000, CRC(7cf877a9) SHA1(54a87391832a641bf5f7104968b919dbb2bfa1eb) )
	ROM_LOAD( "dftad.p1", 0x0000, 0x010000, CRC(045cedc1) SHA1(0f833077dee2b942e17ce49b5f506d9754ed0bc1) )
	ROM_LOAD( "dftb.p1", 0x0000, 0x010000, CRC(93567c8b) SHA1(8dc7d662ae4a5dd58240e90144c0c9905afc04f1) )
	ROM_LOAD( "dftbd.p1", 0x0000, 0x010000, CRC(b5e5b19a) SHA1(8533865e8c63498e808fb9b1da86fe0ac2a7efdc) )
	ROM_LOAD( "dftd.p1", 0x0000, 0x010000, CRC(9ac1f31f) SHA1(541a761c8755d1d85cedbba306ff7330d284480f) )
	ROM_LOAD( "dftdk.p1", 0x0000, 0x010000, CRC(dbb4bf41) SHA1(c20b102a53f4d4ccbdb83433a80c77aa444a982d) )
	ROM_LOAD( "dftdy.p1", 0x0000, 0x010000, CRC(6b12a337) SHA1(57cfa667a2ae3bea36d82ef32429638dc36533ad) )
	ROM_LOAD( "dftk.p1", 0x0000, 0x010000, CRC(ad9bb027) SHA1(630e334fdffbdecc903f75b9447c2c7993cf2656) )
	ROM_LOAD( "dfts.p1", 0x0000, 0x010000, CRC(d6585e76) SHA1(91538ff218d8dd7a0d6747daaa9921d3e4b3ec33) )
	ROM_LOAD( "dfty.p1", 0x0000, 0x010000, CRC(0dead807) SHA1(a704ec65b1d6f91b4950181a792bb082c81fe668) )
	ROM_LOAD( "dutb.p1", 0x0000, 0x010000, CRC(479acab7) SHA1(645e876b2c59dd4c091b5f168dcfd2cfa7eda0a3) )
	ROM_LOAD( "dutc.p1", 0x0000, 0x010000, CRC(654858eb) SHA1(4e95d6f1b84360b747a04d34bfda4d8c8ee3ea3b) )
	ROM_LOAD( "duty-freeo6.p1", 0x0000, 0x010000, CRC(8c7d6567) SHA1(8e82c4168d4d455c7cb95a895c04f7ad327894ec) )
	ROM_LOAD( "duty2010", 0x0000, 0x010000, CRC(48617f20) SHA1(dd35eef2357af6f88be42bb81608696ed97522c5) )
	ROM_LOAD( "xd502ad.p1", 0x0000, 0x010000, CRC(62700345) SHA1(9825a9a6161e217ba4682902ac25528287d4ecf3) )
	ROM_LOAD( "xd502b.p1", 0x0000, 0x010000, CRC(40069386) SHA1(0d065c2b528b406468354be68bbafdcac05f779d) )
	ROM_LOAD( "xd502bd.p1", 0x0000, 0x010000, CRC(2cdc9833) SHA1(d3fa76c0a9a0113fbb7a83a47e3f7a72aeb942aa) )
	ROM_LOAD( "xd502c.p1", 0x0000, 0x010000, CRC(17124bb6) SHA1(4ab22cffe11e84ff08bf0f026b0ca6d9a0d32bed) )
	ROM_LOAD( "xd502d.p1", 0x0000, 0x010000, CRC(7b44a085) SHA1(d7e4c25e0d42a32f72afdb17b66425e1127373fc) )
	ROM_LOAD( "xd502dk.p1", 0x0000, 0x010000, CRC(790aac05) SHA1(db697b9a87d0266fabd23e1b085234e36c816170) )
	ROM_LOAD( "xd502dr.p1", 0x0000, 0x010000, CRC(77a14f87) SHA1(651b58c0a9ec13441c9bf8d7bf0d7c736337f171) )
	ROM_LOAD( "xd502dy.p1", 0x0000, 0x010000, CRC(eaca6769) SHA1(1d3d1264d849043f0adcf9a32520e5f80ae17b5f) )
	ROM_LOAD( "xd502k.p1", 0x0000, 0x010000, CRC(c9a3b787) SHA1(c7166c9e809a37037dfdc616df5fbd6b6ff8b2f8) )
	ROM_LOAD( "xd502r.p1", 0x0000, 0x010000, CRC(4ddbd944) SHA1(c3df807ead3a50c7be73b084f65771e4b9d1f2d0) )
	ROM_LOAD( "xd502s.p1", 0x0000, 0x010000, CRC(223117c7) SHA1(9c017c4165db7076c76c081404d27742fd1f62e7) )
	ROM_LOAD( "xd502y.p1", 0x0000, 0x010000, CRC(d0b0f1aa) SHA1(39560550083952cae568d4d634c04bf48b7baca6) )
	ROM_LOAD( "xd5s.p1", 0x0000, 0x010000, CRC(235ba9d1) SHA1(3a58c986f63c9ee75e91c59455b0a02582b4301b) )
	ROM_LOAD( "xft01ad.p1", 0x0000, 0x010000, CRC(7299da07) SHA1(eb1371ce52e24fbfcac8f45166ca56d8aee9d403) )
	ROM_LOAD( "xft01b.p1", 0x0000, 0x010000, CRC(c24904c4) SHA1(1c1b94b499f7a50e04b1287ce95633a8b0a5c0ea) )
	ROM_LOAD( "xft01bd.p1", 0x0000, 0x010000, CRC(e67a0e47) SHA1(8115a5ab8b508ff30b28fa8f5d33f598385ee115) )
	ROM_LOAD( "xft01c.p1", 0x0000, 0x010000, CRC(ee915038) SHA1(a0239268eae757e8e7ee16d9acb5dc28e7820b4e) )
	ROM_LOAD( "xft01d.p1", 0x0000, 0x010000, CRC(88391d1c) SHA1(f1b1034b962a03efd7d2cbe6ac0cc7328871a180) )
	ROM_LOAD( "xft01dk.p1", 0x0000, 0x010000, CRC(dfef8231) SHA1(7610a7bcdb91a39cf86ac926818d02f4d751f099) )
	ROM_LOAD( "xft01dr.p1", 0x0000, 0x010000, CRC(213f7fe5) SHA1(7e9cad6df7f4a58a0b98dbac552bf545a53ebfcd) )
	ROM_LOAD( "xft01dy.p1", 0x0000, 0x010000, CRC(25fc8e71) SHA1(54c4c8c2118b4758dedb15f0a11f918f2ee0fb7d) )
	ROM_LOAD( "xft01k.p1", 0x0000, 0x010000, CRC(fbdc88b2) SHA1(231b6b8ba92a794ec363c1b853921e28e6b34fec) )
	ROM_LOAD( "xft01r.p1", 0x0000, 0x010000, CRC(dd8b05e6) SHA1(64a5aaaa6e7fb162c23ad0e36d39923e986b0fb4) )
	ROM_LOAD( "xft01s.p1", 0x0000, 0x010000, CRC(fc107ba0) SHA1(661f1ab0d0192f77c355d5570885940d71174592) )
	ROM_LOAD( "xft01y.p1", 0x0000, 0x010000, CRC(39e49e72) SHA1(459e0d81b6d0d2aa44aa6a7a00cbdec4d9536df0) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "4df5.10", 0x0000, 0x010000, CRC(01c9e06f) SHA1(6d9d4a43f621c4a80259040875a1fe851459b662) )
	ROM_LOAD( "4df5.4", 0x0000, 0x010000, CRC(60e21664) SHA1(2a343f16ece19396ad41eeac8c94a23d8e648d4f) )
	ROM_LOAD( "4df5.8t", 0x0000, 0x010000, CRC(e8abec56) SHA1(84f6abc5e8b46c55052d308266000085374b12af) )
	ROM_LOAD( "bwb duty free 5.8.bin", 0x0000, 0x010000, CRC(c67e7315) SHA1(a70183b0937c138c96fd1a0cd5bacff1acd0cbdb) )
	ROM_LOAD( "df5 (2).4", 0x0000, 0x010000, CRC(50f8566c) SHA1(364d33de4b34d0052ffc98536468c0a13f847a2a) )
	ROM_LOAD( "df5 (2).8t", 0x0000, 0x010000, CRC(eb4cf0ae) SHA1(45c4e143a3e358c4bdc0c10e38039cba48a9e6dc) )
	ROM_LOAD( "df5.10", 0x0000, 0x010000, CRC(96acf53f) SHA1(1297a9162dea474079d0ea63b2b1b8e7f649230a) )
	ROM_LOAD( "df5.4", 0x0000, 0x010000, CRC(14de7ecb) SHA1(f7445b33b2febbf93fd0398ab310ac104e79443c) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "df503s.chr", 0x0000, 0x000048, CRC(46c28f35) SHA1(e229b211180f9f7b30cd0bb9de162971d16b2d33) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "dutsnd.p1", 0x000000, 0x080000, CRC(a5829cec) SHA1(eb65c86125350a7f384f9033f6a217284b6ff3d1) )
	ROM_LOAD( "dutsnd.p2", 0x080000, 0x080000, CRC(1e5d8407) SHA1(64ee6eba3fb7700a06b89a1e0489a0cd54bb89fd) )
ROM_END


ROM_START( m4eighth )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "wonder.hex", 0x0000, 0x010000, CRC(6fcaab11) SHA1(a462d4c50000e62af4c52980338cee073e4175a9) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ewc02__0.3", 0x0000, 0x010000, CRC(3ea4c626) SHA1(c10b3ba1f806f9d685b9de25fab3a15cbb8e94c3) )
	ROM_LOAD( "ewc05__0.3", 0x0000, 0x010000, CRC(691d2694) SHA1(08deeb25a23a65059be877d11c570db2db66564c) )
	ROM_LOAD( "ewc10__0.3", 0x0000, 0x010000, CRC(7a3b03d5) SHA1(13fb31ffb17edb0502ec47488d7f6b169834b0e4) )
	ROM_LOAD( "ewc10d_0.3", 0x0000, 0x010000, CRC(9131f8d4) SHA1(404ab0e359b81a26fcddaa9773ed3234dae0a754) )
	ROM_LOAD( "ewc20__0.3", 0x0000, 0x010000, CRC(1b6babd3) SHA1(7b919f48a1a0a1a5ecc930a59fd27b2f9fe7509b) )
	ROM_LOAD( "ewc20c_0.3", 0x0000, 0x010000, CRC(9f6f8836) SHA1(e519f3c42ef7f157c88e68a706a96e42a8cfba4d) )
	ROM_LOAD( "ewc20d_0.3", 0x0000, 0x010000, CRC(dd26057e) SHA1(dfada02b18f748620966b351b2fe5e03af2b9c7e) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "wonder.chr", 0x0000, 0x000048, CRC(df5dd758) SHA1(b8ae33480a13d621cf104da770e419b6e485bf33) )
ROM_END

ROM_START( m4elite )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "e6ls.p1", 0x0000, 0x010000, CRC(ef4c0d3a) SHA1(1d9433689c457f19d31bd68df4728a87120e474a) )
ROM_END

ROM_START( m4eaw )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "er4s.p1", 0x0000, 0x010000, CRC(163fc987) SHA1(8e1768ed2fbddbd5e00652ff40614de3978c9567) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cet03ad.p1", 0x0000, 0x010000, CRC(33afe7a5) SHA1(5d3bdb74c6babd49e88915282ad81c184bd7aa68) )
	ROM_LOAD( "cet03b.p1", 0x0000, 0x010000, CRC(7674e2a5) SHA1(188e683eac91f64fe563b0f09f2b934e709c47fb) )
	ROM_LOAD( "cet03bd.p1", 0x0000, 0x010000, CRC(406843a2) SHA1(7d4bf6cd3c5be0f6df687b0ba97b3b88fd377170) )
	ROM_LOAD( "cet03d.p1", 0x0000, 0x010000, CRC(2c03d5b6) SHA1(e79fd15b6a05168eb08dcd2b5f7e00d015618a22) )
	ROM_LOAD( "cet03dk.p1", 0x0000, 0x010000, CRC(7a81b524) SHA1(71856b90379af946fbc9263f596a16e1701f3564) )
	ROM_LOAD( "cet03dr.p1", 0x0000, 0x010000, CRC(63a5622a) SHA1(1e3cf5487623b850598d21c0bb5ef8a0b9dffd4f) )
	ROM_LOAD( "cet03dy.p1", 0x0000, 0x010000, CRC(fece4ac4) SHA1(badf4f94d565958fc9f42a443f53ec9624925ee1) )
	ROM_LOAD( "cet03k.p1", 0x0000, 0x010000, CRC(f6531a43) SHA1(75ec5c8fc0012fee144daab7761f3717c17fa22d) )
	ROM_LOAD( "cet03r.p1", 0x0000, 0x010000, CRC(fec4a6c0) SHA1(89fac7e4df77f526d0e357f1874b73be932548ce) )
	ROM_LOAD( "cet03s.p1", 0x0000, 0x010000, CRC(bec3ea51) SHA1(740a73da105d8329dc9ceaa5e8c25b305124e2dd) )
	ROM_LOAD( "cet03y.p1", 0x0000, 0x010000, CRC(63af8e2e) SHA1(97b9dd02bf8a72ca0be7c1a9cb753fbd55644497) )
	ROM_LOAD( "ceu02ad.p1", 0x0000, 0x010000, CRC(5805182c) SHA1(c15ef2e05061fd89944b039f007d92bc4bdf66d5) )
	ROM_LOAD( "ceu02b.p1", 0x0000, 0x010000, CRC(cbf62a02) SHA1(20fb16ac4602d4e386e5dc01e1b7e83c459f614d) )
	ROM_LOAD( "ceu02bd.p1", 0x0000, 0x010000, CRC(6e197566) SHA1(16f44ca77bc02c7eb186c3684b4e837da0d73553) )
	ROM_LOAD( "ceu02d.p1", 0x0000, 0x010000, CRC(470cab31) SHA1(f42045f25022cc5e4b07a687f55f7698435b550e) )
	ROM_LOAD( "ceu02dk.p1", 0x0000, 0x010000, CRC(bea24ff7) SHA1(1bf8464136732ee8433c73747e854be3c991a2fe) )
	ROM_LOAD( "ceu02dr.p1", 0x0000, 0x010000, CRC(3e2e4183) SHA1(e52bca2913509f26af9ca4a93ab2a2bbf74d1ac9) )
	ROM_LOAD( "ceu02dy.p1", 0x0000, 0x010000, CRC(a345696d) SHA1(a189eb6a6a6a83fe0d490f4a7c8e9c4c52aa91f7) )
	ROM_LOAD( "ceu02k.p1", 0x0000, 0x010000, CRC(0e0a1ba9) SHA1(e1ee2595a3fd4fe874f50dc027f6c931636aadcc) )
	ROM_LOAD( "ceu02r.p1", 0x0000, 0x010000, CRC(1a882a6a) SHA1(c966be957e7a78c33a28afd79ba60c69a6de42b8) )
	ROM_LOAD( "ceu02s.p1", 0x0000, 0x010000, CRC(d52099e6) SHA1(10f1acb948fa7c4b547f801ddb5e15111992ca91) )
	ROM_LOAD( "ceu02y.p1", 0x0000, 0x010000, CRC(87e30284) SHA1(4c598a33b73cfe6338c0f51408f2a6c1abfa978b) )
	ROM_LOAD( "enn01ad.p1", 0x0000, 0x010000, CRC(913ba1d6) SHA1(1167ccce2f0b528ec8eba140b1f9c8358fa19f54) )
	ROM_LOAD( "enn01b.p1", 0x0000, 0x010000, CRC(76cf750c) SHA1(7f3ede643c5b92d9e313c4450a0d4ef3bd9eefd3) )
	ROM_LOAD( "enn01bd.p1", 0x0000, 0x010000, CRC(c6c29211) SHA1(a49759c4c00633405a338eeb89fcb00f7503990c) )
	ROM_LOAD( "enn01c.p1", 0x0000, 0x010000, CRC(1ea8f766) SHA1(3f08da014727b50e0375b8470a37c75042b089c6) )
	ROM_LOAD( "enn01d.p1", 0x0000, 0x010000, CRC(3691905e) SHA1(131b3384c2399b214fb70670c9945be4afdb470e) )
	ROM_LOAD( "enn01dk.p1", 0x0000, 0x010000, CRC(1abb5196) SHA1(952451f637d890a51a2567b5b02826f7647e5deb) )
	ROM_LOAD( "enn01dr.p1", 0x0000, 0x010000, CRC(e50fb399) SHA1(5d4d5a933efe7e122e4d0cecab9b7e6f01398a8f) )
	ROM_LOAD( "enn01dy.p1", 0x0000, 0x010000, CRC(be3e5901) SHA1(ea3f366724135682da7cddad3c82e5f4c434f4a9) )
	ROM_LOAD( "enn01k.p1", 0x0000, 0x010000, CRC(273d7b10) SHA1(5577355c918407e548266a16b225e8a4f58c921c) )
	ROM_LOAD( "enn01r.p1", 0x0000, 0x010000, CRC(aee3f31e) SHA1(72676bc6b3bc287bf3bd3e7719848b40aa1b3627) )
	ROM_LOAD( "enn01s.p1", 0x0000, 0x010000, CRC(d0ba447d) SHA1(744d5448c5318287e58994b684e116ac1a236f05) )
	ROM_LOAD( "enn01y.p1", 0x0000, 0x010000, CRC(91a73867) SHA1(5197fcd5bf3dc036095b8291d7b23776995d84d1) )
	ROM_LOAD( "eon01ad.p1", 0x0000, 0x010000, CRC(998b0e8d) SHA1(f2d0c43073d76d662c3a997b1fd081016e4c7a7d) )
	ROM_LOAD( "eon01b.p1", 0x0000, 0x010000, CRC(66f281db) SHA1(b9bd37c53ab7c8838ec87062c8b9da39779b9fa9) )
	ROM_LOAD( "eon01bd.p1", 0x0000, 0x010000, CRC(66a378ca) SHA1(6639f36df67af8bdd381ad3e16e0adc78a4552f4) )
	ROM_LOAD( "eon01c.p1", 0x0000, 0x010000, CRC(d02e2c30) SHA1(34f1be5f49d50f468bffc425fa2e9d0f8afcf70b) )
	ROM_LOAD( "eon01d.p1", 0x0000, 0x010000, CRC(a2752f68) SHA1(3a47ced5259c6f690b03c3a884f1d25bd68e0d3f) )
	ROM_LOAD( "eon01dk.p1", 0x0000, 0x010000, CRC(efd60656) SHA1(9383a7b183266f75edd3ae519e8dfff858f015c4) )
	ROM_LOAD( "eon01dr.p1", 0x0000, 0x010000, CRC(1062e459) SHA1(de334cdecde0dd414e016e11a54720dee903393c) )
	ROM_LOAD( "eon01dy.p1", 0x0000, 0x010000, CRC(d5a39761) SHA1(9b69f9e45d87f53196e5d4fd595300beb573ff49) )
	ROM_LOAD( "eon01k.p1", 0x0000, 0x010000, CRC(1d34dea7) SHA1(546db8247d0c78501fe4ec818d614e8f451b0076) )
	ROM_LOAD( "eon01r.p1", 0x0000, 0x010000, CRC(7c70a508) SHA1(2c5835f36ef4c215ff9f6f6cc350f0916b397b7b) )
	ROM_LOAD( "eon01s.p1", 0x0000, 0x010000, CRC(e2e9ce10) SHA1(41a08b17285d6591b4a5cb6b1b6cc40ee7d35f01) )
	ROM_LOAD( "eon01y.p1", 0x0000, 0x010000, CRC(ddc4f7d1) SHA1(bbc21ba153541df1507e01d4a25a1a669c8eab62) )
	ROM_LOAD( "er2ad.p1", 0x0000, 0x010000, CRC(4e5fcc8b) SHA1(8176ca01ad49f39e1337a085cf3a1fd33803c517) )
	ROM_LOAD( "er2b.p1", 0x0000, 0x010000, CRC(999c6510) SHA1(bc70b88183df84ea0e18e1017ab9d74545ce7588) )
	ROM_LOAD( "er2bd.p1", 0x0000, 0x010000, CRC(3f50573a) SHA1(46527b08d751372df09d61fd67054600b6e933f3) )
	ROM_LOAD( "er2d.p1", 0x0000, 0x010000, CRC(6c625759) SHA1(65de484632317b7bd1372f20e7cbdedc85a90ea4) )
	ROM_LOAD( "er2dk.p1", 0x0000, 0x010000, CRC(e1e1ab0b) SHA1(353863e2ef1e778b7fce035ae725053fb95c300e) )
	ROM_LOAD( "er2dr.p1", 0x0000, 0x010000, CRC(0d2e1d3f) SHA1(f75f6cf9e0ce6ccf36df83e18f03fc1485242c88) )
	ROM_LOAD( "er2dy.p1", 0x0000, 0x010000, CRC(f20c4b31) SHA1(744ce6065b3bea3a0c128a4848282cbca2bc8056) )
	ROM_LOAD( "er2k.p1", 0x0000, 0x010000, CRC(2c3661bb) SHA1(5f5a6b47dacdb2184d3ac9646da616283743fcbf) )
	ROM_LOAD( "er2r.p1", 0x0000, 0x010000, CRC(cb636e43) SHA1(44df3adc1d5af4c1930596f34f41884e7731be62) )
	ROM_LOAD( "er2s.p1", 0x0000, 0x010000, CRC(bfee8157) SHA1(3ce5a2ec16f06c753a054a9f645efbcd26f411ab) )
	ROM_LOAD( "er2y.p1", 0x0000, 0x010000, CRC(91369b00) SHA1(7427fcf9e350bc9a3883577de5ee4a4ab5ff63b0) )
	ROM_LOAD( "er4ad.p1", 0x0000, 0x010000, CRC(93fff89d) SHA1(3f90168efa5ecaf7707ef357616638a9d5ab746f) )
	ROM_LOAD( "er4b.p1", 0x0000, 0x010000, CRC(cb39fda7) SHA1(4a31d2ff53942a658992a5e13c2b617da5fb03ce) )
	ROM_LOAD( "er4bd.p1", 0x0000, 0x010000, CRC(a5e395d7) SHA1(3f134a2ce3788ac84a6de096306c651e6b2d6a4a) )
	ROM_LOAD( "er4d.p1", 0x0000, 0x010000, CRC(33612923) SHA1(1129ced207aaf46045f20a1ef1a37af8ec537bb0) )
	ROM_LOAD( "er4dk.p1", 0x0000, 0x010000, CRC(df41d570) SHA1(4a2db04ee51bb811ac3aee5b2c3c1f1a2201f7ec) )
	ROM_LOAD( "er4dy.p1", 0x0000, 0x010000, CRC(7df882e6) SHA1(1246220a5ac8a4454a7f3a359a5a00319395095d) )
	ROM_LOAD( "er4k.p1", 0x0000, 0x010000, CRC(9803cc0d) SHA1(1516c3836919a7a2cc32711a9bf2d3bf3d6b82c0) )
	ROM_LOAD( "er4y.p1", 0x0000, 0x010000, CRC(d8dece2d) SHA1(8482092434e1e94e6648e402c8b518c2f0fcc28e) )
	ROM_LOAD( "er8ad.p1", 0x0000, 0x010000, CRC(ba059e06) SHA1(f6bb9092c9d18bccde111f8e20e79b8b4e6d8593) )
	ROM_LOAD( "er8b.p1", 0x0000, 0x010000, CRC(27c7f954) SHA1(93305d1d4a5781de56f1e54801e25b29b6713ef0) )
	ROM_LOAD( "er8c.p1", 0x0000, 0x010000, CRC(cee94fb3) SHA1(01ec098016b6946c3fbf96b2071076316bbd5795) )
	ROM_LOAD( "er8dk.p1", 0x0000, 0x010000, CRC(789c5e1d) SHA1(5f5b686a770f4ab0cfa8e8ae21b3805ef6102516) )
	ROM_LOAD( "er8dy.p1", 0x0000, 0x010000, CRC(4adf568b) SHA1(dd21b547211566ad5cb018a0205d887b7f860bc9) )
	ROM_LOAD( "er8k.p1", 0x0000, 0x010000, CRC(c76140e4) SHA1(6c097fdd018eb594a84ceb7712a45201490ca370) )
	ROM_LOAD( "er8s.p1", 0x0000, 0x010000, CRC(5d36bbc6) SHA1(4d0cd8e939f22d919671dc97c3d97bf6191e738f) )
	ROM_LOAD( "er8y.p1", 0x0000, 0x010000, CRC(8a1aa409) SHA1(a7ae62e1038e52a111de3004e2160838e0d102d0) )
	ROM_LOAD( "ertad.p1", 0x0000, 0x010000, CRC(75798f2d) SHA1(68939c187d841aa046a4f7dd8f39e8387969460c) )
	ROM_LOAD( "ertb.p1", 0x0000, 0x010000, CRC(c6407839) SHA1(79d73d79b389682586fdf7c9c25d8e2ea5943bb6) )
	ROM_LOAD( "ertbd.p1", 0x0000, 0x010000, CRC(4365e267) SHA1(b1853c3ddb707cb114e6bb2d780b142b80f099b6) )
	ROM_LOAD( "ertd.p1", 0x0000, 0x010000, CRC(2fabc730) SHA1(8a43afd6048006e906892d35bb0cfaa127fc0415) )
	ROM_LOAD( "ertdk.p1", 0x0000, 0x010000, CRC(21264f37) SHA1(9819cf120e81525f18096152a555859a4f48f8ad) )
	ROM_LOAD( "ertdr.p1", 0x0000, 0x010000, CRC(1b644f23) SHA1(94c5a307126cada90eeb45439aaab82a30228ffa) )
	ROM_LOAD( "ertdy.p1", 0x0000, 0x010000, CRC(5a7c77fa) SHA1(37c212db131b682fd8d293a8cf8efad2e80a8a18) )
	ROM_LOAD( "ertk.p1", 0x0000, 0x010000, CRC(19959bd3) SHA1(617f7079b39b0ef41ebb0b5f89053d723a28824d) )
	ROM_LOAD( "ertr.p1", 0x0000, 0x010000, CRC(3264f04a) SHA1(88d1f6857f3a0acd89db1563fd5f24582b578765) )
	ROM_LOAD( "erts.p1", 0x0000, 0x010000, CRC(185b47bb) SHA1(377cb42878572a3e94dd6be6fb106ecacb3c5059) )
	ROM_LOAD( "erty.p1", 0x0000, 0x010000, CRC(38adc77e) SHA1(7a925e2aa946fdcf38df454ec733da1ce9bdc495) )
	ROM_LOAD( "eun01ad.p1", 0x0000, 0x010000, CRC(0148eb57) SHA1(7ebf73402ffe68cfb045a906ed039407bd173b88) )
	ROM_LOAD( "eun01b.p1", 0x0000, 0x010000, CRC(ad152cda) SHA1(ca5c72a54e14f8b44fddfbc5c38c4e149432f593) )
	ROM_LOAD( "eun01bd.p1", 0x0000, 0x010000, CRC(6b0abd7c) SHA1(a6f74096bfffa082a441c094b5acadd5929ac36a) )
	ROM_LOAD( "eun01c.p1", 0x0000, 0x010000, CRC(a65fcf8b) SHA1(13a5bc1f2918a4f3590a1cdc34b439a874934ee7) )
	ROM_LOAD( "eun01d.p1", 0x0000, 0x010000, CRC(400af364) SHA1(ca67e98624d50717763e9965c45f0beecb07d2f9) )
	ROM_LOAD( "eun01dk.p1", 0x0000, 0x010000, CRC(c11914fa) SHA1(884fee07227f46dde056eb8e082bb821eeab99cb) )
	ROM_LOAD( "eun01dr.p1", 0x0000, 0x010000, CRC(0eb075d4) SHA1(3977f03f6aac765c556618616919a5e10660b35d) )
	ROM_LOAD( "eun01dy.p1", 0x0000, 0x010000, CRC(93db5d3a) SHA1(ddd209b22ed396d3329b9522649db6dda64958b7) )
	ROM_LOAD( "eun01k.p1", 0x0000, 0x010000, CRC(9fca43fd) SHA1(f7626f122dedb217002888971100d8a34910b48d) )
	ROM_LOAD( "eun01r.p1", 0x0000, 0x010000, CRC(15b8eb9e) SHA1(e4babaf526e6dd45bb4b7f7441a08cfbec12c661) )
	ROM_LOAD( "eun01s.p1", 0x0000, 0x010000, CRC(d0b49fc6) SHA1(4062d9763010d42666660e383e52818d572b61b9) )
	ROM_LOAD( "eun01y.p1", 0x0000, 0x010000, CRC(88d3c370) SHA1(6c3839a9c89ae67f80ab932ec70ebaf1240de9bb) )
	ROM_LOAD( "every1.hex", 0x0000, 0x010000, CRC(406843a2) SHA1(7d4bf6cd3c5be0f6df687b0ba97b3b88fd377170) )
	ROM_LOAD( "everyones a winner v2-5p (27256)", 0x0000, 0x008000, CRC(eb8f2fc5) SHA1(0d3614bd5ff561d17bef0d1e620f2f812b8fed5b) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "m683.chr", 0x0000, 0x000048, CRC(cbe68b44) SHA1(60efc69eba86531f51230dee17efdbbf8917f907) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "er4snd.p1", 0x000000, 0x080000, CRC(32fd0836) SHA1(ea68252b690fe1d6070209cbcfb65fe20926c6ce) )
	ROM_LOAD( "er4snd.p2", 0x080000, 0x080000, CRC(1df9c24f) SHA1(f0d31b1bec6f3a9791f7fabe57b45687df900efa) )
	//ROM_LOAD( "evsnd1.hex", 0x0000, 0x080000, CRC(32fd0836) SHA1(ea68252b690fe1d6070209cbcfb65fe20926c6ce) )
	//ROM_LOAD( "evsnd2.hex", 0x0000, 0x080000, CRC(1df9c24f) SHA1(f0d31b1bec6f3a9791f7fabe57b45687df900efa) )
ROM_END

ROM_START( m4exprs )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dxp20.bin", 0x0000, 0x010000, CRC(09e68942) SHA1(2253ab76286b7c7af34ff99cc6d8e60b26edcacb) )
ROM_END

ROM_START( m4exgam )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "czep30.bin", 0x0000, 0x010000, CRC(4614e6f6) SHA1(5602a68e9b47394cb31bbcd49a9920e19af6242f) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "ceg.chr", 0x0000, 0x000048, CRC(f694224e) SHA1(936ab5e349fa59accbb37959cce9519fd97f3978) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sczep.bin", 0x0000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END

ROM_START( m4fastfw )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fastf206", 0x0000, 0x010000, CRC(a830b121) SHA1(0bf813ee75bd8e109e6688b91bd0983d341a6695) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ffo05__1.0", 0x0000, 0x010000, CRC(8b683969) SHA1(7469b551e4d6f65550d54ee39b2bac07cf3dbd4b) )
	ROM_LOAD( "ffo10__1.0", 0x0000, 0x010000, CRC(294288fd) SHA1(87d25f6333b6862fcc57a550b5cc7c0bc64e72cd) )
	ROM_LOAD( "ffo10d_1.0", 0x0000, 0x010000, CRC(8d96f3d4) SHA1(2070a335cfa3f9de1bd9e9094d91cce81b91347d) )
	ROM_LOAD( "ffo20__1.0", 0x0000, 0x010000, CRC(9528291e) SHA1(61c0eb8ce955f708e8a68a28f253706267e28254) )
	ROM_LOAD( "ffo20d_1.0", 0x0000, 0x010000, CRC(5bae35fe) SHA1(7e4d61ed97ddd170bd1424f34d0327093668da3f) )
	ROM_LOAD( "ffo20dy1.0", 0x0000, 0x010000, CRC(37167d46) SHA1(94b87697615f81b746ce3bcc64fc893f865e00dc) )
ROM_END

ROM_START( m4class )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dfc20.bin", 0x0000, 0x010000, CRC(1fa0c771) SHA1(b374d2cdc1bfc35a2e6fe35b9d21f2784b8c52e8) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "dfc20dsl.bin", 0x0000, 0x010000, CRC(dc1c8b87) SHA1(52235f9d393c574fdd26aa2ec60e6db70538fb9d) )
ROM_END

ROM_START( m4flash )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fc10.bin", 0x0000, 0x010000, CRC(497862b8) SHA1(3409ff455e77ed85baf540a2ebeb979ab1b6e1e7) )
ROM_END

ROM_START( m4fortcb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cfod.p1", 0x0000, 0x010000, CRC(9d0e2b63) SHA1(cce871d2bbe486793de5de9fadfbddf67c382e5c) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cfof.p1", 0x0000, 0x010000, CRC(010b3c1f) SHA1(b44c22c21d22603b277138eabf803e6d46ad4aae) )
	ROM_LOAD( "cfos.p1", 0x0000, 0x010000, CRC(f3b47df4) SHA1(3ad674864ba3a24283af14caaf2c999d4fde11fc) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cfosnd.p1", 0x000000, 0x080000, CRC(74bbf913) SHA1(52ddc89ab34b11ede2c0e9b9b27e119b0c1eb2d9) )
	ROM_LOAD( "cfosnd.p2", 0x080000, 0x080000, CRC(1b2bb79a) SHA1(5f19ea000f34bb404ed6c8ea5ec7b809ccb1ae36) )
ROM_END


ROM_START( m4frtlt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pfr03i.p1", 0x0000, 0x010000, CRC(bf13d4b5) SHA1(4de2b5c55a0022c97804233bc5c6b4fc8ee05c24) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pfr03o.p1", 0x0000, 0x010000, CRC(89a918e2) SHA1(afc2f80ea7539b68dc6bfd040e5717b607d22284) )
	ROM_LOAD( "pfr03s.p1", 0x0000, 0x010000, CRC(0ea80adb) SHA1(948a23fe8ccf6f423957a478a57bb875cc7b2cc2) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "pfrsnd.p1", 0x0000, 0x080000, CRC(71d1af20) SHA1(d87d61c561acbe9cb3dec18d8decf5e970efa272) )
ROM_END

ROM_START( m4frtfl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "frtfull", 0x0000, 0x010000, CRC(4f5389e2) SHA1(bb6d43d428c1e8db07fe58d1b83c05ce5fcdcc7d) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "fruitfull.hex", 0x0000, 0x010000, CRC(c264d497) SHA1(93843efbf1b4207a4722f49dd5dddf2c52bb1b8f) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "fruitfull.chr", 0x0000, 0x000048, CRC(b6057802) SHA1(ef6dbb45f5594e759f6d1363f36ba05097100be4) )
ROM_END

ROM_START( m4frtflc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ffcs.p1", 0x0000, 0x010000, CRC(db917142) SHA1(0f32f0c1ed6733b4557fd19f24f2b1dda26ccc44) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "fruitfull.chr", 0x0000, 0x000048, CRC(b6057802) SHA1(ef6dbb45f5594e759f6d1363f36ba05097100be4) )
ROM_END

ROM_START( m4frtgm )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fruit.bin", 0x0000, 0x010000, CRC(dbe44316) SHA1(15cd49dd2e6166f7a7668663f7fea802d6cbb12f) )

	ROM_REGION( 0x800000, "msm6376", 0 ) /* this isn't OKI, or is corrupt (bad size) */
	ROM_LOAD( "fruitsnd.bin", 0x0000, 0x010000, CRC(86547dc7) SHA1(4bf64f22e84c0ee82d961b0ba64932b8bf6a521f) ) // matches 'Replay' on SC1 hardware, probably just belongs there.. or this is eurocoin with different sound hw here?
ROM_END

ROM_START( m4frtlnk )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "flcs.p1", 0x0000, 0x010000, CRC(f66a6810) SHA1(e91cdba6b52df6e633d6ce5036a82f5c2dbb1d19) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "flinkv6", 0x0000, 0x010000, CRC(ca9c1034) SHA1(f7e02372a1c7cd41097db63d4a921387d22e02b4) )
ROM_END

ROM_START( m4frtprs )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "f4p11lp1.bin", 0x8000, 0x008000, CRC(817bb364) SHA1(65a9ca6fd5689e547d8286c482d8cb9fe1a5fb61) )
	ROM_LOAD( "f4p11lp2.bin", 0x6000, 0x002000, CRC(ba10c3bc) SHA1(8f5f294b13a0d71cfe9c0e400ac72ce6aed6b763) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "fruitp.hex", 0x0000, 0x00a000, CRC(28ab9a79) SHA1(83be8b7ee4de93c426d93daa64718407709b0b1a) )
ROM_END

ROM_START( m4gambal )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "gbbx.p1",	0x0000, 0x10000,  CRC(0b5adcd0) SHA1(1a198bd4a1e7d6bf4cf025c43d35aaef351415fc))

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "gabcx.p1", 0x0000, 0x010000, CRC(52c35266) SHA1(bda49005de88094fbc84621f63b33f0e0a9c0bd3) )
	ROM_LOAD( "gabx.p1", 0x0000, 0x010000, CRC(74a8ed7e) SHA1(7363031c8a634ac13de957c62f32611963f797bd) )
	ROM_LOAD( "gbll20-6", 0x0000, 0x010000, CRC(f34d233a) SHA1(3f13563b2821b2f36267470c36ba346879521bc9) )
ROM_END



ROM_START( m4gb006 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "006s.p1", 0x0000, 0x010000, CRC(6e750ab9) SHA1(2e1f08df7991efe450633e0bcec201e6fa7fdbaa) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "006d.p1", 0x0000, 0x010000, CRC(7e0a4282) SHA1(8fd0cbdd9cf3ac74b7b202ce7615392c1a746906) )
	ROM_LOAD( "006y.p1", 0x0000, 0x010000, CRC(2947f4ed) SHA1(7d212bcef36e2bd792ded3e1e1638218e76da119) )
	ROM_LOAD( "bond20_11", 0x0000, 0x010000, CRC(8d810cb1) SHA1(065d8df33472a3476dd6cf21a684db9d7c8ba829) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "006s.chr", 0x0000, 0x000048, CRC(ee3d06eb) SHA1(570a715e71d4184e4df02b7e5b68fee70e03aeb0) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "006snd.p1", 0x000000, 0x080000, CRC(44afef7d) SHA1(d8a4b6dc04e0f337db6d3b5322d066ae5f5bda41) )
	ROM_LOAD( "006snd.p2", 0x080000, 0x080000, CRC(5f3c7cf8) SHA1(500f8fb07ef344d44c062f8d01878df1c917bcfc) )
ROM_END

ROM_START( m4gbust )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ghostbusters 2p.bin", 0x0000, 0x010000, CRC(abb288c4) SHA1(2012e027711996a552ab59674ae3bce1bf14f44b) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "gb_02___.2n3", 0x0000, 0x010000, CRC(973b3538) SHA1(31df04d9f35cbde4d5e395256927f146d1613178) )
	ROM_LOAD( "gb_02___.3a3", 0x0000, 0x010000, CRC(2b9d94b6) SHA1(ca433240f9e926cdf5240209589951e6018a496a) )
	ROM_LOAD( "gb_02___.3n3", 0x0000, 0x010000, CRC(99514ddd) SHA1(432d484525867c6ad68cd93a4bfded4dba36cf56) )
	ROM_LOAD( "gb_02___.3s3", 0x0000, 0x010000, CRC(2634aa5f) SHA1(58ab973940138bdfd2690867e2ac3eb52bffb633) )
	ROM_LOAD( "gb_05___.4a3", 0x0000, 0x010000, CRC(8be6949e) SHA1(9731a1cb0d17c3cec2bec263cd6348f05662d917) )
	ROM_LOAD( "gb_05___.4n3", 0x0000, 0x010000, CRC(621b25f0) SHA1(bf699068284def8bad9143c5841f667f2cb6f20f) )
	ROM_LOAD( "gb_05___.4s3", 0x0000, 0x010000, CRC(e2227701) SHA1(271682c7bf6e0f6f49f6d6b138aa19b6ef6bc626) )
	ROM_LOAD( "gb_05_d_.4a3", 0x0000, 0x010000, CRC(a1b2b32f) SHA1(c1504b3768920f90dbd441b9d50db9676528ca97) )
	ROM_LOAD( "gb_10___.2a3", 0x0000, 0x010000, CRC(a5c692f3) SHA1(8305c88ab8b80b407f4723df25135c25a4c0794f) )
	ROM_LOAD( "gb_10___.2n3", 0x0000, 0x010000, CRC(de18c441) SHA1(5a7055fcd755c1ac58e1b94af243801f169f29f5) )
	ROM_LOAD( "gb_10___.3s3", 0x0000, 0x010000, CRC(427e043b) SHA1(2f64c11a04306692ac5eb9919892f7226156dce0) )
	ROM_LOAD( "gb_10_b_.3s3", 0x0000, 0x010000, CRC(091afb66) SHA1(ac32d7be1e1f4f1453e37017966990a481506024) )
	ROM_LOAD( "gb_10_d_.2a3", 0x0000, 0x010000, CRC(f1446bf5) SHA1(4011d60e13045476741c5a02c64dabbe6a1ae2d6) )
	ROM_LOAD( "gb_10_d_.2n3", 0x0000, 0x010000, CRC(cac5057d) SHA1(afcc21dbd07515ed134675b7dbfb53c048a465b0) )
	ROM_LOAD( "gb_10_d_.3s3", 0x0000, 0x010000, CRC(776736de) SHA1(4f80d9ffdf4468801cf830e9774b6028f7684864) )
	ROM_LOAD( "gb_20___.2n3", 0x0000, 0x010000, CRC(27fc2ee1) SHA1(2e6a042f7117b4594b2601ae166ee0db72c70ed5) )
	ROM_LOAD( "gb_20___.3s3", 0x0000, 0x010000, CRC(4a86d879) SHA1(72e92b6482fdeb4dca36d9426a712ac24d60f7f7) )
	ROM_LOAD( "gb_20_b_.2a3", 0x0000, 0x010000, CRC(4dd7d38f) SHA1(8a71c27189ec3089c016a8292db68f7cdc91b083) )
	ROM_LOAD( "gb_20_b_.2n3", 0x0000, 0x010000, CRC(28cbb217) SHA1(a74978ff5e1511a33f543006b3f8ad30a77ea462) )
	ROM_LOAD( "gb_20_b_.3s3", 0x0000, 0x010000, CRC(1a7cc3cf) SHA1(0d5764d35489bde284965c197b217a06f26a3e3b) )
	ROM_LOAD( "gb_20_d_.2a3", 0x0000, 0x010000, CRC(70f40688) SHA1(ed14f8f460825ffa087394ef5984ae064e02f7b6) )
	ROM_LOAD( "gb_20_d_.2n3", 0x0000, 0x010000, CRC(431c2965) SHA1(eb24e560d5c4bf419465fc760621a4fa853fff95) )
	ROM_LOAD( "gb_20_d_.3s3", 0x0000, 0x010000, CRC(4fc69155) SHA1(09a0f2122893d9fd90204a74c8862e01386503a4) )

ROM_END

ROM_START( m4giant )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dgi21.bin", 0x0000, 0x010000, CRC(07d8685a) SHA1(1b51db748543f2e4b6b7d7ad16b77864bbfe5a66) )
ROM_END

ROM_START( m4gclue )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "c2002ad.p1", 0x0000, 0x010000, CRC(39507216) SHA1(dc49d9cea63cd5e88e4076bfca3aae88521056be) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "c2002b.p1", 0x0000, 0x010000, CRC(1a552423) SHA1(3025c7a8f98817a8b0233c7682452d5d6df081c5) )
	ROM_LOAD( "c2002bd.p1", 0x0000, 0x010000, CRC(1eff74d1) SHA1(7cfba92237b3de1ea54c0d8b8619dd09a68c3b51) )
	ROM_LOAD( "c2002c.p1", 0x0000, 0x010000, CRC(3c73d6c8) SHA1(63bb5df7063bf33e2b9f88db53ad64666967ecca) )
	ROM_LOAD( "c2002d.p1", 0x0000, 0x010000, CRC(1c7c2851) SHA1(2dfc3de4fed92c0e4972289646611c82e4ea491b) )
	ROM_LOAD( "c2002dk.p1", 0x0000, 0x010000, CRC(25729d8c) SHA1(6d5b89f9a063c35b4cdbf73102144f752ec96f70) )
	ROM_LOAD( "c2002dr.p1", 0x0000, 0x010000, CRC(869844e2) SHA1(b7c51877b803c80b0e0acfa9c7b29dbda4c917f7) )
	ROM_LOAD( "c2002dy.p1", 0x0000, 0x010000, CRC(1bf36c0c) SHA1(584fa498821129cfe9fb5c64cbf29c10abef0c57) )
	ROM_LOAD( "c2002k.p1", 0x0000, 0x010000, CRC(05e65abe) SHA1(560c2a7ac5af90ce5d0f1b34ec097bf5f733ec90) )
	ROM_LOAD( "c2002r.p1", 0x0000, 0x010000, CRC(49ce30ab) SHA1(501f509aae61059349107657516b559106d06f49) )
	ROM_LOAD( "c2002s.p1", 0x0000, 0x010000, CRC(fe640d18) SHA1(598e5a92bd26457cbd0cbd1f73cddb56054ff826) )
	ROM_LOAD( "c2002y.p1", 0x0000, 0x010000, CRC(d4a51845) SHA1(7808ff2d62eeadbb894379857266770fe9954384) )
	ROM_LOAD( "c2504ad.p1", 0x0000, 0x010000, CRC(f721de72) SHA1(8e64360f5b0de9d9b2afda6361e2b6d4ec3b1baf) )
	ROM_LOAD( "c2504b.p1", 0x0000, 0x010000, CRC(4cd01058) SHA1(705b3979c8728e98810cb3cd4d4b4e926e52d78b) )
	ROM_LOAD( "c2504bd.p1", 0x0000, 0x010000, CRC(34d6d202) SHA1(1c596abdbcce801f5363871f9959d07ba9568083) )
	ROM_LOAD( "c2504c.p1", 0x0000, 0x010000, CRC(e0256ff2) SHA1(b1a7840b30198f9870dd326166f3b1606c4f8412) )
	ROM_LOAD( "c2504d.p1", 0x0000, 0x010000, CRC(97ee13c6) SHA1(9474923202e0dc34763037fd6ceb01677a5915ad) )
	ROM_LOAD( "c2504dk.p1", 0x0000, 0x010000, CRC(9b5504e8) SHA1(3d1c07503f7d987d34e4cd93d9c42b347131a1b1) )
	ROM_LOAD( "c2504dr.p1", 0x0000, 0x010000, CRC(3a77e230) SHA1(62460ad5f41fe058e5f82389bf63a761a1e0796d) )
	ROM_LOAD( "c2504dy.p1", 0x0000, 0x010000, CRC(a71ccade) SHA1(65cd823aa4136fcf8d93058e4ef708e4b01caa3a) )
	ROM_LOAD( "c2504k.p1", 0x0000, 0x010000, CRC(aa4af6e9) SHA1(18654cf751e157d11010e991e74127aa15cb3cfc) )
	ROM_LOAD( "c2504r.p1", 0x0000, 0x010000, CRC(62bbd71d) SHA1(0b7f97a213a8f5b457aa54f760e19ebd00b1d334) )
	ROM_LOAD( "c2504s.p1", 0x0000, 0x010000, CRC(47d6791f) SHA1(e232586605b096849480002ddb7b77a8b113a388) )
	ROM_LOAD( "c2504y.p1", 0x0000, 0x010000, CRC(ffd0fff3) SHA1(5f30353e73331315be99281c7ed435d05a9bfc5b) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "c25snd.p1", 0x000000, 0x080000, CRC(cd8f4ee0) SHA1(a7b9ae93b3a3d231a8239fff12689ec2084ce0c1) )
	ROM_LOAD( "c95snd.p1", 0x080000, 0x080000, CRC(ae952e15) SHA1(a9eed61c3d65ded5e1faa67362f181393cb6339a) )
ROM_END

ROM_START( m4gldstr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "gs20mp1.bin", 0x8000, 0x008000, CRC(409e0375) SHA1(da231769cd439bc665634d73b0b18f31da332b47) )
	ROM_LOAD( "gs20mp2.bin", 0x6000, 0x002000, CRC(4979bc57) SHA1(53683600c8b3140deab8eac55786460b45c3b143) )
ROM_END

ROM_START( m4gldgat )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dgg22.bin", 0x0000, 0x010000, CRC(ef8498df) SHA1(6bf164ef18445e83e4510a000bc924cbe916ad99) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "m450.chr", 0x0000, 0x000048, CRC(fb7b2a45) SHA1(b6d5537bde9c05a3e79221a5577b8ae77bace9e6) )
ROM_END

ROM_START( m4gldjok )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dgj12.bin", 0x0000, 0x010000, CRC(93ee0c35) SHA1(5ae67b14f7f3d8528fa106519a8a27437c997a70) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sdgj.snd", 0x0000, 0x080000, CRC(b6cd118b) SHA1(51c5d694ed0dfde8d3fd682f2471d83eec236736) )
ROM_END

ROM_START( m4gldnud )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "gdjb", 0x0000, 0x010000, CRC(b26cd308) SHA1(4e29f6cce773232a1c43cd2fb3ce9b844c446bb8) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "gn.snd", 0x0000, 0x080000, CRC(f652cd0c) SHA1(9ce986bc12bcf22a57e065329e82671d19cc96d7) )
ROM_END

ROM_START( m4grbbnk )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "g4b20p1.bin", 0xc000, 0x004000, CRC(35354cf5) SHA1(d14414bf257dbc44652e8b1f8a329709fb8912c5) )
	ROM_LOAD( "g4b20p2.bin", 0x8000, 0x004000, CRC(039cb9aa) SHA1(6e648f7af5f04701bb70a8f68593b80f777f57bb) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "g4b_1_0.p1", 0x0000, 0x004000, CRC(56e5be9e) SHA1(46a4643b16648441fa7e03844d8f40794400dc9a) )
	ROM_LOAD( "g4b_2_1.p1", 0x0000, 0x004000, CRC(60bdeb67) SHA1(7f9b61391d785e5ecfc1f7555fb38c5f1c7f7c58) )
	ROM_LOAD( "g4b_2_1.p2", 0x0000, 0x004000, CRC(20952ee9) SHA1(46a9195814440471537b415c2e71ed5e9719e513) )
ROM_END

ROM_START( m4graff )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "graffo6", 0x0000, 0x010000, CRC(7349c9ca) SHA1(2744035d6c7897394c8fead27f48779047590fba) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "grax.p1", 0x0000, 0x010000, CRC(2e03a7d8) SHA1(333373fe15ae165dd24d5c11fef23f2e9b0388bf) )
	ROM_LOAD( "graxc.p1", 0x0000, 0x010000, CRC(7620657b) SHA1(2aec38ee0f826c7bb012522fd098a6fdb857c9da) )
	ROM_LOAD( "grfi20o6", 0x0000, 0x010000, CRC(7349c9ca) SHA1(2744035d6c7897394c8fead27f48779047590fba) )
ROM_END

ROM_START( m4graffd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "grafittirom.bin", 0x0000, 0x010000, CRC(36135d6e) SHA1(e71eedabae36971739f8a6fd56a4a954de29944b) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "grafittisound.bin", 0x0000, 0x010000, CRC(2d9bfff9) SHA1(ded774bcd2a7e10e4e2fc7b40976c9dcea0de6e3) )
	ROM_LOAD( "grafittisound2.bin", 0x0000, 0x010000, CRC(c06c5517) SHA1(9e11f93638d37ba6f7b34a78eea043821ca4e188) )
ROM_END

ROM_START( m4grands )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "g2d4_0.p1", 0x8000, 0x008000, CRC(550eaf03) SHA1(789e4bd7ffe71841dbf22413aa0fd16be5477482) )
	ROM_LOAD( "g2d4_0.p2", 0x6000, 0x002000, CRC(fecf1270) SHA1(7826abc21988f751a2b1190706fe4717a4f028ec) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "gd11p1.bin", 0x0000, 0x008000, CRC(f9989c86) SHA1(a4e3fa71ab0a07259751cbb07f943fea04b83946) )
	ROM_LOAD( "gd11p2.bin", 0x0000, 0x002000, CRC(3fe26112) SHA1(bfc1c83fc6f341c6f4be937496f9bd0d69e49485) )
ROM_END

ROM_START( m4gnsmk )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dgu16", 0x0000, 0x010000, CRC(6aa23345) SHA1(45e129ec95b1a796f334bedd08469f2ab47a18f8) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "sdgu01.s1", 0x000000, 0x080000, CRC(bfb284a2) SHA1(860b98d54a3180fbb00b7b03feae049fb4cf9d7f) )
	ROM_LOAD( "sdgu01.s2", 0x080000, 0x080000, CRC(1a46ba28) SHA1(d7154e5f92be8631207620eb313b28990c6a1c7f) )
	ROM_LOAD( "sdgu01.s3", 0x100000, 0x080000, CRC(88bffcf4) SHA1(1da853193f6a22889edff5aafd9440c676a82ea6) )
	ROM_LOAD( "sdgu01.s4", 0x180000, 0x080000, CRC(a6160bef) SHA1(807f7d470728a479a55c782fca3df1eacd0b594c) )
ROM_END

ROM_START( m4hpyjok )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dhj12", 0x0000, 0x010000, CRC(982439d7) SHA1(8d27fcecf7a6a7fd774678580074f945675758f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "dhjsnd", 0x0000, 0x080000, CRC(8ac4aae6) SHA1(70dba43b398010a8bd0d82cf91553d3f5e0921f0) )
ROM_END

ROM_START( m4hijinx )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "hijinx.hex", 0x0000, 0x020000, CRC(792b3bae) SHA1(d30aecce42953f1ec49753cc2d1df00ad9bd088f) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "hij15g", 0x0000, 0x020000, CRC(73271cca) SHA1(8177e10e30386464a7e5a33dc3c02adbf4c93101) )
	ROM_LOAD( "hij15t", 0x0000, 0x020000, CRC(c7d54c64) SHA1(d3537c8412b583f2812f87ab68ac8855e9cdbd2f) )
	ROM_LOAD( "jns02ad.p1", 0x0000, 0x020000, CRC(436a6632) SHA1(25ab01b8785cf6f9d2316b15d2d2887e898358de) )
	ROM_LOAD( "jns02b.p1", 0x0000, 0x020000, CRC(171b8941) SHA1(8383f0ce3c21b187f031302b4d930e13c131f862) )
	ROM_LOAD( "jns02bd.p1", 0x0000, 0x020000, CRC(6a8b03ba) SHA1(ecc6826474a96a7d2e5cee851d672b954906bfbe) )
	ROM_LOAD( "jns02d.p1", 0x0000, 0x020000, CRC(3ca43ce0) SHA1(d369cc103ce1c506e85cdce87ac7836c99b03df6) )
	ROM_LOAD( "jns02dh.p1", 0x0000, 0x020000, CRC(84b760a1) SHA1(2312a8177c474d5bd402e5d8039e37430a7c37ae) )
	ROM_LOAD( "jns02dk.p1", 0x0000, 0x020000, CRC(3ee77376) SHA1(b19aea9254b9315c805e74a7d20dc626254a4834) )
	ROM_LOAD( "jns02dr.p1", 0x0000, 0x020000, CRC(f065dd03) SHA1(b7f3352b0807f30d4eebfca1c276bd4a53ede632) )
	ROM_LOAD( "jns02dy.p1", 0x0000, 0x020000, CRC(09adce64) SHA1(09a671ef097dbd4233aca241976c23375b4af789) )
	ROM_LOAD( "jns02h.p1", 0x0000, 0x020000, CRC(f927ea5a) SHA1(c1af9f2b20421c66d2141b85a61cd51e8cb6a67b) )
	ROM_LOAD( "jns02k.p1", 0x0000, 0x020000, CRC(4377f98d) SHA1(0e7b4e655acec07fea221697179045f06d4f3c48) )
	ROM_LOAD( "jns02r.p1", 0x0000, 0x020000, CRC(8df557f8) SHA1(899e73b265065f09eac785e45e06fd755a618c21) )
	ROM_LOAD( "jns02s.p1", 0x0000, 0x020000, CRC(42df2639) SHA1(8ed6addfc85cfeab4c5f03c24a692a9c392a8bc2) )
	ROM_LOAD( "jns02y.p1", 0x0000, 0x020000, CRC(743d449f) SHA1(739e41b28ee53465b40138cdccf0fcd1782c8b45) )
	ROM_LOAD( "jns03ad.p1", 0x0000, 0x020000, CRC(9d3e4a13) SHA1(5780eaeb148d64f7d2769207f7973c02a5e5a3de) )
	ROM_LOAD( "jns03b.p1", 0x0000, 0x020000, CRC(07eddb71) SHA1(05f5bbcf6c7e69407163c9dc76c6c587f0cdf10e) )
	ROM_LOAD( "jns03bd.p1", 0x0000, 0x020000, CRC(b4df2f9b) SHA1(1d6025bfb119007c7b17214577cf69f978fa2830) )
	ROM_LOAD( "jns03d.p1", 0x0000, 0x020000, CRC(2c526ed0) SHA1(99f7b144ca6a3924c2b5b5f38ac09cec12f20486) )
	ROM_LOAD( "jns03dh.p1", 0x0000, 0x020000, CRC(5ae34c80) SHA1(7fd217c4c251c2506fd41f4c6173ffb12a680ba8) )
	ROM_LOAD( "jns03dk.p1", 0x0000, 0x020000, CRC(e0b35f57) SHA1(06a322e2ad37bf4f3c7959fdcb213f95c414a16d) )
	ROM_LOAD( "jns03dr.p1", 0x0000, 0x020000, CRC(2e31f122) SHA1(f00b9d47314b270a8e222a46cb1493387bebbd84) )
	ROM_LOAD( "jns03dy.p1", 0x0000, 0x020000, CRC(d7f9e245) SHA1(1605efd18a9234e72a13c4df362e1007fc443a23) )
	ROM_LOAD( "jns03h.p1", 0x0000, 0x020000, CRC(e9d1b86a) SHA1(4a982370bfb788ef53ae9479a72dbca6c7f9ec00) )
	ROM_LOAD( "jns03k.p1", 0x0000, 0x020000, CRC(5381abbd) SHA1(8967579f31ef9a0ec868af1cb64d9fa049314f94) )
	ROM_LOAD( "jns03r.p1", 0x0000, 0x020000, CRC(9d0305c8) SHA1(525b36b43a09c27f807dcbdc8bff89af82bdffc0) )
	ROM_LOAD( "jns03s.p1", 0x0000, 0x020000, CRC(ef9f3d18) SHA1(cc2239a4dca1c092025216a16bb39fe9126bd6f8) )
	ROM_LOAD( "jns03y.p1", 0x0000, 0x020000, CRC(64cb16af) SHA1(f4bc3557f84cd0054c475e0354f0562f63df3146) )
	ROM_LOAD( "jnx05d.p1", 0x0000, 0x020000, CRC(27d9db28) SHA1(df333d884735a94d3c4d460deb05e50fcfce8896) )
	ROM_LOAD( "jnx05dh.p1", 0x0000, 0x020000, CRC(d7dc5d8b) SHA1(6b94ae9169b522a80455cbe42378427d2f1019f3) )
	ROM_LOAD( "jnx05dy.p1", 0x0000, 0x020000, CRC(f44fafc0) SHA1(996411b41071cc3c5c84f00106f9685b27e95352) )
	ROM_LOAD( "jnx05h.p1", 0x0000, 0x020000, CRC(4cd3511c) SHA1(2f283f50d2313eb9da49e4efbdb9a098f90c0afb) )
	ROM_LOAD( "jnx05y.p1", 0x0000, 0x020000, CRC(6f40a357) SHA1(7d019f925d7920df23782b1e6e742bc467e7767b) )
	ROM_LOAD( "jnx10d.p1", 0x0000, 0x020000, CRC(31b243d1) SHA1(029dd8ecbe83b63ca799b6507262193ef56d4b36) )
	ROM_LOAD( "jnx10dh.p1", 0x0000, 0x020000, CRC(ffc421b0) SHA1(28439c5d2ec371edae5e7b84e1da96d468bb8556) )
	ROM_LOAD( "jnx10dy.p1", 0x0000, 0x020000, CRC(dc57d3fb) SHA1(e622485f37f638a69e0c5dbb06faa519b10eafc9) )
	ROM_LOAD( "jnx10h.p1", 0x0000, 0x020000, CRC(5ab8c9e5) SHA1(f12094bd95369288a76dac9d1e62810fa478cfd6) )
	ROM_LOAD( "jnx10s.p1", 0x0000, 0x020000, CRC(a291147e) SHA1(818172bab2fad210a937d91e0be4ddf165f1cf99) )
	ROM_LOAD( "jnx10y.p1", 0x0000, 0x020000, CRC(792b3bae) SHA1(d30aecce42953f1ec49753cc2d1df00ad9bd088f) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "hijinx1.hex", 0x000000, 0x080000, CRC(8d5afedb) SHA1(6bf6dadddf8dd3672e3d05167ab9a0793c269176) )
	ROM_LOAD( "hijinx2.hex", 0x080000, 0x080000, CRC(696c8a92) SHA1(d54a1020fea80bacb678bc8bd6b7d4d0854af603) )
ROM_END

ROM_START( m4hirise )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "hirs.p1", 0x0000, 0x010000, CRC(a38f771e) SHA1(c1502200671389a1fe6dcb9c043d22583d5991dc) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "hiix.p1", 0x0000, 0x010000, CRC(c68c816c) SHA1(2ec89d83f3b658700433fc165358290ce58eba64) )
	ROM_LOAD( "hirs20dd", 0x0000, 0x010000, CRC(89941670) SHA1(28859adfa79dce53c348c63b46f6f5a068f2b2de) )
	ROM_LOAD( "hirx.p1", 0x0000, 0x010000, CRC(4280a16b) SHA1(c9179ec17404a6f084679ad5f04e53a50f00af98) )
	ROM_LOAD( "hirxc.p1", 0x0000, 0x010000, CRC(1ad1d942) SHA1(91d02212606e22b280be9640433e013bc50e5ea8) )
	ROM_LOAD( "hrise206", 0x0000, 0x010000, CRC(58b4bbdd) SHA1(0b76d27147fbadba97328eb9d2dc81cff9d576e0) )
ROM_END

ROM_START( m4hiroll )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "hr-3.0-p1.bin", 0xc000, 0x004000, CRC(73d022c6) SHA1(de94f5903cab94a8f93aa293397d32840cc37ab4) )
	ROM_LOAD( "hr-3.0-p2.bin", 0x8000, 0x004000, CRC(617d0e2c) SHA1(5e5d59133e3b7a4eea205b79194754994cfe6d07) )
ROM_END

ROM_START( m4hittop )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "hi4s.p1", 0x0000, 0x010000, CRC(3a04ee7a) SHA1(d23e9da2c22f6983a855bc519597ea9cea84f2dd) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "chuad.p1", 0x0000, 0x010000, CRC(01d3b86c) SHA1(27af0e76661495d5b91ee6a53507f9a5d4e5ab85) )
	ROM_LOAD( "chub.p1", 0x0000, 0x010000, CRC(17ff4ed4) SHA1(f193a00a46c82d4989af18055f9f69d93df79ec6) )
	ROM_LOAD( "chubd.p1", 0x0000, 0x010000, CRC(3e7a6b1b) SHA1(8939a0cac8578ff5e1d1ab2b3a64b3809793c44a) )
	ROM_LOAD( "chud.p1", 0x0000, 0x010000, CRC(26875ed3) SHA1(06dbf594e2c5202ee624f4202f634281a89a3870) )
	ROM_LOAD( "chudk.p1", 0x0000, 0x010000, CRC(10c1f6c3) SHA1(e6ff6ea40f35cfd9ed7643e69eca62775f20b3a2) )
	ROM_LOAD( "chudy.p1", 0x0000, 0x010000, CRC(65302d8c) SHA1(de340cc182212b576cae46669492d0d760d2f288) )
	ROM_LOAD( "chuk.p1", 0x0000, 0x010000, CRC(7f333a2c) SHA1(73719997c200ec5291ceaa12f8667979a731212e) )
	ROM_LOAD( "chur.p1", 0x0000, 0x010000, CRC(dbb89a00) SHA1(70b2f2c78011b8b470aa58153d524f920d553b28) )
	ROM_LOAD( "chus.p1", 0x0000, 0x010000, CRC(8a39816e) SHA1(3869f7ae0c9b681cfb07e2f6c1a94fc81fa13fe3) )
	ROM_LOAD( "chuy.p1", 0x0000, 0x010000, CRC(e0902d74) SHA1(a34db63f1354853ad5a1026e4402ccd2e564c7d7) )
	ROM_LOAD( "h4t20lp1.bin", 0x0000, 0x004000, CRC(c0b2c9df) SHA1(29b3bb78da7d1848321b81a123e8247a74170bc2) )
	ROM_LOAD( "h4t20lp2.bin", 0x0000, 0x004000, CRC(dc3921c7) SHA1(413d4a9c2f273129ca898e8da66e27254da5565e) )
	ROM_LOAD( "h4t2_0.1", 0x0000, 0x004000, CRC(8293cf7b) SHA1(6e7014bc0257b37406e59651ef0ac0f0563d5aa6) )
	ROM_LOAD( "hi4ad.p1", 0x0000, 0x010000, CRC(eeb958f3) SHA1(ee7f7615df2141ad5183288101949b74c4543de9) )
	ROM_LOAD( "hi4b.p1", 0x0000, 0x010000, CRC(68af264b) SHA1(e7f75b5294cc7541f9397c492c171c79b7a21a36) )
	ROM_LOAD( "hi4bd.p1", 0x0000, 0x010000, CRC(d72cd485) SHA1(d0d38cbb518c824d4a8107e1711f85120c39bc4c) )
	ROM_LOAD( "hi4d.p1", 0x0000, 0x010000, CRC(59d7364c) SHA1(1e665b178b8bf7314ca5b5ea97dc185491fc2930) )
	ROM_LOAD( "hi4dk.p1", 0x0000, 0x010000, CRC(76d4bb70) SHA1(7365f072a7e3e8141e15fbf56c3355bc6310895f) )
	ROM_LOAD( "hi4dy.p1", 0x0000, 0x010000, CRC(b1ddf7fe) SHA1(a334619b5dfc7a44e9082cc37cb5187413adb29f) )
	ROM_LOAD( "hi4k.p1", 0x0000, 0x010000, CRC(99cb8bc9) SHA1(106bf6e327643c49024f9422d6b87f5b157b452f) )
	ROM_LOAD( "hi4y.p1", 0x0000, 0x010000, CRC(c60e01e6) SHA1(c4a7ea44c36c78401cab3ef87d7e02add0b48ab5) )
	ROM_LOAD( "hit04ad.p1", 0x0000, 0x010000, CRC(cc9d10fa) SHA1(b7ce14fecfd8142fa7127c23f152c749dae74701) )
	ROM_LOAD( "hit04b.p1", 0x0000, 0x010000, CRC(da511063) SHA1(3f4fb8518cb2057ec4c2bb13fd3e61ee73bfa457) )
	ROM_LOAD( "hit04bd.p1", 0x0000, 0x010000, CRC(40a84b97) SHA1(416f78c19e08f405a3b36f886f69e7b88e5aa90a) )
	ROM_LOAD( "hit04d.p1", 0x0000, 0x010000, CRC(89607e84) SHA1(280209ca3030383547cc91eee2f71a810768353f) )
	ROM_LOAD( "hit04dk.p1", 0x0000, 0x010000, CRC(b89e606f) SHA1(9096126b719ecd92185d9c1d50d13c9339d09583) )
	ROM_LOAD( "hit04dr.p1", 0x0000, 0x010000, CRC(1b4d8099) SHA1(505e0948a78b5d57f0986896ab900d25a20d7877) )
	ROM_LOAD( "hit04dy.p1", 0x0000, 0x010000, CRC(25f54881) SHA1(9f4ae52295df5810cbe6c18cae66877bec006a28) )
	ROM_LOAD( "hit04k.p1", 0x0000, 0x010000, CRC(5ef3f78d) SHA1(e72727b3dc7793c36f182b3e7d363741254c0be7) )
	ROM_LOAD( "hit04r.p1", 0x0000, 0x010000, CRC(d87a9f60) SHA1(614224b80afaa6e407f9b40b45b8aecdf999e13a) )
	ROM_LOAD( "hit04s.p1", 0x0000, 0x010000, CRC(05376f9f) SHA1(e59bdd6541669b150bb68eb97ea316c3fe451778) )
	ROM_LOAD( "hit04y.p1", 0x0000, 0x010000, CRC(c398df63) SHA1(5e93cb95da37b1593d030e99e97996252ad6cda1) )
	ROM_LOAD( "ht201ad.p1", 0x0000, 0x010000, CRC(b0f3873b) SHA1(6e7d1b20dff4b81ebd171d6d92c95e46817bdf90) )
	ROM_LOAD( "ht201b.p1", 0x0000, 0x010000, CRC(9dbe41fc) SHA1(ce5ed2707ab63057a2f66a1098e3752acaa72dac) )
	ROM_LOAD( "ht201bd.p1", 0x0000, 0x010000, CRC(be23c8b6) SHA1(0d4ab2d3c7ac063ec1ce10b2af28c8770d8bd818) )
	ROM_LOAD( "ht201d.p1", 0x0000, 0x010000, CRC(25b9fcd7) SHA1(8bebbf0b621a704ed9811e67eab003f4ddebcde2) )
	ROM_LOAD( "ht201dk.p1", 0x0000, 0x010000, CRC(1c77872e) SHA1(a728811efee6f40779b01a6d60f2d0167e204a09) )
	ROM_LOAD( "ht201dr.p1", 0x0000, 0x010000, CRC(9836d075) SHA1(b015e9706c5ec7b03133eda70fe0322c24969d7e) )
	ROM_LOAD( "ht201dy.p1", 0x0000, 0x010000, CRC(811cafc0) SHA1(e31ad353ee8ce4ea059d6a469baaa14357b738c9) )
	ROM_LOAD( "ht201k.p1", 0x0000, 0x010000, CRC(191ca612) SHA1(a2a80b64cc04aa590046413f1474340cd3a5b03a) )
	ROM_LOAD( "ht201r.p1", 0x0000, 0x010000, CRC(154643be) SHA1(280ae761c434bbed84317d85aef2ad4a78c61d1d) )
	ROM_LOAD( "ht201s.p1", 0x0000, 0x010000, CRC(37b20464) SHA1(e87b0a2023416fa7b63201e19850319723eb6c10) )
	ROM_LOAD( "ht201y.p1", 0x0000, 0x010000, CRC(84778efc) SHA1(bdc43973913d0e8be0e16ee89da01b1bcdc2da6f) )
	ROM_LOAD( "ht501ad.p1", 0x0000, 0x010000, CRC(7bf00848) SHA1(700d90218d0bd31860dc905c00d0afbf3a1e8704) )
	ROM_LOAD( "ht501b.p1", 0x0000, 0x010000, CRC(c06dd046) SHA1(a47c62fc299842e66694f34844b43a55d6f20c3d) )
	ROM_LOAD( "ht501bd.p1", 0x0000, 0x010000, CRC(d4a3843f) SHA1(cc66ebaa334bab86b9bcb1623316c31318e84d2a) )
	ROM_LOAD( "ht501d.p1", 0x0000, 0x010000, CRC(67d3c040) SHA1(beec134c53715544080327319b5d6231b625fbb4) )
	ROM_LOAD( "ht501dk.p1", 0x0000, 0x010000, CRC(feec7950) SHA1(7bcd8d0166847f72871a78e4b287c72e1a06d26e) )
	ROM_LOAD( "ht501dr.p1", 0x0000, 0x010000, CRC(c73b60b1) SHA1(9f14957eb0eec7e833b6bb5b162286d94c8f03c4) )
	ROM_LOAD( "ht501dy.p1", 0x0000, 0x010000, CRC(756c7ae9) SHA1(42a731e472f073845b98d7fcc47fe70f57181ce6) )
	ROM_LOAD( "ht501k.p1", 0x0000, 0x010000, CRC(93269e53) SHA1(7e40ac4e9f4b26755867353fdccadf0f976402b4) )
	ROM_LOAD( "ht501r.p1", 0x0000, 0x010000, CRC(9aec0493) SHA1(6b0b7e5f4a988ff4d2bc123978adc09195eb4232) )
	ROM_LOAD( "ht501s.p1", 0x0000, 0x010000, CRC(ac440a2b) SHA1(f3f3d0c9c8dcb41509307c970f0776ebcfffdeb0) )
	ROM_LOAD( "ht501y.p1", 0x0000, 0x010000, CRC(a7f8ece6) SHA1(f4472c040c9255eaef5b1109c3bec44f4978b600) )
	ROM_LOAD( "httad.p1", 0x0000, 0x010000, CRC(e5a3df45) SHA1(70bebb33cbe466c379f278347d0b47862b1d01a8) )
	ROM_LOAD( "httb.p1", 0x0000, 0x010000, CRC(5c921ff2) SHA1(a9184e4e3916c1ab92761d0e33b42cce4a58e7b1) )
	ROM_LOAD( "httbd.p1", 0x0000, 0x010000, CRC(9d19fac9) SHA1(17072ac5b49cd947bf397dfbe9b6b0bd269dd1b4) )
	ROM_LOAD( "httd.p1", 0x0000, 0x010000, CRC(5e5bacb9) SHA1(d673010cdf2fb9352fc510409deade42b5508b29) )
	ROM_LOAD( "httdk.p1", 0x0000, 0x010000, CRC(17b1db87) SHA1(196163f68c82c4600ecacee52ee8044739568fbf) )
	ROM_LOAD( "httdy.p1", 0x0000, 0x010000, CRC(428af7bf) SHA1(954a512105d1a5998d4ffcbf21be0c9d9a65bbeb) )
	ROM_LOAD( "httk.p1", 0x0000, 0x010000, CRC(581dd34a) SHA1(00cad1860f5edf056b8f9397ca46165593be4755) )
	ROM_LOAD( "htts.p1", 0x0000, 0x010000, CRC(6c794eb2) SHA1(347a7c74b1fd7631fbcd398bf5e7c36af088109e) )
	ROM_LOAD( "htty.p1", 0x0000, 0x010000, CRC(c9b402b2) SHA1(2165c1892fc1f0b9b0c39127f322f15c9e1912b1) )
	ROM_LOAD( "nht01s.p1", 0x0000, 0x010000, CRC(a4a44ddf) SHA1(e64953f3cd2559a8ebdacb2b0c12c84fd5c4b836) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "httsnd.p1",   0x000000, 0x080000, CRC(1cfb12d2) SHA1(d909c7ee8ea10587a9a9251af943b0151d2c4a16) )
	ROM_LOAD( "nhtsnd01.p1", 0x000000, 0x080000, CRC(2d1d93c6) SHA1(80a8d131bafdb74d20d1ca5cbe2219ee4df0b675) )

	ROM_LOAD( "hi4snd.p1", 0x000000, 0x080000, CRC(066f262b) SHA1(fd48da486592740c68ee497396602199101711a6) )
	ROM_LOAD( "hi4snd.p2", 0x080000, 0x080000, CRC(0ee89f6c) SHA1(7088149000efd1dcdf37aa9b88f7c6491184da24) )
ROM_END

ROM_START( m4thehit )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dth17.bin", 0x0000, 0x010000, CRC(93947de4) SHA1(e04c34edf39d264e3fa91bf6dfd757088e1c08e4) )
ROM_END

ROM_START( m4holdon )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dho25.bin", 0x0000, 0x010000, CRC(9c22690d) SHA1(a2474dd1901628551804ba2bf652a8a5a1de5739) )
ROM_END

ROM_START( m4holdtm )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dht10.hex", 0x0000, 0x010000, CRC(217d382b) SHA1(a27dd107c554d4787967633dff998d3962ee0ea5) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "ht01.chr", 0x0000, 0x000048, CRC(0fc2bb52) SHA1(0d0e47938f6e00166e7352732ddfb7c610f44db2) )
	ROM_LOAD( "m400.chr", 0x0000, 0x000048, CRC(8f00f720) SHA1(ea59fa2a3b016a7ae83be3caf863de87ce7aeffa) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sun01.hex", 0x0000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END

ROM_START( m4hotrod )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rodk.p1", 0x0000, 0x010000, CRC(298d85ff) SHA1(3c9374be1f6b5e58a1b9004f74f3a33d0fff4214) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "hot rod 5p 4 p1 (27512)", 0x0000, 0x010000, CRC(b6212af8) SHA1(9453c4424244895b3ad15d5fba45fe8822e7ff2b) )
	ROM_LOAD( "hr056c", 0x0000, 0x010000, CRC(c062f285) SHA1(917e82cadf242aa815c525ff435cd4b04ea87e39) )
	ROM_LOAD( "hrod05_11", 0x0000, 0x010000, CRC(61f35723) SHA1(743b71ecde4923c359a1202eaad7e4d74b0d1611) )
	ROM_LOAD( "hrod10_11", 0x0000, 0x010000, CRC(5b924a86) SHA1(6b86dce6ba3789750de05dca996202c000ecfbae) )
	ROM_LOAD( "hrod20_11", 0x0000, 0x010000, CRC(b81a57b6) SHA1(442c119b9ed70d4da2f9082ec01e410cfee76102) )
	ROM_LOAD( "hrod55", 0x0000, 0x010000, CRC(dd6d3153) SHA1(27f3324b43c026abf2ae4c584afeb6971a3fe57a) )
	ROM_LOAD( "hrod58c", 0x0000, 0x010000, CRC(079474db) SHA1(257b1086277cd0b8398b80a4b95cf1212c10c4c3) )
	ROM_LOAD( "rodc.p1", 0x0000, 0x010000, CRC(2f6b53d3) SHA1(fa4df1e6a2f6158cbc099d7e2d5ec96355079f36) )
	ROM_LOAD( "roddy.p1", 0x0000, 0x010000, CRC(53e508ac) SHA1(24df8b949211e7bc5c7b8d704562b36e52cb8d5c) )
	ROM_LOAD( "rods.p1", 0x0000, 0x010000, CRC(93d73857) SHA1(dcfd1dbf368f68ba3e7aa163eedd89c68aaccec8) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "hr_05___.1o1", 0x0000, 0x010000, CRC(abdb0a16) SHA1(5db2721326a22b9d8653773ec8de8a845d147eee) )
	ROM_LOAD( "hr_05_d_.1o1", 0x0000, 0x010000, CRC(8a14fa8d) SHA1(8d64a75514d0a58fcdc2d5a81c0b85a49ab8322b) )
	ROM_LOAD( "hr_10___.1o1", 0x0000, 0x010000, CRC(5e09202f) SHA1(06991f5fd451fff77ef7ab0b866543613c3dcc02) )
	ROM_LOAD( "hr_10_d_.1o1", 0x0000, 0x010000, CRC(329409c5) SHA1(e9ba0f36048f46a381c8a408b9c1e10acea0bde3) )
	ROM_LOAD( "hri05___.101", 0x0000, 0x010000, CRC(43e5e86e) SHA1(8bf00b1af1f86f1a361537a1117d857fa8fa7af4) )
	ROM_LOAD( "hri05___.1o1", 0x0000, 0x010000, CRC(43e5e86e) SHA1(8bf00b1af1f86f1a361537a1117d857fa8fa7af4) )
	ROM_LOAD( "hri10___.1o1", 0x0000, 0x010000, CRC(a855f93c) SHA1(2b63aa7c632f14457c2ae0312cef7b22bbf1df22) )
	ROM_LOAD( "hrod_05_.4", 0x0000, 0x010000, CRC(c58aa0e8) SHA1(8a2b5a9bd4e93a7a12cae4e92e0faf35e2ebbe4c) )
	ROM_LOAD( "hrod_05_.8", 0x0000, 0x010000, CRC(b3c9e0c9) SHA1(4a549876121dd7fc5c11d3b03322d1e5f90eaa86) )
	ROM_LOAD( "hrod_10_.4", 0x0000, 0x010000, CRC(b9e84451) SHA1(7566aef1604992376010758cb079fe9da67ad454) )
	ROM_LOAD( "hrod_10_.8", 0x0000, 0x010000, CRC(62ac8057) SHA1(d2085ec0f29ff85251ef2c576e828f502420839d) )
	ROM_LOAD( "hrod_20_.4", 0x0000, 0x010000, CRC(c58bb470) SHA1(7bb831d7b647d17eff896ccce0ab7c8cfa8179b8) )
	ROM_LOAD( "hrod_20_.8", 0x0000, 0x010000, CRC(a2d20781) SHA1(3f1b33374ae0a61815b38ad0e57856ae16047adc) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "hotrod.chr", 0x0000, 0x000048, CRC(a76dc7d3) SHA1(43010dab862a98ec2a8f8444bf1411902ba03c63) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "rodsnd.p1", 0x000000, 0x080000, CRC(bfdafedc) SHA1(6acc838ec046d44e7faa727b48925379aa42883d) )
	ROM_LOAD( "rodsnd.p2", 0x080000, 0x080000, CRC(a01e1e67) SHA1(4f86e0bb9bf4c1a4d0190eddfe7dd5bb89c519a2) )
ROM_END

ROM_START( m4hypvip )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "5p4hypervyper.bin", 0x0000, 0x010000, CRC(51ac9288) SHA1(1580079b6e710506ab03e1d8a89af65cd06cedd2) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "h.viper10p610m.bin", 0x0000, 0x010000, CRC(104b0c48) SHA1(ab4cdb596a0cfb877ed1b6bf801e4a759b53971f) )
	ROM_LOAD( "h6yc.p1", 0x0000, 0x010000, CRC(8faca3bc) SHA1(9d666371f1118ccb1a94bfc4e6c79b540a84842b) )
	ROM_LOAD( "h6yd.p1", 0x0000, 0x010000, CRC(862e7f5b) SHA1(2f5bbc31978fb9fd0ba17f0de220152da87cf06f) )
	ROM_LOAD( "h6yk.p1", 0x0000, 0x010000, CRC(51f43c88) SHA1(d6ee4f537d09b33e9b13c972e1bda01a28f54f8e) )
	ROM_LOAD( "h6ys.p1", 0x0000, 0x010000, CRC(4af914ff) SHA1(3d9b7c65ec1129ee64e3f4e14e43e4c39c76166b) )
	ROM_LOAD( "h6yy.p1", 0x0000, 0x010000, CRC(bed4b3bb) SHA1(7c592fbc6541c03777ff0498db90c575b3193222) )
	ROM_LOAD( "hv056c", 0x0000, 0x010000, CRC(91dcef99) SHA1(8fb6245fa8731b58799c0d2edc0e6c6942984a6f) )
	ROM_LOAD( "hv05_101", 0x0000, 0x010000, CRC(e1fa633d) SHA1(3f446c3396142631141cf85db507f3ae288847e3) )
	ROM_LOAD( "hv108c", 0x0000, 0x010000, CRC(4d40ebfe) SHA1(0e355fe5b185ba595c5040335956037b8ed21599) )
	ROM_LOAD( "hv10_101", 0x0000, 0x010000, CRC(57714454) SHA1(de99f5a66081191a7280c54e875fd17cc94e111b) )
	ROM_LOAD( "hv20_101", 0x0000, 0x010000, CRC(b2ab79c9) SHA1(fd097b5b062d725fa0607117d6b52be6cbf7e597) )
	ROM_LOAD( "hvyp10p", 0x0000, 0x010000, CRC(b4af635a) SHA1(420cdf3a6899e432d74e3b10a57414cbedc0913e) )
	ROM_LOAD( "hvyp56c", 0x0000, 0x010000, CRC(297d3cf8) SHA1(78f4de2ed69fb38b944a54d4d5927ff791e7876c) )
	ROM_LOAD( "hvypr206", 0x0000, 0x010000, CRC(e1d96b8c) SHA1(e21b1bdbca1bae41f0e7274e3521f99eb984759e) )
	ROM_LOAD( "hyp55", 0x0000, 0x010000, CRC(07bd7455) SHA1(0d0a017c90e8d28500594f55c9a60dfc08aff5c3) )
	ROM_LOAD( "hypr58c", 0x0000, 0x010000, CRC(d6028f8f) SHA1(54a3188ddb5196808a1161a0e1e6a8c1fe8bfde3) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "hvip_05_.8", 0x0000, 0x010000, CRC(625f1b9d) SHA1(f8dc0cde774f3fc4fb3d66d014ad47e9576c0f44) )
	ROM_LOAD( "hvip_10_.8", 0x0000, 0x010000, CRC(f91d7fec) SHA1(4c8130f9ce0ee3b14744e2b3cab79d4a65767e78) )
	ROM_LOAD( "hvip_20_.8", 0x0000, 0x010000, CRC(61a608c7) SHA1(1ed98c8bd90a3a789ba00b6b39f49e3aa0fcb1ca) )
	ROM_LOAD( "hypv_05_.4", 0x0000, 0x010000, CRC(246f171c) SHA1(7bbefb0cae57cf8097aa6d033df1a428e8bfe744) )
	ROM_LOAD( "hypv_10_.4", 0x0000, 0x010000, CRC(f85d21a1) SHA1(55ed92147335a1471b7b443f68dd700f579d21f3) )
	ROM_LOAD( "hypv_20_.4", 0x0000, 0x010000, CRC(27a0162b) SHA1(2d1342edbfa29c4f2ee1f1a825f3eeb0489fbaf5) )

ROM_END

ROM_START( m4hypclb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "hpcd.p1", 0x0000, 0x010000, CRC(7fac8944) SHA1(32f0f16ef6c4b99fe70464341a1ce226f6221122) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "hpcf.p1", 0x0000, 0x010000, CRC(2931a558) SHA1(2f7fe541edc502738dd6603435deaef1cb26a1e2) )
	ROM_LOAD( "hpcfd.p1", 0x0000, 0x010000, CRC(b127e577) SHA1(da034086bb92934f73d1a2be776f91462274479d) )
	ROM_LOAD( "hpcs.p1", 0x0000, 0x010000, CRC(55601e10) SHA1(78c3f13cd122e86ff8b7750b375c26e56c6b27c6) )
ROM_END

ROM_START( m4intcep )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ints.p1", 0x0000, 0x010000, CRC(6c6b61ca) SHA1(521cf68b9086baffe739d2b12dd28a13afb84b80) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "intx.p1", 0x0000, 0x010000, CRC(6e435db6) SHA1(17114fe54c74a140e116bec5a027219f7f5a70d6) )
	ROM_LOAD( "int11.bin", 0x0000, 0x010000, CRC(0beb156f) SHA1(9ac318a524c549df907a995e896cd90434634e72) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "int.chr", 0x0000, 0x000048, CRC(5915f545) SHA1(f8490a74aefe2a27d8e59b13dbd5fa78b8ab2166) )
ROM_END

ROM_START( m4jpgem )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cg4ad.p1", 0x0000, 0x010000, CRC(417c98c1) SHA1(2ce23e27742c418d5ebaa0f4f0597e29955ea57d) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cg4b.p1", 0x0000, 0x010000, CRC(c57cca63) SHA1(80a440912362d55cac6bc77b6ff6d6672af378c6) )
	ROM_LOAD( "cg4bd.p1", 0x0000, 0x010000, CRC(7604ea50) SHA1(3d6eee763bd21119ab52a2388229da076caf78a4) )
	ROM_LOAD( "cg4d.p1", 0x0000, 0x010000, CRC(87ea1087) SHA1(47f7c17fa3611745c881669ff50559e4b4386fd9) )
	ROM_LOAD( "cg4dk.p1", 0x0000, 0x010000, CRC(230284fb) SHA1(39ab2abdd8d3af4818e4e3738529f020055ba659) )
	ROM_LOAD( "cg4dy.p1", 0x0000, 0x010000, CRC(7d02342d) SHA1(097c9c9dc84bd00f1ddd64b1f9564f0cf7a9023f) )
	ROM_LOAD( "cg4k.p1", 0x0000, 0x010000, CRC(ba4ef5a8) SHA1(1673985aee634aa5c8129cc1239ce08fb9f5da2c) )
	ROM_LOAD( "cg4s.p1", 0x0000, 0x010000, CRC(f25eba0b) SHA1(250189b7fb8aa82a8696c3a0099eb13ec74eeb10) )
	ROM_LOAD( "cg4y.p1", 0x0000, 0x010000, CRC(237098d3) SHA1(9f54ed0d9ce37f3b4e6dca136fe4a12ba79c89f9) )
	ROM_LOAD( "cgt03ad.p1", 0x0000, 0x010000, CRC(88842c4a) SHA1(c86987b44f04cf28a6f68300e4345f635455d4bf) )
	ROM_LOAD( "cgt03b.p1", 0x0000, 0x010000, CRC(99634ce1) SHA1(9fe867b0619070f563fb72b4415e4a9263c808e7) )
	ROM_LOAD( "cgt03bd.p1", 0x0000, 0x010000, CRC(be984100) SHA1(dfa7d97f02dc988b7743a1f57ab08c406f712559) )
	ROM_LOAD( "cgt03d.p1", 0x0000, 0x010000, CRC(aba3a305) SHA1(9a0203f830a0a8c6013eb5824bd48373c589dcb5) )
	ROM_LOAD( "cgt03dk.p1", 0x0000, 0x010000, CRC(be9292b0) SHA1(0d7944ac647c8fd92530389d61f5c1eec0d2c8d1) )
	ROM_LOAD( "cgt03dr.p1", 0x0000, 0x010000, CRC(935fa628) SHA1(5dd93fd27d2e15606ba22bada1ecff85c4f4a8c3) )
	ROM_LOAD( "cgt03dy.p1", 0x0000, 0x010000, CRC(b83879b0) SHA1(b0664e1bd97b76b73c96a9e0d20d1a15707863ff) )
	ROM_LOAD( "cgt03k.p1", 0x0000, 0x010000, CRC(451a8f66) SHA1(e218db61fdaca6824abebe59ec7f8d0f595e2cfa) )
	ROM_LOAD( "cgt03r.p1", 0x0000, 0x010000, CRC(85dd3733) SHA1(10b8c4d147d4b534ce31394d5ba69806b83a297e) )
	ROM_LOAD( "cgt03s.p1", 0x0000, 0x010000, CRC(b516cbcd) SHA1(c04d32818f9f8772b2a945cf40075ce7844b936e) )
	ROM_LOAD( "cgt03y.p1", 0x0000, 0x010000, CRC(57937087) SHA1(489bcbe5598020c24357f4c7b4e9096bc6332aa3) )
	ROM_LOAD( "cgts.p1", 0x0000, 0x010000, CRC(2a6f4489) SHA1(e410dd49cca50b3c051815a1b4be4bf2dc55f1af) )
	ROM_LOAD( "cgu02ad.p1", 0x0000, 0x010000, CRC(eee268a6) SHA1(ebc0d1e14ff27c5497b7c4e90e6fafa58916c83b) )
	ROM_LOAD( "cgu02b.p1", 0x0000, 0x010000, CRC(7d05d069) SHA1(2a94b121528bf39939f5a8b36318c0073171997d) )
	ROM_LOAD( "cgu02bd.p1", 0x0000, 0x010000, CRC(d8fe05ec) SHA1(7e2de5c6ece6779d09daf23f3ab4b61817fad103) )
	ROM_LOAD( "cgu02d.p1", 0x0000, 0x010000, CRC(daaf1fe1) SHA1(f2606c454e191166d217c5f5c82e91794977384b) )
	ROM_LOAD( "cgu02dk.p1", 0x0000, 0x010000, CRC(0487c66b) SHA1(3be30181590e5f5d2181bc76da5fd49fe9796006) )
	ROM_LOAD( "cgu02dr.p1", 0x0000, 0x010000, CRC(68655b09) SHA1(df40c058172d960f4f9393343cf9271fc52c58c8) )
	ROM_LOAD( "cgu02dy.p1", 0x0000, 0x010000, CRC(5f1709d1) SHA1(36ae3cd57e5db956b8ef362043d5c63aea0da06a) )
	ROM_LOAD( "cgu02k.p1", 0x0000, 0x010000, CRC(90058f14) SHA1(0e73410253e422ff2d4182b034624ab8dd996cb8) )
	ROM_LOAD( "cgu02r.p1", 0x0000, 0x010000, CRC(8f1d071b) SHA1(caa05465a12ca7ab6df0dce458caefb40dad818a) )
	ROM_LOAD( "cgu02s.p1", 0x0000, 0x010000, CRC(1cff0517) SHA1(162651a1af6273ea49490d0809a30ee9b13c728e) )
	ROM_LOAD( "cgu02y.p1", 0x0000, 0x010000, CRC(a2468782) SHA1(5f9161cffc6d9ffe8c30c41434ab012c16a48dfd) )
	ROM_LOAD( "hge01.hex", 0x0000, 0x010000, CRC(be6bb0dd) SHA1(3a0550608c8738b92b48b7a12fb43fb82f52cdd7) )
	ROM_LOAD( "jagb.p1", 0x0000, 0x010000, CRC(75b9a4b6) SHA1(ecade0921cd535ee7f1b67767fa7d5ab3cd45b2c) )
	ROM_LOAD( "jagd.p1", 0x0000, 0x010000, CRC(c7546004) SHA1(31bdbd6b681a3a2b13f380f2807691c0b0fec83e) )
	ROM_LOAD( "jagdk.p1", 0x0000, 0x010000, CRC(313f7a1f) SHA1(358a33878ca70f2bcdb1d5d79c39e357586ebe8b) )
	ROM_LOAD( "jagdy.p1", 0x0000, 0x010000, CRC(d105a41e) SHA1(365e382683362c815461801753fb03e2f084de65) )
	ROM_LOAD( "jags.p1", 0x0000, 0x010000, CRC(dd93f084) SHA1(5cb25b3beb6d7a7b83227a6bb8382cfbcc285887) )
	ROM_LOAD( "jagy.p1", 0x0000, 0x010000, CRC(08d510ca) SHA1(b79c9fe8dc17152f3e8c601c27515beff1d67219) )
	ROM_LOAD( "jg3ad.p1", 0x0000, 0x010000, CRC(501bb879) SHA1(a97519042b4a4ed03efbcad9f11f279184dec847) )
	ROM_LOAD( "jg3b.p1", 0x0000, 0x010000, CRC(e568ae84) SHA1(9126f9b45633e7eb44626aa0ab40784c62870c8a) )
	ROM_LOAD( "jg3bd.p1", 0x0000, 0x010000, CRC(435d5d28) SHA1(1ea48323f48edc20ce1c28e4e7080e0824e73d3c) )
	ROM_LOAD( "jg3d.p1", 0x0000, 0x010000, CRC(774f9d41) SHA1(c99e9a46b1216f430007b5ebbf942899b5e691f9) )
	ROM_LOAD( "jg3dk.p1", 0x0000, 0x010000, CRC(c422e514) SHA1(172b25bf75a529b555e328cef77a3340609d818b) )
	ROM_LOAD( "jg3dy.p1", 0x0000, 0x010000, CRC(5d1c886f) SHA1(b49ab97ba6cdc810e7baa520ffad25f54c0d8412) )
	ROM_LOAD( "jg3k.p1", 0x0000, 0x010000, CRC(8e7985ae) SHA1(8c8de22aab2508b2317d5edde779d54ebe67ac92) )
	ROM_LOAD( "jg3s.p1", 0x0000, 0x010000, CRC(91945adc) SHA1(d80321fc4c2e67461d69df2164e3e290caa905bc) )
	ROM_LOAD( "jg3y.p1", 0x0000, 0x010000, CRC(bf96ad55) SHA1(48d828398f32c3ddfafeb84cfd777f8e668df1b3) )
	ROM_LOAD( "jg8b.p1", 0x0000, 0x010000, CRC(f2e3d009) SHA1(90c85f9a300d157d560b08ccabfe79f826780d74) )
	ROM_LOAD( "jg8c.p1", 0x0000, 0x010000, CRC(cc24cf15) SHA1(0c4c28633f33c78570f5da17c64c2e90bf3d5cd0) )
	ROM_LOAD( "jg8d.p1", 0x0000, 0x010000, CRC(58eff94c) SHA1(9acde535ad808789233876dd8076c03a8d56a9e7) )
	ROM_LOAD( "jg8db.p1", 0x0000, 0x010000, CRC(3006a36a) SHA1(37297cae02c1fd5308ba9935537b35c565374a07) )
	ROM_LOAD( "jg8dk.p1", 0x0000, 0x010000, CRC(199401d7) SHA1(33eef070e437386c7ad0d834b40353047f1a6a6f) )
	ROM_LOAD( "jg8dy.p1", 0x0000, 0x010000, CRC(ead58bed) SHA1(cd0e151c843f5268edddb2f82555201deccac65a) )
	ROM_LOAD( "jg8k.p1", 0x0000, 0x010000, CRC(f5b14363) SHA1(f0ace838cc0d0c262006bb514eff75903d92d679) )
	ROM_LOAD( "jg8s.p1", 0x0000, 0x010000, CRC(8cdd650a) SHA1(c4cb87513f0d7986e158b3c5ab1f034c8ba933a9) )
	ROM_LOAD( "jgtad.p1", 0x0000, 0x010000, CRC(90e10b6c) SHA1(548c7537829ca9395cac460ccf76e0d566898e44) )
	ROM_LOAD( "jgtb.p1", 0x0000, 0x010000, CRC(5f343a43) SHA1(033824e93b1fcd2f7c5f27a573a728747ef7b21a) )
	ROM_LOAD( "jgtbd.p1", 0x0000, 0x010000, CRC(50ba2771) SHA1(f487ed2eeff0369e3fa718de68e3ba4912fd7576) )
	ROM_LOAD( "jgtd.p1", 0x0000, 0x010000, CRC(2625da4a) SHA1(b1f9d22a46bf20283c5735fce5768d9cef299f59) )
	ROM_LOAD( "jgtdk.p1", 0x0000, 0x010000, CRC(94220901) SHA1(f62c9a59bb419e98f7de358f7fee072b08aab3f5) )
	ROM_LOAD( "jgtdr.p1", 0x0000, 0x010000, CRC(5011d1e3) SHA1(85e5d28d26449a951704698a4419cd2c0f7dd9c4) )
	ROM_LOAD( "jgtdy.p1", 0x0000, 0x010000, CRC(6397e38c) SHA1(ed0c165a5ab27524374c540fd9bdcfd41ce8096c) )
	ROM_LOAD( "jgtk.p1", 0x0000, 0x010000, CRC(cb30d644) SHA1(751fc5c7ae07e64c07a3e89e74ace09dd2b99a02) )
	ROM_LOAD( "jgtr.p1", 0x0000, 0x010000, CRC(6224a93d) SHA1(6d36b64c2eaddf122a6a7e798b5efb44ec2e5b45) )
	ROM_LOAD( "jgts.p1", 0x0000, 0x010000, CRC(0e3810a7) SHA1(cf840bd84eba65d9dec2d6821a48112b6f2f9bca) )
	ROM_LOAD( "jgty.p1", 0x0000, 0x010000, CRC(84830d1f) SHA1(a4184a5bd08393c35f22bc05315377bff74f666c) )
	ROM_LOAD( "jgu02ad.p1", 0x0000, 0x010000, CRC(ccec7d40) SHA1(75cc1a0dfda9592e35c24c030e04a768871a9e41) )
	ROM_LOAD( "jgu02b.p1", 0x0000, 0x010000, CRC(daf0ebe3) SHA1(2e73f7b8171c0be7d06bf6da22e0395d5241b043) )
	ROM_LOAD( "jgu02bd.p1", 0x0000, 0x010000, CRC(faf0100a) SHA1(c97b8eadfd473650ec497c7caa98e8efc59ecb6f) )
	ROM_LOAD( "jgu02d.p1", 0x0000, 0x010000, CRC(b46e3f66) SHA1(1ede9d794effbc8cc9f097a06b7df4023d3d47ba) )
	ROM_LOAD( "jgu02dk.p1", 0x0000, 0x010000, CRC(cdbf0041) SHA1(fb90d4f8112e169dab16f78fdea9d1b5306e05d6) )
	ROM_LOAD( "jgu02dr.p1", 0x0000, 0x010000, CRC(41f1f723) SHA1(96623358e6dc450dbdc769d176703917f67e767a) )
	ROM_LOAD( "jgu02dy.p1", 0x0000, 0x010000, CRC(2023388d) SHA1(c9f1abaa12c78ac61304966b46044b82ea2ea3ff) )
	ROM_LOAD( "jgu02k.p1", 0x0000, 0x010000, CRC(615029e8) SHA1(aecba0fad8c74fef9a4d04e95df961432ac999b7) )
	ROM_LOAD( "jgu02r.p1", 0x0000, 0x010000, CRC(4bc55daa) SHA1(996f23bd66a4ef6ad8f77a28dc6ee67d9a293248) )
	ROM_LOAD( "jgu02s.p1", 0x0000, 0x010000, CRC(f8abd287) SHA1(906d2817f73ea21cf830b0bd9a1938d344cc0341) )
	ROM_LOAD( "jgu02y.p1", 0x0000, 0x010000, CRC(9b4325a8) SHA1(3d7c54691ed4d596acacec97e452a66b324957db) )
	ROM_LOAD( "rrh01ad.p1", 0x0000, 0x010000, CRC(d4f21930) SHA1(cba034b42a3587c0e173bc06d80142d7e494c849) )
	ROM_LOAD( "rrh01b.p1", 0x0000, 0x010000, CRC(c85f9099) SHA1(f3c8f79c2e0cc58024202564761f4935f5d241b1) )
	ROM_LOAD( "rrh01bd.p1", 0x0000, 0x010000, CRC(e2ee747a) SHA1(7f6cb93e3cbe4a2dd97d1ad15d17fa4f2f0a4b12) )
	ROM_LOAD( "rrh01c.p1", 0x0000, 0x010000, CRC(00dfced2) SHA1(c497cb9835dca0d67f5ec6b6b1321a7b92612c9a) )
	ROM_LOAD( "rrh01d.p1", 0x0000, 0x010000, CRC(aef7ddbd) SHA1(8db2f1dbc11af7ef4357a90a77838c01588f8108) )
	ROM_LOAD( "rrh01dk.p1", 0x0000, 0x010000, CRC(56206f0a) SHA1(3bdb5824ab6d748eb83b56f08cf2d1074a94b38a) )
	ROM_LOAD( "rrh01dr.p1", 0x0000, 0x010000, CRC(8f622573) SHA1(9bbfcb2f3cf5f6edd2c6222b25c4971a59d2c235) )
	ROM_LOAD( "rrh01dy.p1", 0x0000, 0x010000, CRC(d89136fd) SHA1(40fd0978bc76d81bfb5dc2f1e4a0c1c95b7c4e00) )
	ROM_LOAD( "rrh01k.p1", 0x0000, 0x010000, CRC(da4a08c9) SHA1(3a86c0a543a7192680663b465ddfd1fa338cfec5) )
	ROM_LOAD( "rrh01r.p1", 0x0000, 0x010000, CRC(fb45c547) SHA1(8d9c35c47c0f03c9dc6727fc5f952d64e25336f7) )
	ROM_LOAD( "rrh01s.p1", 0x0000, 0x010000, CRC(dea2f376) SHA1(92f43c75950553d9b76af8179192d106de95fc03) )
	ROM_LOAD( "rrh01y.p1", 0x0000, 0x010000, CRC(27014453) SHA1(febc118fcb8f048806237b38958c02d02b9f2874) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
//  ROM_LOAD( "cg401.hex", 0x0000, 0x080000, CRC(e4addde8) SHA1(6b84de51cc5195d551e0787ff92bfa4371ab27a3) )
	ROM_LOAD( "cg4snd.p1", 0x000000, 0x080000, CRC(e4addde8) SHA1(6b84de51cc5195d551e0787ff92bfa4371ab27a3) )
	ROM_LOAD( "jagsnd.p1", 0x080000, 0x080000, CRC(7488f7a7) SHA1(d581e9d6b5052ee8fee353a83e9d9031443d060a) )
ROM_END

ROM_START( m4jpgemc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "gtc01ad.p1", 0x0000, 0x010000, CRC(e4f61afd) SHA1(36e007275cce0565c50b150dba4c8df272cd4c2e) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "gtc01b.p1", 0x0000, 0x010000, CRC(e4e27c71) SHA1(b46da3f00134d3a2f17ceb35529adb598c75ee4e) )
	ROM_LOAD( "gtc01bd.p1", 0x0000, 0x010000, CRC(d2ea77b7) SHA1(4f66fa8d692f26ffa92ae3aff4f43257fc573e93) )
	ROM_LOAD( "gtc01c.p1", 0x0000, 0x010000, CRC(21c4c4f7) SHA1(f8a2de8453c095db80ff19018a72b15b949bace9) )
	ROM_LOAD( "gtc01d.p1", 0x0000, 0x010000, CRC(b6a3d2c3) SHA1(48cbd3cf14b8f8e9ecbee1f351e781506ca6c17f) )
	ROM_LOAD( "gtc01dk.p1", 0x0000, 0x010000, CRC(70cf4790) SHA1(b6aac10fd9ad3aafa277e6de58db3f1a28501529) )
	ROM_LOAD( "gtc01dr.p1", 0x0000, 0x010000, CRC(4cb11885) SHA1(288da6617868f7d082fc72f50c13671fdaf9442a) )
	ROM_LOAD( "gtc01dy.p1", 0x0000, 0x010000, CRC(713dec4a) SHA1(3cb1e3f5299a5145addaa677022e7d9a164072d9) )
	ROM_LOAD( "gtc01k.p1", 0x0000, 0x010000, CRC(fb5102ec) SHA1(36c9c50c8266707542b00cfc55f57ec454401f70) )
	ROM_LOAD( "gtc01r.p1", 0x0000, 0x010000, CRC(e311ca39) SHA1(602aee41400793f46f47ac9c8a9e6ce7f2d5f203) )
	ROM_LOAD( "gtc01s.p1", 0x0000, 0x010000, CRC(af33337b) SHA1(97d28e224b73baa9d6d7b0c309385f57b6dd5d9b) )
	ROM_LOAD( "gtc01y.p1", 0x0000, 0x010000, CRC(59e8557a) SHA1(8493b160427c21bbb2834c01b39f8a6a8b221bb3) )
	ROM_LOAD( "hge01ad.p1", 0x0000, 0x010000, CRC(bb201074) SHA1(eb954d165c2d96f952439277d255e3ec3326ada3) )
	ROM_LOAD( "hge01b.p1", 0x0000, 0x010000, CRC(d7ad2482) SHA1(ed90c4531608e66b14eb1079e85ea59573adf451) )
	ROM_LOAD( "hge01bd.p1", 0x0000, 0x010000, CRC(3ea0f524) SHA1(1967e5ec14c41c4140c7c39b07085f740c2d1f01) )
	ROM_LOAD( "hge01c.p1", 0x0000, 0x010000, CRC(498de7bf) SHA1(32dc31852fa69f7d2dd47bbcef695fcf5337f01f) )
	ROM_LOAD( "hge01d.p1", 0x0000, 0x010000, CRC(be6bb0dd) SHA1(3a0550608c8738b92b48b7a12fb43fb82f52cdd7) )
	ROM_LOAD( "hge01dk.p1", 0x0000, 0x010000, CRC(80904843) SHA1(8030def4c0e80ac8f28452662487dbfc21a761ee) )
	ROM_LOAD( "hge01dr.p1", 0x0000, 0x010000, CRC(d89b36b7) SHA1(22c334f1aa314ff288c65eb01ad0415db8e05b15) )
	ROM_LOAD( "hge01dy.p1", 0x0000, 0x010000, CRC(18ca3ae3) SHA1(ebb434a060564d3a1bc51876257729650e2903a6) )
	ROM_LOAD( "hge01k.p1", 0x0000, 0x010000, CRC(4161f733) SHA1(b551bb278666790f0c293c76d5c3fabf8f4d368e) )
	ROM_LOAD( "hge01r.p1", 0x0000, 0x010000, CRC(6dc8dc70) SHA1(e96fc4284ece65f76d5e9bd06c4a002de65bf4da) )
	ROM_LOAD( "hge01s.p1", 0x0000, 0x010000, CRC(b79f8c42) SHA1(7d8b3352fbd9a80b86f5a8b22833d6f5c4b9854b) )
	ROM_LOAD( "hge01y.p1", 0x0000, 0x010000, CRC(a96db093) SHA1(17520306112cee6f082829811e1f8c432c6aa354) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END

ROM_START( m4jpjmp )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "vjcs.p1", 0x0000, 0x010000, CRC(90280752) SHA1(bc2fcefc00adbae9ca2e116108b53ab932ab57b2) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "jj.chr", 0x0000, 0x000048, CRC(abf2340a) SHA1(6cfeb84db61e647da0a46faa87fa003a34b46f5c) )
ROM_END

ROM_START( m4jwlcwn )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cje0.8", 0x0000, 0x020000, CRC(2074bf61) SHA1(d84201fb7d2590b16816e0369e89789d02088a6d) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cje10ad.p1", 0x0000, 0x020000, CRC(b245d706) SHA1(704cc3bcae099c71dcc2bd96095cb4b48857a23a) )
	ROM_LOAD( "cje10b.p1", 0x0000, 0x020000, CRC(0ef3387b) SHA1(852bdac93fb448089633133a546bdb8da4d6887b) )
	ROM_LOAD( "cje10bd.p1", 0x0000, 0x020000, CRC(3f5f79c3) SHA1(a89502dae9843fddd471fd5eb1d39e84d7124c7e) )
	ROM_LOAD( "cje10c.p1", 0x0000, 0x020000, CRC(39b98569) SHA1(f349a309b716250545137716acc899b42a358037) )
	ROM_LOAD( "cje10d.p1", 0x0000, 0x020000, CRC(73e8330d) SHA1(d4c169bf27cd88e66e90e1ed8d7646561c5e7338) )
	ROM_LOAD( "cje10dk.p1", 0x0000, 0x020000, CRC(7598d195) SHA1(ec575bea5c6aa1c3b6fe997b612b78f2af506180) )
	ROM_LOAD( "cje10dr.p1", 0x0000, 0x020000, CRC(c6976aa4) SHA1(94ee1f6355bec27bf91a813e0188ba0e0e3c7037) )
	ROM_LOAD( "cje10dy.p1", 0x0000, 0x020000, CRC(f27bf16b) SHA1(f6c6986ed96c9fca90f94921fb984e58425179b9) )
	ROM_LOAD( "cje10k.p1", 0x0000, 0x020000, CRC(4434902d) SHA1(31c7be1235cdfd00099d1e09644a0f76fc7a26f7) )
	ROM_LOAD( "cje10r.p1", 0x0000, 0x020000, CRC(f73b2b1c) SHA1(bd9ee8047b4b0cc30b92d5460d34fa2628a72dde) )
	ROM_LOAD( "cje10s.p1", 0x0000, 0x020000, CRC(5f3b72b7) SHA1(8faf0de0282a67c88170c13856b8816c38396e19) )
	ROM_LOAD( "cje10y.p1", 0x0000, 0x020000, CRC(c3d7b0d3) SHA1(5c314fcaab08e7a551e6ad7a2e0fa08a03d4c80d) )
	ROM_LOAD( "cjh10ad.p1", 0x0000, 0x020000, CRC(db6a3f77) SHA1(9986150dd84839ea726405dec1b731b0477d1d29) )
	ROM_LOAD( "cjh10b.p1", 0x0000, 0x020000, CRC(67009ea4) SHA1(84e97bcb23ba876e33976a6081f24561e0b3faac) )
	ROM_LOAD( "cjh10bd.p1", 0x0000, 0x020000, CRC(567091b2) SHA1(8b7f33802e03d7e4ede06d345afedf8631f69412) )
	ROM_LOAD( "cjh10c.p1", 0x0000, 0x020000, CRC(504a23b6) SHA1(b2268c2ef8387023da7d66682ed63ebbc8b8b635) )
	ROM_LOAD( "cjh10d.p1", 0x0000, 0x020000, CRC(1a1b95d2) SHA1(2393d0ab5758da6eabd3f61fe45272c1aab71807) )
	ROM_LOAD( "cjh10dk.p1", 0x0000, 0x020000, CRC(1cb739e4) SHA1(9dc1b5475e6d397d1a90a55225c4aa77cb6a19bd) )
	ROM_LOAD( "cjh10dr.p1", 0x0000, 0x020000, CRC(afb882d5) SHA1(116a43a7e46810d11b5fcc56960bd3706e3f8e25) )
	ROM_LOAD( "cjh10dy.p1", 0x0000, 0x020000, CRC(9b54191a) SHA1(ae01b3842ab83572abc4966e94956623103b2bda) )
	ROM_LOAD( "cjh10k.p1", 0x0000, 0x020000, CRC(2dc736f2) SHA1(eae27aad3faca98c3dc0873cd00f3babe4d67302) )
	ROM_LOAD( "cjh10r.p1", 0x0000, 0x020000, CRC(9ec88dc3) SHA1(01016ca0785a11e800fbddb7a7cc7e4be6ffdb09) )
	ROM_LOAD( "cjh10s.p1", 0x0000, 0x020000, CRC(eb22d1bb) SHA1(4a8c19a8c71ef018f1fae146ba60632a94d895fc) )
	ROM_LOAD( "cjh10y.p1", 0x0000, 0x020000, CRC(aa24160c) SHA1(2014420ce92297dbe1ef286d801c25aa67976b2e) )
	ROM_LOAD( "jewel15g", 0x0000, 0x020000, CRC(bf3b8b63) SHA1(1ee91745438b9458ffbd43380bf9c6fd784fd054) )
	ROM_LOAD( "jewel15t", 0x0000, 0x020000, CRC(5828fd3b) SHA1(be95d5c3c9729547dcb0815c868e8d654826e34e) )
	ROM_LOAD( "jewelinthecrowno8.bin", 0x0000, 0x020000, CRC(5f3b72b7) SHA1(8faf0de0282a67c88170c13856b8816c38396e19) )
	ROM_LOAD( "jitc2010", 0x0000, 0x020000, CRC(1c946895) SHA1(43215c4099197a67bf0a6100e3dc3b81759cfc76) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "jc__x___.4_1", 0x0000, 0x020000, CRC(5bf060ca) SHA1(a13795b145ff230437764f5414ec443e8fe4d783) )
	ROM_LOAD( "jc__x__c.3_1", 0x0000, 0x020000, CRC(b5e11e92) SHA1(87d7febf350ff7e4175bb6b8544181de66415e12) )
	ROM_LOAD( "jc__xa_4.3_1", 0x0000, 0x020000, CRC(e6abb23e) SHA1(05b9286c4c1ec6364fd57d412336192ca61325a9) )
	ROM_LOAD( "jc__xa_5.1_1", 0x0000, 0x020000, CRC(09f897c7) SHA1(5f6ad23f92b9fa4fdde57dd80317e1e998de9d54) )
	ROM_LOAD( "jc__xa_8.4_1", 0x0000, 0x020000, CRC(27346ae8) SHA1(0fa13205e45e8dab0e1a25e6492ff2987633eb0f) )
	ROM_LOAD( "jc_xx__c.3_1", 0x0000, 0x020000, CRC(0787fd51) SHA1(90fc71e0ea9b79d3296611c1e6f720150e17d51b) )


	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cjesnd.p1", 0x000000, 0x080000, CRC(a2f20c95) SHA1(874b22850732514a26448cee8e0b68f8d042a7c7) )
	ROM_LOAD( "cjesnd.p2", 0x080000, 0x080000, CRC(3dcb7c38) SHA1(3c0e91f4d2ea9e6b25a01702c6f6fdc7cc2e0b65) )
	ROM_LOAD( "jewelp2",   0x080000, 0x080000, CRC(84996453) SHA1(74fe377545503f1b8da9b8998514811f0c1c037c) ) // alt cje

	ROM_LOAD( "cjhsnd.p1", 0x000000, 0x080000, CRC(4add4eca) SHA1(98dc644d3f3d67e764c215bd26ae010e4b23c738) )
	ROM_LOAD( "cjhsnd.p2", 0x080000, 0x080000, CRC(5eec51f0) SHA1(834d9d13f79a61c51db9df067064f64a15c956a9) )
ROM_END

ROM_START( m4jok300 )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cjo", 0x0000, 0x020000, CRC(386e99db) SHA1(5bb0b513ef63ffaedd98b8e9e7206658fe784fda) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASEFF )
	/* missing? */
ROM_END

ROM_START( m4jokmil )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cjm03.epr", 0x0000, 0x020000, CRC(e5e4986e) SHA1(149b950a739ad308f7759927c344de8193ce67c5) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASEFF )
	/* missing? */
ROM_END

ROM_START( m4jolgem )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "gem07s.p1", 0x0000, 0x020000, CRC(945ad0d2) SHA1(d636bc41a4f887d24affc0f5b644c5d5351cf0df) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "gem05s", 0x0000, 0x020000, CRC(b7ceafc2) SHA1(b66d846da5ff20df912d31695eaef146dbbe759e) )
	ROM_LOAD( "gem06ad.p1", 0x0000, 0x020000, CRC(a3270974) SHA1(59992779415ff20b8589843510099b77c9b157fd) )
	ROM_LOAD( "gem06b.p1", 0x0000, 0x020000, CRC(188ea295) SHA1(b8a2bdede4478a582f041fd3ff84b5563feaedd3) )
	ROM_LOAD( "gem06bd.p1", 0x0000, 0x020000, CRC(4d1b6a6f) SHA1(6cb733b3f8e011e1d12a9aee25577e2bee7deb1a) )
	ROM_LOAD( "gem06d.p1", 0x0000, 0x020000, CRC(b68ce7e3) SHA1(ec4400f7c2bb79204fdec1d061801afdd4de70f2) )
	ROM_LOAD( "gem06dh.p1", 0x0000, 0x020000, CRC(0dae55fa) SHA1(3f8a23e4efc0c059852801882865baa6654a4eb3) )
	ROM_LOAD( "gem06dr.p1", 0x0000, 0x020000, CRC(d7f5b4d6) SHA1(18ff01d9f56d772863698bc72d8e9ec61a9ac9d8) )
	ROM_LOAD( "gem06dy.p1", 0x0000, 0x020000, CRC(19771aa3) SHA1(e785196e55353952000d805d23502bc220b2c747) )
	ROM_LOAD( "gem06h.p1", 0x0000, 0x020000, CRC(583b9d00) SHA1(6cd43f74c9d4d9d9a4b995277313049a353e6d80) )
	ROM_LOAD( "gem06r.p1", 0x0000, 0x020000, CRC(82607c2c) SHA1(a30366773305aacbba96f0c211a67448dd8a2702) )
	ROM_LOAD( "gem06s.p1", 0x0000, 0x020000, CRC(e0d82632) SHA1(35a51394a68311d03800db671fbd634bae087e86) )
	ROM_LOAD( "gem06y.p1", 0x0000, 0x020000, CRC(4ce2d259) SHA1(bca3e5f79048965bc5c0e80565bbb5ebeefeac87) )
	ROM_LOAD( "gem07ad.p1", 0x0000, 0x020000, CRC(21496739) SHA1(05771636542275ecae1cd45bc248ed104a422f03) )
	ROM_LOAD( "gem07b.p1", 0x0000, 0x020000, CRC(70ca3435) SHA1(ff631f9adea268c1160646bacca976c069751ba8) )
	ROM_LOAD( "gem07bd.p1", 0x0000, 0x020000, CRC(cf750422) SHA1(01c275c90f33afe4cd54c1b9c4963b6c5e66596b) )
	ROM_LOAD( "gem07d.p1", 0x0000, 0x020000, CRC(dec87143) SHA1(080483f5500bedac6dfbd252d2ac42e23c1b5ac5) )
	ROM_LOAD( "gem07dh.p1", 0x0000, 0x020000, CRC(8fc03bb7) SHA1(dc240714d59f9055ce9c098f7cd60f06a92a0a28) )
	ROM_LOAD( "gem07dr.p1", 0x0000, 0x020000, CRC(559bda9b) SHA1(91ec97aab73717eb401672ca4e4b58a87a71f099) )
	ROM_LOAD( "gem07dy.p1", 0x0000, 0x020000, CRC(9b1974ee) SHA1(7d6b7ee79ac4401d7e65aa2240ea01cad26eb881) )
	ROM_LOAD( "gem07h.p1", 0x0000, 0x020000, CRC(307f0ba0) SHA1(518acd033599b188e972e51753ac610623038aca) )
	ROM_LOAD( "gem07r.p1", 0x0000, 0x020000, CRC(ea24ea8c) SHA1(8d3598e44d219f1d66b63bf3ca4062eb84ecbe60) )
	ROM_LOAD( "gem07y.p1", 0x0000, 0x020000, CRC(24a644f9) SHA1(b97b83ce0ebf315529efe6d5be051ad71f2e648a) )
	ROM_LOAD( "gms04ad.p1", 0x0000, 0x020000, CRC(afd91cf8) SHA1(43f2549d81d9a414c5e2e049fe62a6939ba48943) )
	ROM_LOAD( "gms04b.p1", 0x0000, 0x020000, CRC(a11cd9fd) SHA1(16b23c8091da2ada9823f204e7cd5f02b68d37c3) )
	ROM_LOAD( "gms04bd.p1", 0x0000, 0x020000, CRC(41e57fe3) SHA1(77322a0988422df6feb5f6871758fbdcf410dae5) )
	ROM_LOAD( "gms04d.p1", 0x0000, 0x020000, CRC(0f1e9c8b) SHA1(39661143619230ae64c70ee6d6e6f553e47691a1) )
	ROM_LOAD( "gms04dh.p1", 0x0000, 0x020000, CRC(01504076) SHA1(93e44465d74abdbdf5f651e0e8b96ea5b05c1597) )
	ROM_LOAD( "gms04dk.p1", 0x0000, 0x020000, CRC(68041a6b) SHA1(03a246ff17001fd937d5556ff8da7165cb95b67c) )
	ROM_LOAD( "gms04dr.p1", 0x0000, 0x020000, CRC(db0ba15a) SHA1(85880bf152309b4e81b066a62ace827b899236cd) )
	ROM_LOAD( "gms04dy.p1", 0x0000, 0x020000, CRC(15890f2f) SHA1(760806e506d1ff406fab2e50f75428c9b9762804) )
	ROM_LOAD( "gms04h.p1", 0x0000, 0x020000, CRC(e1a9e668) SHA1(8b732c52dc221934f57a369e8a20ac2df1aa562c) )
	ROM_LOAD( "gms04k.p1", 0x0000, 0x020000, CRC(88fdbc75) SHA1(b1842e2f29e2f3d81c6679a407827601955f02f4) )
	ROM_LOAD( "gms04r.p1", 0x0000, 0x020000, CRC(3bf20744) SHA1(9b8d1273545abbf5a7e8e0735e4f23ce024505b9) )
	ROM_LOAD( "gms04s.p1", 0x0000, 0x020000, CRC(93f25eef) SHA1(d4bfb2787df10dd09281d33341fbf666850ac23d) )
	ROM_LOAD( "gms04y.p1", 0x0000, 0x020000, CRC(f570a931) SHA1(dd50e4626feb9a3e6f0868fa9030204c737f1567) )
	ROM_LOAD( "gms05ad.p1", 0x0000, 0x020000, CRC(77d4829c) SHA1(45a240cc2aa3829160854274c928d20655966087) )
	ROM_LOAD( "gms05b.p1", 0x0000, 0x020000, CRC(b929f905) SHA1(028d74687f5c6443d84e84a03666278b9df8e657) )
	ROM_LOAD( "gms05bd.p1", 0x0000, 0x020000, CRC(99e8e187) SHA1(cbb8d3403e6d21fce45b05d1889d17d0355857b4) )
	ROM_LOAD( "gms05d.p1", 0x0000, 0x020000, CRC(172bbc73) SHA1(932907d70b18d25fa912fe895602b0adc52b8e6c) )
	ROM_LOAD( "gms05dh.p1", 0x0000, 0x020000, CRC(d95dde12) SHA1(e509f65b8f0575193002dd0906b1751cae9352f7) )
	ROM_LOAD( "gms05dk.p1", 0x0000, 0x020000, CRC(b009840f) SHA1(5c8f7c081e577642fc7738f7d6fc816155ffc99b) )
	ROM_LOAD( "gms05dr.p1", 0x0000, 0x020000, CRC(03063f3e) SHA1(83995208d6c392eacee794c75075296d87e07b13) )
	ROM_LOAD( "gms05dy.p1", 0x0000, 0x020000, CRC(cd84914b) SHA1(8470ba93c518893771ee45190bce5f6f93df7b68) )
	ROM_LOAD( "gms05h.p1", 0x0000, 0x020000, CRC(f99cc690) SHA1(c3cf8aaf8376e9d4eff37afdd818802f9ec4fe64) )
	ROM_LOAD( "gms05k.p1", 0x0000, 0x020000, CRC(90c89c8d) SHA1(4b8e23c9a6bd85563be041d8e95175dc4c39b8e7) )
	ROM_LOAD( "gms05r.p1", 0x0000, 0x020000, CRC(23c727bc) SHA1(0187a14a789f018e6161f2ae160f82c87d03b6a8) )
	ROM_LOAD( "gms05s.p1", 0x0000, 0x020000, CRC(1b830e70) SHA1(eb053a629bd7854759d14acd9793f7eb545fc008) )
	ROM_LOAD( "gms05y.p1", 0x0000, 0x020000, CRC(ed4589c9) SHA1(899622c22d29c0ef05bf562176943c9749e236a2) )
	ROM_LOAD( "jgem15g", 0x0000, 0x020000, CRC(8288eb80) SHA1(bfb1004b49914b6ae1b0608c9e5c61efe4635ba3) )
	ROM_LOAD( "jgem15t", 0x0000, 0x020000, CRC(248f0b95) SHA1(89257c583d6540ffc0fa39f7cb31a87ec8f1f45b) )
	ROM_LOAD( "jjem0", 0x0000, 0x020000, CRC(9b54a881) SHA1(3b1bfacf8fe295c771c558154fe2fca70f049df0) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "jgs_xa_x.1_0", 0x0000, 0x020000, CRC(7ac16252) SHA1(b01b2333e1e99f9404a7e0ac80e5e8ee834ec39d) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "jolly1.hex", 0x000000, 0x080000, CRC(de0edae5) SHA1(e3e21e28ae5e838bd6eacc7cf7b20204d7b0327d) )
	ROM_LOAD( "jolly2.hex", 0x080000, 0x080000, CRC(08ae81a2) SHA1(6459a694cd820f1a55b636f7c5c77674d3fe4bdb) )
ROM_END



ROM_START( m4joljok )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "jlyjk16l.bin", 0x0000, 0x010000, CRC(a0af938f) SHA1(484e075c3b9199d0d9e20185c6fa0be560845029) )
ROM_END


ROM_START( m4joljokd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "djj15.bin", 0x0000, 0x010000, CRC(155cb134) SHA1(c1026effeceba131df9681afd91ccd6fb43b738a) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sdld.bin", 0x0000, 0x080000, CRC(9b035fa6) SHA1(51b7e5bc3abdf4f1beba2347146a91a2b3f4de35) )
ROM_END

ROM_START( m4joljokh )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "jollyjokerhungarian.bin", 0x0000, 0x010000, CRC(85b6a406) SHA1(e277f9d3b62faead04d65efbc06de7f4a50ae38d) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "jollyjokerhungariansnd.bin", 0x0000, 0x080000, CRC(93460383) SHA1(2b179a1dde09ebdfe8c84641899df7be87d443e5) )
ROM_END


ROM_START( m4joltav )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tavs.p1", 0x0000, 0x010000, CRC(12bdf083) SHA1(c1b73bfd05ae6128d1760083383805fdaf328003) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "jto20__1.1", 0x0000, 0x010000, CRC(4790c4ec) SHA1(2caab4ccc91158f6b76817e76c1d092ef1a79cd9) )
	ROM_LOAD( "jto20d_1.1", 0x0000, 0x010000, CRC(dff09dfc) SHA1(c13f31f7d96075f7c94ae5e79fc1f9b8ce7e4c80) )
ROM_END

ROM_START( m4jpmcla )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "jcv2.epr",  0x00000, 0x10000,  CRC(da095666) SHA1(bc7654dc9da1f830a43f925db8079f27e18bb61e) ) // == old timer

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "jcchr.chr", 0x0000, 0x000048, CRC(e370e271) SHA1(2b712dd3590c31356e8b0b62ffc64ff8ce444f73) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sjcv2.snd", 0x0000, 0x080000, CRC(f247ba83) SHA1(9b173503e63a4a861d1380b2ab1fe14af1a189bd) )
	ROM_LOAD( "sjcv22.snd", 0x080000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END

ROM_START( m4lastrp )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "uvsk.p1", 0x0000, 0x020000, CRC(7827bfa9) SHA1(720d9793e97f2e11c1c9b18e3b4fa6ec7e29250a) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "lvs", 0x0000, 0x020000, CRC(dcb0dc80) SHA1(6045b332eb4af09f6e0a669ea0b78ef4ac389ac2) )
	ROM_LOAD( "vsgs", 0x0000, 0x020000, CRC(126365e3) SHA1(1e648b7a8cb1ff49e43e2fdc30f482b2b73ed6d7) )
	ROM_LOAD( "vspa20st", 0x0000, 0x010000, CRC(267388eb) SHA1(2621724ebdd5031fc513692ff90989bf3b6115d1) )
	ROM_LOAD( "vstr2010", 0x0000, 0x020000, CRC(126365e3) SHA1(1e648b7a8cb1ff49e43e2fdc30f482b2b73ed6d7) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "lasv.chr", 0x0000, 0x000048, CRC(49ec2385) SHA1(1204c532897acc953867691124fc0b700c7aed47) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "uvssnd.p1", 0x000000, 0x080000, CRC(04a47007) SHA1(cfe1f4aa9d29c784b2034c2daa09b8bd7181562e) )
	ROM_LOAD( "uvssnd.p2", 0x080000, 0x080000, CRC(3b35d824) SHA1(e4007d5d13898ed0f91cd270c75b5df8cc62e003) )
ROM_END

ROM_START( m4goodtm )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "gtr20d.p1", 0x0000, 0x020000, CRC(a19eaef1) SHA1(5e9f9cffd841b9d4f21175e3dcec7436d016bb19) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "gta01ad.p1", 0x0000, 0x020000, CRC(2b556e66) SHA1(50a042fdb53294f74ab23a41a8a850dd14ad580d) )
	ROM_LOAD( "gta01b.p1", 0x0000, 0x020000, CRC(67dc4342) SHA1(bade42f329b4ab19e5802d8ac8b139486b05ac5a) )
	ROM_LOAD( "gta01bd.p1", 0x0000, 0x020000, CRC(6192c630) SHA1(f18c5042dc45add52b3bd0c28ad1574a85a9f3c4) )
	ROM_LOAD( "gta01d.p1", 0x0000, 0x020000, CRC(eac6ed87) SHA1(14e424b6a3232d751e5b800395b2962f997afb74) )
	ROM_LOAD( "gta01dh.p1", 0x0000, 0x020000, CRC(4201347b) SHA1(f0c086f15baa0f458f64a6d4ff7da297d9b53c8f) )
	ROM_LOAD( "gta01dk.p1", 0x0000, 0x020000, CRC(985ad557) SHA1(404a42f03d733ca6d89ec558ba6b5d815e8ce339) )
	ROM_LOAD( "gta01dr.p1", 0x0000, 0x020000, CRC(02b40bee) SHA1(a5bc063b0eac1689b13e9b523844aee105086c6b) )
	ROM_LOAD( "gta01dy.p1", 0x0000, 0x020000, CRC(acb64e98) SHA1(93d7252fe15e6a3db760b0218b4610cb5acbaca3) )
	ROM_LOAD( "gta01h.p1", 0x0000, 0x020000, CRC(444fb109) SHA1(ad04dbc67dade0012ce61718854f7656abd9d342) )
	ROM_LOAD( "gta01k.p1", 0x0000, 0x020000, CRC(9e145025) SHA1(d26b4aaee4d08d7470e862dc0e5a80c914025991) )
	ROM_LOAD( "gta01r.p1", 0x0000, 0x020000, CRC(04fa8e9c) SHA1(9ac94b59dcf8e4e123dd0f1f422d23698f1c8c38) )
	ROM_LOAD( "gta01s.p1", 0x0000, 0x020000, CRC(4340d9f6) SHA1(e9ccd419318bc3a3aba35a0104a98d1756b41731) )
	ROM_LOAD( "gta01y.p1", 0x0000, 0x020000, CRC(aaf8cbea) SHA1(a027a0a243538a6b417b3859db90adc64eeb4d31) )
	ROM_LOAD( "gtk02k.p1", 0x0000, 0x020000, CRC(a1665c5d) SHA1(056dcd9370df56129a65267fb70bbfac498f5a97) )
	ROM_LOAD( "gtr10ad.p1", 0x0000, 0x020000, CRC(b776f5dd) SHA1(9b6bf6b4d02e432ef411a9d2501c0dc7c5b551b1) )
	ROM_LOAD( "gtr10b.p1", 0x0000, 0x020000, CRC(3a0aee1d) SHA1(d7adf7118943ac946082b6c3687e10773f25608e) )
	ROM_LOAD( "gtr10bd.p1", 0x0000, 0x020000, CRC(fdb15d8b) SHA1(2542765c382cd84be1a1e7d6654e4668b1fc7fea) )
	ROM_LOAD( "gtr10d.p1", 0x0000, 0x020000, CRC(19991c56) SHA1(bed33027d8bc5dacc88d940ff3505be8186f5324) )
	ROM_LOAD( "gtr10dh.p1", 0x0000, 0x020000, CRC(80aa56fd) SHA1(f01edf442eee7dd78fbc934deb361d7e90c2025a) )
	ROM_LOAD( "gtr10dk.p1", 0x0000, 0x020000, CRC(04794eec) SHA1(3e2b95ed2092ad1c0242f8a6cb540de0c6f995a3) )
	ROM_LOAD( "gtr10dr.p1", 0x0000, 0x020000, CRC(9e979055) SHA1(b95111c4c986c4e9ceca9771ccb97fdeb67ffb02) )
	ROM_LOAD( "gtr10dy.p1", 0x0000, 0x020000, CRC(3095d523) SHA1(eed9cf874f13fe9576e8ba0da8296546311fff01) )
	ROM_LOAD( "gtr10h.p1", 0x0000, 0x020000, CRC(4711e56b) SHA1(3717b418758039286174594563281c21e4752eb5) )
	ROM_LOAD( "gtr10k.p1", 0x0000, 0x020000, CRC(c3c2fd7a) SHA1(d6e91fed1276c660cfe8e92c6902951803947a56) )
	ROM_LOAD( "gtr10r.p1", 0x0000, 0x020000, CRC(592c23c3) SHA1(17e7a22c9b80e8669cb46f691d56251bbf0717d0) )
	ROM_LOAD( "gtr10s.p1", 0x0000, 0x020000, CRC(f43fd459) SHA1(7247048088bd39cf8b20d96b1c08be48b005ac86) )
	ROM_LOAD( "gtr10y.p1", 0x0000, 0x020000, CRC(f72e66b5) SHA1(14e09a94e1cde77e87573ad7e0f438485f637dfc) )
	ROM_LOAD( "gtr11s", 0x0000, 0x020000, CRC(ff4bd1fb) SHA1(959a7975209e2d17c5b3e4adc72bd52bd3005035) )
	ROM_LOAD( "gtr15g", 0x0000, 0x020000, CRC(9da85042) SHA1(3148e654380f1bcca93c01a282f1c409e4f2d393) )
	ROM_LOAD( "gtr15t", 0x0000, 0x020000, CRC(581bcc5b) SHA1(cf80bba2b2e44c886b9bf6ae6ab1c83e2fbd7888) )
	ROM_LOAD( "gtr20ad.p1", 0x0000, 0x020000, CRC(38cf724f) SHA1(e58c02e5ca4ff0ecab41fe4597aa652ff8cc604f) )
	ROM_LOAD( "gtr20b.p1", 0x0000, 0x020000, CRC(820d5cba) SHA1(d297500c3a5388d7c9203fcb15778079e8671329) )
	ROM_LOAD( "gtr20bd.p1", 0x0000, 0x020000, CRC(7208da19) SHA1(c28b50eb91204a7494664b082ce57a910fdb29fd) )
	ROM_LOAD( "gtr20dh.p1", 0x0000, 0x020000, CRC(0f13d16f) SHA1(3a159982a3c5231d577848ca1cde17e8d84660a1) )
	ROM_LOAD( "gtr20dk.p1", 0x0000, 0x020000, CRC(8bc0c97e) SHA1(3b3eb7e44aa3f9ce19f6f612ea6bd24cc78630ff) )
	ROM_LOAD( "gtr20dr.p1", 0x0000, 0x020000, CRC(112e17c7) SHA1(519a62d61a0aa8dc2433369bd74f62f61a85994c) )
	ROM_LOAD( "gtr20dy.p1", 0x0000, 0x020000, CRC(bf2c52b1) SHA1(c8f97b56e9782b23c0ca8795ae2663c89660225e) )
	ROM_LOAD( "gtr20h.p1", 0x0000, 0x020000, CRC(ff1657cc) SHA1(07dfbc8c2b2d8fb7001ef51d80ef46562111d3ac) )
	ROM_LOAD( "gtr20k.p1", 0x0000, 0x020000, CRC(7bc54fdd) SHA1(287c1a403429bae1cc3a4ba9dfe9b510777a64b1) )
	ROM_LOAD( "gtr20r.p1", 0x0000, 0x020000, CRC(e12b9164) SHA1(a54993c4f9ffc08c03905e6f50e499eba13db0d6) )
	ROM_LOAD( "gtr20s.p1", 0x0000, 0x020000, CRC(91d2632d) SHA1(b8a7ef106a16e0526626cd69e82d07616d5c07d9) )
	ROM_LOAD( "gtr20y.p1", 0x0000, 0x020000, CRC(4f29d412) SHA1(c6a72e6fa7daaa6d8622936d10ae745814f4a8b7) )
	ROM_LOAD( "gts01ad.p1", 0x0000, 0x020000, CRC(b415d3f3) SHA1(a22d4cbce9b66049c000806b565c86cd3d91fd82) )
	ROM_LOAD( "gts01b.p1", 0x0000, 0x020000, CRC(a05ea188) SHA1(5b6acadaf8b0d18b9c9daaf4aea45202cad13355) )
	ROM_LOAD( "gts01bd.p1", 0x0000, 0x020000, CRC(fed27ba5) SHA1(14f6fd7c3e964c3e6547a4bf5fa5d7cd802f715d) )
	ROM_LOAD( "gts01d.p1", 0x0000, 0x020000, CRC(2d440f4d) SHA1(09881d8aede3a2011010022ee0e03ba8f02ea907) )
	ROM_LOAD( "gts01dh.p1", 0x0000, 0x020000, CRC(dd4189ee) SHA1(f68da5f688ffa08625c92fd139bef263c43f674a) )
	ROM_LOAD( "gts01dk.p1", 0x0000, 0x020000, CRC(071a68c2) SHA1(ea2052a389df09e0d5614106cc20ee0eccd339dd) )
	ROM_LOAD( "gts01dr.p1", 0x0000, 0x020000, CRC(9df4b67b) SHA1(608de0bb5280ac20c8c47eb22f8a575ad87a4fdd) )
	ROM_LOAD( "gts01dy.p1", 0x0000, 0x020000, CRC(33f6f30d) SHA1(aae905d0d278c38497513eb131a7cdaab4d22a85) )
	ROM_LOAD( "gts01h.p1", 0x0000, 0x020000, CRC(83cd53c3) SHA1(a4233e22d0bf53b3ddc4c951894a653e7951f5ad) )
	ROM_LOAD( "gts01k.p1", 0x0000, 0x020000, CRC(5996b2ef) SHA1(874d69a4adc31667ea177b3a0350b77bb735ea8d) )
	ROM_LOAD( "gts01r.p1", 0x0000, 0x020000, CRC(c3786c56) SHA1(c9659df6302046c466e6447b8b427628c88b773d) )
	ROM_LOAD( "gts01s.p1", 0x0000, 0x020000, CRC(b3819e1f) SHA1(fb44500e06b8a6b09e6b707ee8c0cfe7844870a2) )
	ROM_LOAD( "gts01y.p1", 0x0000, 0x020000, CRC(6d7a2920) SHA1(7d31087e3645e05baf6b0100966d4773a6d023cd) )
	ROM_LOAD( "gts02s.t", 0x0000, 0x020000, CRC(e4f5ebcc) SHA1(3e070628375db980583c3b38e2676d73fbeaae68) )
	ROM_LOAD( "gts10ad.p1", 0x0000, 0x020000, CRC(b754490c) SHA1(26811ae53b3ee8ae0a381604109f0f77f096e2c6) )
	ROM_LOAD( "gts10b.p1", 0x0000, 0x020000, CRC(21410553) SHA1(2060269ab6dc9375b6e7101b3944305fcd4b6d12) )
	ROM_LOAD( "gts10bd.p1", 0x0000, 0x020000, CRC(fd93e15a) SHA1(b9574f34f1e7a92cc75d6aed8914f94ad661bba3) )
	ROM_LOAD( "gts10d.p1", 0x0000, 0x020000, CRC(ac5bab96) SHA1(2b4ab1f096a5d63d5688228debb25b392b94a297) )
	ROM_LOAD( "gts10dh.p1", 0x0000, 0x020000, CRC(de001311) SHA1(a096937e1f2fd2f9046c0ea2363805112da76c95) )
	ROM_LOAD( "gts10dk.p1", 0x0000, 0x020000, CRC(045bf23d) SHA1(dd5567d4c07fba7ff1cc257db287e1a82d9b930a) )
	ROM_LOAD( "gts10dr.p1", 0x0000, 0x020000, CRC(9eb52c84) SHA1(385b09e424863f3778605c6e768c9b1068eae66e) )
	ROM_LOAD( "gts10dy.p1", 0x0000, 0x020000, CRC(30b769f2) SHA1(7cf1e66b992faf48b1fed39a005469e51d471a0b) )
	ROM_LOAD( "gts10h.p1", 0x0000, 0x020000, CRC(02d2f718) SHA1(9f46ef4cc7d08c42cf617b471e599ca21b4cd72f) )
	ROM_LOAD( "gts10k.p1", 0x0000, 0x020000, CRC(d8891634) SHA1(0019d2b3dd1c59d37d9c13e912907a55f2a9ca5f) )
	ROM_LOAD( "gts10r.p1", 0x0000, 0x020000, CRC(4267c88d) SHA1(22047782c384caeb9cf2de69dcdd05f42ee137ad) )
	ROM_LOAD( "gts10s.p1", 0x0000, 0x020000, CRC(2851ba23) SHA1(7597a2df22aa0e2670be2d5bb2407ea1feace3a0) )
	ROM_LOAD( "gts10y.p1", 0x0000, 0x020000, CRC(ec658dfb) SHA1(a9d8ba1b66811ccd336892160d47e5d42eb83a23) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "gtrsnd.p1", 0x000000, 0x080000, CRC(23317580) SHA1(c0c5244ddcf976211e2a5e5a0b1dbc6faaec22b4) )
	ROM_LOAD( "gtrsnd.p2", 0x080000, 0x080000, CRC(866ce0d2) SHA1(46e800c7364a6d291c6af87b30c680c530100e74) )
ROM_END

ROM_START( m4libty )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dlt10.bin", 0x0000, 0x010000, CRC(25d91c01) SHA1(788ba8669bae5b4cdfb7231c7225d6745038a575) )
ROM_END

ROM_START( m4lineup )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "lineup5p1.bin", 0xc000, 0x004000, CRC(9ba9edbd) SHA1(385e01816b5631b6896e85343ae96b3c36f9647a) )
	ROM_LOAD( "lineup5p2.bin", 0x8000, 0x004000, CRC(e9e4dfb0) SHA1(46a0efa84770036366c7a6a33ef1d42c7b2b782b) )
	ROM_LOAD( "lineup5p3.bin", 0x6000, 0x002000, CRC(86623376) SHA1(e29442bfcd401361287852b87673368322e946b5) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "lu2_10p1.bin", 0x0000, 0x004000, CRC(2fb89062) SHA1(55e86de8fd0d36cca9aab8ad5aae7b4f5a62b940) )
	ROM_LOAD( "lu2_10p2.bin", 0x0000, 0x004000, CRC(9d820af2) SHA1(63d27df91f80e47eb8c9685fcd2c3eff902a2ef8) )
	ROM_LOAD( "lu2_10p3.bin", 0x0000, 0x002000, CRC(8c8a210c) SHA1(2599d979f1a62e9ef6acc70d0ad5c9b4a65d712a) )
ROM_END

ROM_START( m4loadmn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "la10h.bin", 0x0000, 0x010000, CRC(1f90520e) SHA1(c052c2c751d1ded6077a800be4dedf91ca0bd5ba) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "la11c.bin", 0x0000, 0x010000, CRC(8e6ab8f9) SHA1(cd3367d368c64a74e108fdfda00c4898ca8262c8) )
	ROM_LOAD( "la11l.bin", 0x0000, 0x010000, CRC(2d1f8e7a) SHA1(9e5a8b7827925f757784ea4726e3c4897056cdf6) )
ROM_END

ROM_START( m4luck7 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dl716.bin", 0x0000, 0x010000, CRC(141b23a9) SHA1(3bfb82ea0ee4104bd8739b545aba617f84bef770) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "dl7snd.bin", 0x0000, 0x080000, CRC(c90fa8ad) SHA1(a98f03d4b6f5892333279bff7537d4d6d887da62) )
ROM_END

ROM_START( m4luckdv )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cld_16.bin", 0x0000, 0x010000, CRC(89f63938) SHA1(8d3a5628e2c0bf39784afe2f00a007d40ea35423) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cld_snd1.snd", 0x000000, 0x080000, CRC(f247ba83) SHA1(9b173503e63a4a861d1380b2ab1fe14af1a189bd) )
	ROM_LOAD( "cld_snd2.snd", 0x080000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END

ROM_START( m4luckdvd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dld13", 0x0000, 0x010000, CRC(b8ceb29b) SHA1(84b6ebad300214610635fb8141d18de2b7065435) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sdld01.snd", 0x000000, 0x080000, CRC(9b035fa6) SHA1(51b7e5bc3abdf4f1beba2347146a91a2b3f4de35) )
ROM_END

ROM_START( m4lucklv )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "llvs.p1", 0x0000, 0x010000, CRC(30727bc9) SHA1(c32112d0181f629540b31ce9959834111dbf7e0e) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ll3ad.p1", 0x0000, 0x010000, CRC(e79e7f98) SHA1(7b3a22978f2f5a0b6062f0330fef15ce0e91c010) )
	ROM_LOAD( "ll3b.p1", 0x0000, 0x010000, CRC(bcbbe728) SHA1(4930419e0e524a91687386e8a2fce2150cd8a172) )
	ROM_LOAD( "ll3bd.p1", 0x0000, 0x010000, CRC(aa4e9e1e) SHA1(ebacd42049916a32d1738813d544e34507dd650a) )
	ROM_LOAD( "ll3d.p1", 0x0000, 0x010000, CRC(c968fe37) SHA1(8a1b79928a86b6ba5449f0a454e5115fd37e3d55) )
	ROM_LOAD( "ll3dk.p1", 0x0000, 0x010000, CRC(c03baaef) SHA1(f3c4394d0c9929d439551aa61784a51f67e2dc3a) )
	ROM_LOAD( "ll3dy.p1", 0x0000, 0x010000, CRC(e73fa943) SHA1(3b2a372028ffa5200510e976cd4e8eba8e6c0612) )
	ROM_LOAD( "ll3k.p1", 0x0000, 0x010000, CRC(2519ede2) SHA1(d9e4b57824cddc04d1166cee16c309a77ff510d6) )
	ROM_LOAD( "ll3s.p1", 0x0000, 0x010000, CRC(fdda2e78) SHA1(f68b274b6af7b44347b8f684f6e8a9342d222590) )
	ROM_LOAD( "ll3y.p1", 0x0000, 0x010000, CRC(195023d4) SHA1(a4fabaa44fa76c4e77fb40bb89c65dadeab5927c) )
	ROM_LOAD( "ll8ad.p1", 0x0000, 0x010000, CRC(08f307ef) SHA1(1a613d8d2cbadffe1a97a7e2c3eafc709e6895c7) )
	ROM_LOAD( "ll8b.p1", 0x0000, 0x010000, CRC(7dd12d38) SHA1(4466bca407500741e74dfec07361d030c1736c18) )
	ROM_LOAD( "ll8bd.p1", 0x0000, 0x010000, CRC(3eef6aa5) SHA1(e01b95ca085a326207ef2ada1d5f31745b9c15c9) )
	ROM_LOAD( "ll8c.p1", 0x0000, 0x010000, CRC(a2de47ff) SHA1(bc4179749474fec1963314dddac69f7f1cec1ce3) )
	ROM_LOAD( "ll8d.p1", 0x0000, 0x010000, CRC(b6385af0) SHA1(e32251b4c7c0db4957f87a9eb3f6c07efd66142c) )
	ROM_LOAD( "ll8dk.p1", 0x0000, 0x010000, CRC(f325c2af) SHA1(25a0818776c0cf4edcd2678fe457e21bf10dfe61) )
	ROM_LOAD( "ll8dy.p1", 0x0000, 0x010000, CRC(10b33cc3) SHA1(fe0a41b98b1f3dde82365c30236586737c5fdb9d) )
	ROM_LOAD( "ll8k.p1", 0x0000, 0x010000, CRC(d7c284be) SHA1(c33dbfa15ead02a581b4a874a6981e6094537fb4) )
	ROM_LOAD( "ll8s.p1", 0x0000, 0x010000, CRC(1448c6fe) SHA1(211627a0f397658a8241c5e4e138a1a609beaabe) )
	ROM_LOAD( "ll8y.p1", 0x0000, 0x010000, CRC(eb8b4a88) SHA1(f0de0d50de848e7fea35ebe754f20eaa1c02f295) )
	ROM_LOAD( "lltad.p1", 0x0000, 0x010000, CRC(35e43f52) SHA1(4ca2c43c59f5c5c3f53527bbf8128f0e92a3ca5c) )
	ROM_LOAD( "lltb.p1", 0x0000, 0x010000, CRC(ddc19b4e) SHA1(b44a56e5d543d895e37986d19649d7afa8d3d238) )
	ROM_LOAD( "lltbd.p1", 0x0000, 0x010000, CRC(fe25aa83) SHA1(68ccae76938af64e0c5df0e56333fd6814734779) )
	ROM_LOAD( "lltd.p1", 0x0000, 0x010000, CRC(200930a6) SHA1(5dc054231402b43ebb980d190cde20c3b89b0f13) )
	ROM_LOAD( "lltdk.p1", 0x0000, 0x010000, CRC(02969e73) SHA1(a4344e198d8aba3ba85325de7eb650afdd1a491c) )
	ROM_LOAD( "lltdr.p1", 0x0000, 0x010000, CRC(c8af8031) SHA1(1c683e9320a528d214afa7284cce8325fbd29cdf) )
	ROM_LOAD( "lltdy.p1", 0x0000, 0x010000, CRC(55c4a8df) SHA1(a7cc7fa2aa893a3f134d6d1c04a16ecbcad99a40) )
	ROM_LOAD( "lltk.p1", 0x0000, 0x010000, CRC(88c7f5e5) SHA1(c30651c6daa6c82157879e72a74ee5e03f9ea64f) )
	ROM_LOAD( "lltr.p1", 0x0000, 0x010000, CRC(189c8fc7) SHA1(c06bedd4477ead2c071374d17fc0ef617ef6e924) )
	ROM_LOAD( "llts.p1", 0x0000, 0x010000, CRC(3ccc9e2f) SHA1(15c90bbc135ddaa05768bde6970bda15f1b69d44) )
	ROM_LOAD( "llty.p1", 0x0000, 0x010000, CRC(85f7a729) SHA1(b3a58071bb6dae36354280a8f0de1faaa190899f) )
	ROM_LOAD( "lluad.p1", 0x0000, 0x010000, CRC(7a2acea9) SHA1(dfb5763664143dec4927e2a8f4088f04ca85c458) )
	ROM_LOAD( "llub.p1", 0x0000, 0x010000, CRC(d3683b6b) SHA1(e67e996f5f2ed55fbc47b7f1a0e225125baf2a99) )
	ROM_LOAD( "llubd.p1", 0x0000, 0x010000, CRC(4c36a3e3) SHA1(afbe3c4d60dd92e7bc34915f3d15cadd46da6738) )
	ROM_LOAD( "llud.p1", 0x0000, 0x010000, CRC(f82d26a3) SHA1(4c3dffbdebd6e3f372f95633cec985c7595a1b6c) )
	ROM_LOAD( "lludk.p1", 0x0000, 0x010000, CRC(1bf8f2fe) SHA1(d9f04fc91dd2da95844d78d928b01d7bcf20326e) )
	ROM_LOAD( "lludr.p1", 0x0000, 0x010000, CRC(2d780bfc) SHA1(f787daebcc0706f018ad0ce1388ee08f1bbfa07c) )
	ROM_LOAD( "lludy.p1", 0x0000, 0x010000, CRC(6d042cfe) SHA1(3812f99d45cff5f147a7965f3cb8c96eda9a2e8d) )
	ROM_LOAD( "lluk.p1", 0x0000, 0x010000, CRC(476cd76c) SHA1(1a122bfbe3d95b2ce9ecf18bc6d4f85d1ea09bc7) )
	ROM_LOAD( "llur.p1", 0x0000, 0x010000, CRC(89a1a999) SHA1(e21007e2db0f69f0753becbf499c54cb29f46a39) )
	ROM_LOAD( "llus.p1", 0x0000, 0x010000, CRC(07745135) SHA1(ec602d01910ac52d20ff9c54914a5261f538233b) )
	ROM_LOAD( "lluy.p1", 0x0000, 0x010000, CRC(1cfc18d6) SHA1(eb34e5a43cee1d4b64443f1fe2b1d12ccf35b847) )
	ROM_LOAD( "llvb.p1", 0x0000, 0x010000, CRC(72e27d9b) SHA1(2355c48c4ac8bc94dcea74b04e68be8a461f09a3) )
	ROM_LOAD( "llvc.p1", 0x0000, 0x010000, CRC(00ac2db0) SHA1(a8ec3a2862abf4eb56734d15cd9b7db0333c98f2) )
	ROM_LOAD( "llvd.p1", 0x0000, 0x010000, CRC(fdb4dcc5) SHA1(77fa51dcf1895b83182e7ee4137ed68c06d2593b) )
	ROM_LOAD( "llvr.p1", 0x0000, 0x010000, CRC(a806c43f) SHA1(8c36d50c911f956a0458f942cc8e313104f00005) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "llvsnd.p1", 0x000000, 0x080000, CRC(36be26f3) SHA1(b1c66d3ebebd7eb18266bf6b30c4a4db7acdf10d) )
	ROM_LOAD( "llvsnd.p2", 0x080000, 0x080000, CRC(d5c2bb99) SHA1(e2096b8a33e89218d44200e87d1962790120a96c) )
ROM_END

ROM_START( m4luckst )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "lss06s.p1", 0x0000, 0x020000, CRC(b6a69478) SHA1(6b05b7f9af94a83adfdff328d4132f72a1dfb19f) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ls15g", 0x0000, 0x020000, CRC(b942ac91) SHA1(e77b2acd07cac9b747731f9e0637112fc6bf94c7) )
	ROM_LOAD( "ls15t", 0x0000, 0x020000, CRC(20447a20) SHA1(ca2ba566317ca87afcc2501e551c1326b9712526) )
	ROM_LOAD( "lsgame.bin", 0x0000, 0x020000, CRC(089c1ad7) SHA1(bcfacbeaf1845d91f18c6f57bf166c82469cb460) )
	ROM_LOAD( "lss06ad.p1", 0x0000, 0x020000, CRC(9a512ec1) SHA1(c1eb4d0f5c915f392411d0ff2c931eb22a41b3a8) )
	ROM_LOAD( "lss06b.p1", 0x0000, 0x020000, CRC(f2e1cb20) SHA1(7867085e0d1166419360bf1b6f39e3ec31bf2c7f) )
	ROM_LOAD( "lss06bd.p1", 0x0000, 0x020000, CRC(174b8004) SHA1(9cce3124797b80886ab744885444f7a69711f6f1) )
	ROM_LOAD( "lss06c.p1", 0x0000, 0x020000, CRC(c5ab7632) SHA1(de6fea314be38e6a779008f81cac0825ed6eda54) )
	ROM_LOAD( "lss06d.p1", 0x0000, 0x020000, CRC(8ffac056) SHA1(896dbbfe6265c77e10d6f36859ba92c13b748517) )
	ROM_LOAD( "lss06dh.p1", 0x0000, 0x020000, CRC(d9a7d047) SHA1(e85e48104ca76534bcb8f38a76a67eabfdbe8ba0) )
	ROM_LOAD( "lss06dk.p1", 0x0000, 0x020000, CRC(5d8c2852) SHA1(ab3959defb9c129a455177857da894942065231c) )
	ROM_LOAD( "lss06dr.p1", 0x0000, 0x020000, CRC(ee839363) SHA1(ff418c507a8ef2f28ebe7493801bf09afbd6a48a) )
	ROM_LOAD( "lss06dy.p1", 0x0000, 0x020000, CRC(da6f08ac) SHA1(597ff1d3623dda6ffb3b8f7b8f5ef0683d433079) )
	ROM_LOAD( "lss06h.p1", 0x0000, 0x020000, CRC(3c0d9b63) SHA1(57a0408908c521a2a44c3a825b3e4480dff4f778) )
	ROM_LOAD( "lss06k.p1", 0x0000, 0x020000, CRC(b8266376) SHA1(7d84dce05224c8882ed103796a054b50e2390234) )
	ROM_LOAD( "lss06r.p1", 0x0000, 0x020000, CRC(0b29d847) SHA1(3412bff6f38ab12b9d5e30f1ed3e327ad58dc470) )
	ROM_LOAD( "lss06s.p1", 0x0000, 0x020000, CRC(b6a69478) SHA1(6b05b7f9af94a83adfdff328d4132f72a1dfb19f) )
	ROM_LOAD( "lss06y.p1", 0x0000, 0x020000, CRC(3fc54388) SHA1(f57667cc0263efe05d0f0538fae2f4f8adc0c405) )
	ROM_LOAD( "lss07ad.p1", 0x0000, 0x020000, CRC(c4e113c0) SHA1(e5e81c08c2487ee8802ff4374b3affdff3e70003) )
	ROM_LOAD( "lss07b.p1", 0x0000, 0x020000, CRC(eec5db67) SHA1(c7b76b50524b256ec42adc33c99f933790d9d578) )
	ROM_LOAD( "lss07bd.p1", 0x0000, 0x020000, CRC(49fbbd05) SHA1(4edebdf607ae996e0f4a2df02f181e75b784e1ac) )
	ROM_LOAD( "lss07c.p1", 0x0000, 0x020000, CRC(d98f6675) SHA1(0ab59dad98a879a5010e84ba9f2feefe55b08cf5) )
	ROM_LOAD( "lss07d.p1", 0x0000, 0x020000, CRC(93ded011) SHA1(f9588096aa8d14b62ab4d7984988f567955aeb9f) )
	ROM_LOAD( "lss07dh.p1", 0x0000, 0x020000, CRC(8717ed46) SHA1(67725fb72f1182544479e554f1efea5f167647f5) )
	ROM_LOAD( "lss07dk.p1", 0x0000, 0x020000, CRC(033c1553) SHA1(fa07bc86f0a6cfd91c982d0f44fa43c1e54195e0) )
	ROM_LOAD( "lss07dr.p1", 0x0000, 0x020000, CRC(b033ae62) SHA1(8c58e4b881c73c9c7450f81026f982b5cfb56197) )
	ROM_LOAD( "lss07dy.p1", 0x0000, 0x020000, CRC(84df35ad) SHA1(eb9e5c0a4fcb85bc6a81862ff4852de380ef0a5a) )
	ROM_LOAD( "lss07h.p1", 0x0000, 0x020000, CRC(20298b24) SHA1(004d6d04d9e8223e7b97dbd39abc66e80d6ea0eb) )
	ROM_LOAD( "lss07k.p1", 0x0000, 0x020000, CRC(a4027331) SHA1(64320519f22ebac3fbb7d93e2e70e999e8e7cd29) )
	ROM_LOAD( "lss07r.p1", 0x0000, 0x020000, CRC(170dc800) SHA1(bebd8b41b756c7ca5ddc60647e6031ac4cf874db) )
	ROM_LOAD( "lss07s.p1", 0x0000, 0x020000, CRC(f4546d1a) SHA1(fed65704693e11087825b1dfda4df28ee6d2d3be) )
	ROM_LOAD( "lss07y.p1", 0x0000, 0x020000, CRC(23e153cf) SHA1(75c5c13ad735aba1bf62d26eb37f9aaa5804fdd2) )
	ROM_LOAD( "lst09ad.p1", 0x0000, 0x020000, CRC(2fc94fb9) SHA1(cda6ce0a9d9e124326e29b57bc553b517408f1c6) )
	ROM_LOAD( "lst09b.p1", 0x0000, 0x020000, CRC(c5b8927f) SHA1(15d227be23404b9393cb3d3625fd054eaf0da3a4) )
	ROM_LOAD( "lst09bd.p1", 0x0000, 0x020000, CRC(650ee7ef) SHA1(2422e4f9857376a029f6b4df46b5b81c73b2640a) )
	ROM_LOAD( "lst09d.p1", 0x0000, 0x020000, CRC(48a23cba) SHA1(04ce2a07ed89152da69e5d5309390060fbe30e8d) )
	ROM_LOAD( "lst09dh.p1", 0x0000, 0x020000, CRC(469d15a4) SHA1(6729b73da48f1bc2f169932a95bba5d0a7795905) )
	ROM_LOAD( "lst09dk.p1", 0x0000, 0x020000, CRC(9cc6f488) SHA1(ded41be5841bd9717bfe0526b89cfcfd98a1e757) )
	ROM_LOAD( "lst09dr.p1", 0x0000, 0x020000, CRC(06282a31) SHA1(97c1d3aab87ec2a8602f840ce8b58056c28dc34c) )
	ROM_LOAD( "lst09dy.p1", 0x0000, 0x020000, CRC(a82a6f47) SHA1(0694ca9ff9882547f1ca8cfad4c9c824f6da799f) )
	ROM_LOAD( "lst09h.p1", 0x0000, 0x020000, CRC(e62b6034) SHA1(37ae4b6250795087475ea8e51b49e225572118b4) )
	ROM_LOAD( "lst09k.p1", 0x0000, 0x020000, CRC(3c708118) SHA1(61ab1751dec08f7407fd3360cf129736d7d5ec16) )
	ROM_LOAD( "lst09r.p1", 0x0000, 0x020000, CRC(a69e5fa1) SHA1(10ca0a2135c4d6832693bf97812f4eb5f1380efd) )
	ROM_LOAD( "lst09s.p1", 0x0000, 0x020000, CRC(285d0255) SHA1(89f74ad19525b636d6e1ea36308b659389f68245) )
	ROM_LOAD( "lst09y.p1", 0x0000, 0x020000, CRC(089c1ad7) SHA1(bcfacbeaf1845d91f18c6f57bf166c82469cb460) )
	ROM_LOAD( "lst10ad.p1", 0x0000, 0x020000, CRC(41e8dd7d) SHA1(2b794554f57dfb5b6fe9f64b9d3e6b73ec306056) )
	ROM_LOAD( "lst10b.p1", 0x0000, 0x020000, CRC(afd0d023) SHA1(7ee5c004d9d0cacf2d55457f9846fd638b584482) )
	ROM_LOAD( "lst10bd.p1", 0x0000, 0x020000, CRC(0b2f752b) SHA1(655f50bbe163fca4386b356e7db0b019eba5d94f) )
	ROM_LOAD( "lst10d.p1", 0x0000, 0x020000, CRC(22ca7ee6) SHA1(487da48e02f11aef5a2c42efe54c5e9f43a7915d) )
	ROM_LOAD( "lst10dh.p1", 0x0000, 0x020000, CRC(28bc8760) SHA1(f442853ac0f6cedbdb4b928bfee82e6f9d0e3824) )
	ROM_LOAD( "lst10dk.p1", 0x0000, 0x020000, CRC(f2e7664c) SHA1(bc869ec0e533143bdcef52f06fcf34c5eb30d928) )
	ROM_LOAD( "lst10dr.p1", 0x0000, 0x020000, CRC(6809b8f5) SHA1(626859168f33ca3e2ce133c645a32a73a811ac75) )
	ROM_LOAD( "lst10dy.p1", 0x0000, 0x020000, CRC(c60bfd83) SHA1(276d65498c8d1da56765c451b8c9bf2b4af096a6) )
	ROM_LOAD( "lst10h.p1", 0x0000, 0x020000, CRC(8c432268) SHA1(c8707c587ab2cc1450bd47069206767d2d930b29) )
	ROM_LOAD( "lst10k.p1", 0x0000, 0x020000, CRC(5618c344) SHA1(61565a9814fd1b4e77944e73d8f18f8545685928) )
	ROM_LOAD( "lst10r.p1", 0x0000, 0x020000, CRC(ccf61dfd) SHA1(01772dd0b252b41b5cba024a717d5898e74971f8) )
	ROM_LOAD( "lst10s.p1", 0x0000, 0x020000, CRC(0e1ad810) SHA1(bd439f2857ebbde2b7941c411ac7edf7c66af7eb) )
	ROM_LOAD( "lst10y.p1", 0x0000, 0x020000, CRC(62f4588b) SHA1(ee7c06e2cc79f7d18d45b0a1793e5279a46ffcc0) )
	ROM_LOAD( "lstrikegame10-8t.bin", 0x0000, 0x020000, CRC(709c2dbf) SHA1(bba8d7af9502911ffa1c086b993484ab78ad38ac) )
//  ROM_LOAD( "lstrikesnd-p1.bin", 0x0000, 0x020000, CRC(0c8f9dcd) SHA1(026be80620bd4afc3e45cb7a374b93fff4c13dd2) ) // bad
//  ROM_LOAD( "lstrikesnd-p2.bin", 0x0000, 0x020000, CRC(2a746ba5) SHA1(7f9d727a849a7a1ecfd750f214deef150ec3d9eb) ) // bad

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ls55", 0x0000, 0x020000, CRC(823e805b) SHA1(17f09fd53188950a8d98ac04cd94785947b52b01) )
	ROM_LOAD( "ls__xa_x.1_1", 0x0000, 0x020000, CRC(a9642503) SHA1(2765c4d8943678446c516918035d7a888a812aae) )


	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "lssnd1.bin", 0x000000, 0x080000, CRC(401686bc) SHA1(ab62e6e097b0af2f68ae7f8f686f00cede5ec3aa) )
	ROM_LOAD( "lssnd2.bin", 0x080000, 0x080000, CRC(d9e0c0db) SHA1(3eba5b19ca98d23a94edf2be27ccefaa0e526a56) )
ROM_END

ROM_START( m4lucksc )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "clu14d.p1", 0x0000, 0x020000, CRC(7a64199f) SHA1(62c7c8a4475a8005a1f969550d0717c9cc44bada) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "clu14f.p1", 0x0000, 0x020000, CRC(07e90cdb) SHA1(5d4bf7f6f84f2890a0119de898f01e3e99bfbb7f) )
	ROM_LOAD( "clu14s.p1", 0x0000, 0x020000, CRC(5f66d7cc) SHA1(bd8a832739d7aef4d04b89a94dd2886e89a6e0c2) )
	ROM_LOAD( "gls06d.p1", 0x0000, 0x020000, CRC(2f7f8a9a) SHA1(04243f190597e3d3bdd258b8146b71e9c7cd90c7) )
	ROM_LOAD( "gls06f.p1", 0x0000, 0x020000, CRC(52f29fde) SHA1(97df15c89540d8bbb15abb86f6a2e9d0e022c9df) )
	ROM_LOAD( "gls06s.p1", 0x0000, 0x020000, CRC(975adb8d) SHA1(b92d1ad93e51f55111921060939359471c2e5384) )
	ROM_LOAD( "gs301d.p1", 0x0000, 0x020000, CRC(e0807af7) SHA1(1740d4d56ad71407a4d2bb13b43c9d5f31caf638) )
	ROM_LOAD( "gs301f.p1", 0x0000, 0x020000, CRC(9d0d6fb3) SHA1(9206299604190deace09136ca2eebb9ad2792815) )
	ROM_LOAD( "gs301s.p1", 0x0000, 0x020000, CRC(53314dc6) SHA1(28b5d2a03b8f6221b80f10c46985fa906cc9be32) )
	ROM_LOAD( "ls301d.p1", 0x0000, 0x020000, CRC(39fb0ddf) SHA1(3a6934892585bde6a99f1d2e2fd95677cf37fcfe) )
	ROM_LOAD( "ls301f.p1", 0x0000, 0x020000, CRC(4476189b) SHA1(b94c6abbbf37ae28869b1f9c882de8fa56b2c676) )
	ROM_LOAD( "ls301s.p1", 0x0000, 0x020000, CRC(7e9e97f1) SHA1(43760792b529db8acb497d38ad3951abdebcf76b) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "lsc_.1_1", 0x0000, 0x020000, CRC(79ce3db0) SHA1(409e9d3b08284dee3af696fb7c839c0ca35eddee) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "clu14s.chr", 0x0000, 0x000048, CRC(be933239) SHA1(52dbcbbcbfe25b6f8c186ce9af67b533c8da9a88) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "clusnd.p1", 0x000000, 0x080000, CRC(9c1042ba) SHA1(e4630bbcb3fe2f7d133275892eaf58c12402c610) )
	ROM_LOAD( "clusnd.p2", 0x080000, 0x080000, CRC(b4b28b80) SHA1(a40b6801740d64e54c5c1738d69737ab9f4cf950) )
ROM_END

ROM_START( m4luckwb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "lwb10.bin", 0x0000, 0x010000, CRC(6d43a14e) SHA1(267aba1a01bfd5f0eaa7683d041d5fcb2d301934) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "lwb15.bin", 0x0000, 0x010000, CRC(b5af8cb2) SHA1(474975b83803627ad3ac4217d8cecb2d2db16fec) )
	ROM_LOAD( "lwb21.bin", 0x0000, 0x010000, CRC(6c570733) SHA1(7488318ca9689371e4f80be0a0fddd8ad141733e) )
	ROM_LOAD( "lwb22.bin", 0x0000, 0x010000, CRC(05b952a7) SHA1(952e328b280a18c1ffe253b6a56f2b5e893b1b72) )
	ROM_LOAD( "lwb27.bin", 0x0000, 0x010000, CRC(9d6b6637) SHA1(65bad12cd08de128ca31c9488e32e3cebfb8eedb) )
	ROM_LOAD( "lwb6.bin", 0x0000, 0x010000, CRC(8e7d4594) SHA1(4824a9a4628585a170c41e00f7b3fcb8a2330c02) )
	ROM_LOAD( "lwb7.bin", 0x0000, 0x010000, CRC(8e651705) SHA1(bd4d09d586d14759a17d4d7d4016c427f3eef015) )

	ROM_REGION( 0x100000, "msm6376", 0 ) // these are all different sound roms...
	ROM_LOAD( "lwbs3.bin", 0x0000, 0x07dc89, CRC(ee102376) SHA1(3fed581a4654acf285dd430fbfbac33cd67411b8) )
	ROM_LOAD( "lwbs7.bin", 0x0000, 0x080000, CRC(5d4177c7) SHA1(e13f145885bb719b0021ae4ce289261a3eaa2e18) )
	ROM_LOAD( "lwbs8.bin", 0x0000, 0x080000, CRC(187cdf5b) SHA1(87ec189af27c95f278a7531ec13df53a08889af8) )
	ROM_LOAD( "lwbs9.bin", 0x0000, 0x080000, CRC(2e02b617) SHA1(2502a1d2cff155a7fc5148e23a4723d4d60e9d42) )
ROM_END

ROM_START( m4luxor )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "luxor.hex", 0x0000, 0x010000, CRC(55277510) SHA1(9a866c36a398df52c54b554cd8085078c1f1954b) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "lux05_101", 0x0000, 0x010000, CRC(8f4dc4f4) SHA1(c9743a1b79b377313504296a060dff3f413a7a9d) )
	ROM_LOAD( "lux10_101", 0x0000, 0x010000, CRC(8965c7be) SHA1(ca05803bc7d7a96e25dc0b025c2683b4679789fb) )
	ROM_LOAD( "lux208c", 0x0000, 0x010000, CRC(f57bae67) SHA1(3a2523a2121948480381f49e26e870b10d541304) )
	ROM_LOAD( "lux55", 0x0000, 0x010000, CRC(997419ab) SHA1(c616a5d6cb347963e7e5c5b88912c248bae184ca) )
	ROM_LOAD( "lux58c", 0x0000, 0x010000, CRC(da408721) SHA1(971413620d1f304a026d3adc68f6ac5c1d104e20) )
	ROM_LOAD( "luxc.p1", 0x0000, 0x010000, CRC(47d1c4dc) SHA1(0856fac4a7ec14dc1df24446e1355ed05bb5f1c1) )
	ROM_LOAD( "luxd.p1", 0x0000, 0x010000, CRC(8f949379) SHA1(4f0a94d06b8e7036acaae5c0c42c91481837d3a1) )
	ROM_LOAD( "luxk.p1", 0x0000, 0x010000, CRC(bd5eaf2d) SHA1(f9a3f3139d6b7ff4fcec805e0ca6e8ab1c3c10dd) )
	ROM_LOAD( "luxor_std.bin", 0x0000, 0x010000, CRC(2c565bf7) SHA1(61612abbda037b63e2cda7746be8cf64b4563d43) )
	ROM_LOAD( "luxs.p1", 0x0000, 0x010000, CRC(78d6f05a) SHA1(53de98b9248c67c83f255d33d5963bebb757d0af) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "lux_05_4", 0x0000, 0x010000, CRC(335503ec) SHA1(dd03096aa98e4cac9fade6e77f9f8a8ad9a64287) )
	ROM_LOAD( "lux_05_8", 0x0000, 0x010000, CRC(43a15814) SHA1(694c8c6ee695bb746391f5269f540c995fc18002) )
	ROM_LOAD( "lux_10_4", 0x0000, 0x010000, CRC(122461d9) SHA1(a347c834b27a00abc1864a1e00316a491d04d84b) )
	ROM_LOAD( "lux_10_8", 0x0000, 0x010000, CRC(544208e7) SHA1(85e2ff663b7500ee6bb0a900ee5ef48f7bf1934a) )
	ROM_LOAD( "lux_20.4", 0x0000, 0x010000, CRC(50b3e5cc) SHA1(ff08095c01d8eeff320b5a04fe9f7e1888690cf8) )
	ROM_LOAD( "lux_20_8", 0x0000, 0x010000, CRC(6c9a7152) SHA1(e38e8452e0d3f5b0e8ac51da272ab9f2e57e1d89) )
	ROM_LOAD( "lx_05a__.1o1", 0x0000, 0x010000, CRC(7b81f1b9) SHA1(412a8961571f279d70c05ef26c565b4b2a588060) )
	ROM_LOAD( "lx_05s__.1o1", 0x0000, 0x010000, CRC(2bf86940) SHA1(cf96a7a12db84fc028766da55ca06d2350f9d08f) )
	ROM_LOAD( "lx_05sb_.1o1", 0x0000, 0x010000, CRC(e210c1b6) SHA1(023b1e0b36c4d146af5e958be72575590588b3fd) )
	ROM_LOAD( "lx_05sd_.1o1", 0x0000, 0x010000, CRC(8727963a) SHA1(4585c0e3fb14f54684ff199be9010ed7b5cb97c3) )
	ROM_LOAD( "lx_10a__.1o1", 0x0000, 0x010000, CRC(ce8e6c05) SHA1(b48bc01d1a069881e9b9db1a4959c7b57e80f28a) )
	ROM_LOAD( "lx_10s__.1o1", 0x0000, 0x010000, CRC(9f0f5b6b) SHA1(9f67500d62921dd680bd864856206306adc3f2f6) )
	ROM_LOAD( "lx_10sb_.1o1", 0x0000, 0x010000, CRC(bd020920) SHA1(a6b5c11c82344afc1cdd350b9f31d1257be72615) )
	ROM_LOAD( "lx_10sd_.1o1", 0x0000, 0x010000, CRC(cc59d370) SHA1(a428d93c005b629e86810c85ea91630a354e170b) )
	ROM_LOAD( "lxi05a__.1o1", 0x0000, 0x010000, CRC(7a5fe065) SHA1(c44b41d01175c10051ae4cd1453be3411842825e) )
	ROM_LOAD( "lxi10a__.1o1", 0x0000, 0x010000, CRC(17989464) SHA1(67aa9cc01d89ed4caeb33885f53dcaee762ccb6d) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "luxor.chr", 0x0000, 0x000048, CRC(21676e79) SHA1(b8f69e9aa35be1491655c0c52df277619892bdd8) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "luxorsnd1.hex", 0x000000, 0x080000, CRC(428daceb) SHA1(eec2b7efded3d0e0eea7faa5759a65a021465b13) )
	ROM_LOAD( "luxorsnd2.hex", 0x080000, 0x080000, CRC(860178e6) SHA1(705b1b0ad62a1b594bb123aec3c2b571a6500ce8) )

	ROM_LOAD( "luxor-snd1.bin", 0x000000, 0x080000, CRC(d09394e9) SHA1(d3cbdbaf048d829271a6c2846b16ceee7775d767) )
	ROM_LOAD( "luxor-snd2.bin", 0x080000, 0x080000, CRC(bc720cb9) SHA1(a83c25ecec602ba047dd21de2beec6cd7ac76cbe) )
ROM_END

ROM_START( m4madhse )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "madhouse.hex", 0x0000, 0x010000, CRC(3ec1955a) SHA1(6939e6f5d749249825c41df8e05957450eaf1007) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "madc.p1", 0x0000, 0x010000, CRC(96da2d58) SHA1(23686a4dc5adaac81ba173f8fa0ea5ff8ac26260) )
	ROM_LOAD( "madhouse.bin", 0x0000, 0x010000, CRC(e86e4542) SHA1(fb1b1d319c443daa1184eac4f6b0668ff3c6a1c5) )
	ROM_LOAD( "mads.p1", 0x0000, 0x010000, CRC(d4ea7f14) SHA1(808c64b65542c6bfd8336feec025e947c8c904ee) )
	ROM_LOAD( "md8c.p1", 0x0000, 0x010000, CRC(3d9c9cf7) SHA1(3f663fe9bd61d163d58bb4c51bea59121678aa76) )
	ROM_LOAD( "md8d.p1", 0x0000, 0x010000, CRC(6d150df3) SHA1(e93fda497696b06ad854b3b06e2b61737cef3fc1) )
	ROM_LOAD( "md8dy.p1", 0x0000, 0x010000, CRC(af01407f) SHA1(46359220e34dede4cc5e8c11699d01460dd9a469) )
	ROM_LOAD( "md8k.p1", 0x0000, 0x010000, CRC(c713f706) SHA1(65480171d3d69a670b9ce2c566425998134ad502) )
	ROM_LOAD( "md8s.p1", 0x0000, 0x010000, CRC(0d8a1a3e) SHA1(4df8b1c1834bbffb4798d9ed5135b6cb29b08e73) )
	ROM_LOAD( "md8y.p1", 0x0000, 0x010000, CRC(15fc9590) SHA1(a01bd5cea5873c8175262f668e330b2975a03eb1) )
	ROM_LOAD( "mh502ad.p1", 0x0000, 0x010000, CRC(55714741) SHA1(287ed4c0b070537e3cf9bf3a47bdf205e34b7ea8) )
	ROM_LOAD( "mh502b.p1", 0x0000, 0x010000, CRC(7d6a3e3a) SHA1(5ac43616bde8079d430c7a8f78884770396cc9e9) )
	ROM_LOAD( "mh502bd.p1", 0x0000, 0x010000, CRC(63bd7096) SHA1(ded53d603f7d1ed65914d9483923c74424964b59) )
	ROM_LOAD( "mh502d.p1", 0x0000, 0x010000, CRC(f618a54e) SHA1(947e155bf52a37edf38346b21a9e7171d12e07a1) )
	ROM_LOAD( "mh502dk.p1", 0x0000, 0x010000, CRC(968c9881) SHA1(30952b3811c13ebf6801df150ce24602a37414bb) )
	ROM_LOAD( "mh502dr.p1", 0x0000, 0x010000, CRC(430c2bee) SHA1(37fc62b1fda1de6b52d2a47b4015f6a57503bf2f) )
	ROM_LOAD( "mh502dy.p1", 0x0000, 0x010000, CRC(de670300) SHA1(32e17baed9c971e879974489ee9da375a0dbf735) )
	ROM_LOAD( "mh502k.p1", 0x0000, 0x010000, CRC(3aa341ec) SHA1(8012ee1d3f67fbdd5682ee07ac77dbb482b027ca) )
	ROM_LOAD( "mh502r.p1", 0x0000, 0x010000, CRC(a3aabdb4) SHA1(74e53a315b10264c23b8b00d6a4d5f99d3f204a3) )
	ROM_LOAD( "mh502s.p1", 0x0000, 0x010000, CRC(063cc07b) SHA1(0b43a5cf6094bd8c99e4395f31ff073389dd56ce) )
	ROM_LOAD( "mh502y.p1", 0x0000, 0x010000, CRC(3ec1955a) SHA1(6939e6f5d749249825c41df8e05957450eaf1007) )
	ROM_LOAD( "mhtad.p1", 0x0000, 0x010000, CRC(edfe01be) SHA1(5d738acc0a39906f085c1bc55caf683d6b6a4f6c) )
	ROM_LOAD( "mhtb.p1", 0x0000, 0x010000, CRC(272a3c62) SHA1(c6d71295d11350a0b778382a276b8bdf88faede9) )
	ROM_LOAD( "mhtbd.p1", 0x0000, 0x010000, CRC(f73a3808) SHA1(5f74eb64a9b12b9c2c1141b30e64325b8a5beece) )
	ROM_LOAD( "mhtd.p1", 0x0000, 0x010000, CRC(60fcc377) SHA1(e7ca02e2f966a72449a6a5c239160c8f3e46d249) )
	ROM_LOAD( "mhtdk.p1", 0x0000, 0x010000, CRC(ceafb47e) SHA1(32b7e23229524cc79ca24ee00368a9b4c76a35c7) )
	ROM_LOAD( "mhtdy.p1", 0x0000, 0x010000, CRC(36748788) SHA1(04a541f1a6b94dca2bff16d50674f968e896bea7) )
	ROM_LOAD( "mhtk.p1", 0x0000, 0x010000, CRC(1ebfb014) SHA1(493bf3ca37f2e49c5f00d7b8f6122e42f7b71f73) )
	ROM_LOAD( "mhts.p1", 0x0000, 0x010000, CRC(751b4574) SHA1(a04820f48e0df936813ca984c77da08d703e6474) )
	ROM_LOAD( "mhty.p1", 0x0000, 0x010000, CRC(e86e4542) SHA1(fb1b1d319c443daa1184eac4f6b0668ff3c6a1c5) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "m574.chr", 0x0000, 0x000048, CRC(cc4b7911) SHA1(9f8a96a1f8b0f9b33b852e93483ce5c684703349) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "madh1.hex", 0x000000, 0x080000, CRC(2b2af5dd) SHA1(eec0808bf724a055ece3c964d8a43cc5f837a3bd) )
	ROM_LOAD( "madh2.hex", 0x080000, 0x080000, CRC(487d8e1d) SHA1(89e01a153d17564eba112d882b686c91b6c3aecc) )
ROM_END


ROM_START( m4magdrg )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dmd10.bin", 0x0000, 0x010000, CRC(9cc4f2f8) SHA1(46a90ffa18d35ad2b06542f91120c02bc34f0c40) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "mdsnd.bin", 0x000000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END

ROM_START( m4maglin )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dma21.bin", 0x0000, 0x010000, CRC(836a25e6) SHA1(5f83bb8a2c77dd3b02724c076d6b37d2c1c93b93) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "mlsound1.p1", 0x000000, 0x080000, CRC(ff8749ff) SHA1(509b53f09cdfe5ee865e60ab42fd578586ac53ea) )
	ROM_LOAD( "mlsound2.p2", 0x080000, 0x080000, CRC(c8165b6c) SHA1(7c5059ee8630da31fc3ad50d84a4730297757d46) )
ROM_END

ROM_START( m4magrep )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dmr13.bin", 0x0000, 0x010000, CRC(c3015da3) SHA1(23cd505eedf666c012e4064a5fcf5a983f098e83) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "mrdsound.bin", 0x000000, 0x080000, CRC(9b035fa6) SHA1(51b7e5bc3abdf4f1beba2347146a91a2b3f4de35) )
ROM_END

ROM_START( m4magtbo )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "crmtb14.epr", 0x0000, 0x010000, CRC(79e1746c) SHA1(794317f3aba7b1a7994cde89d81abc2b687d0821) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "ctp.chr", 0x0000, 0x000048, CRC(ead61793) SHA1(f38a38601a67804111b8f8cf0a05d35ed79b7ed1) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "scrmtb.snd", 0x000000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END

ROM_START( m4mag7s )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "mag7.hex", 0x0000, 0x020000, CRC(5f012d8e) SHA1(069b493285df9ac3639c43349245a77890333dcc) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ma714s", 0x0000, 0x020000, CRC(9c1d4f97) SHA1(7875f044f992b313f4dfaae2e7b604baf16387a3) )
	ROM_LOAD( "ma715ad.p1", 0x0000, 0x020000, CRC(f807cc3f) SHA1(d402a1bf6b9a69d26b0806da83d6a943760aa6ed) )
	ROM_LOAD( "ma715b.p1", 0x0000, 0x020000, CRC(bedfd8b5) SHA1(a2bcd42e7163779aa7bb74ddc4a44d07f1179994) )
	ROM_LOAD( "ma715bd.p1", 0x0000, 0x020000, CRC(b2c06469) SHA1(00ed3998c24c7f03fc270e3025d3815163b6ff38) )
	ROM_LOAD( "ma715d.p1", 0x0000, 0x020000, CRC(9d4c2afe) SHA1(2833f9de808b2630ca67405098c3c502d597b6ca) )
	ROM_LOAD( "ma715dh.p1", 0x0000, 0x020000, CRC(9615f2ba) SHA1(ac6bc67bc740b6efd3050f9f5bc553acc25d9be6) )
	ROM_LOAD( "ma715dk.p1", 0x0000, 0x020000, CRC(4b08770e) SHA1(e748b800804cd2505d10e5e7168a3dffaf007c7c) )
	ROM_LOAD( "ma715dr.p1", 0x0000, 0x020000, CRC(d1e6a9b7) SHA1(d5c67cbc07931831159c801e0f4abfdc477980b8) )
	ROM_LOAD( "ma715dy.p1", 0x0000, 0x020000, CRC(7fe4ecc1) SHA1(ce4abc96d8d50683bee31c9bce7835dd76065a9b) )
	ROM_LOAD( "ma715h.p1", 0x0000, 0x020000, CRC(9a0a4e66) SHA1(ef8acb1f2deda724ff7b2850b1ce7a09f88ac011) )
	ROM_LOAD( "ma715k.p1", 0x0000, 0x020000, CRC(4717cbd2) SHA1(b88b17dda9e16ecbcce55bdb99c23d9e8267b7b9) )
	ROM_LOAD( "ma715r.p1", 0x0000, 0x020000, CRC(ddf9156b) SHA1(b8fb10944d910cd72a3268a80e8cd1b07dbee8f3) )
	ROM_LOAD( "ma715s.p1", 0x0000, 0x020000, CRC(6518c171) SHA1(df884c875f3cfb8e12fb35550dd1f5b331e4b204) )
	ROM_LOAD( "ma715y.p1", 0x0000, 0x020000, CRC(73fb501d) SHA1(2449da89e811ebf27970a8a9336107f85d876229) )
	ROM_LOAD( "ma716ad.p1", 0x0000, 0x020000, CRC(7632ce64) SHA1(2308d777110aa7636c4c0fc08be23d1732ba3b69) )
	ROM_LOAD( "ma716b.p1", 0x0000, 0x020000, CRC(cc8e289a) SHA1(7f5bac0374ff0eb0395f8d3c18ddad82c3e9c51d) )
	ROM_LOAD( "ma716bd.p1", 0x0000, 0x020000, CRC(3cf56632) SHA1(43be834b4d2d3a7fb9893e966ee909971319a10e) )
	ROM_LOAD( "ma716d.p1", 0x0000, 0x020000, CRC(ef1ddad1) SHA1(67f865856ba497e712a93ae3d2d308fe7e17fc01) )
	ROM_LOAD( "ma716dh.p1", 0x0000, 0x020000, CRC(1820f0e1) SHA1(4592cb213002d28450ac3da748a77501483a6816) )
	ROM_LOAD( "ma716dk.p1", 0x0000, 0x020000, CRC(c53d7555) SHA1(4d9ab16b7d28cf99fb8cbe3dc84295c634f7460e) )
	ROM_LOAD( "ma716dr.p1", 0x0000, 0x020000, CRC(5fd3abec) SHA1(a0187c62ed1949400c3e1d82071b2d500192bc90) )
	ROM_LOAD( "ma716dy.p1", 0x0000, 0x020000, CRC(f1d1ee9a) SHA1(d67d47891235624b4606244b275fbfee56517c77) )
	ROM_LOAD( "ma716h.p1", 0x0000, 0x020000, CRC(e85bbe49) SHA1(83ff53b1f4d2c14d313905b3194139089b1f0363) )
	ROM_LOAD( "ma716k.p1", 0x0000, 0x020000, CRC(35463bfd) SHA1(c288189f5ca7f00d1ae66521f77976275da39cc1) )
	ROM_LOAD( "ma716r.p1", 0x0000, 0x020000, CRC(afa8e544) SHA1(83f7d80ec359ba832caa9814e9a9fa04174ae596) )
	ROM_LOAD( "ma716s.p1", 0x0000, 0x020000, CRC(30fd2e9f) SHA1(9ed06ee736a09b36f48fb3b69be03b39861b0ea5) )
	ROM_LOAD( "ma716y.p1", 0x0000, 0x020000, CRC(01aaa032) SHA1(9b27b5b90cbf89e537110964837507cff4573094) )
	ROM_LOAD( "mag715g", 0x0000, 0x020000, CRC(5a28a94b) SHA1(16621ac2294dd7f0b9e58125ba800331e879f39e) )
	ROM_LOAD( "mag715t", 0x0000, 0x020000, CRC(cdea8e84) SHA1(ad38be8bb5e1247c53c7b48ec6dceebe3e757c9c) )
	ROM_LOAD( "mag7s1-6_15.bin", 0x0000, 0x020000, CRC(2a4d7328) SHA1(78b6b358e7ff3efd086512550e7690f59ee4b225) )
	ROM_LOAD( "mas12ad.p1", 0x0000, 0x020000, CRC(46c6522a) SHA1(e8bfa00afb0e07c524023a92c715f191ad26e759) )
	ROM_LOAD( "mas12b.p1", 0x0000, 0x020000, CRC(9225a526) SHA1(7c9139c45ddfbcbbabf59c28f17d19ce3b8f7c05) )
	ROM_LOAD( "mas12bd.p1", 0x0000, 0x020000, CRC(cbdcfcef) SHA1(11eb24b04535e8fc922bc802ab2f8af5e70881d5) )
	ROM_LOAD( "mas12c.p1", 0x0000, 0x020000, CRC(a56f1834) SHA1(9a3bcca0ff62100a36b1de06c621108910da51d8) )
	ROM_LOAD( "mas12d.p1", 0x0000, 0x020000, CRC(ef3eae50) SHA1(0eb26a6ffc963a84bfb11934bd9c1fecfec15f2a) )
	ROM_LOAD( "mas12dh.p1", 0x0000, 0x020000, CRC(0530acac) SHA1(43d73351b5002afbc0e14c49056c7cac17c93854) )
	ROM_LOAD( "mas12dk.p1", 0x0000, 0x020000, CRC(811b54b9) SHA1(c54b50b64856615eb362e0de889b7d758866ee77) )
	ROM_LOAD( "mas12dr.p1", 0x0000, 0x020000, CRC(3214ef88) SHA1(8f0ab310363882c92fb4d58197ee093822e9611c) )
	ROM_LOAD( "mas12dy.p1", 0x0000, 0x020000, CRC(06f87447) SHA1(9df82a89c9846eb51a3719870b91dd675a97cbe6) )
	ROM_LOAD( "mas12h.p1", 0x0000, 0x020000, CRC(5cc9f565) SHA1(833c77ebd895e884430193d14edf0882b5bc5a75) )
	ROM_LOAD( "mas12k.p1", 0x0000, 0x020000, CRC(d8e20d70) SHA1(a411a06b89ff21ad7f55196f80dd2766a99bfc21) )
	ROM_LOAD( "mas12r.p1", 0x0000, 0x020000, CRC(6bedb641) SHA1(39b5a8dfb581ae6ed209c6f2a12c06b38ad861a3) )
	ROM_LOAD( "mas12s.p1", 0x0000, 0x020000, CRC(0a94e574) SHA1(e4516638fb7f783e79cfcdbbef1188965351eae2) )
	ROM_LOAD( "mas12y.p1", 0x0000, 0x020000, CRC(5f012d8e) SHA1(069b493285df9ac3639c43349245a77890333dcc) )
	ROM_LOAD( "mas13ad.p1", 0x0000, 0x020000, CRC(f39f7e0e) SHA1(39453c09c2e8b84fce9e53fbe86244f2381e2ac5) )
	ROM_LOAD( "mas13b.p1", 0x0000, 0x020000, CRC(71863b1f) SHA1(6214d485509f827dd7bbcd00855d6caaec13ba09) )
	ROM_LOAD( "mas13bd.p1", 0x0000, 0x020000, CRC(7e85d0cb) SHA1(598338482bbe0e903983a4061b8f772cb24e4961) )
	ROM_LOAD( "mas13c.p1", 0x0000, 0x020000, CRC(46cc860d) SHA1(cd6dda5a822ac451cb9d9dcc21fa961a4bd26a71) )
	ROM_LOAD( "mas13d.p1", 0x0000, 0x020000, CRC(0c9d3069) SHA1(160b242d01fcf752d4bce35bb1b1b81a44b6afa0) )
	ROM_LOAD( "mas13dh.p1", 0x0000, 0x020000, CRC(b0698088) SHA1(3b7999cb85661a63914d9eacdb3e0d223b245a3d) )
	ROM_LOAD( "mas13dk.p1", 0x0000, 0x020000, CRC(3442789d) SHA1(3ae7b8d18e7fdc5104ed43b35743094e25e2819e) )
	ROM_LOAD( "mas13dr.p1", 0x0000, 0x020000, CRC(874dc3ac) SHA1(cd20aa0b794748d7b853d1e5c8bee49bec03efc8) )
	ROM_LOAD( "mas13dy.p1", 0x0000, 0x020000, CRC(b3a15863) SHA1(d57365c3a1291ccf5019be1bb6b6245168c2bcb8) )
	ROM_LOAD( "mas13h.p1", 0x0000, 0x020000, CRC(bf6a6b5c) SHA1(a89632a6f71d9a480193924e9e3d678bc06b6f3e) )
	ROM_LOAD( "mas13k.p1", 0x0000, 0x020000, CRC(3b419349) SHA1(d72f6493a6f7631b44f27e9cde499bdd192b5e0a) )
	ROM_LOAD( "mas13r.p1", 0x0000, 0x020000, CRC(884e2878) SHA1(c4f070019116543f4683c088d915709c040145ff) )
	ROM_LOAD( "mas13s.p1", 0x0000, 0x020000, CRC(80ca53c0) SHA1(19e67a259fca2fca3990f032d7825d67309d47d3) )
	ROM_LOAD( "mas13y.p1", 0x0000, 0x020000, CRC(bca2b3b7) SHA1(ff48b91578230bc77529cb59fbcb7e3bd77b946d) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "m7_sj_dc.2r1", 0x0000, 0x020000, CRC(0eefd40c) SHA1(2c30bc42d23c7cfb0a382b47f7ed865865341e2f) )
	ROM_LOAD( "m7_sja_c.2r1", 0x0000, 0x020000, CRC(3933772c) SHA1(5e9d12a9f58ce5129634b8a4c8c0f083031df295) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "mag7.chr", 0x0000, 0x000048, CRC(4835e911) SHA1(7171cdabf6cf76e09ea55b41f0f8a98b94032486) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "ma7snd1.hex", 0x000000, 0x080000, CRC(f0e31329) SHA1(60b94c3223c8863fe801b93f65ff65e94f3dec83) )
	ROM_LOAD( "ma7snd2.hex", 0x080000, 0x080000, CRC(12110d16) SHA1(fa93a263d1e3fa8b0b2f618f52e5145330f4315d) )
ROM_END

ROM_START( m4makmnt )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "mams.p1", 0x0000, 0x020000, CRC(af08e1e6) SHA1(c7e87d351f67592084d758ee53ba4d354bb28866) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "mam04ad.p1", 0x0000, 0x020000, CRC(9b750bc7) SHA1(10a86f0a0d18ce0be502a9d36282f6b5eef0ece5) )
	ROM_LOAD( "mam04b.p1", 0x0000, 0x020000, CRC(8f5cefa9) SHA1(fc0dfb67794d090ef15facd0f2b60e1d505b295f) )
	ROM_LOAD( "mam04bd.p1", 0x0000, 0x020000, CRC(166fa502) SHA1(345ad131d8757445c549312350507a3a804ca20e) )
	ROM_LOAD( "mam04c.p1", 0x0000, 0x020000, CRC(b81652bb) SHA1(3e8544ac4b3e2d3845a1608dc821608e2d87088f) )
	ROM_LOAD( "mam04d.p1", 0x0000, 0x020000, CRC(f247e4df) SHA1(de4afd3058292e3aea1abd718da16b06691b5623) )
	ROM_LOAD( "mam04dk.p1", 0x0000, 0x020000, CRC(5ca80d54) SHA1(a82e9ebeb83cddf63f8fef129da4168734abcd48) )
	ROM_LOAD( "mam04dr.p1", 0x0000, 0x020000, CRC(efa7b665) SHA1(4cb48215d69ee75efa2eccb6edf1526d497dba9c) )
	ROM_LOAD( "mam04dy.p1", 0x0000, 0x020000, CRC(db4b2daa) SHA1(ba2fce6d7b6aa95a11f0054720e577db7415f1d2) )
	ROM_LOAD( "mam04k.p1", 0x0000, 0x020000, CRC(c59b47ff) SHA1(8022c6c988532d3f98edf5c59c97403ab2d0fb90) )
	ROM_LOAD( "mam04r.p1", 0x0000, 0x020000, CRC(7694fcce) SHA1(a67c76551240129914cc071543db96962f3b198f) )
	ROM_LOAD( "mam04s.p1", 0x0000, 0x020000, CRC(08eac690) SHA1(e35793da266bd9dd8a018ba9773f368e36ce501d) )
	ROM_LOAD( "mam04y.p1", 0x0000, 0x020000, CRC(42786701) SHA1(6efb0cbf630cd1b87715e692f76e368e3fba0856) )
	ROM_LOAD( "mam15g", 0x0000, 0x020000, CRC(d3fd61f9) SHA1(1c738f818ea84f4bfca3c62fd9c34ce5e983b10a) )
	ROM_LOAD( "mam15t", 0x0000, 0x020000, CRC(a5975cbe) SHA1(eb6dd70c79c6b051190055d71ec8421080e5ba39) )
	ROM_LOAD( "mamad.p1", 0x0000, 0x020000, CRC(82f63f55) SHA1(2cbc514a49e826505580a57f17ee696bdf9bf436) )
	ROM_LOAD( "mamb.p1", 0x0000, 0x020000, CRC(233c2e35) SHA1(823fc5736469c7e1d1da72bec8d64aabb277f9ab) )
	ROM_LOAD( "mambd.p1", 0x0000, 0x020000, CRC(0fec9190) SHA1(6cea986853efc042c6325f31f790f80ae2993308) )
	ROM_LOAD( "mamc.p1", 0x0000, 0x020000, CRC(14769327) SHA1(435406a46a60666bbe6ebc9392b21d5b0404cffd) )
	ROM_LOAD( "mamd.p1", 0x0000, 0x020000, CRC(5e272543) SHA1(c974b21ba488568e12d47f51f21d3b94d40255f3) )
	ROM_LOAD( "mamdk.p1", 0x0000, 0x020000, CRC(452b39c6) SHA1(8b065391abd01c7b9d6c5beb95cf17a52c8ebe1a) )
	ROM_LOAD( "mamdy.p1", 0x0000, 0x020000, CRC(c2c81938) SHA1(de95d1e28af33dc96343cc615a945c5dfe3f04a7) )
	ROM_LOAD( "mamk.p1", 0x0000, 0x020000, CRC(69fb8663) SHA1(741b6e7b97d0c9b243e8e318ed169f92f8fbd5e9) )
	ROM_LOAD( "mamr.p1", 0x0000, 0x020000, CRC(daf43d52) SHA1(066cf554178d4e4fdff10c1e93567618f711d196) )
	ROM_LOAD( "mamy.p1", 0x0000, 0x020000, CRC(ee18a69d) SHA1(d3cf2c5ed4ec00be4d68c895ca3973da1acccb79) )
	ROM_LOAD( "mint2010", 0x0000, 0x020000, CRC(e60d10b9) SHA1(3abe7a7f33a73827ed6585d92fe53d4058c87baf) )
	ROM_LOAD( "mmg04ad.p1", 0x0000, 0x020000, CRC(9cbf9691) SHA1(a68f2e9e0ec03dc47017c221a3e780e5cc992a15) )
	ROM_LOAD( "mmg04b.p1", 0x0000, 0x020000, CRC(2507742d) SHA1(3bdd6f43da4d4c923bedcc1eba5cb1e1b92ee473) )
	ROM_LOAD( "mmg04bd.p1", 0x0000, 0x020000, CRC(11a53854) SHA1(e52ee92f39645380864e86c694a345c028cb42cf) )
	ROM_LOAD( "mmg04c.p1", 0x0000, 0x020000, CRC(124dc93f) SHA1(f58a4d6830ece84467a0b247ba66132d659d1383) )
	ROM_LOAD( "mmg04d.p1", 0x0000, 0x020000, CRC(581c7f5b) SHA1(224a9e4f6078d38606c7b08a15df05fbcb31a209) )
	ROM_LOAD( "mmg04dk.p1", 0x0000, 0x020000, CRC(5b629002) SHA1(e08f55e157973518837d1045fe714221d5ba812d) )
	ROM_LOAD( "mmg04dr.p1", 0x0000, 0x020000, CRC(e86d2b33) SHA1(594c4fdba38f2463467e4906981738dafc15369d) )
	ROM_LOAD( "mmg04dy.p1", 0x0000, 0x020000, CRC(dc81b0fc) SHA1(3154899d80375413936d2c08edaaf2e97d490c5b) )
	ROM_LOAD( "mmg04k.p1", 0x0000, 0x020000, CRC(6fc0dc7b) SHA1(4ad8ef1e666e4796d3a09ba4bda9e48c60266380) )
	ROM_LOAD( "mmg04r.p1", 0x0000, 0x020000, CRC(dccf674a) SHA1(a8b2ebeec8587e0655349baca700afc552c3e62d) )
	ROM_LOAD( "mmg04s.p1", 0x0000, 0x020000, CRC(1c46683e) SHA1(d08589bb32056ea599e6ffbbb795f46f8eff0782) )
	ROM_LOAD( "mmg04y.p1", 0x0000, 0x020000, CRC(e823fc85) SHA1(10414bbac8ac3bdbf11bc4092370c499fb9db650) )
	ROM_LOAD( "mmg05ad.p1", 0x0000, 0x020000, CRC(d1e70066) SHA1(9635da90808628ae84c6073fef9622a8f37bd069) )
	ROM_LOAD( "mmg05b.p1", 0x0000, 0x020000, CRC(cc0335ff) SHA1(9ca52a49cc48cbfa7c394e0c22cc5075ad1096a1) )
	ROM_LOAD( "mmg05bd.p1", 0x0000, 0x020000, CRC(5cfdaea3) SHA1(f9c5c4b4021ace3e17818c4d4462fe9c87a64d70) )
	ROM_LOAD( "mmg05c.p1", 0x0000, 0x020000, CRC(fb4988ed) SHA1(a6ec8dac9e9a5eda67314286c27a8ce177663030) )
	ROM_LOAD( "mmg05d.p1", 0x0000, 0x020000, CRC(b1183e89) SHA1(0f93a811e29ecf40de4338660c30b75f1d565c63) )
	ROM_LOAD( "mmg05dk.p1", 0x0000, 0x020000, CRC(163a06f5) SHA1(cd5fa8d7a2edfdadcfc2d96158bbdfdc74c76068) )
	ROM_LOAD( "mmg05dr.p1", 0x0000, 0x020000, CRC(a535bdc4) SHA1(0d2314f5cb72949c57e759b8f29955c762d9d2fc) )
	ROM_LOAD( "mmg05dy.p1", 0x0000, 0x020000, CRC(91d9260b) SHA1(4442dea0682b737c0053ec4f1109114cdeb3d422) )
	ROM_LOAD( "mmg05k.p1", 0x0000, 0x020000, CRC(86c49da9) SHA1(bf29075a87574009f9ad8fd36e2d3a84c50e6b26) )
	ROM_LOAD( "mmg05r.p1", 0x0000, 0x020000, CRC(35cb2698) SHA1(6371313a179559240ddb55976546ecc8d511e104) )
	ROM_LOAD( "mmg05s.p1", 0x0000, 0x020000, CRC(771c17c8) SHA1(d9e595ae020c48769fcbf3de718b6986b6fd8bc5) )
	ROM_LOAD( "mmg05y.p1", 0x0000, 0x020000, CRC(0127bd57) SHA1(3d1b59fda52f09fd8af59177b3f5c614b453ac25) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ma_x6__5.3_1", 0x0000, 0x010000, CRC(2fe3c309) SHA1(5dba65b29ea5492a78866863629d89f9a8588959) )
	ROM_LOAD( "ma_x6__c.3_1", 0x0000, 0x010000, CRC(e9259a4d) SHA1(9a8e9590403f507f83197a898af5d543bda81b2b) )
	ROM_LOAD( "ma_x6_d5.3_1", 0x0000, 0x010000, CRC(a93dba0d) SHA1(6fabe994ac6c9ea4ce2bae99df699fa100098926) )
	ROM_LOAD( "ma_x6_dc.3_1", 0x0000, 0x010000, CRC(805d75c2) SHA1(b2433556b72f89887c1e404c80d85c940535e8af) )
	ROM_LOAD( "ma_x6a_5.3_1", 0x0000, 0x010000, CRC(79f673de) SHA1(805ea08f5ed016d25ec23dbc3952aad4873a1cde) )
	ROM_LOAD( "ma_x6a_c.3_1", 0x0000, 0x010000, CRC(43ede82a) SHA1(d6ec3dd170c56e90018568480bca72cd8390aa2d) )


	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "mamsnd.p1", 0x000000, 0x080000, CRC(8dc408e3) SHA1(48a9ffc5cf4fd04ed1320619ca915bbfa2406750) )
	ROM_LOAD( "mamsnd.p2", 0x080000, 0x080000, CRC(6034e17a) SHA1(11e044c87b5fc6461b0c6cfac5c419daee930d7b) )
ROM_END

ROM_START( m4megbks )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bucx.p1", 0x0000, 0x010000, CRC(8ca051da) SHA1(d70c462a9c90b637b1a32c5e333d0d1e9d779caa) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "bucxc.p1", 0x0000, 0x010000, CRC(f2a8c98c) SHA1(97096b899d8873c8484bcb23cb2fc74bfe96fbb8) )
	ROM_LOAD( "bucxd.p1", 0x0000, 0x010000, CRC(7b0746f9) SHA1(9edd13b2ce75b149604ed032f4871c03e469712d) )
	ROM_LOAD( "mbucks.hex", 0x0000, 0x010000, CRC(9a3329b1) SHA1(77af020a278db0c50886bbf5c629e6509bdba0c8) )
ROM_END

ROM_START( m4meglnk )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dml20.bin", 0x0000, 0x010000, CRC(bbf48b45) SHA1(0ca9adf6a4171efad1af7b411e713dc35c654d30) )
ROM_END

ROM_START( m4milclb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "mi2d.p1", 0x0000, 0x010000, CRC(ce697bbd) SHA1(86c1729014eff9925a5f62189236a9c5bd11534b) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "mi2f.p1", 0x0000, 0x010000, CRC(224922a4) SHA1(59bc1fbfe20c533eb6462f01196a5f2d35ceb92d) )
	ROM_LOAD( "mi2s.hex", 0x0000, 0x010000, CRC(f69b69c7) SHA1(4f881f5307db2c100535fa75a8eb42d0f7382c93) )
	ROM_LOAD( "mi2s.p1", 0x0000, 0x010000, CRC(f69b69c7) SHA1(4f881f5307db2c100535fa75a8eb42d0f7382c93) )
	ROM_LOAD( "mild.p1", 0x0000, 0x010000, CRC(16da3df9) SHA1(0f1838f99c14763132c2a3b79363496c6baa5e88) )
	ROM_LOAD( "mils.p1", 0x0000, 0x010000, CRC(0742defd) SHA1(ac25d8adb40bc5b4124241cc5d970d4c10c6f5fd) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "millc.chr", 0x0000, 0x000048, CRC(5e94580c) SHA1(d8251caf825ba0a23c06459d270e1e0999bde5c4) )
ROM_END

ROM_START( m4mirage )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ragy.p1", 0x0000, 0x010000, CRC(4f56b698) SHA1(6b2726ee951c9c08ed8b8e7130bcd52210b8bf92) )
ROM_END

ROM_START( m4moneym )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "moneymaker-v16.bin", 0x0000, 0x010000, CRC(77f52665) SHA1(48a67b651385834658f1acb11ccde12b73393ced) )
ROM_END

ROM_START( m4monte )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "nm8b.p1", 0x0000, 0x010000, CRC(1632080e) SHA1(9ca2cd8f00e49c29f4a216d3c9eacba221ada6ce) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "nm8ad.p1", 0x0000, 0x010000, CRC(92a07e05) SHA1(94015b219fffb8ad9a40a804a4e0b0fad61cdf21) )
	ROM_LOAD( "nm8bd.p1", 0x0000, 0x010000, CRC(a4bc134f) SHA1(72af6b66a5ea7566289bd9bdf8975c29dbb547cf) )
	ROM_LOAD( "nm8c.p1", 0x0000, 0x010000, CRC(7e558a64) SHA1(9f325aa9a5b036c317686b901b4c65c1e23fd845) )
	ROM_LOAD( "nm8d.p1", 0x0000, 0x010000, CRC(66716e7d) SHA1(719d32a3486accfa1c2e8e2ca53c05f916927e7a) )
	ROM_LOAD( "nm8dk.p1", 0x0000, 0x010000, CRC(ae4866e8) SHA1(5ec210b6b69f72b85abe5844b800b251fef20fc5) )
	ROM_LOAD( "nm8dy.p1", 0x0000, 0x010000, CRC(9d6f71a5) SHA1(577d39eef82761fff30f851282cd85b84ac22953) )
	ROM_LOAD( "nm8k.p1", 0x0000, 0x010000, CRC(47c00612) SHA1(647216e7489043f90e0cd807ddc3d631842b3f7f) )
	ROM_LOAD( "nm8s.p1", 0x0000, 0x010000, CRC(cf8fd333) SHA1(4b2b98d0c3d043a6425a6d82f7a98cf662582832) )
	ROM_LOAD( "nm8y.p1", 0x0000, 0x010000, CRC(cbb96053) SHA1(9fb6c449d8e26ecacfa9ba40979134c705ecb1be) )
	ROM_LOAD( "nmnc.p1", 0x0000, 0x010000, CRC(c2fdcc91) SHA1(aa3ec11425adee94c24b3a1472541e7e04e4000a) )
	ROM_LOAD( "nmnd.p1", 0x0000, 0x010000, CRC(94985809) SHA1(636b9106ea330a238f3d4168636fbf21021a7216) )
	ROM_LOAD( "nmnk.p1", 0x0000, 0x010000, CRC(8d022ae6) SHA1(01e12acbed34a2d4fb81dc9da12441ddc31f605b) )
	ROM_LOAD( "nmns.p1", 0x0000, 0x010000, CRC(48e2ab70) SHA1(bc452a36374a6e62516aad1a4887876ee9da37f7) )
ROM_END


ROM_START( m4multcl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "mp28p1.bin", 0xc000, 0x004000, CRC(f0173557) SHA1(c86419fe12cca2b7395d83f6de68c5928bce64cb) )
	ROM_LOAD( "mp28p2.bin", 0x8000, 0x004000, CRC(416f6372) SHA1(745d5663e857cadd6fbee68786b6a04aa384f9c9) )
	ROM_LOAD( "mp28p3.bin", 0x6000, 0x002000, CRC(eedd1bb8) SHA1(518ad1d7b38c38613b42f2e00bcf47f92b4d7828) )
ROM_END

ROM_START( m4multwy )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dmu17.bin", 0x0000, 0x010000, CRC(336b128e) SHA1(1d8268bfa0ffee62c76ffbf0ee89731626cf90ca) )
ROM_END

ROM_START( m4nhtt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "nht01b.p1", 0x0000, 0x010000, CRC(8201a051) SHA1(a87550c0cdc0b14a30e8814bfef939eb5cf414f8) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "nht01ad.p1", 0x0000, 0x010000, CRC(a5c6ce9a) SHA1(f21dcc1a70fa45637f236aede9c6fa2e962af8f5) )
	ROM_LOAD( "nht01bd.p1", 0x0000, 0x010000, CRC(21c50c56) SHA1(66c7dfa15447a2519cad58daebe0832c4c2f6f5e) )
	ROM_LOAD( "nht01d.p1", 0x0000, 0x010000, CRC(4d0868a0) SHA1(1f70273928582b87693f046e10e22c19d6bcf87e) )
	ROM_LOAD( "nht01dk.p1", 0x0000, 0x010000, CRC(0826d154) SHA1(154d4bd702ffc13603adc4a72aa1f18e486aec30) )
	ROM_LOAD( "nht01dr.p1", 0x0000, 0x010000, CRC(77862195) SHA1(69f4a50507cf608e0a060231aaf09fc5690787ad) )
	ROM_LOAD( "nht01dy.p1", 0x0000, 0x010000, CRC(954df9ba) SHA1(61cfb2c68921576549d40ba6776877322e4dd338) )
	ROM_LOAD( "nht01k.p1", 0x0000, 0x010000, CRC(c9ab03b3) SHA1(acc26a54bfe6e26fc2c8ac58268ee9347bc4ddb9) )
	ROM_LOAD( "nht01r.p1", 0x0000, 0x010000, CRC(f5ec653e) SHA1(aed9320ab164dd0f2b3dfaee3aacde5ba62e31ef) )
	ROM_LOAD( "nht01s.p1", 0x0000, 0x010000, CRC(a4a44ddf) SHA1(e64953f3cd2559a8ebdacb2b0c12c84fd5c4b836) )
	ROM_LOAD( "nht01y.p1", 0x0000, 0x010000, CRC(54c02b5d) SHA1(75b3056d714ee232325f8a1058bef46d902d0b64) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "nhtsnd01.p1", 0x0000, 0x080000, CRC(2d1d93c6) SHA1(80a8d131bafdb74d20d1ca5cbe2219ee4df0b675) )
ROM_END

ROM_START( m4nick )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "nick_21.bin", 0x0000, 0x010000, CRC(77a0ba57) SHA1(d56bdd52f81d707b5138f00e68b10130cac1225f) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "nilc.p1", 0x0000, 0x010000, CRC(8e612b50) SHA1(a33142ca3988e449ae94978946ed0f171c52c5fa) )
	ROM_LOAD( "nilx.p1", 0x0000, 0x010000, CRC(210be67b) SHA1(b4f7b955ffe6a991f06334cb7eb2aebcf5fe11b3) )
	ROM_LOAD( "nilxb.p1", 0x0000, 0x010000, CRC(41fde39d) SHA1(fbb179d942a1ffb9c84925402179c7c7fd0a7692) )
	ROM_LOAD( "nilxc.p1", 0x0000, 0x010000, CRC(38cabacf) SHA1(67aafaecc93a348dcdf7beaf6c93c16101fccb55) )
	ROM_LOAD( "nilxk.p1", 0x0000, 0x010000, CRC(ceb04af2) SHA1(1cd65356fba532b4c34e03b418708b982e8e0828) )
ROM_END

ROM_START( m4nifty )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "nifty.hex", 0x0000, 0x010000, CRC(84931755) SHA1(b4e568e1e4c237ea3f7ca6156b8a89cb40faf425) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "nf2_1.p2", 0x0000, 0x002000, CRC(fde4010e) SHA1(440c888e80bd65f4d8c4081be66ad79db8e19618) )
	ROM_LOAD( "nf2_1c.p1", 0x0000, 0x008000, CRC(52e954a5) SHA1(c402eaaf3f482d996f9a1312f97f057627734416) )
	ROM_LOAD( "nf2_1l.p1", 0x0000, 0x008000, CRC(bb297210) SHA1(cb570a015699d396dacb9fb09397ef157ceb8c97) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "nifty.chr", 0x0000, 0x000048, CRC(1ce9977d) SHA1(06616842d4062e3285044a9924e35faee3f8f3f5) )
ROM_END

ROM_START( m4nspot )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "nits.p1", 0x0000, 0x010000, CRC(47c965e6) SHA1(41a337a9a367c4e704a60e32d56b262d03f97b59) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ns2d.p1", 0x0000, 0x010000, CRC(5e66b7e0) SHA1(e82044e3c1e5cf3a2baf1fde7b7ab8b6e221d360) )
	ROM_LOAD( "ns2s.p1", 0x0000, 0x010000, CRC(ba0f5a81) SHA1(7015176d4528636cb8a753249c824c37941e8eae) )
ROM_END

ROM_START( m4nile )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "gjn08.p1", 0x0000, 0x020000, CRC(2bafac0c) SHA1(363d08f798b5bea409510b1a9415098a69f19ee0) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "gjnsnd.p1", 0x000000, 0x080000, CRC(1d839591) SHA1(2e4ba74f96e7c0592b85409a3f50ec81e00e064c) )
	ROM_LOAD( "gjnsnd.p2", 0x080000, 0x080000, CRC(e2829c42) SHA1(2139c1625ad163cce99a522c2cf02ee47a8f9007) )
	ROM_LOAD( "gjnsnd.p3", 0x100000, 0x080000, CRC(db084eb4) SHA1(9b46a3cb16974942b0edd25b1b080d30fc60c3df) )
	ROM_LOAD( "gjnsnd.p4", 0x180000, 0x080000, CRC(da785b0a) SHA1(63358ab197eb1de8e489a9fd6ffbc2039efc9536) )
ROM_END

ROM_START( m4nudgew )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "naw0_4.bin", 0x0000, 0x010000, CRC(0201f6f9) SHA1(48772611db7ae0cda48b8d725fdc8ef50e64d6ad) )
	ROM_IGNORE(0x10000) // rom too big?
ROM_END

ROM_START( m4nudbnk )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "nb6", 0x0000, 0x010000, CRC(010dd3fc) SHA1(645cbe54200a6c3327e10909b1ef3a80579e96e5) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "nbncx.p1", 0x0000, 0x010000, CRC(57bbbedf) SHA1(d42d3176f41aedf2ddc15cdf73ab97e963b92213) )
	ROM_LOAD( "nbnx.p1", 0x0000, 0x010000, CRC(075053d5) SHA1(43b9f6bb3a4ab531eb168007ceaf713261736144) )
	ROM_LOAD( "sbns.p1", 0x0000, 0x010000, CRC(92aa5b8d) SHA1(4f6e309e152266b8f40077a7d734b2b9042570d2) )
	ROM_LOAD( "sbnx.p1", 0x0000, 0x010000, CRC(861cbc50) SHA1(61166ea9092e2890ea9de421cc031d3a79335233) )
ROM_END

ROM_START( m4nnww )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "wink.hex", 0x0000, 0x010000, CRC(56cc9559) SHA1(53e109a579e422932dd25c52cf2beca51d3a53e3) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cf301s", 0x0000, 0x010000, CRC(1d8abf59) SHA1(81e47797baddd777fbbb1b1e044df1bfe3d49cb2) )
	ROM_LOAD( "cni01ad.p1", 0x0000, 0x010000, CRC(788e47b1) SHA1(6d07500a38b54e1a9038e35d82fdb4a0f22d23ba) )
	ROM_LOAD( "cni01b.p1", 0x0000, 0x010000, CRC(33512643) SHA1(865ed3b68fe3b737833734513b5045c5db97791e) )
	ROM_LOAD( "cni01bd.p1", 0x0000, 0x010000, CRC(8a00d73b) SHA1(702579ea1bc586aacd5cba889919f3e86ea05771) )
	ROM_LOAD( "cni01c.p1", 0x0000, 0x010000, CRC(b836ee44) SHA1(832914461492f120894ec7e63f6aa1ad00b89b41) )
	ROM_LOAD( "cni01d.p1", 0x0000, 0x010000, CRC(94fbe9cb) SHA1(7daabf1cd315f8d18796ba34f8c2ec271cc1e396) )
	ROM_LOAD( "cni01dk.p1", 0x0000, 0x010000, CRC(708fbcca) SHA1(7e97d8adf660099873a94d1915c79f110614cb11) )
	ROM_LOAD( "cni01dr.p1", 0x0000, 0x010000, CRC(5b7ed753) SHA1(8072ac849dc61e50963ae6730fa32823bd038c77) )
	ROM_LOAD( "cni01dy.p1", 0x0000, 0x010000, CRC(fcf6da8b) SHA1(95d86af30035884211ed26ccb5db9aae12ac7bf2) )
	ROM_LOAD( "cni01k.p1", 0x0000, 0x010000, CRC(f7c90833) SHA1(3b3b44e61f24e9fb45f465fd9c381fe81b6851a0) )
	ROM_LOAD( "cni01r.p1", 0x0000, 0x010000, CRC(c611b1eb) SHA1(524ee18da8a086d15277d9fb0ea383ee3d49d47a) )
	ROM_LOAD( "cni01s.p1", 0x0000, 0x010000, CRC(5ed6a396) SHA1(299767467b56d1aa93602f98cc387e7ff18bda9d) )
	ROM_LOAD( "cni01y.p1", 0x0000, 0x010000, CRC(d3612bf2) SHA1(40a8ff08a38c4411946a67f380891945d166d199) )
	ROM_LOAD( "cnuad.p1", 0x0000, 0x010000, CRC(f4b28628) SHA1(7323525a44477e2a3f89562f6094ed7bb47a16cc) )
	ROM_LOAD( "cnub.p1", 0x0000, 0x010000, CRC(735260a3) SHA1(e08fff6314d7cb4e396107366fdc16dcbf7f5d67) )
	ROM_LOAD( "cnubd.p1", 0x0000, 0x010000, CRC(fbe1ee39) SHA1(21bdaa6f9af686b4e44958ee09a131d0e12c2c53) )
	ROM_LOAD( "cnud.p1", 0x0000, 0x010000, CRC(d3a0eff1) SHA1(2b18c3e14a43d072ae5702bc77fcac65dbd8305c) )
	ROM_LOAD( "cnudk.p1", 0x0000, 0x010000, CRC(a7b506e8) SHA1(40d712076b434a339dfa60b937eec91038568312) )
	ROM_LOAD( "cnudr.p1", 0x0000, 0x010000, CRC(e163caea) SHA1(273a13567e5cb7fd071dfc9c8a9bc923e25d7679) )
	ROM_LOAD( "cnudy.p1", 0x0000, 0x010000, CRC(7c08e204) SHA1(34c906f3a284fde0c997232738a51b709a0dca93) )
	ROM_LOAD( "cnuk.p1", 0x0000, 0x010000, CRC(b9c08873) SHA1(9c5a754a7b57c8ab4334afdcbe30884a7181ac48) )
	ROM_LOAD( "cnur.p1", 0x0000, 0x010000, CRC(729d89ea) SHA1(c98a89dd8f85dde7ab005bcb7eba1fcc31162e08) )
	ROM_LOAD( "cnus.p1", 0x0000, 0x010000, CRC(6afee8e1) SHA1(35464eef29a5a66b8efea890987ff120ca5b7409) )
	ROM_LOAD( "cnuy.p1", 0x0000, 0x010000, CRC(eff6a104) SHA1(021baf5fe88defca05627a85501622d86e846233) )
	ROM_LOAD( "nn3xad.p1", 0x0000, 0x010000, CRC(8ccfceb8) SHA1(762ab26826d3d2a4dd7999a71724389344e9dafb) )
	ROM_LOAD( "nn3xb.p1", 0x0000, 0x010000, CRC(9b0dd473) SHA1(9975dafea8c7d6ccfc9f826adb1a0d3d0ed9740a) )
	ROM_LOAD( "nn3xbd.p1", 0x0000, 0x010000, CRC(21bf4a89) SHA1(200c9ccc4bc2a93fcd0f68bb00ad4391bdeecda1) )
	ROM_LOAD( "nn3xd.p1", 0x0000, 0x010000, CRC(11e22c45) SHA1(6da31eea7b25612d99cc79f6f9579622f105c862) )
	ROM_LOAD( "nn3xdk.p1", 0x0000, 0x010000, CRC(0f4642c6) SHA1(53a0b8bc102c2b1c0db71887470b70852b09a4e9) )
	ROM_LOAD( "nn3xdy.p1", 0x0000, 0x010000, CRC(ba3c1cf0) SHA1(ab94227018c3f9173e6a648749d455afd1ed36ce) )
	ROM_LOAD( "nn3xk.p1", 0x0000, 0x010000, CRC(ec3a9831) SHA1(0b3ba86faf39cf3a1e42cb1c31fd2c50c24d65dc) )
	ROM_LOAD( "nn3xr.p1", 0x0000, 0x010000, CRC(6416481c) SHA1(b06ed4964d9cbf403905504ac68abdab53131476) )
	ROM_LOAD( "nn3xrd.p1", 0x0000, 0x010000, CRC(0fd3f9b9) SHA1(99115b217cfc54b52469ffc77e7a7592907c53ea) )
	ROM_LOAD( "nn3xs.p1", 0x0000, 0x010000, CRC(13d02d21) SHA1(8e4dac8e60538884d3f3a92fc1bb9f41276be4c8) )
	ROM_LOAD( "nn3xy.p1", 0x0000, 0x010000, CRC(8a5d0f4b) SHA1(ef727e7ee8bb20d1b201927186a1a4f83e1e7497) )
	ROM_LOAD( "nn4ad.p1", 0x0000, 0x010000, CRC(827b832f) SHA1(4448ccb03282b9d39c6a00d02cea4d8ce2225b0e) )
	ROM_LOAD( "nn4b.p1", 0x0000, 0x010000, CRC(65e16330) SHA1(cfd18693155b4b7c5692064a2f693eb198d02749) )
	ROM_LOAD( "nn4bd.p1", 0x0000, 0x010000, CRC(b467ee65) SHA1(79030aa06ca8fd9c8becff62d56628939e9b5075) )
	ROM_LOAD( "nn4d.p1", 0x0000, 0x010000, CRC(548dacb9) SHA1(55949910374fae419ba015b70780e3e9e269caa0) )
	ROM_LOAD( "nn4dk.p1", 0x0000, 0x010000, CRC(9053aa15) SHA1(99d1e6d8776434a4ec69a565d673b45402467b8d) )
	ROM_LOAD( "nn4dy.p1", 0x0000, 0x010000, CRC(5fcd5a18) SHA1(b1b3283a303114ca1daab89cea44211ece7188ef) )
	ROM_LOAD( "nn4k.p1", 0x0000, 0x010000, CRC(09a808c0) SHA1(c74c3acb2c1f52fd1e83003fb1a022f80f55e0b8) )
	ROM_LOAD( "nn4s.p1", 0x0000, 0x010000, CRC(ec4f01ee) SHA1(443da7ed359a3e208417f7bca0dc52a09594a927) )
	ROM_LOAD( "nn4y.p1", 0x0000, 0x010000, CRC(a1eff941) SHA1(369ec89b82f97c3d8266d41e5eb27be7770bdca4) )
	ROM_LOAD( "nn5ad.p1", 0x0000, 0x010000, CRC(22537184) SHA1(aef542a34e2b14a5db624e42d1cd2682de237b52) )
	ROM_LOAD( "nn5b.p1", 0x0000, 0x010000, CRC(e2a99408) SHA1(a0868a38c290a84926089c60d1b5555706485bff) )
	ROM_LOAD( "nn5bd.p1", 0x0000, 0x010000, CRC(56cc9559) SHA1(53e109a579e422932dd25c52cf2beca51d3a53e3) )
	ROM_LOAD( "nn5d.p1", 0x0000, 0x010000, CRC(ef1a21b6) SHA1(ba763b06583af1273e384b878fbacc68f88714dc) )
	ROM_LOAD( "nn5dk.p1", 0x0000, 0x010000, CRC(74c48e28) SHA1(db6be2275b6122845c662dd5f12266b66e888221) )
	ROM_LOAD( "nn5dr.p1", 0x0000, 0x010000, CRC(f52c9f87) SHA1(e8b1037c9ed5d9452abccb6b07bae46b45c4705e) )
	ROM_LOAD( "nn5dy.p1", 0x0000, 0x010000, CRC(6847b769) SHA1(1b4d42774c72a3c7b40551c7181413ea1fca0b88) )
	ROM_LOAD( "nn5k.p1", 0x0000, 0x010000, CRC(ceab49d9) SHA1(633e7bab6a30176dbcea2bd3e7bab0f7833409ba) )
	ROM_LOAD( "nn5r.p1", 0x0000, 0x010000, CRC(144523cd) SHA1(d12586ccea659ecb75af944d87ddd480da917eaf) )
	ROM_LOAD( "nn5s.p1", 0x0000, 0x010000, CRC(459e5663) SHA1(66ae821e5202d6d3ba05be44d0c1f26da60a3a32) )
	ROM_LOAD( "nn5y.p1", 0x0000, 0x010000, CRC(892e0b23) SHA1(ff3f550e20e71e868d52b60740f743a7d2d6c645) )
	ROM_LOAD( "nnu40x.bin", 0x0000, 0x010000, CRC(63e3d7df) SHA1(1a5a00185ec5150f5b05765f06297d7884540aaf) )
	ROM_LOAD( "nnus.p1", 0x0000, 0x010000, CRC(3e3a829e) SHA1(5aa3a56e007bad4dacdc3c993c87569e4250eecd) )
	ROM_LOAD( "nnux.p1", 0x0000, 0x010000, CRC(38806ebf) SHA1(a897a33e3260de1b284b01a65d1da7cbe05d51f8) )
	ROM_LOAD( "nnuxb.p1", 0x0000, 0x010000, CRC(c4dba8df) SHA1(0f8516cc9b2f0be9d1c936667974cd8116018dad) )
	ROM_LOAD( "nnuxc.p1", 0x0000, 0x010000, CRC(797e0c4d) SHA1(211b0a804643731275d0075461f8d94985fde1db) )
	ROM_LOAD( "nnwink.hex", 0x0000, 0x010000, CRC(f77bd6c4) SHA1(1631040fbfe3fc37c2cbd3145857c31d16b92bde) )
	ROM_LOAD( "nnww2010", 0x0000, 0x010000, CRC(67b1c7b5) SHA1(495e25bc2051ab78e473cd0c36e0c1825c06db14) )
	ROM_LOAD( "wink2010", 0x0000, 0x010000, CRC(056a2ffa) SHA1(9da96d70ff850b6672ae7009253e179fa7159db4) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "m574.chr", 0x0000, 0x000048, CRC(cc4b7911) SHA1(9f8a96a1f8b0f9b33b852e93483ce5c684703349) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cn4snd.p1", 0x0000, 0x080000, CRC(720011ce) SHA1(fa9108463131ea7e64525e080ac0eff2f6708db8) )
	ROM_LOAD( "winksnd1.hex", 0x0000, 0x080000, CRC(720011ce) SHA1(fa9108463131ea7e64525e080ac0eff2f6708db8) )
ROM_END

ROM_START( m4nnwwc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cn302c.p1", 0x0000, 0x010000, CRC(fd9de050) SHA1(14c80deba1396aa5be0a1d02964ecd4b946f2ee8) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cf302ad.p1", 0x0000, 0x010000, CRC(6c6aa0cd) SHA1(5a58a19c35b0b195f3b4e7a21f57ca61d45ec1fb) )
	ROM_LOAD( "cf302b.p1", 0x0000, 0x010000, CRC(9ca07939) SHA1(6eb0a5675bb803a11c4c874dc0516d94c48194b7) )
	ROM_LOAD( "cf302bd.p1", 0x0000, 0x010000, CRC(8ba33b7d) SHA1(ebfb62a390de512dc1482cfb9ab64196cbcc5831) )
	ROM_LOAD( "cf302c.p1", 0x0000, 0x010000, CRC(26be2dc4) SHA1(157ca96ebd36f2fbfb501945d0351cc3be38f3b7) )
	ROM_LOAD( "cf302d.p1", 0x0000, 0x010000, CRC(b52d5b47) SHA1(1583963b0bac1288bd20ed0550ad793be0980b03) )
	ROM_LOAD( "cf302dk.p1", 0x0000, 0x010000, CRC(c3d4c74d) SHA1(9a34c1f2fabb20da17988f63c9190ec4dd0b65fb) )
	ROM_LOAD( "cf302dr.p1", 0x0000, 0x010000, CRC(0b25e6b9) SHA1(5fd42abbe985dbdcfe09da50673551330dd26175) )
	ROM_LOAD( "cf302dy.p1", 0x0000, 0x010000, CRC(420b47c1) SHA1(0cb1a843cec3ace21d806fe98212250201a72f12) )
	ROM_LOAD( "cf302k.p1", 0x0000, 0x010000, CRC(07ca4c45) SHA1(8f6ee3c17527b05a6652845019919d490cc00c64) )
	ROM_LOAD( "cf302r.p1", 0x0000, 0x010000, CRC(e09f43bd) SHA1(65dcdf8d223936c4415ddc3f734b83367d6b8db7) )
	ROM_LOAD( "cf302s.p1", 0x0000, 0x010000, CRC(7a3e8ead) SHA1(590dc78b98f9928d6fa87ef661234f88dccfdff8) )
	ROM_LOAD( "cf302y.p1", 0x0000, 0x010000, CRC(c1063a32) SHA1(e1c8fc463b1a1db87110f272a8727435f9d9b97a) )
	ROM_LOAD( "ch302ad.p1", 0x0000, 0x010000, CRC(20405f4e) SHA1(7f87c881f428f704c98b0f4be459980062ccd29a) )
	ROM_LOAD( "ch302b.p1", 0x0000, 0x010000, CRC(cf7543ac) SHA1(2fe810741bfc18f800ad8028724218557d93a830) )
	ROM_LOAD( "ch302bd.p1", 0x0000, 0x010000, CRC(4c3e5664) SHA1(87a1f2133cad624683dac89f1da85d70b018f846) )
	ROM_LOAD( "ch302c.p1", 0x0000, 0x010000, CRC(dcde4d0a) SHA1(d1535f8754d2c0f8183c2c9db97edafdcdfed82e) )
	ROM_LOAD( "ch302d.p1", 0x0000, 0x010000, CRC(e1a02108) SHA1(fa8271a1246a3ae1289bb314494743cfec31f4e2) )
	ROM_LOAD( "ch302dk.p1", 0x0000, 0x010000, CRC(a3f636af) SHA1(c3de325ef5baa3cccd4c9997e615e87521b9e537) )
	ROM_LOAD( "ch302dr.p1", 0x0000, 0x010000, CRC(0620b0c0) SHA1(31aabba5f5b096254908221f884b5088a5a6e883) )
	ROM_LOAD( "ch302dy.p1", 0x0000, 0x010000, CRC(9b4b982e) SHA1(c7c9c501eb1c936ffb8bc2fe1fe9258e92b1d548) )
	ROM_LOAD( "ch302k.p1", 0x0000, 0x010000, CRC(908d8b10) SHA1(a80a5ce1a83d05f1e68e66d14bacc424bc833aa7) )
	ROM_LOAD( "ch302r.p1", 0x0000, 0x010000, CRC(c31c4c28) SHA1(e94c7588211044dae7c5ac587e6232b0ace2fc7b) )
	ROM_LOAD( "ch302s.p1", 0x0000, 0x010000, CRC(e7d0ceb2) SHA1(b75d58136b9e1e4bfde86730ef4e95bc98494813) )
	ROM_LOAD( "ch302y.p1", 0x0000, 0x010000, CRC(5e7764c6) SHA1(05a61a57ac906cbea1d72fffd1c8ea707852b895) )
	ROM_LOAD( "cn302ad.p1", 0x0000, 0x010000, CRC(7a6acd9b) SHA1(9a1f0ed19d66428c6b541ce1c8e169d9b4be3ef1) )
	ROM_LOAD( "cn302b.p1", 0x0000, 0x010000, CRC(b69cb520) SHA1(7313f2740960ca86ecea8609fe8fd58d84a3248c) )
	ROM_LOAD( "cn302bd.p1", 0x0000, 0x010000, CRC(ab828a0b) SHA1(53fa6dad9bdae1d46479596c98cf2c3f4454bb95) )
	ROM_LOAD( "cn302d.p1", 0x0000, 0x010000, CRC(8c6ac365) SHA1(a32b104968aaa4da060072a241a4c54fbdf3c404) )
	ROM_LOAD( "cn302dk.p1", 0x0000, 0x010000, CRC(24cbab96) SHA1(77fe3b21fc9470653bada31c700ce926d55ce82e) )
	ROM_LOAD( "cn302dr.p1", 0x0000, 0x010000, CRC(09069f0e) SHA1(68b2a34644ee1fca3ce5191e2f25aa808b85fb09) )
	ROM_LOAD( "cn302dy.p1", 0x0000, 0x010000, CRC(946db7e0) SHA1(fe29c1da478e3f1a53ad55c661ddcc7003679304) )
	ROM_LOAD( "cn302k.p1", 0x0000, 0x010000, CRC(7a3202f1) SHA1(2dd5e8195120b1efc3eb51214cf054432fc50aed) )
	ROM_LOAD( "cn302r.p1", 0x0000, 0x010000, CRC(e7cf9e1e) SHA1(66a1e54fc928c09d16f7ac1c002685eee841315f) )
	ROM_LOAD( "cn302s.p1", 0x0000, 0x010000, CRC(87703a1a) SHA1(6582ffa42a61b60e92e456a794c4c219a9901a1c) )
	ROM_LOAD( "cn302y.p1", 0x0000, 0x010000, CRC(7aa4b6f0) SHA1(2c185a9a7c8a4957fb5901305883661c41cb0cb4) )
	ROM_LOAD( "cnc03s.p1", 0x0000, 0x010000, CRC(57a03b29) SHA1(52cc8eb3f02c4a812de06ceec0588ca930e07876) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cl__x__x.2_0", 0x0000, 0x010000, CRC(c3de4791) SHA1(220d32b961b6710d508c0c7e6b2d8e4d292746f4) )
	ROM_LOAD( "cl__x_dx.2_0", 0x0000, 0x010000, CRC(c79833f8) SHA1(b3519b55f6f2a4f081b69483ac0b8860aa8190d9) )
	ROM_LOAD( "cl__xa_x.2_0", 0x0000, 0x010000, CRC(4c3021a1) SHA1(7e7258808dd1693adb956a5e6b076f925eb0a026) )
	ROM_LOAD( "cl__xb_x.2_0", 0x0000, 0x010000, CRC(75a5add7) SHA1(6802ec81b4ebcde9ed014a0440fdc50211a8a350) )


	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing */
ROM_END

ROM_START( m4nudqst )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "nq20h.bin", 0x0000, 0x010000, CRC(438329d4) SHA1(6c67d01785e116aa4b22cadab8065e801f4e89c8) )
ROM_END

ROM_START( m4nudshf )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "nus6", 0x0000, 0x010000, CRC(017c5354) SHA1(07491e4b03ab62ad923f8479300c1af4633e3e8c) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "nuss.bin", 0x0000, 0x010000, CRC(d3b860ee) SHA1(d5d1262c715e4684748b0cae708eeed31b1dc50f) )
	ROM_LOAD( "nusx.p1", 0x0000, 0x010000, CRC(87caab84) SHA1(e2492ad0d25ded4d760c4cbe05e9b51ca1a10544) )
	ROM_LOAD( "nusxc.p1", 0x0000, 0x010000, CRC(e2557b45) SHA1(a9d1514d4fe3897f6fcef22a5039d6bdff8126ff) )
ROM_END

ROM_START( m4nudup )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dnu25.bin", 0x0000, 0x010000, CRC(c8d83f94) SHA1(fa0834d41c7506cab14e50b4036943a61411ed0e) )
ROM_END

ROM_START( m4num1 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dno17.bin", 0x0000, 0x010000, CRC(3b302160) SHA1(ff52803472e119aa46fe1cff134b5503858dfee1) )
ROM_END

ROM_START( m4oldtmr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dot11.bin",  0x00000, 0x10000,  CRC(da095666) SHA1(bc7654dc9da1f830a43f925db8079f27e18bb61e))

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "m470.chr", 0x0000, 0x000048, CRC(10d302d4) SHA1(5858e550470a25dcd64efe004c79e6e9783bce07) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sdot01.bin", 0x0000, 0x080000, CRC(f247ba83) SHA1(9b173503e63a4a861d1380b2ab1fe14af1a189bd) )
ROM_END

ROM_START( m4omega )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dom23.bin", 0x0000, 0x010000, CRC(d51e078c) SHA1(2c38d271b9ce4731ce26106764529839b5110b3e) )
ROM_END



ROM_START( m4ordmnd )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "rab01.p1", 0x0000, 0x020000, CRC(99964fe7) SHA1(3745d09e7a4f417c8e85270d3ffec3e37ee1344d) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "odsnd1.bin", 0x000000, 0x080000, CRC(d746bae4) SHA1(293e1dc9edf88a183cc23dbb4576cefbc8f9d028) )
	ROM_LOAD( "odsnd2.bin", 0x080000, 0x080000, CRC(84ace1f4) SHA1(9cc70e59e9d26006870ea1cc522de33e71b71692) )
	ROM_LOAD( "odsnd3.bin", 0x100000, 0x080000, CRC(b1b12def) SHA1(d8debf8cfb3af2157d5d1571927588dc1c8d07b6) )
ROM_END

ROM_START( m4overmn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "otts.p1", 0x0000, 0x010000, CRC(6daf58a4) SHA1(e505a18b67dec54446e6d94a5d1c3bba13099619) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ot8b.p1", 0x0000, 0x010000, CRC(243c7f7c) SHA1(24b9d2cce1af75811d1e625ac8df5b58356776dc) )
	ROM_LOAD( "ot8c.p1", 0x0000, 0x010000, CRC(af5bb77b) SHA1(6a9eeb803fdaa03970b3a3a0738e804027aedd20) )
	ROM_LOAD( "ot8dk.p1", 0x0000, 0x010000, CRC(0798d12c) SHA1(068a676d4ccaf2964a3f6f6673199f8d62c45452) )
	ROM_LOAD( "ot8dy.p1", 0x0000, 0x010000, CRC(0904a38d) SHA1(0a0668ae384fe371abf2597ab66a56dd79a90c03) )
	ROM_LOAD( "ot8k.p1", 0x0000, 0x010000, CRC(8d83f697) SHA1(2fff475d44f1535c85988b195c3610201ece21ae) )
	ROM_LOAD( "ot8s.p1", 0x0000, 0x010000, CRC(db1bacdb) SHA1(fc2257eedec532094f3c229bcf215a0fde430d2b) )
	ROM_LOAD( "ot8y.p1", 0x0000, 0x010000, CRC(6e1508fb) SHA1(6a45a394e48f758456dc6cf17a5b134ca6887421) )
	ROM_LOAD( "otnb.p1", 0x0000, 0x010000, CRC(047aae70) SHA1(bf620b60f1107fff07a28944dec66fd71aab65c0) )
	ROM_LOAD( "otnc.p1", 0x0000, 0x010000, CRC(536e7640) SHA1(3a079bed9217c857efb8d435c7efacca69cfcabc) )
	ROM_LOAD( "otnd.p1", 0x0000, 0x010000, CRC(c6c15fc2) SHA1(99cb3fd2eaea636313085e0a6a9aff9c027cb187) )
	ROM_LOAD( "otndy.p1", 0x0000, 0x010000, CRC(6b22206e) SHA1(0714ddc445d530e1ff2055cd5e5d8b31704733d9) )
	ROM_LOAD( "otnk.p1", 0x0000, 0x010000, CRC(992cc40d) SHA1(6059ecaf91390e2a2ea80d3da5e44156273892ad) )
	ROM_LOAD( "otns.p1", 0x0000, 0x010000, CRC(7e03f295) SHA1(f874ddf8de8037aa251a8c3fb7c183e6dfb93dfa) )
	ROM_LOAD( "otny.p1", 0x0000, 0x010000, CRC(67cba8fa) SHA1(234cc5b4a0b60d33b2f4c00d082beee59236a126) )
	ROM_LOAD( "ottad.p1", 0x0000, 0x010000, CRC(682b01a3) SHA1(cb71fd56ad6d4fc67894bf86c54c49a7e45aae15) )
	ROM_LOAD( "ottb.p1", 0x0000, 0x010000, CRC(541c2d54) SHA1(3b42e9dcb468cb9bbf2092a4e7eabeb172dc90d0) )
	ROM_LOAD( "ottbd.p1", 0x0000, 0x010000, CRC(5e376ce9) SHA1(0628461395ebd233ca7b0513ea272ddd83c5accd) )
	ROM_LOAD( "ottd.p1", 0x0000, 0x010000, CRC(9b013a2b) SHA1(734cc79bc9452c86434bd085463ab512b5421dae) )
	ROM_LOAD( "ottdk.p1", 0x0000, 0x010000, CRC(c205194f) SHA1(bdfdffe09fd995c8ded80cc3042d2ce1eebad8bc) )
	ROM_LOAD( "ottdr.p1", 0x0000, 0x010000, CRC(f2b9bf4c) SHA1(5f1c5c347930b75473dcd83cf2ad5870ba26e289) )
	ROM_LOAD( "ottdy.p1", 0x0000, 0x010000, CRC(936b70e2) SHA1(6d434f399dc851621b703a1bf93bc71bed78d867) )
	ROM_LOAD( "ottk.p1", 0x0000, 0x010000, CRC(68c984d3) SHA1(b1cf87630ab093629eaa8d199dfcfd6343d9c31d) )
	ROM_LOAD( "ottr.p1", 0x0000, 0x010000, CRC(ceb322d1) SHA1(a62bd1f947fc15f1d42dae8e933d2fcb672bcce4) )
	ROM_LOAD( "otty.p1", 0x0000, 0x010000, CRC(974af7ff) SHA1(e0aecb91c1fc476a9258d6d57ba5ca8f249141b0) )
	ROM_LOAD( "otuad.p1", 0x0000, 0x010000, CRC(2576654b) SHA1(7fae2bd057d96af4c50fd84a5261ae750ba34033) )
	ROM_LOAD( "otub.p1", 0x0000, 0x010000, CRC(1463877d) SHA1(ea41e048aead52aabc1b8a2a224ef87b9011c163) )
	ROM_LOAD( "otubd.p1", 0x0000, 0x010000, CRC(8ac2d17b) SHA1(09f21f1233d82fd02830b6ece6a773402393a447) )
	ROM_LOAD( "otud.p1", 0x0000, 0x010000, CRC(8f1632c2) SHA1(729f2182c40f98e9b2fb9996d14c11d2334ba15f) )
	ROM_LOAD( "otudk.p1", 0x0000, 0x010000, CRC(2edbbe5d) SHA1(f6b8b625bf2d021524595ef1f69e730e78f42aa8) )
	ROM_LOAD( "otudr.p1", 0x0000, 0x010000, CRC(f799f424) SHA1(f0d1a72088dd3cd6f9ccaa1bf0e9a28f656194e0) )
	ROM_LOAD( "otudy.p1", 0x0000, 0x010000, CRC(562da6fd) SHA1(899f971124969c52a018634b2b2f2dd7cb634195) )
	ROM_LOAD( "otuk.p1", 0x0000, 0x010000, CRC(cbb66497) SHA1(ade033fb3d226bfcb3cdf3e3612fb65cfc22b030) )
	ROM_LOAD( "otur.p1", 0x0000, 0x010000, CRC(d05a8c2f) SHA1(754e2351431aa7bf6dea3a1498581da0c4283c1e) )
	ROM_LOAD( "otus.p1", 0x0000, 0x010000, CRC(5f2b8d0b) SHA1(1e3ac59fa0b108549c265eeba027591bce5122f3) )
	ROM_LOAD( "otuy.p1", 0x0000, 0x010000, CRC(fc65136d) SHA1(048f81de92a1db4e4e4e9aa7a87228805d57b263) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "otts.chr", 0x0000, 0x000048, CRC(2abec763) SHA1(307399724a994a5d0914a5d7e0931a5d94439a37) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "otnsnd.p1", 0x0000, 0x080000, CRC(d4f7ed82) SHA1(16e80bf0956f39a9e8e23384615a07594419db59) )
ROM_END

ROM_START( m4placbt )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "pyb07s.p1", 0x0000, 0x020000, CRC(ad02705a) SHA1(027bcbbd828e4fd23831af9554d582857e6784e1) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pyb06ad.p1", 0x0000, 0x020000, CRC(e08b6176) SHA1(ccfb43ee033b4ed36e8656bcb4ba62230dde8466) )
	ROM_LOAD( "pyb06b.p1", 0x0000, 0x020000, CRC(b6486055) SHA1(e0926720aba1e9d1327c32db29220d91050ea338) )
	ROM_LOAD( "pyb06bd.p1", 0x0000, 0x020000, CRC(6d91cfb3) SHA1(82d6ed6d6b2022d0945ec0fb8012fa4fef7029e0) )
	ROM_LOAD( "pyb06c.p1", 0x0000, 0x020000, CRC(8102dd47) SHA1(7b834d896e104b1f42069d6fa0bce75b5c15b899) )
	ROM_LOAD( "pyb06d.p1", 0x0000, 0x020000, CRC(cb536b23) SHA1(874eaaa434a0212edefa05a440e7e5f826d1f92e) )
	ROM_LOAD( "pyb06dk.p1", 0x0000, 0x020000, CRC(275667e5) SHA1(4ac40eaa03462c1a70f0366f589bc2a59972827b) )
	ROM_LOAD( "pyb06dr.p1", 0x0000, 0x020000, CRC(9459dcd4) SHA1(ae8b559205c6cb3f2e3b1131cd99ff7ce037c573) )
	ROM_LOAD( "pyb06dy.p1", 0x0000, 0x020000, CRC(a0b5471b) SHA1(95400779bb1d3fdeada5d8fca4fd66a89d3e13a2) )
	ROM_LOAD( "pyb06k.p1", 0x0000, 0x020000, CRC(fc8fc803) SHA1(81fa3104075b56f51c35d944fa3652aa8cce988c) )
	ROM_LOAD( "pyb06r.p1", 0x0000, 0x020000, CRC(4f807332) SHA1(e3852ac9811d780ac87f375acaf5ec1026071b2e) )
	ROM_LOAD( "pyb06s.p1", 0x0000, 0x020000, CRC(acd9d628) SHA1(93d8f0ffa3b9ebdd9fef39b2bc49bb85b2fac00f) )
	ROM_LOAD( "pyb06y.p1", 0x0000, 0x020000, CRC(7b6ce8fd) SHA1(096fb2e8a4ac5f723810766bc4245d403814a20f) )
	ROM_LOAD( "pyb07ad.p1", 0x0000, 0x020000, CRC(427a7489) SHA1(fb0a24da5ef7a948152e8180968aaaebbd85afa0) )
	ROM_LOAD( "pyb07b.p1", 0x0000, 0x020000, CRC(35cdf803) SHA1(94953da72c2ee8792f53bf677483ffed15d4709c) )
	ROM_LOAD( "pyb07bd.p1", 0x0000, 0x020000, CRC(cf60da4c) SHA1(8667308f750e944894f68f20b70d42244b751e22) )
	ROM_LOAD( "pyb07c.p1", 0x0000, 0x020000, CRC(02874511) SHA1(b5acdcfb7d901faa1271eb16e5d36d0a484d97cb) )
	ROM_LOAD( "pyb07d.p1", 0x0000, 0x020000, CRC(48d6f375) SHA1(1891b6f8f4599d94280bcb68e9d0e9259351e2b8) )
	ROM_LOAD( "pyb07dk.p1", 0x0000, 0x020000, CRC(85a7721a) SHA1(e3af55577b4ad4ae48e95b576e336cb019f3ecd0) )
	ROM_LOAD( "pyb07dr.p1", 0x0000, 0x020000, CRC(36a8c92b) SHA1(a1091dea9ffe53c9ba1495f7e0d2aebe92d9bb64) )
	ROM_LOAD( "pyb07dy.p1", 0x0000, 0x020000, CRC(024452e4) SHA1(a92c887ab467be6bcccaec1cd5dcc1304eddba19) )
	ROM_LOAD( "pyb07k.p1", 0x0000, 0x020000, CRC(7f0a5055) SHA1(5620d8a3333f2f56ea24bcecf1a791e6ee0f43d9) )
	ROM_LOAD( "pyb07r.p1", 0x0000, 0x020000, CRC(cc05eb64) SHA1(1329decc84de231e8c1929f057233b25cc8b5942) )
	ROM_LOAD( "pyb07y.p1", 0x0000, 0x020000, CRC(f8e970ab) SHA1(66a54a9c2750ea1aa4ce562aad74c98775865ed6) )
	ROM_LOAD( "pyb10h", 0x0000, 0x020000, CRC(69be6185) SHA1(f697350912505cd857acae2733ad8b48e67cab6b) )
	ROM_LOAD( "pyb15g", 0x0000, 0x020000, CRC(369fd852) SHA1(4c532a59451352aa54a1e47d12f04403d2e9c8cb) )
	ROM_LOAD( "pyb15t", 0x0000, 0x020000, CRC(c38d7b04) SHA1(5785344084498cab4ce2734b3d8c0dc8f0cbed5a) )
	ROM_LOAD( "pyh04s", 0x0000, 0x020000, CRC(c824b937) SHA1(9bc0a1e75540520ef3448dc7a3c95c81f93abe78) )
	ROM_LOAD( "pyh05ad.p1", 0x0000, 0x020000, CRC(948d1ad6) SHA1(66c580f0ef9035de5f50600db51d63336a8d3fbb) )
	ROM_LOAD( "pyh05b.p1", 0x0000, 0x020000, CRC(26c5fca4) SHA1(8166a195eb1aa7df99cc27e7cd6207a9192d14b9) )
	ROM_LOAD( "pyh05bd.p1", 0x0000, 0x020000, CRC(1997b413) SHA1(cf701534243b302b83908bdf359050b325bd037a) )
	ROM_LOAD( "pyh05c.p1", 0x0000, 0x020000, CRC(118f41b6) SHA1(29b74182d24d8d301a7aa899f4c1dd2b1a4eb84c) )
	ROM_LOAD( "pyh05d.p1", 0x0000, 0x020000, CRC(5bdef7d2) SHA1(c00a602ea6f0a53d53b13c2e4921aabd9d10b0f3) )
	ROM_LOAD( "pyh05dk.p1", 0x0000, 0x020000, CRC(53501c45) SHA1(fa1161a0fe6916cd84370886688517e9905561d2) )
	ROM_LOAD( "pyh05dr.p1", 0x0000, 0x020000, CRC(e05fa774) SHA1(44d53a0480b382f01ab42ca5b0521a612f672433) )
	ROM_LOAD( "pyh05dy.p1", 0x0000, 0x020000, CRC(d4b33cbb) SHA1(e9681c025b3b661a26b89ef1fa6bbcffb6c2e233) )
	ROM_LOAD( "pyh05k.p1", 0x0000, 0x020000, CRC(6c0254f2) SHA1(df14735ec9bc77fc35f52094598bb3fbb015944f) )
	ROM_LOAD( "pyh05r.p1", 0x0000, 0x020000, CRC(df0defc3) SHA1(d259f6eb770130671d06e221d467df658ba0b29b) )
	ROM_LOAD( "pyh05s.p1", 0x0000, 0x020000, CRC(3c544ad9) SHA1(50780424382fd4ccd023a784e43bb60b8f862456) )
	ROM_LOAD( "pyh05y.p1", 0x0000, 0x020000, CRC(ebe1740c) SHA1(424707d023fd026cf43a687ed02d4ee4398b299f) )
	ROM_LOAD( "pyh06ad.p1", 0x0000, 0x020000, CRC(7ae70380) SHA1(c0e2b67ade2275a903359b6df7f55e44ef78828f) )
	ROM_LOAD( "pyh06b.p1", 0x0000, 0x020000, CRC(dc588857) SHA1(dd2d2ffa87c61b200aa82337beea7d2205f1176c) )
	ROM_LOAD( "pyh06bd.p1", 0x0000, 0x020000, CRC(f7fdad45) SHA1(78334007b0a414fd3d2b8ec1645d1f04e711eb77) )
	ROM_LOAD( "pyh06c.p1", 0x0000, 0x020000, CRC(eb123545) SHA1(410400c219fb15f3267c2f94737fa9d2785318a0) )
	ROM_LOAD( "pyh06d.p1", 0x0000, 0x020000, CRC(a1438321) SHA1(53f7e0156b137bea91264fe662642083ca9f5f9c) )
	ROM_LOAD( "pyh06dk.p1", 0x0000, 0x020000, CRC(bd3a0513) SHA1(98dfd874dcbee5a3c16a0ebf42f944bf0ba50672) )
	ROM_LOAD( "pyh06dr.p1", 0x0000, 0x020000, CRC(0e35be22) SHA1(8ff910601bfec1b47e50ba92cdb80c1bdfe287ec) )
	ROM_LOAD( "pyh06dy.p1", 0x0000, 0x020000, CRC(3ad925ed) SHA1(af584333e443127071b874bafea0c8667da33f6c) )
	ROM_LOAD( "pyh06k.p1", 0x0000, 0x020000, CRC(969f2001) SHA1(e934aa7e95e91f155ee82e8eaac5c949b35e024b) )
	ROM_LOAD( "pyh06r.p1", 0x0000, 0x020000, CRC(25909b30) SHA1(8a44e59a46ffec3badb27ee62e7e9bb0adff62a4) )
	ROM_LOAD( "pyh06s.p1", 0x0000, 0x020000, CRC(10b75ddf) SHA1(d093ac51c64642400d2cf24a713dc7adb4a6a9d0) )
	ROM_LOAD( "pyh06y.p1", 0x0000, 0x020000, CRC(117c00ff) SHA1(ace5d8c4f4e0647c89608db2c2ad35f241be3672) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "pyb.chr", 0x0000, 0x000048, CRC(663e9d8e) SHA1(08e898967d41fbc582c9bfdebb461ad51269089d) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "pybsnd.p1", 0x000000, 0x080000, CRC(3a91784a) SHA1(7297ccec3264aa9f1e7b3a2841f5f8a1e4ca6c54) )
	ROM_LOAD( "pybsnd.p2", 0x080000, 0x080000, CRC(a82f0096) SHA1(45b6b5a2ae06b45add9cdbb9f5e6f834687b4902) )
ROM_END

ROM_START( m4pont )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pon3.0.bin", 0x0000, 0x010000, CRC(12d83177) SHA1(5b88b9618f53af2b2a4f75e73c3eb334a17791c0) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pons.p1", 0x0000, 0x010000, CRC(5186daa7) SHA1(78f853e221307270e1725895201d08f358e34986) )
	ROM_LOAD( "pontoon.hex", 0x0000, 0x010000, CRC(5186daa7) SHA1(78f853e221307270e1725895201d08f358e34986) )
ROM_END

ROM_START( m4potblk )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "potblack.hex", 0x0000, 0x020000, CRC(36a1c679) SHA1(bf2eb5c2a07e61b7a2c0d8402b0e0583adfa22dc) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pb15g", 0x0000, 0x020000, CRC(650a54be) SHA1(80a5bb95857c911c1972f8be5bf794637cb02323) )
	ROM_LOAD( "pb15t", 0x0000, 0x020000, CRC(98628744) SHA1(1a0df7036c36f3b87d5a239e1c9edfd7c74d2ae8) )
	ROM_LOAD( "pbg14s.p1", 0x0000, 0x020000, CRC(c9316c92) SHA1(d9248069c4702d4ce780ab82bdb783ba5aea034b) )
	ROM_LOAD( "pbg15ad.p1", 0x0000, 0x020000, CRC(ded4ba89) SHA1(f8b4727987bef1e74894df4e7549d3c28ba4de98) )
	ROM_LOAD( "pbg15b.p1", 0x0000, 0x020000, CRC(5a6570be) SHA1(f44e4511cc0c0f410104f9a36ae51b3972bd4522) )
	ROM_LOAD( "pbg15bd.p1", 0x0000, 0x020000, CRC(941312df) SHA1(51ec4052cfaa245873146d0ecb8834be5cc22db2) )
	ROM_LOAD( "pbg15d.p1", 0x0000, 0x020000, CRC(79f682f5) SHA1(ee6f31009b8a5354db930d6f228a2969dbebb9ad) )
	ROM_LOAD( "pbg15dh.p1", 0x0000, 0x020000, CRC(e90819a9) SHA1(f3d423e56205f6b18892fe8771aa853f7185336a) )
	ROM_LOAD( "pbg15dk.p1", 0x0000, 0x020000, CRC(6ddb01b8) SHA1(aeefd2145f328f0f7af87b16f9bc2324d134b7e1) )
	ROM_LOAD( "pbg15dr.p1", 0x0000, 0x020000, CRC(f735df01) SHA1(f669cd719cdf9fa170babc652be164bd7c580344) )
	ROM_LOAD( "pbg15dy.p1", 0x0000, 0x020000, CRC(59379a77) SHA1(91b56aef53de7c554924ebab56faa3e8655dcbfd) )
	ROM_LOAD( "pbg15h.p1", 0x0000, 0x020000, CRC(277e7bc8) SHA1(9f89a048fcf268883002bb0dcb14854949ebec46) )
	ROM_LOAD( "pbg15k.p1", 0x0000, 0x020000, CRC(a3ad63d9) SHA1(0e65ff6ae02bd42cb1b3c9249ac85dc13b4eb8ad) )
	ROM_LOAD( "pbg15r.p1", 0x0000, 0x020000, CRC(3943bd60) SHA1(cfcd1e2b76f592e3bc5b8ed33af66ad183e829e1) )
	ROM_LOAD( "pbg15s.p1", 0x0000, 0x020000, CRC(f31c9a6a) SHA1(f2c7dceaabbe0689227f2c59d063ac20403eae1d) )
	ROM_LOAD( "pbg15y.p1", 0x0000, 0x020000, CRC(9741f816) SHA1(52482fed48574582ed48424666284695fe661880) )
	ROM_LOAD( "pbg16ad.p1", 0x0000, 0x020000, CRC(919e90ba) SHA1(b32459b394595a5c3d238c6eec47c7d4d34fcdf8) )
	ROM_LOAD( "pbg16b.p1", 0x0000, 0x020000, CRC(db445653) SHA1(145af560641f9becb6d98c2f157f94b0fdd6459c) )
	ROM_LOAD( "pbg16bd.p1", 0x0000, 0x020000, CRC(db5938ec) SHA1(323f190c62f23b8092274dd17f07198f53abc828) )
	ROM_LOAD( "pbg16d.p1", 0x0000, 0x020000, CRC(f8d7a418) SHA1(81a2d020ec03574b041b9be0b8ed96386804f4af) )
	ROM_LOAD( "pbg16dh.p1", 0x0000, 0x020000, CRC(a642339a) SHA1(104b405923b71d69bc996dced4dc284d41397c5f) )
	ROM_LOAD( "pbg16dk.p1", 0x0000, 0x020000, CRC(22912b8b) SHA1(cdbaaf0509fb6115182c8ac79002e1d983bcc765) )
	ROM_LOAD( "pbg16dr.p1", 0x0000, 0x020000, CRC(b87ff532) SHA1(d64d4733523d4afe5b4e3ca5f2c33ee8d4ab1c2b) )
	ROM_LOAD( "pbg16dy.p1", 0x0000, 0x020000, CRC(167db044) SHA1(291982c6af2486724be32b8f41ee66d699afad22) )
	ROM_LOAD( "pbg16h.p1", 0x0000, 0x020000, CRC(a65f5d25) SHA1(6ea4bc92ecf849653c1abbb5c1e1d97d0e58a373) )
	ROM_LOAD( "pbg16k.p1", 0x0000, 0x020000, CRC(228c4534) SHA1(e4a4e7ec059e4da568c507d8f1f006c04e1c13c4) )
	ROM_LOAD( "pbg16r.p1", 0x0000, 0x020000, CRC(b8629b8d) SHA1(08fc4498b45f2e939c4c465d4d979aa4532b5ce4) )
	ROM_LOAD( "pbg16s.p1", 0x0000, 0x020000, CRC(36a1c679) SHA1(bf2eb5c2a07e61b7a2c0d8402b0e0583adfa22dc) )
	ROM_LOAD( "pbg16y.p1", 0x0000, 0x020000, CRC(1660defb) SHA1(9f6112759b029e71056a92137f890910b6afb708) )
	ROM_LOAD( "pbs04ad.p1", 0x0000, 0x020000, CRC(c4fedb7d) SHA1(1ce1e524bd11e775c0f8498849c1d9ca41fcf912) )
	ROM_LOAD( "pbs04b.p1", 0x0000, 0x020000, CRC(f1fcaf2d) SHA1(786513cbfdde2f37c6ab3649781bc7c70bbdfb61) )
	ROM_LOAD( "pbs04bd.p1", 0x0000, 0x020000, CRC(49e475b8) SHA1(95548a22c67bd8df6df05503b6318b15bd84bb7a) )
	ROM_LOAD( "pbs04c.p1", 0x0000, 0x020000, CRC(c6b6123f) SHA1(ccd286dc09606d6cbf45835701dc83ca361c8dc0) )
	ROM_LOAD( "pbs04dh.p1", 0x0000, 0x020000, CRC(34ff7ece) SHA1(a659aee7b093c00e920428f78e96e4246ad469ba) )
	ROM_LOAD( "pbs04dk.p1", 0x0000, 0x020000, CRC(0323ddee) SHA1(74ac57542d6bf32a105682daf661546d18e1eab4) )
	ROM_LOAD( "pbs04dr.p1", 0x0000, 0x020000, CRC(b02c66df) SHA1(7f2b24d747349a7a17b5f90dbada2a6c9f620b1d) )
	ROM_LOAD( "pbs04dy.p1", 0x0000, 0x020000, CRC(84c0fd10) SHA1(586a41908165211b6d6386f12170aeda1ff3fbe9) )
	ROM_LOAD( "pbs04h.p1", 0x0000, 0x020000, CRC(8ce7a45b) SHA1(1eab7d504b4a8f6158aa5878dcc62c9531169a85) )
	ROM_LOAD( "pbs04k.p1", 0x0000, 0x020000, CRC(bb3b077b) SHA1(584fd9f1578c61e1a1c30068c42b16716c5d490f) )
	ROM_LOAD( "pbs04r.p1", 0x0000, 0x020000, CRC(0834bc4a) SHA1(0064b1ec9db506c4dd14ed7ffeffa08bebc117b1) )
	ROM_LOAD( "pbs04s.p1", 0x0000, 0x020000, CRC(b4a7eaac) SHA1(295b793802a6145758861142133ced98f2258119) )
	ROM_LOAD( "pbs04y.p1", 0x0000, 0x020000, CRC(3cd82785) SHA1(fb2cb5acfc60d0896da9c22b7a9370e7c0271cf7) )
	ROM_LOAD( "pbs06ad.p1", 0x0000, 0x020000, CRC(6344d6c7) SHA1(7c01149d9f21a15b1067a42d3f8def2868f15181) )
	ROM_LOAD( "pbs06b.p1", 0x0000, 0x020000, CRC(2056d268) SHA1(ac978d59ff3cead2678d56579e404eb7494ab957) )
	ROM_LOAD( "pbs06bd.p1", 0x0000, 0x020000, CRC(ee5e7802) SHA1(568b6b2e6d58ee766a74badb60118dc0899b8b68) )
	ROM_LOAD( "pbs06c.p1", 0x0000, 0x020000, CRC(171c6f7a) SHA1(e0c7455b64105cdd41ab24ef4cec7b044732faf6) )
	ROM_LOAD( "pbs06d.p1", 0x0000, 0x020000, CRC(03c52023) SHA1(534fbee8e19217002c428cc2d9a6693b8bccf974) )
	ROM_LOAD( "pbs06dh.p1", 0x0000, 0x020000, CRC(93457374) SHA1(40f4ed7260f234b69084676448705b53ff4700e4) )
	ROM_LOAD( "pbs06dk.p1", 0x0000, 0x020000, CRC(a499d054) SHA1(6c740d6e765c5ac3690814f71cb340a67f0bb113) )
	ROM_LOAD( "pbs06dr.p1", 0x0000, 0x020000, CRC(17966b65) SHA1(2a785268954388dba259df162298316e0d187ceb) )
	ROM_LOAD( "pbs06dy.p1", 0x0000, 0x020000, CRC(237af0aa) SHA1(c6f1cf33506517eac98d449c54be33d1f220241c) )
	ROM_LOAD( "pbs06h.p1", 0x0000, 0x020000, CRC(5d4dd91e) SHA1(a715dde45ce7f3c6e62cb08eb5eaacb918803280) )
	ROM_LOAD( "pbs06k.p1", 0x0000, 0x020000, CRC(6a917a3e) SHA1(b8e6fb7ea83c5a363fdd756a5479a51d15cb246d) )
	ROM_LOAD( "pbs06r.p1", 0x0000, 0x020000, CRC(d99ec10f) SHA1(62ffc2772495fd165b2ad9f76a54154f51464394) )
	ROM_LOAD( "pbs06s.p1", 0x0000, 0x020000, CRC(d2b42b29) SHA1(a077605b1f9f3082a03882b4f5b360a530a97135) )
	ROM_LOAD( "pbs06y.p1", 0x0000, 0x020000, CRC(ed725ac0) SHA1(4c2c38e1c2ce7e15c409e06b6f21410f04b70348) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "po_x6__5.1_1", 0x0000, 0x020000, CRC(1fe40fd1) SHA1(5e16ff5b1019d83c1f40d63f89c16030dae0ab11) )
	ROM_LOAD( "po_x6__t.1_1", 0x0000, 0x020000, CRC(c9314f6e) SHA1(4f9226883f9e1963c568eea327775688fb966431) )
	ROM_LOAD( "po_x6_d5.1_1", 0x0000, 0x020000, CRC(404d5a99) SHA1(7a846df3b7f9f0108d84e4a4c2d199e5971b6375) )
	ROM_LOAD( "po_x6_dt.1_1", 0x0000, 0x020000, CRC(7213fd77) SHA1(07482cb54d4f03aad62c54d66322f7101f6c8dcf) )
	ROM_LOAD( "po_x6a_t.1_1", 0x0000, 0x020000, CRC(1b47a76a) SHA1(3587e4c0b50e359529e132376af3cd239194db31) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "pbsnd1.hex", 0x000000, 0x080000, CRC(72a3331d) SHA1(b7475ba0ad86a6277e3d4f7b4311a98f3fc29802) )
	ROM_LOAD( "pbsnd2.hex", 0x080000, 0x080000, CRC(c2460eec) SHA1(7c62fbc69ffaa788bf3839e37a75a812a7b8caef) )
ROM_END

ROM_START( m4ptblkc )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "potblackcasinoprg.bin", 0x0000, 0x020000, CRC(29190084) SHA1(c7a778331369c0fac796ef3e306e12c98605f365) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "potblackcasinosnd.p1", 0x000000, 0x080000, CRC(72a3331d) SHA1(b7475ba0ad86a6277e3d4f7b4311a98f3fc29802) )
	ROM_LOAD( "potblackcasinosnd.p2", 0x080000, 0x080000, CRC(c2460eec) SHA1(7c62fbc69ffaa788bf3839e37a75a812a7b8caef) )
ROM_END



ROM_START( m4potlck )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pil22p1.bin", 0xc000, 0x004000, CRC(b7a926ad) SHA1(2a624256244cd0d3f29f96f8611356b2ec3b6cac) )
	ROM_LOAD( "pil22p2.bin", 0x8000, 0x004000, CRC(b967583e) SHA1(b60f61f09b036b7acb2d740f0d47bfa5f8fba7a5) )
	ROM_LOAD( "pil22p3.bin", 0x6000, 0x002000, CRC(498d8e8a) SHA1(e03d67e22e5709497d9139963113cc1d07fd50d9) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pl2_7.p1", 0xc000, 0x004000, CRC(859fad87) SHA1(4bfc2d699a3abf66cbe52ad8be3f57f3eb66eb92) )
	ROM_LOAD( "pl2_7.p2", 0x8000, 0x004000, CRC(da7f8c1e) SHA1(05957f1767f7558ca10c4190ebeb0081bee6bb66) )
	ROM_LOAD( "pl2_7.p3", 0x6000, 0x002000, CRC(35f70dd5) SHA1(07baebb4904a41f00b50112b89fb70dd80e59493) )
ROM_END


ROM_START( m4prem )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dpm14.bin", 0x0000, 0x010000, CRC(de344759) SHA1(d3e7514da83bbf1eba63661fb0675a6230af93cd) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "dpms.bin", 0x0000, 0x080000, CRC(93fd4253) SHA1(69feda7ffc56defd515c9cd1ce204af3d9731a3f) )
ROM_END


ROM_START( m4przdty )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pdus.p1", 0x0000, 0x010000, CRC(eaa2ae08) SHA1(a4cef3ee8c005fb717625699260d24ef6a368824) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pd8ad.p1", 0x0000, 0x010000, CRC(ff2bde9d) SHA1(6f75d1c4f8b136ad9dbfd6c0182dbe0f54f856a9) )
	ROM_LOAD( "pd8b.p1", 0x0000, 0x010000, CRC(123f8081) SHA1(1619e23f563f9c70e64dccf36743c60ee597cad4) )
	ROM_LOAD( "pd8bd.p1", 0x0000, 0x010000, CRC(6136acca) SHA1(616cfc419beef50b642714df9b257ef0322bdfd4) )
	ROM_LOAD( "pd8d.p1", 0x0000, 0x010000, CRC(855896b5) SHA1(b093b1851cdfdf04d1f39b0a0c374de3594da97e) )
	ROM_LOAD( "pd8dj.p1", 0x0000, 0x010000, CRC(fa898fc4) SHA1(7c873ba80ed479b929a4223fafa031508d2dcb61) )
	ROM_LOAD( "pd8dk.p1", 0x0000, 0x010000, CRC(b76193c7) SHA1(ea7ae0f3031654435263fcf8b85dc8969216de94) )
	ROM_LOAD( "pd8dy.p1", 0x0000, 0x010000, CRC(8446848a) SHA1(23840190a3543c7fee0334bd1e9c0000eb2b7908) )
	ROM_LOAD( "pd8j.p1", 0x0000, 0x010000, CRC(8d74c338) SHA1(482fc028a04bd257a36b46ba3e6949f95cacd271) )
	ROM_LOAD( "pd8k.p1", 0x0000, 0x010000, CRC(f4753cad) SHA1(4d41a2c40f56267ea31375046058ab2b22700414) )
	ROM_LOAD( "pd8s.p1", 0x0000, 0x010000, CRC(65816bdb) SHA1(52717f789676ad66e4b8c5c023e23262408ef0b3) )
	ROM_LOAD( "pd8y.p1", 0x0000, 0x010000, CRC(c958ed40) SHA1(35c1905656d12c788e8766424dd400669189e2c7) )
	ROM_LOAD( "pdub.p1", 0x0000, 0x010000, CRC(e50a571b) SHA1(b8412ae7211bfbf8098ae3ae70dfc2a99cd8558d) )
	ROM_LOAD( "pdud.p1", 0x0000, 0x010000, CRC(24cddc59) SHA1(c4fa0530387c5cd172d51b766315d3874cc61618) )
	ROM_LOAD( "pdudy.p1", 0x0000, 0x010000, CRC(b852ea1f) SHA1(375f0baaf64b1ea1e118f6d93417877174e094bb) )
	ROM_LOAD( "pduk.p1", 0x0000, 0x010000, CRC(7d1c1897) SHA1(aa7753bef9b580f0a134960d74115cb43b91494f) )
	ROM_LOAD( "pduy.p1", 0x0000, 0x010000, CRC(460d967b) SHA1(ea55c87674d62ee6f525ae1ff08267e8b4b126aa) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "pdusnd.p1", 0x000000, 0x080000, CRC(1e5d8407) SHA1(64ee6eba3fb7700a06b89a1e0489a0cd54bb89fd) )
	ROM_LOAD( "pdusnd.p2", 0x080000, 0x080000, CRC(a5829cec) SHA1(eb65c86125350a7f384f9033f6a217284b6ff3d1) )
ROM_END


ROM_START( m4przfrt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pfr03s.p1", 0x0000, 0x010000, CRC(0ea80adb) SHA1(948a23fe8ccf6f423957a478a57bb875cc7b2cc2) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pfr03ad.p1", 0x0000, 0x010000, CRC(860cbd1b) SHA1(a3a3c0c3c5aff9b469ae82cf514937973b752421) )
	ROM_LOAD( "pfr03b.p1", 0x0000, 0x010000, CRC(2a7ba02c) SHA1(178fbf0301d263b32f9a8ac00e79731d074576d9) )
	ROM_LOAD( "pfr03bd.p1", 0x0000, 0x010000, CRC(dfff487c) SHA1(bf4bbb17241595ceb2c373c2bbd72fcecddedfd2) )
	ROM_LOAD( "pfr03d.p1", 0x0000, 0x010000, CRC(f5b1c305) SHA1(74c8c4a2f32af250cc44994425166fc8b6177610) )
	ROM_LOAD( "pfr03dk.p1", 0x0000, 0x010000, CRC(dc59bba1) SHA1(3ddb259dcda34efe0e8303dc39a3c082665709e9) )
	ROM_LOAD( "pfr03dr.p1", 0x0000, 0x010000, CRC(047fee03) SHA1(c0beb050062c175190dc5326ee21e9af6268bf81) )
	ROM_LOAD( "pfr03dy.p1", 0x0000, 0x010000, CRC(052c60fb) SHA1(bb3ac21ce7f9d41fbb5d62311c5f4a977d417adb) )
	ROM_LOAD( "pfr03k.p1", 0x0000, 0x010000, CRC(d8c2527c) SHA1(0c5925f3eb2e48038a2995367865daf687ee5db6) )
	ROM_LOAD( "pfr03r.p1", 0x0000, 0x010000, CRC(0a1aa1c9) SHA1(c4dcd3550e99a908fe4a9db34f3d8b685af57e30) )
	ROM_LOAD( "pfr03y.p1", 0x0000, 0x010000, CRC(9e230f5d) SHA1(f89c01b930af9cf952fa79baa0deab6503577a90) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "pfrsnd.p1", 0x0000, 0x080000, CRC(71d1af20) SHA1(d87d61c561acbe9cb3dec18d8decf5e970efa272) )
ROM_END


ROM_START( m4przhr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "prly.p1", 0x0000, 0x010000, CRC(feeac121) SHA1(e01f32fb4cdfbe61fdcd89749a33185ac0410720) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pr3ad.p1", 0x0000, 0x010000, CRC(8b047599) SHA1(fd2f21c2ed3e5cb4e4ace7ffa620131a1897cf92) )
	ROM_LOAD( "pr3b.p1", 0x0000, 0x010000, CRC(11d42c71) SHA1(ede99d2bbe597e4057a28c843b4b1b089e3427d2) )
	ROM_LOAD( "pr3bd.p1", 0x0000, 0x010000, CRC(b682a11f) SHA1(a5cb9d016e0ff877f506c890aa6733551aef5507) )
	ROM_LOAD( "pr3d.p1", 0x0000, 0x010000, CRC(7d82c742) SHA1(d51434779b43fd569fefaa09a89d3339be07b9bb) )
	ROM_LOAD( "pr3dk.p1", 0x0000, 0x010000, CRC(6f6b1df4) SHA1(c4db1a793e79a47d614154fb0091be2253012489) )
	ROM_LOAD( "pr3dy.p1", 0x0000, 0x010000, CRC(1ecf832e) SHA1(6b72bc6b25e8019b1867f17cfd74913e2850eacb) )
	ROM_LOAD( "pr3k.p1", 0x0000, 0x010000, CRC(41423db6) SHA1(928b7c91fe12b4cef2c6b9828f0dd0f51e223d75) )
	ROM_LOAD( "pr3s.p1", 0x0000, 0x010000, CRC(e4968894) SHA1(92b4b930f3bf370b213a72ad8328f19d5ebbd471) )
	ROM_LOAD( "pr3y.p1", 0x0000, 0x010000, CRC(81b214c0) SHA1(792db44df880ac58e0da8ed47fe25881a24891b0) )
	ROM_LOAD( "prlb.p1", 0x0000, 0x010000, CRC(b76f96cb) SHA1(2b0196542a99e60215ced488c7f5b2ae47b66ada) )
	ROM_LOAD( "prlbd.p1", 0x0000, 0x010000, CRC(efad3703) SHA1(2a2dd6e913936a3232aa51972bfd1d2f6f4e9857) )
	ROM_LOAD( "prld.p1", 0x0000, 0x010000, CRC(aeff3794) SHA1(84bdd743ec49ff8f1d4f34a2c9e14f427bc38b83) )
	ROM_LOAD( "prldk.p1", 0x0000, 0x010000, CRC(c003cabf) SHA1(1f031d362591d675d2cffec041a0762e431e64f5) )
	ROM_LOAD( "prldy.p1", 0x0000, 0x010000, CRC(15b4e8f3) SHA1(92c3be901f038a18906db674129e153ea61d70f4) )
	ROM_LOAD( "prlk.p1", 0x0000, 0x010000, CRC(f2be8c36) SHA1(411a5e1614a4f7963ebbb87e1a3a63209801f6da) )
	ROM_LOAD( "prls.p1", 0x0000, 0x010000, CRC(8cc08272) SHA1(8b25b99291a288f198573272d705c3592c7c60e6) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "prlsnd.p1", 0x0000, 0x080000, CRC(d60181ea) SHA1(4ca872e50d59dc96e90ade8cac24ebbab8a3f397) )
ROM_END


ROM_START( m4przlux )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "plxs.p1", 0x0000, 0x010000, CRC(0aea0339) SHA1(28da52924fe2bf00799ef466143103e08399f5f5) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "plxad.p1", 0x0000, 0x010000, CRC(e52ddf4f) SHA1(ec3f198fb6658cadd45046ef7586f9178f95d814) )
	ROM_LOAD( "plxb.p1", 0x0000, 0x010000, CRC(03b0f7bd) SHA1(0ce1cec1afa0a2efee3bc55a2b9cdf8fec7d3ebc) )
	ROM_LOAD( "plxd.p1", 0x0000, 0x010000, CRC(46ae371e) SHA1(a164d0336ed6bf7d25f406e28a01bbec86f4b723) )
	ROM_LOAD( "plxdy.p1", 0x0000, 0x010000, CRC(40fbaad9) SHA1(d3da773d49941e87d008313e309c4dbe7c9bade2) )
	ROM_LOAD( "plxk.p1", 0x0000, 0x010000, CRC(8a15d3bc) SHA1(4536a52101f79ff352b446d130f7a15a0e4cb7df) )
	ROM_LOAD( "plxy.p1", 0x0000, 0x010000, CRC(0e5a2c5c) SHA1(7c64f9ad3aac30b4140be12ec451d17fd3b83b7a) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "plxsnd.p1", 0x000000, 0x080000, CRC(0e682b6f) SHA1(459a7ca216c47af58c03c15d6ef1f9aa7489eba0) )
	ROM_LOAD( "plxsnd.p2", 0x080000, 0x080000, CRC(3ef95a7f) SHA1(9c918769fbf0e687f27e431d934e2327df9ed3bb) )
ROM_END


ROM_START( m4przmon )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fp8ad.p1", 0x0000, 0x010000, CRC(9c1c443a) SHA1(58e45501c33d0fd8ecca7e7bc40fef60ebb519e9) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "fp8b.p1", 0x0000, 0x010000, CRC(2a8cd9da) SHA1(2364853f3c78ca4f47aac8609649f06bf3a98ba1) )
	ROM_LOAD( "fp8bd.p1", 0x0000, 0x010000, CRC(bbb342fd) SHA1(5117304284a25ce43798a0a1c8c1c45d25f707ab) )
	ROM_LOAD( "fp8d.p1", 0x0000, 0x010000, CRC(2e6dea1e) SHA1(8b0877277c414693b0d6c9d22ef86cbb487b4d2e) )
	ROM_LOAD( "fp8dj.p1", 0x0000, 0x010000, CRC(8e3121ef) SHA1(9770af9fa1ac14c85a1f856ef2ef5e2867ff06ad) )
	ROM_LOAD( "fp8dk.p1", 0x0000, 0x010000, CRC(1368d4cd) SHA1(2f77fefe2a0f355115ad7c173fc1552c4893095a) )
	ROM_LOAD( "fp8dy.p1", 0x0000, 0x010000, CRC(2c8d3a96) SHA1(413e619c76209f948885ea0ff2388a2fcb0134d6) )
	ROM_LOAD( "fp8j.p1", 0x0000, 0x010000, CRC(2a834685) SHA1(184a5e157dc2994823f4a1077b3bc0e3b69fda34) )
	ROM_LOAD( "fp8k.p1", 0x0000, 0x010000, CRC(48cf748a) SHA1(2116f6cc00822ac9d4d3b090443d0f84fe3b5194) )
	ROM_LOAD( "fp8s.p1", 0x0000, 0x010000, CRC(b43eef89) SHA1(15991ad9223ddce77277f5451b5557ff59e2647c) )
	ROM_LOAD( "fp8y.p1", 0x0000, 0x010000, CRC(c3ee5211) SHA1(02c51f28bdeb7b7fdc7bb95cdc79117eb733789c) )
	ROM_LOAD( "fpmb.p1", 0x0000, 0x010000, CRC(e3265d54) SHA1(e283d1675e529c600454f12f87fce370d517e11c) )
	ROM_LOAD( "fpmd.p1", 0x0000, 0x010000, CRC(60b2051c) SHA1(9543997fe8fa168bcc66edc3aef6f7e69b4fb326) )
	ROM_LOAD( "fpmdy.p1", 0x0000, 0x010000, CRC(422b8f68) SHA1(d18926c7228dbd8f5228b6bd03d265318b5296fe) )
	ROM_LOAD( "fpmk.p1", 0x0000, 0x010000, CRC(84f58f68) SHA1(e2297d53c8a7ee3c5058fc734b1f4ec533e93734) )
	ROM_LOAD( "fpms.p1", 0x0000, 0x010000, CRC(2d71e7f5) SHA1(16040a042cb0824b44869e618f38edcabd9d47d6) )
	ROM_LOAD( "fpmy.p1", 0x0000, 0x010000, CRC(2728c725) SHA1(d36f8129731f9479ed526f9abfab8647cf43fdce) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "mt_05a__.3o3", 0x0000, 0x010000, CRC(4175f4a9) SHA1(b0e172e4862aa3b7be7accefc90e98d07d449b65) )
	ROM_LOAD( "mt_05a__.4o1", 0x0000, 0x010000, CRC(637fecee) SHA1(8c970bdf703177c71dde5c774c75929ac42b6eb0) )
	ROM_LOAD( "mt_05s__.3o3", 0x0000, 0x010000, CRC(92d674b7) SHA1(a828a9b0d870122bc09d865de90b8efa428f3fd0) )
	ROM_LOAD( "mt_05sb_.3o3", 0x0000, 0x010000, CRC(1158e506) SHA1(8c91bfe29545bbbc0d136a8c9abef785cadc3c64) )
	ROM_LOAD( "mt_05sd_.3o3", 0x0000, 0x010000, CRC(5ed3d947) SHA1(4b9bc9be6e79014ad6ca95293eb464af39e40dc1) )
	ROM_LOAD( "mt_10a__.3o3", 0x0000, 0x010000, CRC(6a8172a4) SHA1(92c081535258677e90d9f9748a168926c7a0cbed) )
	ROM_LOAD( "mt_10a__.4o1", 0x0000, 0x010000, CRC(36eeac30) SHA1(daa662392874806d18d4a161d39caed7e0abca73) )
	ROM_LOAD( "mt_10s__.3o3", 0x0000, 0x010000, CRC(1b66f0f8) SHA1(308227b0144f0568df8190810e0de627b413a742) )
	ROM_LOAD( "mt_10sb_.3o3", 0x0000, 0x010000, CRC(06a33d34) SHA1(5fa1269a7cf42ef14e2a19143a07bf28b38ad920) )
	ROM_LOAD( "mt_10sd_.3o3", 0x0000, 0x010000, CRC(42629cb1) SHA1(12f695e1f70bf93100c1af8052dcee9131711510) )
	ROM_LOAD( "mti05___.4o1", 0x0000, 0x010000, CRC(0e82c258) SHA1(c4aa7d32bcd9418e2919be8be8a2f9e60d46f316) )
	ROM_LOAD( "mti10___.4o1", 0x0000, 0x010000, CRC(a35e0571) SHA1(9a22946047e76392f0c4534f892ee9ae9e700503) )


	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing */
ROM_END

ROM_START( m4przmns )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "spmy.p1", 0x0000, 0x010000, CRC(2b27b2a0) SHA1(07950616da39e39d19452859390d3eaad89ea377) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sm8ad.p1", 0x0000, 0x010000, CRC(6272ae09) SHA1(96130f62646424dd9f2f34f2858a2635ec615f03) )
	ROM_LOAD( "sm8b.p1", 0x0000, 0x010000, CRC(25d95c1b) SHA1(7aa448d1fb383d1b89e71bbc63a554eaa5e06141) )
	ROM_LOAD( "sm8bd.p1", 0x0000, 0x010000, CRC(bf58108f) SHA1(a0dfc2447a014f4a9b1abad3f954ee9c58251289) )
	ROM_LOAD( "sm8d.p1", 0x0000, 0x010000, CRC(b4524fb3) SHA1(11d2542a43e61cee6cce0e621fe80f9aa6811ec2) )
	ROM_LOAD( "sm8dk.p1", 0x0000, 0x010000, CRC(f077ac65) SHA1(9baa5d2fd9833838d48c202a57aaa98783130dbc) )
	ROM_LOAD( "sm8dy.p1", 0x0000, 0x010000, CRC(2df61788) SHA1(003d6e172cee41cf9704dc285c2a0b39ee247ea8) )
	ROM_LOAD( "sm8k.p1", 0x0000, 0x010000, CRC(8d02ca2b) SHA1(b5defdc50fee9e9f1379571b638702c0779fd450) )
	ROM_LOAD( "sm8s.p1", 0x0000, 0x010000, CRC(be159855) SHA1(277884b5417857fa661b09d3e41bef2b22b89f6c) )
	ROM_LOAD( "sm8y.p1", 0x0000, 0x010000, CRC(51e76e1d) SHA1(3045ab447871c7369c5ed53da75326e64d6e57d9) )
	ROM_LOAD( "spmb.p1", 0x0000, 0x010000, CRC(752dd1c6) SHA1(e180c959bc3fb8bce9da22ed6e74fa03e4562a74) )
	ROM_LOAD( "spmd.p1", 0x0000, 0x010000, CRC(34172b4f) SHA1(8594d3863e3de3e6300cd5f4588545bf82c89e00) )
	ROM_LOAD( "spmdy.p1", 0x0000, 0x010000, CRC(1abed85e) SHA1(0b2d7e0127c30f6704a7f64a2955ecf3e8010206) )
	ROM_LOAD( "spmk.p1", 0x0000, 0x010000, CRC(ba2f467a) SHA1(327ebad946b028f387e04e9db9f882320995d175) )
	ROM_LOAD( "spms.p1", 0x0000, 0x010000, CRC(7d684358) SHA1(b07b13d6827e5ea4127eb763f4233a3d35ea99e6) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "s&fpmsnd.p1", 0x000000, 0x080000, CRC(e5bfc522) SHA1(38c8430f539d38a51a3d7fb846b625ae2080e930) )
	ROM_LOAD( "s&fpmsnd.p2", 0x080000, 0x080000, CRC(e14803ab) SHA1(41d501f61f202df2dbd2ac13c40a32fff6afc861) )
ROM_END


ROM_START( m4przmc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "mssb.p1", 0x0000, 0x010000, CRC(5210dae0) SHA1(cc9916718249505e031ccdbc126f3fa1e6675f27) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "mssad.p1", 0x0000, 0x010000, CRC(e3690c35) SHA1(fdaacda0d03ce8d54841525feff2529b1ee1f970) )
	ROM_LOAD( "mssd.p1", 0x0000, 0x010000, CRC(cf59305e) SHA1(7ba6f37aa1077561129f66ab663730fb6e5108ed) )
	ROM_LOAD( "mssdy.p1", 0x0000, 0x010000, CRC(12d7db63) SHA1(6e1e6b13783888f3d508d7cbecc52c65ffc99fb0) )
	ROM_LOAD( "mssk.p1", 0x0000, 0x010000, CRC(d56f62dc) SHA1(7df1fad20901607e710e8a7f64033f77d613a0fa) )
	ROM_LOAD( "msss.p1", 0x0000, 0x010000, CRC(c854c12f) SHA1(917d091383b07a995dc2c441717885b181a02d3c) )
	ROM_LOAD( "mssy.p1", 0x0000, 0x010000, CRC(159f4baa) SHA1(073c13e6bff4a641b29e5a45f88e3533aff460e4) )
ROM_END


ROM_START( m4przrf )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "pr8ad.p1", 0x0000, 0x020000, CRC(ebada7c9) SHA1(4a1e2f746116c23f87b53d25bd8b11322962306f) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pr8b.p1", 0x0000, 0x020000, CRC(4a6448b6) SHA1(061dbc1603fff0cb60e02acdf21881047b2b7d43) )
	ROM_LOAD( "pr8bd.p1", 0x0000, 0x020000, CRC(66b7090c) SHA1(774f5b1403109ccc7ac1bc188f30e8b3a5025aad) )
	ROM_LOAD( "pr8d.p1", 0x0000, 0x020000, CRC(377f43c0) SHA1(14e29f1832afc47f06752d7da11cc2cb40fcb368) )
	ROM_LOAD( "pr8dj.p1", 0x0000, 0x020000, CRC(42629fdf) SHA1(79148956b0b2da42400fe3cc0a61955c77a6bf32) )
	ROM_LOAD( "pr8dk.p1", 0x0000, 0x020000, CRC(2c70a15a) SHA1(9dacf5eca4d7e41b09ee53ffc532a2928b1f60b4) )
	ROM_LOAD( "pr8dy.p1", 0x0000, 0x020000, CRC(ab9381a4) SHA1(90c3a048ad5c1e19007b6e089750a9e4b299d2a3) )
	ROM_LOAD( "pr8j.p1", 0x0000, 0x020000, CRC(6eb1de65) SHA1(b9e13173191e9a45fab29936b303a914e372918f) )
	ROM_LOAD( "pr8k.p1", 0x0000, 0x020000, CRC(00a3e0e0) SHA1(c0671052de5cdd7f169ca50590b9c4f0f10cb678) )
	ROM_LOAD( "pr8s.p1", 0x0000, 0x020000, CRC(bbbdd4f4) SHA1(72c2a8b3404384b524f49fc2d6507e2d8dab85cb) )
	ROM_LOAD( "pr8y.p1", 0x0000, 0x020000, CRC(8740c01e) SHA1(c75f4ad724e735a2ffabc9f7cce96dcb341eaf4a) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4przrfm )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "prub.p1", 0x0000, 0x010000, CRC(748f220f) SHA1(5d729057d521fa656375610e424cfd4088f6ea02) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "prud.p1", 0x0000, 0x010000, CRC(426bf7c1) SHA1(998b7968d4ed2fb0d1fcaf13929c76670100d9df) )
	ROM_LOAD( "prudy.p1", 0x0000, 0x010000, CRC(e9f76ebd) SHA1(8f1151e123e73ac40fdb6f071960d1ed3e72692a) )
	ROM_LOAD( "pruk.p1", 0x0000, 0x010000, CRC(b995d098) SHA1(22107fbbc8c4e026fc34159114cdbfcd130f814e) )
	ROM_LOAD( "prus.p1", 0x0000, 0x010000, CRC(d6c22253) SHA1(f9a25dd1c6f16849a6eb1febdc2da16080cc6838) )
	ROM_LOAD( "pruy.p1", 0x0000, 0x010000, CRC(fcd8add4) SHA1(14e922daf24d981a3a65463bf64213722d8ba758) )
	ROM_LOAD( "rm8b.p1", 0x0000, 0x010000, CRC(181da11e) SHA1(c06a9626a541a56d707f9b80806714020cefa7b2) )
	ROM_LOAD( "rm8bd.p1", 0x0000, 0x010000, CRC(b3d983b5) SHA1(7881c31617855983981f93190afddb0aa880ce0a) )
	ROM_LOAD( "rm8d.p1", 0x0000, 0x010000, CRC(94377ab0) SHA1(2c43dfd11eeca53faae661d7af4a986fdbb6d7e9) )
	ROM_LOAD( "rm8dj.p1", 0x0000, 0x010000, CRC(601b8f3b) SHA1(3cc130adb5e78e9a5380b27a219a022201293988) )
	ROM_LOAD( "rm8dk.p1", 0x0000, 0x010000, CRC(8b281018) SHA1(5d1f68662b206f9c9948d32fdcda98d99a53987b) )
	ROM_LOAD( "rm8dy.p1", 0x0000, 0x010000, CRC(bac738e3) SHA1(21bd359cfeaf1e33268cecef08d8c7d23d89360c) )
	ROM_LOAD( "rm8j.p1", 0x0000, 0x010000, CRC(b825b8fd) SHA1(6fa58784018fd7be6528e60d8642803cca55c15d) )
	ROM_LOAD( "rm8k.p1", 0x0000, 0x010000, CRC(3f559f9e) SHA1(f70c127490859a3b4c405fd0efd18168dd3b0728) )
	ROM_LOAD( "rm8s.p1", 0x0000, 0x010000, CRC(9ab83f24) SHA1(bdc72a9d6f22244a2be86b035fac84433705ce78) )
	ROM_LOAD( "rm8y.p1", 0x0000, 0x010000, CRC(47a3873e) SHA1(51baf82a7a4dee10b1a2f7862030f960912d8d7c) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing */
ROM_END


ROM_START( m4przsss )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps302b.p1", 0x0000, 0x010000, CRC(1749ae18) SHA1(f04f91a1d534f2d2dc844862bb21160c5903d1df) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ps302ad.p1", 0x0000, 0x010000, CRC(e57f52d7) SHA1(25384517b68c488acd38956aeb69dda26d63c3ca) )
	ROM_LOAD( "ps302bd.p1", 0x0000, 0x010000, CRC(d3633f9d) SHA1(2500425d736a5c45f5bf40a7660b549f822266dc) )
	ROM_LOAD( "ps302d.p1", 0x0000, 0x010000, CRC(df1bfe3b) SHA1(a82574ff9eb04deccfbb8907ca8936b53f691b2c) )
	ROM_LOAD( "ps302dk.p1", 0x0000, 0x010000, CRC(88b49246) SHA1(122384d6c350e28fdbb3e2a02e5db7076ec4bb43) )
	ROM_LOAD( "ps302dy.p1", 0x0000, 0x010000, CRC(ada3ab8c) SHA1(421aaf0951cb1d47b7138ca611d2ebd6caf24a61) )
	ROM_LOAD( "ps302k.p1", 0x0000, 0x010000, CRC(23719bee) SHA1(13b7fd4f9edc60727e37078f6f2e24a63abd09f1) )
	ROM_LOAD( "ps302s.p1", 0x0000, 0x010000, CRC(4521c521) SHA1(90b5e444829ecc9a9b3e46f942830d263fbf02d3) )
	ROM_LOAD( "ps302y.p1", 0x0000, 0x010000, CRC(2ffed329) SHA1(a917161a7ea8312ef6a4a9a85f36f3b0a42b3a0c) )
	ROM_LOAD( "ps8ad.p1", 0x0000, 0x010000, CRC(48917a87) SHA1(d32ac9e30ebddb6ca1d6a7d6c38026338c6df2cd) )
	ROM_LOAD( "ps8b.p1", 0x0000, 0x010000, CRC(7633226d) SHA1(581dfb56719682a744fe2b4f63bd1c20eb943903) )
	ROM_LOAD( "ps8bd.p1", 0x0000, 0x010000, CRC(92e384db) SHA1(ab1c2c7aebb9c8c0cff6dd43d74551c15de0c805) )
	ROM_LOAD( "ps8d.p1", 0x0000, 0x010000, CRC(4b8a1374) SHA1(112fc0f0d1311482d292704ab807e15024b37cb9) )
	ROM_LOAD( "ps8dj.p1", 0x0000, 0x010000, CRC(9949fe88) SHA1(8ba8fd30bb12e47b97ddb9f4aba1eac880e5a12e) )
	ROM_LOAD( "ps8dk.p1", 0x0000, 0x010000, CRC(61e56c80) SHA1(93ef6601397063f412b35cbe90a5f7ecb3af2491) )
	ROM_LOAD( "ps8dy.p1", 0x0000, 0x010000, CRC(d4080a4a) SHA1(9907fea71237742595e5acd583c190a6180b4af9) )
	ROM_LOAD( "ps8j.p1", 0x0000, 0x010000, CRC(a9dcd1a8) SHA1(ec840aace95cab8c626a54636b47058401ef1eed) )
	ROM_LOAD( "ps8k.p1", 0x0000, 0x010000, CRC(7ed46dac) SHA1(481556298696d7f73d834034d0ce8628eb95b76c) )
	ROM_LOAD( "ps8s.p1", 0x0000, 0x010000, CRC(820a600a) SHA1(48701e315a94f92048ceb2e98df2bac1f04415e1) )
	ROM_LOAD( "ps8y.p1", 0x0000, 0x010000, CRC(a4d6934b) SHA1(215ed246f37daf1f8cdd0113b7b87e89c1aa2514) )
	ROM_LOAD( "sspb.p1", 0x0000, 0x010000, CRC(a781cdb8) SHA1(cbb1b9a85a80db7c91752349546bf55df4aea3f2) )
	ROM_LOAD( "sspd.p1", 0x0000, 0x010000, CRC(bcce54d7) SHA1(00a967188ddf1588331cda60e2589f6635e0a7ea) )
	ROM_LOAD( "sspdb.p1", 0x0000, 0x010000, CRC(edb5961e) SHA1(e1127d34148f04f9e34074269ee3740269105c63) )
	ROM_LOAD( "sspdy.p1", 0x0000, 0x010000, CRC(a368812e) SHA1(f377f13b866196fdbba07529f25713f9b5b91df5) )
	ROM_LOAD( "sspr.p1", 0x0000, 0x010000, CRC(720bad67) SHA1(3ee25abfc15e1c36a3ac6ac94e5229f938a39991) )
	ROM_LOAD( "ssprd.p1", 0x0000, 0x010000, CRC(b2ec7b80) SHA1(b562fbf2501dbaf0ec7c66d993df867384e750ff) )
	ROM_LOAD( "ssps.p1", 0x0000, 0x010000, CRC(e36f4d48) SHA1(fb88e8bcddb7dd2722b203a0ebb3a64c6b75ff24) )
	ROM_LOAD( "sspy.p1", 0x0000, 0x010000, CRC(0ea8f052) SHA1(3134ff47e6c5c4d200ffcdf0a5a3cb7b05b0fc2c) )
ROM_END


ROM_START( m4przve )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pess.p1", 0x0000, 0x010000, CRC(d8e79833) SHA1(f68fd1bd057a353832c7de3e2818906ab2b844b7) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pe8ad.p1", 0x0000, 0x010000, CRC(3a81422e) SHA1(bb77365ed7bc7c2cd9e1cfe6e266c6edfd3562a3) )
	ROM_LOAD( "pe8b.p1", 0x0000, 0x010000, CRC(9f36b112) SHA1(265451557afcfdc1aa8e77616f4b871698b20c5f) )
	ROM_LOAD( "pe8bd.p1", 0x0000, 0x010000, CRC(af0689b5) SHA1(0e3cf464c855b0dcfeac403bda80818287707abe) )
	ROM_LOAD( "pe8d.p1", 0x0000, 0x010000, CRC(d21ee810) SHA1(bb95e217a383332a6617644bf17239c1ebd7c7e7) )
	ROM_LOAD( "pe8dj.p1", 0x0000, 0x010000, CRC(2bdf3b80) SHA1(2d27373e01c4308c9b0818c8193b459b1b335634) )
	ROM_LOAD( "pe8dk.p1", 0x0000, 0x010000, CRC(fcece282) SHA1(95e5bfeef8618f422501bcb9b703887587b3067a) )
	ROM_LOAD( "pe8dy.p1", 0x0000, 0x010000, CRC(6abc5682) SHA1(ae8754f0e214738adae4bc856cd72b0920aaa67a) )
	ROM_LOAD( "pe8j.p1", 0x0000, 0x010000, CRC(d3bf07f1) SHA1(3e539f24ef25c8d6fbfbdcd469fa8a2908dd2ec2) )
	ROM_LOAD( "pe8k.p1", 0x0000, 0x010000, CRC(efba0d3f) SHA1(2205b94f5a6ed23e834cfeb0d3ebe5ed66d942a1) )
	ROM_LOAD( "pe8s.p1", 0x0000, 0x010000, CRC(e8463e69) SHA1(923d6c79470a65cf66b089ef09898acea928aa9b) )
	ROM_LOAD( "pe8y.p1", 0x0000, 0x010000, CRC(c324b75f) SHA1(bf3409a193539e1e032c856a5316bec674043d57) )
	ROM_LOAD( "pesb.p1", 0x0000, 0x010000, CRC(bf0ffed9) SHA1(ab8cd98ae7dfb3582aad7ae8c669a6d97f144f88) )
	ROM_LOAD( "pesd.p1", 0x0000, 0x010000, CRC(6f7b1e16) SHA1(412a22ebb61b77541da067ba74621c8e54364471) )
	ROM_LOAD( "pesdy.p1", 0x0000, 0x010000, CRC(94db27b4) SHA1(fe745a991a5e78fc9054480d3ce5bf6b7f5f9fe4) )
	ROM_LOAD( "pesk.p1", 0x0000, 0x010000, CRC(9e7b9f58) SHA1(86c2a83964f925448dda189546d9909b10e52673) )
	ROM_LOAD( "pesy.p1", 0x0000, 0x010000, CRC(fbfc1563) SHA1(870239cab39eff33303fe06dfd1dd3db708f0f2d) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "pessnd.p1", 0x000000, 0x080000, CRC(e7975c75) SHA1(407c3bcff29f4b6599de2c35d96f62c72a897bd1) )
	ROM_LOAD( "pessnd.p2", 0x080000, 0x080000, CRC(9f22f32d) SHA1(af64f6bde0b825d474c42c56f6e2253b28d4f90f) )
ROM_END


ROM_START( m4przwo )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pwo206ac", 0x0000, 0x010000, CRC(b9dd88e7) SHA1(4c60e7a28b538ff2483839fc66600037ccd99440) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pwob.p1", 0x0000, 0x010000, CRC(9e9f65d7) SHA1(69d28a1e08d2bde1a9c4d55555478808546ad4f0) )
	ROM_LOAD( "pwod.p1", 0x0000, 0x010000, CRC(ae97b585) SHA1(d6b90d8b696a21f9fa6b06c63a329b1370edd224) )
	ROM_LOAD( "pwody.p1", 0x0000, 0x010000, CRC(3abfd1c9) SHA1(131811807396103641d73cd7cef1797a6cecb35b) )
	ROM_LOAD( "pwok.p1", 0x0000, 0x010000, CRC(b8631e11) SHA1(c01aff60dad14945c2b45992f0112c6fc0ae7c5a) )
	ROM_LOAD( "pwos.p1", 0x0000, 0x010000, CRC(6a87aa68) SHA1(3dc8c006de3adcada43c3581be0ff921081ecff0) )
	ROM_LOAD( "pwoy.p1", 0x0000, 0x010000, CRC(1ada4987) SHA1(05a0480f5a92faaedc8183d948c7e2d657bda2a4) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "pwos.chr", 0x0000, 0x000048, CRC(352b86c4) SHA1(59c26a1948ffd6ecea08d8ca8e62735ec9732c0f) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "pwo.s1", 0x000000, 0x080000, CRC(1dbd8a33) SHA1(37bd71688475591232422eb0841e23aff58e3800) )
	ROM_LOAD( "pwo.s2", 0x080000, 0x080000, CRC(6c7badef) SHA1(416c36fe2b4253bf7944b3ba412561bd0d21cbe5) )
ROM_END


ROM_START( m4przwta )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "pwnr.p1", 0x0000, 0x020000, CRC(cf619ad2) SHA1(3eeccccb304afd5faf2563e0e65f8123e463d363) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pw8ad.p1", 0x0000, 0x020000, CRC(71257e43) SHA1(1db17aa1fc684873511a46e5e7421b459040d0cc) )
	ROM_LOAD( "pw8b.p1", 0x0000, 0x020000, CRC(52b2af18) SHA1(1ce00b94a2d16b5140a110e604b97af6860fd577) )
	ROM_LOAD( "pw8bd.p1", 0x0000, 0x020000, CRC(fc3fd086) SHA1(8fa8b75faf2196e87acbafc3a48a2ba628f6cc66) )
	ROM_LOAD( "pw8d.p1", 0x0000, 0x020000, CRC(2fa9a46e) SHA1(4f351fc4e5e0ea9316893981420041d0d5613aec) )
	ROM_LOAD( "pw8dj.p1", 0x0000, 0x020000, CRC(d8ea4655) SHA1(f3c02638d20eaef95e09e51301445c9a4c215514) )
	ROM_LOAD( "pw8dk.p1", 0x0000, 0x020000, CRC(b6f878d0) SHA1(51bbeb36cc5c086442ec3951d2679fbf0a5ceebd) )
	ROM_LOAD( "pw8dy.p1", 0x0000, 0x020000, CRC(311b582e) SHA1(f53254b8c6f65087f67d60f0d0441228e1024cc8) )
	ROM_LOAD( "pw8j.p1", 0x0000, 0x020000, CRC(766739cb) SHA1(8b7cd7f02fb25f5e50febdb90c8f39f3a6840a35) )
	ROM_LOAD( "pw8k.p1", 0x0000, 0x020000, CRC(1875074e) SHA1(fb25c4ed4ba9d6aa5fb45a8ba9c73b18062173f0) )
	ROM_LOAD( "pw8s.p1", 0x0000, 0x020000, CRC(3d77c91d) SHA1(3ab79073f5d9c13f751892aa33c2668521887bf8) )
	ROM_LOAD( "pw8y.p1", 0x0000, 0x020000, CRC(9f9627b0) SHA1(19c9dc7033b1e85676222a8b3a866392a4afdd1e) )
	ROM_LOAD( "pwna.p1", 0x0000, 0x020000, CRC(bbb32770) SHA1(36815dca6550a1a417b3e809b041a7b4670f5b75) )
	ROM_LOAD( "pwnb.p1", 0x0000, 0x020000, CRC(36a989b5) SHA1(631399c65b697417ed9a95961463b8349a97b142) )
	ROM_LOAD( "pwndy.p1", 0x0000, 0x020000, CRC(6b739a41) SHA1(64f4b380c725f6159c6201147ab4062d6375d98b) )
	ROM_LOAD( "pwnk.p1", 0x0000, 0x020000, CRC(7c6e21e3) SHA1(d6aeb5948e0800050193575a3b5c06c11f46eed8) )
	ROM_LOAD( "pwns.p1", 0x0000, 0x020000, CRC(b3b87954) SHA1(f998ebf8047930f006213040ed5e6a9f90844143) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "pwnsnd.p1", 0x000000, 0x080000, CRC(c0f5e160) SHA1(eff218a36912fe599e9d73a96b49e75335bba272) )
	ROM_LOAD( "pwnsnd.p2", 0x080000, 0x080000, CRC(d81dfc8f) SHA1(5fcfcba836080b5752911d69dfe650614acbf845) )
ROM_END

ROM_START( m4randr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "drr22.bin", 0x0000, 0x010000, CRC(9fdcee66) SHA1(196c8d3ea4141c209ecc5b0acdab7a872f791dc0) )
ROM_END


ROM_START( m4rsg )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rsg1.bin", 0xc000, 0x004000, CRC(10c64308) SHA1(b681720bb6edc0fd241c2961b6b2b065d1bbe38c) )
	ROM_LOAD( "rsg2.bin", 0x8000, 0x004000, CRC(975f66e1) SHA1(f74fb6db0b41596c7f0e12ff643b078ac6b93beb) )
	ROM_LOAD( "rsg3.bin", 0x6000, 0x002000, CRC(9a355e95) SHA1(041ec72af65e5dca32dc524b3874449ffd40df64) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "rsg_p1.bin", 0xc000, 0x004000, CRC(cd582779) SHA1(6970568298229cfa9ef9f3dd7793c1637842f396) )
	ROM_LOAD( "rsg_p2.bin", 0x8000, 0x004000, CRC(1a167031) SHA1(b539bc4ba79cd316c0492082d54e148fd50beb19) )
	ROM_LOAD( "rsg_p3.bin", 0x6000, 0x002000, CRC(a7112951) SHA1(5abc89d360ef60d7e6cf5c89562cbc648b615544) )
ROM_END


ROM_START( m4rgsa )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rgob.p1", 0x0000, 0x010000, CRC(43ac7b73) SHA1(994d6256432543e1353521359f8faaea671a7bea) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cgo11ad.p1", 0x0000, 0x010000, CRC(9f8bbdaf) SHA1(210cdc9ce493edbf55d43a3127b10931e3ce2fee) )
	ROM_LOAD( "cgo11b.p1", 0x0000, 0x010000, CRC(2ea96acb) SHA1(ffcf1fcb2b769b29b53b00c9ce80af061cc21b9d) )
	ROM_LOAD( "cgo11bd.p1", 0x0000, 0x010000, CRC(4cabc589) SHA1(2b0b91f4ac6ebd18edb7a913b8079acc9f026e7d) )
	ROM_LOAD( "cgo11c.p1", 0x0000, 0x010000, CRC(76d36b80) SHA1(2699982fed3c2116ff0187d24059f59d3b6c1cae) )
	ROM_LOAD( "cgo11d.p1", 0x0000, 0x010000, CRC(63516954) SHA1(abefafe43e3386a5c916e55503bcb623d74840e1) )
	ROM_LOAD( "cgo11dk.p1", 0x0000, 0x010000, CRC(84f112ef) SHA1(85fa44c7b25aeb83fa2c199abafe099a8ae92bf8) )
	ROM_LOAD( "cgo11dr.p1", 0x0000, 0x010000, CRC(07d13cf6) SHA1(11685efebf9c7091191654fec1f2ac6ad3d05ce1) )
	ROM_LOAD( "cgo11dy.p1", 0x0000, 0x010000, CRC(13c5b934) SHA1(3212ba2534726c8fca9a70325acff3f6e85dd1f7) )
	ROM_LOAD( "cgo11k.p1", 0x0000, 0x010000, CRC(4f46e7f6) SHA1(9485edbcbb3a81b1a335a7c420aa676af8b14050) )
	ROM_LOAD( "cgo11r.p1", 0x0000, 0x010000, CRC(f44dd36f) SHA1(6623daaa237e97b9d63815393562fe8abdb8d732) )
	ROM_LOAD( "cgo11s.p1", 0x0000, 0x010000, CRC(a6b9ddd4) SHA1(b06d5d19b165b82c76b29f7925e0936aeccedb8c) )
	ROM_LOAD( "cgo11y.p1", 0x0000, 0x010000, CRC(d91653f6) SHA1(6445958cd07088fbf08c37a8b5540e3eb561d021) )
	ROM_LOAD( "drr02ad.p1", 0x0000, 0x010000, CRC(5acc5189) SHA1(abf66b90f4a64c3fb9ac4bf16f3bba2758f54482) )
	ROM_LOAD( "drr02b.p1", 0x0000, 0x010000, CRC(729e13c9) SHA1(dcefdd44592464616570101a5e05db31289fc66c) )
	ROM_LOAD( "drr02bd.p1", 0x0000, 0x010000, CRC(70c5b183) SHA1(b1431d0c2c48941d1ff6d6115c8d1ab026d71f63) )
	ROM_LOAD( "drr02c.p1", 0x0000, 0x010000, CRC(258acbf9) SHA1(ced9dbef9162ddadb4838ad430d50aa14574e97d) )
	ROM_LOAD( "drr02d.p1", 0x0000, 0x010000, CRC(60940b5a) SHA1(a4d293944e0e65f99dea9391d9d7e1066aa7b83d) )
	ROM_LOAD( "drr02dk.p1", 0x0000, 0x010000, CRC(0335775e) SHA1(4d943c3e522f5c42ddd2104c316f75eec90f494f) )
	ROM_LOAD( "drr02dr.p1", 0x0000, 0x010000, CRC(c05eef66) SHA1(ac5966ea0ff036d9c9179df6bc7aabd149f41d6c) )
	ROM_LOAD( "drr02dy.p1", 0x0000, 0x010000, CRC(6a700473) SHA1(4025a99aa9e87a80875d150e965650d339d2a143) )
	ROM_LOAD( "drr02k.p1", 0x0000, 0x010000, CRC(525e370e) SHA1(9849399643731beb31b7163b7eebd8774caf9289) )
	ROM_LOAD( "drr02r.p1", 0x0000, 0x010000, CRC(352613a0) SHA1(052e7770d55dd379d1bf3501e46d973bc4fc48d8) )
	ROM_LOAD( "drr02s.p1", 0x0000, 0x010000, CRC(67b03b7f) SHA1(61e09db8b7622e6e094c4e585dbcfea724155829) )
	ROM_LOAD( "drr02y.p1", 0x0000, 0x010000, CRC(009c7ece) SHA1(48463d7d0e521d51bad83ac5ddaaffabc68bf610) )
	ROM_LOAD( "hjj.hex", 0x0000, 0x010000, CRC(48ab2375) SHA1(4d9360a89e97a6bb7bdb099940d73f425eadd63d) )
	ROM_LOAD( "hjj02ad.p1", 0x0000, 0x010000, CRC(9f787e01) SHA1(d6cae1c1ae15b74285076e87c7fd8105f6a114ae) )
	ROM_LOAD( "hjj02b.p1", 0x0000, 0x010000, CRC(778ec121) SHA1(98454562da1da56d57ce3e6279805207671d7337) )
	ROM_LOAD( "hjj02bd.p1", 0x0000, 0x010000, CRC(7e8dbab0) SHA1(5b40536503b2d62792f874535367f5658acf8d2e) )
	ROM_LOAD( "hjj02c.p1", 0x0000, 0x010000, CRC(fbb149fc) SHA1(6a8305a3ef4a1818a12dab3d380e79b7e642a904) )
	ROM_LOAD( "hjj02d.p1", 0x0000, 0x010000, CRC(9e657e28) SHA1(3eaf9f8a0511f4533e9b47105d4417c71248fab2) )
	ROM_LOAD( "hjj02dk.p1", 0x0000, 0x010000, CRC(2e1bab77) SHA1(76a2784bc183c6d79a845bb7306eae687ced82a0) )
	ROM_LOAD( "hjj02dr.p1", 0x0000, 0x010000, CRC(9d26064f) SHA1(6596b3a671ab8e38b8357023f8994948ef1c1f0f) )
	ROM_LOAD( "hjj02dy.p1", 0x0000, 0x010000, CRC(0ab388b6) SHA1(cc8f157a8a91e3fb8bd1fbdd35989d72c8684c50) )
	ROM_LOAD( "hjj02k.p1", 0x0000, 0x010000, CRC(c224c58a) SHA1(5f9b5ff92e2f1b0438380d635b255ec8b4fc080f) )
	ROM_LOAD( "hjj02r.p1", 0x0000, 0x010000, CRC(32fefefe) SHA1(f58e228a1496b0858903c2d850c8453835b6f24b) )
	ROM_LOAD( "hjj02s.p1", 0x0000, 0x010000, CRC(39de9801) SHA1(c29e883c45ed6b272d65c7922b1871199a424244) )
	ROM_LOAD( "hjj02y.p1", 0x0000, 0x010000, CRC(0178cc91) SHA1(d618ff2eb0a1992b88f3b5427ffc54d34bf8c124) )
	ROM_LOAD( "ppl02ad.p1", 0x0000, 0x010000, CRC(d8b3be27) SHA1(95d1d979b439303817670fd686b5df324feb618f) )
	ROM_LOAD( "ppl02b.p1", 0x0000, 0x010000, CRC(dbd56cf1) SHA1(968c1d09626e493c51d7637e19a7f092047b283f) )
	ROM_LOAD( "ppl02bd.p1", 0x0000, 0x010000, CRC(eeafd36d) SHA1(68c314e937d24a59ca305facc409218c63bef24e) )
	ROM_LOAD( "ppl02c.p1", 0x0000, 0x010000, CRC(ad6f5c6d) SHA1(91f3d7bad3cdb7014ff3caa1631e6567cb95f47e) )
	ROM_LOAD( "ppl02d.p1", 0x0000, 0x010000, CRC(041f0fd8) SHA1(8e32c88f7b0a541a9460926a2fec0318a7239279) )
	ROM_LOAD( "ppl02dk.p1", 0x0000, 0x010000, CRC(632ecd46) SHA1(9e90279db99aa22923a79e309b053b35b70c9f8e) )
	ROM_LOAD( "ppl02dy.p1", 0x0000, 0x010000, CRC(4fb07726) SHA1(f234018ea18511217d176023b489254cf5a5a15e) )
	ROM_LOAD( "ppl02k.p1", 0x0000, 0x010000, CRC(7fcc03d7) SHA1(359743edec7ca54bd9a780f81ac25d314ada2d7e) )
	ROM_LOAD( "ppl02r.p1", 0x0000, 0x010000, CRC(e35580e3) SHA1(397bd2ce068aa45f3c55a3ddd97ae3e09391a7da) )
	ROM_LOAD( "ppl02s.p1", 0x0000, 0x010000, CRC(40c1d256) SHA1(abd55dcc06b49d54976743c610ad3de21278ac2d) )
	ROM_LOAD( "ppl02y.p1", 0x0000, 0x010000, CRC(7802f70c) SHA1(c96758c02bebff4b85436a93ae012c80c6cb2963) )
	ROM_LOAD( "rgoad.p1", 0x0000, 0x010000, CRC(d4ed739c) SHA1(6a7d5f63eaf59f08a8f870aba8523e2dc59d20cd) )
	ROM_LOAD( "rgobd.p1", 0x0000, 0x010000, CRC(0505340c) SHA1(e61b007dc50beb22bf3efa2c3cfab595880d3248) )
	ROM_LOAD( "rgod.p1", 0x0000, 0x010000, CRC(f3898077) SHA1(4d2f32b4c3f01a0b54966dd0558dcadcf89fd229) )
	ROM_LOAD( "rgodk.p1", 0x0000, 0x010000, CRC(9a9b61c7) SHA1(756ed419451d1e070809303467789e01949dea2b) )
	ROM_LOAD( "rgody.p1", 0x0000, 0x010000, CRC(a0eef0f0) SHA1(781de603d19eab0ee771b10374f53c149432c877) )
	ROM_LOAD( "rgok.p1", 0x0000, 0x010000, CRC(00413e8f) SHA1(580efbdf3ba092978648d83b6d21b5a4966d57e3) )
	ROM_LOAD( "rgos.p1", 0x0000, 0x010000, CRC(d00d3540) SHA1(0fd6a08477d05d1c129038c8de47de68a28c0a56) )
	ROM_LOAD( "rgoy.p1", 0x0000, 0x010000, CRC(cfdfce82) SHA1(68464381f658f08efb3f790eea1e7dd61086f936) )
	ROM_LOAD( "rgt10ad.p1", 0x0000, 0x010000, CRC(22f65e05) SHA1(d488e4c65059b3b7e8e88e39a05e0cc9eae2d836) )
	ROM_LOAD( "rgt10b.p1", 0x0000, 0x010000, CRC(5d86b45a) SHA1(a7553848a1e4304acaf72f9d293123ca2af629f0) )
	ROM_LOAD( "rgt10bd.p1", 0x0000, 0x010000, CRC(6280594e) SHA1(d2b666aaac8ebe94bfab1c4404d0e42bd6c8b176) )
	ROM_LOAD( "rgt10c.p1", 0x0000, 0x010000, CRC(91f28aa6) SHA1(42c21d11df3145a0919f7aef53a5621b2beca353) )
	ROM_LOAD( "rgt10d.p1", 0x0000, 0x010000, CRC(abd4a67e) SHA1(183746cd2cd661587854a80ad5455074fcf143cc) )
	ROM_LOAD( "rgt10dk.p1", 0x0000, 0x010000, CRC(63ee9525) SHA1(e1fa5348672d05149e6ab26f31af047e38192f2c) )
	ROM_LOAD( "rgt10dr.p1", 0x0000, 0x010000, CRC(481ffebc) SHA1(3608faa929b703d2e45ea37b4f7051d5bb37f073) )
	ROM_LOAD( "rgt10dy.p1", 0x0000, 0x010000, CRC(fe2498e9) SHA1(bd81c775daf860c2484af88a8b11b75df00ccaaa) )
	ROM_LOAD( "rgt10k.p1", 0x0000, 0x010000, CRC(78d6eff1) SHA1(d3172ffc9ef3a4f60680081d993e1487e4229625) )
	ROM_LOAD( "rgt10r.p1", 0x0000, 0x010000, CRC(1992945e) SHA1(716b62ca8edc5523fd83355e650982b50b4f9458) )
	ROM_LOAD( "rgt10s.p1", 0x0000, 0x010000, CRC(dd289204) SHA1(431f73cb45d248c672c50dc8fbc579209e41207d) )
	ROM_LOAD( "rgt10y.p1", 0x0000, 0x010000, CRC(8dab3aca) SHA1(0fe8f87a17acd8df0b7b75b852b58eb1e273eb27) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	//ROM_LOAD( "rgosnd1.hex", 0x0000, 0x080000, CRC(d9345794) SHA1(4ed060fe61b3530e88ba9afea1fb69efed47c955) )
	//ROM_LOAD( "rgosnd2.hex", 0x0000, 0x080000, CRC(4656f94e) SHA1(2f276ced34a43bb7fc69304f519b913d699c3450) )
	ROM_LOAD( "rgosnd.p1", 0x000000, 0x080000, CRC(d9345794) SHA1(4ed060fe61b3530e88ba9afea1fb69efed47c955) )
	ROM_LOAD( "rgosnd.p2", 0x080000, 0x080000, CRC(4656f94e) SHA1(2f276ced34a43bb7fc69304f519b913d699c3450) )
ROM_END


ROM_START( m4ra )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "r2tx.p1", 0x0000, 0x010000, CRC(7efffe3d) SHA1(5472bc76f4450726fc49fce281a6ec69693d0923) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "r2txr.p1", 0x0000, 0x010000, CRC(9ff95e34) SHA1(79d19602b88e1c9d23e910332a968e6b820a39f5) )
	ROM_LOAD( "ra2s.p1", 0x0000, 0x010000, CRC(cd0fd068) SHA1(a347372f7f737ca87f44e692015338831465f123) )
	ROM_LOAD( "ra2x.p1", 0x0000, 0x010000, CRC(8217e235) SHA1(e17483afea2a9d9e70e88687f899e1b98b982b63) )
	ROM_LOAD( "ra2xa.p1", 0x0000, 0x010000, CRC(0e6b2123) SHA1(af7c5ddddbfffef6fa5746a7b7927845457d02f8) )
	ROM_LOAD( "ra2xb.p1", 0x0000, 0x010000, CRC(97fe4933) SHA1(201860b64577828547adb8a216a6a205c4a4f34b) )
	ROM_LOAD( "ra2xr.p1", 0x0000, 0x010000, CRC(12e8eb9b) SHA1(2bcd2c911626a2cb2419f9540649e99d7f335b3b) )
	ROM_LOAD( "ra3xad.p1", 0x0000, 0x010000, CRC(75957d43) SHA1(f7d00842b8390f5464733a6fe1d61d7431a16817) )
	ROM_LOAD( "ra3xb.p1", 0x0000, 0x010000, CRC(f37e9bd5) SHA1(584a1f6f1bfb35de813466448e35fc1251fa90bc) )
	ROM_LOAD( "ra3xbd.p1", 0x0000, 0x010000, CRC(43891009) SHA1(5d9ebe9d48a39f0a121ae7b832b277910bfd0ad6) )
	ROM_LOAD( "ra3xd.p1", 0x0000, 0x010000, CRC(bc59a07a) SHA1(3a8fc99690759ea376660feaf65bfda5386dcf0d) )
	ROM_LOAD( "ra3xdr.p1", 0x0000, 0x010000, CRC(036950ba) SHA1(f0a534352b41c2762330762c3c7024d9a6d49cd4) )
	ROM_LOAD( "ra3xdy.p1", 0x0000, 0x010000, CRC(468508d4) SHA1(ba6db1e1f7bca13b9c40173fb68418f319e2a9d8) )
	ROM_LOAD( "ra3xr.p1", 0x0000, 0x010000, CRC(1a2b813d) SHA1(5d3b5d4ab31dd1848b3d0b2a5ff5798cc01e0c6f) )
	ROM_LOAD( "ra3xs.p1", 0x0000, 0x010000, CRC(a1ba9673) SHA1(7d5441522e8676805f7e75a3d445acae83d8a03b) )
	ROM_LOAD( "ra3xy.p1", 0x0000, 0x010000, CRC(3e2287de) SHA1(ba0861a0bfb6eb76f9786c0a4c098db362117618) )
	ROM_LOAD( "rahx.p1", 0x0000, 0x010000, CRC(6887014e) SHA1(25e4c008588a219895c1b326314fd11e1f0ad35f) )
	ROM_LOAD( "reda_20_.8", 0x0000, 0x010000, CRC(915aff5b) SHA1(e8e58c263e2bdb64a80e9355ac5e114fff1d59f8) )
	ROM_LOAD( "redx_20_.8", 0x0000, 0x010000, CRC(b5e8dec5) SHA1(74777ed7f78ef7cc615beadf097380569832a75a) )
ROM_END


ROM_START( m4rdht )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "drh12", 0x0000, 0x010000, CRC(b26cd308) SHA1(4e29f6cce773232a1c43cd2fb3ce9b844c446bb8) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "drh_1.snd", 0x0000, 0x080000, CRC(f652cd0c) SHA1(9ce986bc12bcf22a57e065329e82671d19cc96d7) )
ROM_END


ROM_START( m4rhr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rhr15.hex", 0x0000, 0x010000, CRC(895ebbda) SHA1(f2117e743a30f3c9fc6af7fd7843bc333699db9d) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cr4ad.p1", 0x0000, 0x010000, CRC(b99b3d14) SHA1(2ff68b33881e9b3c2db48c335ccbad783013084a) )
	ROM_LOAD( "cr4b.p1", 0x0000, 0x010000, CRC(ae2691b8) SHA1(360c5c3d94bf85cf5ead114dd570ea6c61082aa9) )
	ROM_LOAD( "cr4bd.p1", 0x0000, 0x010000, CRC(9ba444bf) SHA1(adebf23827a5ac5e3a6d56e3352e0d3f3dc809c0) )
	ROM_LOAD( "cr4d.p1", 0x0000, 0x010000, CRC(ad9fe2a6) SHA1(e490c5c949559cc222d8491989196b10373ff043) )
	ROM_LOAD( "cr4dk.p1", 0x0000, 0x010000, CRC(200486b4) SHA1(3916e131801c44985668ccd57dc3e812268f9417) )
	ROM_LOAD( "cr4dy.p1", 0x0000, 0x010000, CRC(5b5ebe79) SHA1(6c72271258e6b951f2d6c815cfef5032e23cf7bc) )
	ROM_LOAD( "cr4k.p1", 0x0000, 0x010000, CRC(2cc956e8) SHA1(37fad3d3b9460763ba4d8f569ee71778f9907853) )
	ROM_LOAD( "cr4s.p1", 0x0000, 0x010000, CRC(836c3e49) SHA1(34dde2fd4fe82ab4a9e16dcf7915705f7b8a007f) )
	ROM_LOAD( "cr4y.p1", 0x0000, 0x010000, CRC(5a3588e8) SHA1(b25156f38fb67dc1f1e36a50af0a9b93882572d0) )
	ROM_LOAD( "crt03ad.p1", 0x0000, 0x010000, CRC(5b779273) SHA1(b9a278cc6b4af622af35f7d4fdacdca54c94a47f) )
	ROM_LOAD( "crt03b.p1", 0x0000, 0x010000, CRC(da5b3fa3) SHA1(66c570a193665ae0df4542112547fa6f5f9b7b79) )
	ROM_LOAD( "crt03bd.p1", 0x0000, 0x010000, CRC(6d6bff39) SHA1(08f4235bb2cadcc49c13991fe3e2c806c0be801d) )
	ROM_LOAD( "crt03c.p1", 0x0000, 0x010000, CRC(a5b38945) SHA1(31351667d471c107ade58e97fe5657632d91be80) )
	ROM_LOAD( "crt03d.p1", 0x0000, 0x010000, CRC(7f39cf9d) SHA1(6f8a1660a253cf7f49ba589b3847ca3dc5a9b4ee) )
	ROM_LOAD( "crt03dk.p1", 0x0000, 0x010000, CRC(32933785) SHA1(0ae9b8823ed8c914da0a64913afdf3c348142804) )
	ROM_LOAD( "crt03dr.p1", 0x0000, 0x010000, CRC(2381792a) SHA1(514b9e580d156ec3cfeb460d0895143368e9a360) )
	ROM_LOAD( "crt03dy.p1", 0x0000, 0x010000, CRC(3439dc85) SHA1(092dcd36e2ea43ecf62cfc1bf1498ea7777213dc) )
	ROM_LOAD( "crt03k.p1", 0x0000, 0x010000, CRC(0b841ae9) SHA1(5a78381122a3b718e3f212f30f76dc61e2e3ac5e) )
	ROM_LOAD( "crt03r.p1", 0x0000, 0x010000, CRC(2a8bd767) SHA1(a9547ef37da9494bd4ffe5fbb68eca67fe63c3ba) )
	ROM_LOAD( "crt03s.p1", 0x0000, 0x010000, CRC(2b4c24d2) SHA1(94b19b0e8090dbbde2c67d5949f19d4050972fb1) )
	ROM_LOAD( "crt03y.p1", 0x0000, 0x010000, CRC(40c3a105) SHA1(7ad988f71a3523ad2b19fa7d6cdf74d4328fb3e1) )
	ROM_LOAD( "cruad.p1", 0x0000, 0x010000, CRC(3a680f14) SHA1(cd3c2bf77b148ee4f4ce76b2c1bc142491117890) )
	ROM_LOAD( "crub.p1", 0x0000, 0x010000, CRC(4cee9020) SHA1(b919ba28294c39b49e4fcfa54a75e852f9c873ed) )
	ROM_LOAD( "crubd.p1", 0x0000, 0x010000, CRC(7184b193) SHA1(392cb5887ec988e3aa1cba2491885103da1e503a) )
	ROM_LOAD( "crud.p1", 0x0000, 0x010000, CRC(2528047f) SHA1(0b07470ff756b003c03fd4a7ff3c1d5f79e8307f) )
	ROM_LOAD( "crudk.p1", 0x0000, 0x010000, CRC(73465d95) SHA1(3eddaee64a681727743b23fd0bec0285ed59a5ef) )
	ROM_LOAD( "crudy.p1", 0x0000, 0x010000, CRC(e08696f9) SHA1(37c97bb22ae0d09657d7d589f76adfbe6fb642e0) )
	ROM_LOAD( "cruk.p1", 0x0000, 0x010000, CRC(168627f0) SHA1(c6c21f8442ff88736d3fd25860d815beb5a6b845) )
	ROM_LOAD( "crus.p1", 0x0000, 0x010000, CRC(bf2ff034) SHA1(7ee7ef30da4283dbb2b1b040fdd3313cb2e1b7e5) )
	ROM_LOAD( "cruy.p1", 0x0000, 0x010000, CRC(edf1346b) SHA1(c250178991885a922f676424e70c637e11089efb) )
	ROM_LOAD( "redhot8.bin", 0x0000, 0x010000, CRC(1dc62d7b) SHA1(640a5b29314a7dc67db271cce06c23c676d77eee) )
	ROM_LOAD( "rhr03.r", 0x0000, 0x010000, CRC(98d81b1e) SHA1(17ab0dced53be9755aada7954aff2dc2a6973190) )
	ROM_LOAD( "rhr10", 0x0000, 0x010000, CRC(2a18a033) SHA1(add907c5ab155c28142dcee57825059715afd80d) )
	ROM_LOAD( "rhr2015", 0x0000, 0x010000, CRC(dbfd3b95) SHA1(4fc7ae32f7d76be3d3d07d627391884bd4d6de09) )
	ROM_LOAD( "rhr2515", 0x0000, 0x010000, CRC(e4554c23) SHA1(6d977beb282fd638de3457e467e842ce79b5be7c) )
	ROM_LOAD( "rhr2pprg.bin", 0x0000, 0x010000, CRC(f97047b2) SHA1(d3ed8c93e405f9e7448b3924ff9aa84223b76046) )
	ROM_LOAD( "rhrb.p1", 0x0000, 0x010000, CRC(876fbe46) SHA1(1c7faf68ddef2ccbb8e3cd2cf5c709a7a4f4daef) )
	ROM_LOAD( "rhrbd.p1", 0x0000, 0x010000, CRC(f0fa0c7b) SHA1(96bfce8ea54e392a36cb8d82a032438bff992f07) )
	ROM_LOAD( "rhrc.p1", 0x0000, 0x010000, CRC(76a0e556) SHA1(1a9bae286ca40d8e72022645d006a219f113e31a) )
	ROM_LOAD( "rhrd.p1", 0x0000, 0x010000, CRC(58a5dd6f) SHA1(3646b8cb3d49e8c530e321daad052f27cdf4bb3d) )
	ROM_LOAD( "rhrk.p1", 0x0000, 0x010000, CRC(2212cebb) SHA1(224e7e243b17f3ca90a6daa529984e9a879ff266) )
	ROM_LOAD( "rhrs.p1", 0x0000, 0x010000, CRC(a0e5d5b6) SHA1(c730e6319bbea6f035fb3e249991983783ef5743) )
	ROM_LOAD( "rhtad.p1", 0x0000, 0x010000, CRC(ae3a31a0) SHA1(7e1f05a21cf5b3d2aceba755136c567b5d6ecfcd) )
	ROM_LOAD( "rhtb.p1", 0x0000, 0x010000, CRC(7ceb13c8) SHA1(f0f22149bd0fb12ef06c4c3ecba605df33f52c51) )
	ROM_LOAD( "rhtbd.p1", 0x0000, 0x010000, CRC(e4b290fc) SHA1(bf16d06429d67936118264f6c4f1ae637753d5db) )
	ROM_LOAD( "rhtd.p1", 0x0000, 0x010000, CRC(a08d508c) SHA1(10efbfb4fc4820313b410ec73f9c32ed048e2228) )
	ROM_LOAD( "rhtdk.p1", 0x0000, 0x010000, CRC(6495681a) SHA1(afd3451402e19c4c4bb8507447d6771323219e80) )
	ROM_LOAD( "rhtdr.p1", 0x0000, 0x010000, CRC(df9e5c83) SHA1(88586852c0773de4ee1b4c627eabf3de27e5c2a1) )
	ROM_LOAD( "rhtdy.p1", 0x0000, 0x010000, CRC(42f5746d) SHA1(964bd8801b44de9ea45c43b290b1cd6284e97578) )
	ROM_LOAD( "rhtk.p1", 0x0000, 0x010000, CRC(c3bfb174) SHA1(2579bf17252988de17a1367546ae187420f95cc5) )
	ROM_LOAD( "rhtr.p1", 0x0000, 0x010000, CRC(f53f4876) SHA1(feda495361d384c662554d445a95191a2c52a56a) )
	ROM_LOAD( "rhts.p1", 0x0000, 0x010000, CRC(fecb7076) SHA1(43086c6bfd878d0ca1ec8d45285d3e941a62ac8e) )
	ROM_LOAD( "rhty.p1", 0x0000, 0x010000, CRC(68546098) SHA1(57981c06efcb44915d8c2d4b6e1cba377c4a8590) )
	ROM_LOAD( "rhuad.p1", 0x0000, 0x010000, CRC(2093126b) SHA1(942994793697cec730c461c87b24a1429e46cc02) )
	ROM_LOAD( "rhub.p1", 0x0000, 0x010000, CRC(2be41a3a) SHA1(a50c7b5b93a619e541be480646517e278da8e579) )
	ROM_LOAD( "rhubd.p1", 0x0000, 0x010000, CRC(168f7f21) SHA1(9c9e09673bdadd146883a06a8db3c0ee4b304eab) )
	ROM_LOAD( "rhud.p1", 0x0000, 0x010000, CRC(71932d29) SHA1(e92af5cced251eea2e31c4c1968e77087c64b824) )
	ROM_LOAD( "rhudk.p1", 0x0000, 0x010000, CRC(8de54a5d) SHA1(a275d8c67d38c09f19ffa41e97fbcbea3d297aa4) )
	ROM_LOAD( "rhudr.p1", 0x0000, 0x010000, CRC(ba01ac84) SHA1(d03b3b321abd220f619724e99cc396c38418f2d3) )
	ROM_LOAD( "rhudy.p1", 0x0000, 0x010000, CRC(692bf4eb) SHA1(136f36073f236b48442a20e06aa51a978135f1b3) )
	ROM_LOAD( "rhuk.p1", 0x0000, 0x010000, CRC(9e4e1e91) SHA1(f671858c41dc0e55189e9a86fff1846938b5c2e5) )
	ROM_LOAD( "rhur.p1", 0x0000, 0x010000, CRC(6e9425e5) SHA1(1e2827f3469af15e8d390d9af839c7b474ea95a7) )
	ROM_LOAD( "rhus.p1", 0x0000, 0x010000, CRC(31e776fc) SHA1(e51799e9db5a08cbfb0b6c5466a0a085c3d91db4) )
	ROM_LOAD( "rhuy.p1", 0x0000, 0x010000, CRC(5d12178a) SHA1(18525828fac1931bb8e11f96b79db143ed533771) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cr__x__x.5_0", 0x0000, 0x010000, CRC(278fe91e) SHA1(dcfed3a7796d1ee365e535115b66c7d6cbe0ab74) )
	ROM_LOAD( "cr__x_dx.2_0", 0x0000, 0x010000, CRC(73fb120c) SHA1(4c0f39253dee9b528763a9cb609dec31e8529713) )
	ROM_LOAD( "cr4s.p1", 0x0000, 0x010000, CRC(836c3e49) SHA1(34dde2fd4fe82ab4a9e16dcf7915705f7b8a007f) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "redhotroll10.bin", 0x0000, 0x080000, CRC(64513503) SHA1(4233492f3f6e7ad8459f1ab733727910d3b4bcf8) )
	ROM_LOAD( "rhrsnd1.hex", 0x0000, 0x080000, CRC(3e80f8bd) SHA1(2e3a195b49448da11cc0c089a8a9b462894c766b) )

ROM_END


ROM_START( m4rhrc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cld03ad.p1", 0x0000, 0x010000, CRC(821fde63) SHA1(61f77eeb01331e735cc8c736526d09371e6bdf56) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cld03b.p1", 0x0000, 0x010000, CRC(c67a2e82) SHA1(b76110c73d5bd0290fdd31d8300914f63a56c25e) )
	ROM_LOAD( "cld03bd.p1", 0x0000, 0x010000, CRC(0995fd93) SHA1(c3cc84f78adc54f4698280bf7d0831bb54c3fc3f) )
	ROM_LOAD( "cld03c.p1", 0x0000, 0x010000, CRC(6e7b319f) SHA1(3da4feb72cb9d4ee24a8e0568f8d9c80a71caf9b) )
	ROM_LOAD( "cld03d.p1", 0x0000, 0x010000, CRC(dc46afb0) SHA1(c461ac2ef3fcffac96536b1b1c26abe052edf35c) )
	ROM_LOAD( "cld03dk.p1", 0x0000, 0x010000, CRC(f0b6b60f) SHA1(9addae6af20986c92c3ce71ce9756a6f3db5ebff) )
	ROM_LOAD( "cld03dr.p1", 0x0000, 0x010000, CRC(703ab87b) SHA1(089597927f94bdacc4226900a944cbec85fe2286) )
	ROM_LOAD( "cld03dy.p1", 0x0000, 0x010000, CRC(ed519095) SHA1(ac174166bf2cc6ab81f9782f1be4a9fbe226f34d) )
	ROM_LOAD( "cld03k.p1", 0x0000, 0x010000, CRC(3bad05a9) SHA1(1b00ac52f6c87b5c79088b6fc3e6d00f57876ebc) )
	ROM_LOAD( "cld03r.p1", 0x0000, 0x010000, CRC(2de70bdc) SHA1(d8d0170ca71fde4c79d0b465d09d4bb31acf40cf) )
	ROM_LOAD( "cld03s.p1", 0x0000, 0x010000, CRC(03f8a6bf) SHA1(29ee59fd60d89fca0f236be8b4c12c885db032e7) )
	ROM_LOAD( "cld03y.p1", 0x0000, 0x010000, CRC(b08c2332) SHA1(1cdf7fc0e95a50766df2d1cd51cb803b922c30c8) )
	ROM_LOAD( "hhn03ad.p1", 0x0000, 0x010000, CRC(e7da568e) SHA1(00f9eecd06131bc5770a6ab650b3548f5b7a8c15) )
	ROM_LOAD( "hhn03b.p1", 0x0000, 0x010000, CRC(406e47cd) SHA1(193aed33ac62eb04d89cf63beb33e8e4e28e286e) )
	ROM_LOAD( "hhn03bd.p1", 0x0000, 0x010000, CRC(66aed369) SHA1(6c3151790292a277a1d44a1fceae985e52014749) )
	ROM_LOAD( "hhn03c.p1", 0x0000, 0x010000, CRC(452e623c) SHA1(9350d7e30d8fc2b0f37528a7d0ce6797bab6f504) )
	ROM_LOAD( "hhn03d.p1", 0x0000, 0x010000, CRC(e9ce4ee5) SHA1(45fe3832cc37e8ecbc5101b8b7b94f6243504e3f) )
	ROM_LOAD( "hhn03dk.p1", 0x0000, 0x010000, CRC(2d750f34) SHA1(1672d5a8b4a338cac87281e1329f111f468dc611) )
	ROM_LOAD( "hhn03dr.p1", 0x0000, 0x010000, CRC(88a3895b) SHA1(3e2dcf6728712620724774c16a5d84dbec9c5ab3) )
	ROM_LOAD( "hhn03dy.p1", 0x0000, 0x010000, CRC(15c8a1b5) SHA1(5a2f28f290fa087b5010f778d4ad8d6c63a3d13e) )
	ROM_LOAD( "hhn03k.p1", 0x0000, 0x010000, CRC(95450230) SHA1(3c1c239e84a89ef6acd44ac9c81d33021ac6b0e3) )
	ROM_LOAD( "hhn03r.p1", 0x0000, 0x010000, CRC(d96d6825) SHA1(89c3f5494d97326369f10c982842310592456874) )
	ROM_LOAD( "hhn03s.p1", 0x0000, 0x010000, CRC(b531ae78) SHA1(87d043541c23b88b8ec4067c67be77812095faaa) )
	ROM_LOAD( "hhn03y.p1", 0x0000, 0x010000, CRC(440640cb) SHA1(de6b6edcdc99aaa0122ecd24a9a7437e6b44aad2) )
	ROM_LOAD( "rrd03ad.p1", 0x0000, 0x010000, CRC(6f49d7d1) SHA1(2195a3ad4836e8ffd2e7e6a90e94319d5a5a0ce8) )
	ROM_LOAD( "rrd03b.p1", 0x0000, 0x010000, CRC(e8447a3d) SHA1(8bf5936782e0fbec25a8ef892b8df04b6543bc74) )
	ROM_LOAD( "rrd03bd.p1", 0x0000, 0x010000, CRC(52cf0357) SHA1(ab4668df6d5ad9614410aede7ad4e030283b78ca) )
	ROM_LOAD( "rrd03c.p1", 0x0000, 0x010000, CRC(b03e7b76) SHA1(0b2779b584f8fa0e25e2a5248ecb8fb88aa53413) )
	ROM_LOAD( "rrd03d.p1", 0x0000, 0x010000, CRC(44740c79) SHA1(ab1efb2090ef62795c17a685c7acb45820eb1a9d) )
	ROM_LOAD( "rrd03dk.p1", 0x0000, 0x010000, CRC(78f18187) SHA1(33764416c6e5cccd6ae5fdc5c0d679e1ef451785) )
	ROM_LOAD( "rrd03dr.p1", 0x0000, 0x010000, CRC(039c2869) SHA1(2eb887b36d86295d0e6aacc74d0a6223d32baa5a) )
	ROM_LOAD( "rrd03dy.p1", 0x0000, 0x010000, CRC(b60b6e51) SHA1(eb6ed1de44d7c982ac8aa0621d4c1ed8e41db5de) )
	ROM_LOAD( "rrd03k.p1", 0x0000, 0x010000, CRC(31adc6d6) SHA1(ea68d0d13978bf6cfa7fb9aa1cf91ddfd6258a3a) )
	ROM_LOAD( "rrd03r.p1", 0x0000, 0x010000, CRC(11c61483) SHA1(66cd30096bca2f4356acaaa15179c00301c8bc3a) )
	ROM_LOAD( "rrd03s.p1", 0x0000, 0x010000, CRC(e59b79dd) SHA1(32e515bdc861a4d548caedd56a1825c91a318a34) )
	ROM_LOAD( "rrd03y.p1", 0x0000, 0x010000, CRC(66fff07a) SHA1(586279533d6d85abf7e97124c9c5342a6a1b0496) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cr__x__x.5_0", 0x0000, 0x010000, CRC(278fe91e) SHA1(dcfed3a7796d1ee365e535115b66c7d6cbe0ab74) )
	ROM_LOAD( "cr__x_dx.5_0", 0x0000, 0x010000, CRC(4bcf5c02) SHA1(603935880c87f86e7bc765c176266c1c08a6114f) )


	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )

ROM_END


ROM_START( m4rhrcl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rhrc.hex", 0x0000, 0x010000, CRC(e4b89d53) SHA1(fc222d56cdba2891048726d6e6ecd8a4028ba8ba) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "rh2d.p1", 0x0000, 0x010000, CRC(b55a01c3) SHA1(8c94c2ca509ac7631528df78e82fb39b5f579c45) )
	ROM_LOAD( "rh2f.p1", 0x0000, 0x010000, CRC(83466c89) SHA1(790d626e361bfec1265edc6f6ce51f098eb774ba) )
	ROM_LOAD( "rh2s.p1", 0x0000, 0x010000, CRC(aa15e8a8) SHA1(243e7562a4cf938527afebbd99581acea1ab4134) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "m462.chr", 0x0000, 0x000048, CRC(ab59f1aa) SHA1(04a7deac039bc9bc15ec07b8a4ba3bce6f4d5103) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "rhrcs1.hex", 0x000000, 0x080000, CRC(7e265003) SHA1(3800ddfbdde07bf0af5db5cbe05a85425297fa4a) )
	ROM_LOAD( "rhrcs2.hex", 0x080000, 0x080000, CRC(39843d40) SHA1(7c8efcce4ed4ed53e681680bb33869f14f662609) )
ROM_END


ROM_START( m4rwb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "drw14.bin", 0x0000, 0x010000, CRC(22c30ebe) SHA1(479f66732aac56dae60c80d11f05c084865f9389) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "rwb_1.snd", 0x000000, 0x080000, CRC(e0a6def5) SHA1(e3867b83e588fd6a9039b8d45186480a9d0433ea) )
	ROM_LOAD( "rwb_2.snd", 0x080000, 0x080000, CRC(54a2b2fd) SHA1(25875ff873bf22df510e7a4c56c336fbabcbdedb) )
ROM_END


ROM_START( m4r2r )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "r2r_p1.bin", 0x8000, 0x008000, CRC(50be12e1) SHA1(65967b54ba2cc29bc5bebc8acf374245b7c5fbed) )
	ROM_LOAD( "r2r_p2.bin", 0x0000, 0x008000, CRC(ac6dd496) SHA1(6cf8c77e28a5dcdf6e05d2794eadb90703fb994b) )
ROM_END


ROM_START( m4rmtp )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "crmtb14.epr", 0x0000, 0x010000, CRC(79e1746c) SHA1(794317f3aba7b1a7994cde89d81abc2b687d0821) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "r4iha202.bin", 0x0000, 0x010000, CRC(b1588632) SHA1(ad21bbc5e99fd6b511e6881e8b20dcad177b937f) )
	ROM_LOAD( "r4iha203.bin", 0x0000, 0x010000, CRC(7f31cb76) SHA1(9a2a595afb9ff1b3165638d247ab98475ae0bfcd) )
	ROM_LOAD( "r4iha204.bin", 0x0000, 0x010000, CRC(1cc3a32d) SHA1(b6ed012a6d743ba2416e25e7c49ce9985bbacbd7) )
	ROM_LOAD( "r4iha205.bin", 0x0000, 0x010000, CRC(1a238632) SHA1(a15ca5801d41985387bc65579b6d6ee2ef7d8eee) )
	ROM_LOAD( "r4iua202.bin", 0x0000, 0x010000, CRC(c96d630a) SHA1(90ed759602aa3a052434b3f604ec26ec9e204e68) )
	ROM_LOAD( "r4iua203.bin", 0x0000, 0x010000, CRC(550fdfec) SHA1(d57eaba6690cbff2302559e9cea9e5d0f79cf9f9) )
	ROM_LOAD( "r4iua204.bin", 0x0000, 0x010000, CRC(cd8d166f) SHA1(4d78726df35914444be26ac9e1e3e1949b6a3d99) )
	ROM_LOAD( "r4iua205.bin", 0x0000, 0x010000, CRC(46df24f3) SHA1(31000815a90e47e744091bbf0fe9e96baac8d7e3) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "ctp.chr", 0x0000, 0x000048, CRC(ead61793) SHA1(f38a38601a67804111b8f8cf0a05d35ed79b7ed1) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "rm.s3", 0x0000, 0x080000, CRC(250e64f2) SHA1(627c4dc5cdc7d0a7cb6f74991ae91b71a2f4dbc6) )
	ROM_LOAD( "scrmtb.snd", 0x0000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END


ROM_START( m4rmtpd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "r2iha203.bin", 0x0000, 0x010000, CRC(1cea7710) SHA1(a250569800d3679f317a485ac7a31b4f4fa7db78) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "r2iha204.bin", 0x0000, 0x010000, CRC(c82cd025) SHA1(f26f2bbd83d673c61bd2609914349b45c31f4a5d) )
	ROM_LOAD( "r2iha205.bin", 0x0000, 0x010000, CRC(e53da9a5) SHA1(5019f5bd89c230459629670b808c59888a0f1ee9) )
	ROM_LOAD( "r2iha206.bin", 0x0000, 0x010000, CRC(f89b73b3) SHA1(34a9a8053e881b8aad578ef58209c8ff888b30f7) )
	ROM_LOAD( "r2iua203.bin", 0x0000, 0x010000, CRC(9590a747) SHA1(9f1a1277bdcbe0f23abcf38850eae939997c2e00) )
	ROM_LOAD( "r2iua205.bin", 0x0000, 0x010000, CRC(2eefca1a) SHA1(cabc0c8a3dddc881aab899c5419663efff5412d3) )
	ROM_LOAD( "r2iua206.bin", 0x0000, 0x010000, CRC(c4a1a218) SHA1(8208468ae9ddde7d387f7194e1f7d44f6e7ca730) )
	ROM_LOAD( "r3iha224.bin", 0x0000, 0x010000, CRC(a2e161ac) SHA1(bd63c9726cdf037919c8655221bc6416cef322aa) )
	ROM_LOAD( "r3iha225.bin", 0x0000, 0x010000, CRC(f49a41e9) SHA1(6c29ba4bf76aaafa79ce68f58f6672baa47fe147) )
	ROM_LOAD( "r3iua224.bin", 0x0000, 0x010000, CRC(715b7de7) SHA1(013827680c389968f2f80f97c565716757d696b2) )
	ROM_LOAD( "r3iua225.bin", 0x0000, 0x010000, CRC(37086f91) SHA1(413b32a8e354467a30c71dce3d1cb76795ff813d) )
	ROM_LOAD( "r4iha201.bin", 0x0000, 0x010000, CRC(789cfca1) SHA1(31aa7bf9461cb6c4f692d605463fde1f604b1614) )
	ROM_LOAD( "r4iua201.bin", 0x0000, 0x010000, CRC(ce0e2553) SHA1(4c9df36a7b8950a273cefceb6ba6817d8b862c78) )
	ROM_LOAD( "rdiha202.bin", 0x0000, 0x010000, CRC(02e01481) SHA1(253c2c8e800a4e6d1008745101e2457d76ac57d4) )
	ROM_LOAD( "rdiha212.bin", 0x0000, 0x010000, CRC(90984ae9) SHA1(b25a12f0529af64315c461363c788c22e30d4016) )
	ROM_LOAD( "rdiha213.bin", 0x0000, 0x010000, CRC(e9fa4c97) SHA1(07c75418890231102cf336f2d3f0048fe4884862) )
	ROM_LOAD( "rdiha214.bin", 0x0000, 0x010000, CRC(42f3a5e0) SHA1(3cf26e55edf0dcde9510e50c4b781ba8b906f092) )
	ROM_LOAD( "rdiha215.bin", 0x0000, 0x010000, CRC(2b704591) SHA1(9b880f40d3b268c96af5dab179760994c5a074c9) )
	ROM_LOAD( "rdiha217.bin", 0x0000, 0x010000, CRC(6df58d97) SHA1(df8f419a1e3acc68a3755c49e258db5af9102598) )
	ROM_LOAD( "rdiha219.bin", 0x0000, 0x010000, CRC(66f3ffa6) SHA1(1b1daf4b02e400d943f2a917be0f4452be891aaf) )
	ROM_LOAD( "rdiha220.bin", 0x0000, 0x010000, CRC(2047c55b) SHA1(8f0e6608271634a6a0f06e76df93dddd404c93cd) )
	ROM_LOAD( "rdiha221.bin", 0x0000, 0x010000, CRC(6e87f591) SHA1(750e9f01c1a3143d7d97a5b9b11d09aed72ca928) )
	ROM_LOAD( "rdiha222.bin", 0x0000, 0x010000, CRC(d200f6ec) SHA1(07e0e270a2184f24373cbe0a8a5e44c3d215d9a2) )
	ROM_LOAD( "rdiha223.bin", 0x0000, 0x010000, CRC(042a5a96) SHA1(3bc2dfb89c6781eb9fb105e5f8ea1576d7b49ad3) )
	ROM_LOAD( "rdihb202.bin", 0x0000, 0x010000, CRC(136c31ec) SHA1(abb095bd4ec0a0879f49e668f1ea08df026262e7) )
	ROM_LOAD( "rdiua202.bin", 0x0000, 0x010000, CRC(faa875ea) SHA1(d8d206fed8965a26dd8ded38a3be018311ccf407) )
	ROM_LOAD( "rdiua204.bin", 0x0000, 0x010000, CRC(a6110b45) SHA1(61d08250fa3b5d7eb7cdf63562d7a6cc9a27372c) )
	ROM_LOAD( "rdiua205.bin", 0x0000, 0x010000, CRC(9f20d810) SHA1(e2a576313fa49fc72001d5de67e93c08423e8dd8) )
	ROM_LOAD( "rdiua206.bin", 0x0000, 0x010000, CRC(2954e2c3) SHA1(e4c9f51748bc1296298f95ca817e852f9e0ca38b) )
	ROM_LOAD( "rdiua207.bin", 0x0000, 0x010000, CRC(58f334d1) SHA1(b91288731750445e4cfcf87fe6a9504723b59fa9) )
	ROM_LOAD( "rdiua208.bin", 0x0000, 0x010000, CRC(13e6d84d) SHA1(6f75a75dfd6922349f8d29c955c1849522f8656c) )
	ROM_LOAD( "rdiua209.bin", 0x0000, 0x010000, CRC(f41af938) SHA1(f2d4e23717f49961fe104971b3a0da9aabbf0e05) )
	ROM_LOAD( "rdiua212.bin", 0x0000, 0x010000, CRC(c56a6433) SHA1(7ff8943843c334a79fc3b40bb004abb3f2c2d079) )
	ROM_LOAD( "rdiua213.bin", 0x0000, 0x010000, CRC(cdd5f399) SHA1(a4359c5166fbcd4ea2bb6820bbfead6bc2b2a4ef) )
	ROM_LOAD( "rdiua214.bin", 0x0000, 0x010000, CRC(04fa9d21) SHA1(c337486ece94a7004420edca677e6688eef1ac9e) )
	ROM_LOAD( "rdiua215.bin", 0x0000, 0x010000, CRC(43d8ca5e) SHA1(e5e24ed24bd5c1135392c98910d2797e621ecbd5) )
	ROM_LOAD( "rdiua217.bin", 0x0000, 0x010000, CRC(3c58970e) SHA1(15b7368078750021202ee7b4886a6510fcc1ba0d) )
	ROM_LOAD( "rdiua219.bin", 0x0000, 0x010000, CRC(54f8fe63) SHA1(48d1b04dde6056b839ec84daa40a7d6871893b3e) )
	ROM_LOAD( "rdiua220.bin", 0x0000, 0x010000, CRC(768715f6) SHA1(5c4102b4d2400806dd0f5a6f3e48da4d290d5255) )
	ROM_LOAD( "rdiua222.bin", 0x0000, 0x010000, CRC(07413d93) SHA1(e752fe382d222eefd4fe975fa40559fedd579320) )
	ROM_LOAD( "rdiua223.bin", 0x0000, 0x010000, CRC(80185ebf) SHA1(ae885325f2c63f7dbb034f8e3f3882d0b36aff99) )
	ROM_LOAD( "rdiub202.bin", 0x0000, 0x010000, CRC(4a8d7cf7) SHA1(cb1525d89d3a411163bfe9e70c8b0d1aa6cefdf5) )
	ROM_LOAD( "rdmha210.bin", 0x0000, 0x010000, CRC(7061373e) SHA1(67da39d1de4f3877f12bd1fd5545046f9dabfde9) )
	ROM_LOAD( "rdmhb210.bin", 0x0000, 0x010000, CRC(12c71e8a) SHA1(9bb45e72f202d3af19988ebf30ea4c2248d387fc) )
	ROM_LOAD( "rdpka316.bin", 0x0000, 0x010000, CRC(5178175d) SHA1(a732a82226c34be0b7f84e9f9e4700bd72da1c19) )
	ROM_LOAD( "rdpka318.bin", 0x0000, 0x010000, CRC(2789179f) SHA1(8d4b1e75995ea5b64fac1a36a98506aacfd1800a) )
	ROM_LOAD( "rdpkb316.bin", 0x0000, 0x010000, CRC(09d4f4c5) SHA1(fbc2b0710ef048c221b007692e9a97b99f1edbc0) )
	ROM_LOAD( "rdpkb318.bin", 0x0000, 0x010000, CRC(428aa7f2) SHA1(f85d173c25d0ab9d8c3c4d87b4fc27c3342b3dec) )
	ROM_LOAD( "rduha511.bin", 0x0000, 0x010000, CRC(823e0323) SHA1(4137a05efe87851a9f9ffcd6519bb57398773095) )
	ROM_LOAD( "rduhb511.bin", 0x0000, 0x010000, CRC(2b65eb19) SHA1(b00543b74ad5262b85f66f5e8cfdaee351f62f23) )
	ROM_LOAD( "rmdxi", 0x0000, 0x000b57, CRC(c16021ec) SHA1(df77e410ea2edae1559e40a877e292f0d1969b0a) )
ROM_END


ROM_START( m4reelpk )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "reelpoker3_0.bin", 0x8000, 0x008000, CRC(6a2a0a48) SHA1(6291cb3512a3f03c0cea06b33bcc5846144cc12e) )
ROM_END


ROM_START( m4reeltm )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "real.bin", 0x0000, 0x010000, CRC(5bd54924) SHA1(23fcf13c52ee7b9b39f30f999a9102171fffd642) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "charter.chr", 0x0000, 0x000048, CRC(4ff4eda2) SHA1(092435e34d79775910316a7bed0f90c4f086e5c4) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )

ROM_END

ROM_START( m4richfm )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rfts.p1", 0x0000, 0x010000, CRC(2a747164) SHA1(a4c8e160f09ebea4fca6dd32ff020d3f1a4f1a1c) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "rafc.p1", 0x0000, 0x010000, CRC(d92f602f) SHA1(c93131138deb4018d499b9b45c07d4517c5072b7) )
	ROM_LOAD( "rafd.p1", 0x0000, 0x010000, CRC(b0e9f470) SHA1(cad080a5d7f24968524fe10f6c43b088f35d7364) )
	ROM_LOAD( "rafs.p1", 0x0000, 0x010000, CRC(f312b2e3) SHA1(8bf2cb7b73cfc320143d05d25e28c15fb4f26045) )
	ROM_LOAD( "rafy.p1", 0x0000, 0x010000, CRC(a8812d45) SHA1(c0b89833f87ed90eb3e9c3299fcea362d501ed90) )
	ROM_LOAD( "rchfam8", 0x0000, 0x004000, CRC(55f16698) SHA1(9853b17bbb81371192a564376be7b3074908dbca) )
	ROM_LOAD( "rf5ad.p1", 0x0000, 0x010000, CRC(cd280292) SHA1(605d89608e106979229a00701a2e5b578df50d60) )
	ROM_LOAD( "rf5b.p1", 0x0000, 0x010000, CRC(e1edf753) SHA1(677f0397ec57422241f4669be610cffd33a9b44a) )
	ROM_LOAD( "rf5bd.p1", 0x0000, 0x010000, CRC(2d698365) SHA1(7f91cee0d34550aba9ac0f4ee398df4de6fd6f7e) )
	ROM_LOAD( "rf5d.p1", 0x0000, 0x010000, CRC(034cab0b) SHA1(79eaeb84377dbb8e6bda1dd2ae29a1f79656b9e4) )
	ROM_LOAD( "rf5dk.p1", 0x0000, 0x010000, CRC(14fc0f13) SHA1(a2b294da18c3f5bc9c81eb3f3af5ab5ca58c9cad) )
	ROM_LOAD( "rf5dy.p1", 0x0000, 0x010000, CRC(a2664c64) SHA1(2256b6e0d6472faa901348cb5be849ad012f1d16) )
	ROM_LOAD( "rf5k.p1", 0x0000, 0x010000, CRC(d8787b25) SHA1(885ac7ddd3de4cb475539d02aefbf38fed7c1f2c) )
	ROM_LOAD( "rf5s.p1", 0x0000, 0x010000, CRC(8d1ed193) SHA1(a4ca973dac8a8fd550bf7e57a8cdc627c28da4b8) )
	ROM_LOAD( "rf5y.p1", 0x0000, 0x010000, CRC(ad288548) SHA1(a7222ab5bffe8e5e0844f8e6f13e09afe74b08a8) )
	ROM_LOAD( "rf8b.p1", 0x0000, 0x010000, CRC(105c24e1) SHA1(cb417976a74441bf2ca888198b57fed81d758c15) )
	ROM_LOAD( "rf8c.p1", 0x0000, 0x010000, CRC(8924a706) SHA1(abb1a1f6cdeb15884dfa63fc04882f794453d4ec) )
	ROM_LOAD( "rfamous.hex", 0x0000, 0x010000, CRC(8d1ed193) SHA1(a4ca973dac8a8fd550bf7e57a8cdc627c28da4b8) )
	ROM_LOAD( "rft20.10", 0x0000, 0x010000, CRC(41e6ef75) SHA1(d836fdea5a89b845687d2ff929365bd81737c760) )
	ROM_LOAD( "rftad.p1", 0x0000, 0x010000, CRC(8553386f) SHA1(ad834d52e51c7f375a370dc6d8586668921a9795) )
	ROM_LOAD( "rftb.p1", 0x0000, 0x010000, CRC(0189cc2f) SHA1(62ccc85c50c56aa2e0bcbb42b5c24d402f00d366) )
	ROM_LOAD( "rftbd.p1", 0x0000, 0x010000, CRC(08351e03) SHA1(d08d38d46793828b147ccde8121fbb9bf422cd60) )
	ROM_LOAD( "rftd.p1", 0x0000, 0x010000, CRC(689f02ed) SHA1(1a30aac5454b0c477a698e9c573fe313bc1fe858) )
	ROM_LOAD( "rftdk.p1", 0x0000, 0x010000, CRC(098b88f5) SHA1(4559b561380055c429a5b4741326f64ad89d8481) )
	ROM_LOAD( "rftdy.p1", 0x0000, 0x010000, CRC(26b912f8) SHA1(1719d63b4a25293199b0729235beb5b93c484490) )
	ROM_LOAD( "rftk.p1", 0x0000, 0x010000, CRC(6a48bd98) SHA1(2f17194869ca008f7a2eb622bd3725bc91950a17) )
	ROM_LOAD( "rfty.p1", 0x0000, 0x010000, CRC(723fe46e) SHA1(51bb8aff358d527483eaf1b1e20606d94a937dc6) )
	ROM_LOAD( "rich2010", 0x0000, 0x010000, CRC(baecbdbc) SHA1(5fffecf3c91e832d3cfc13dbf5e6b74fc3d6a146) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "r&f5.10", 0x0000, 0x010000, CRC(45d493d0) SHA1(9a549821a005fa65c2eb8b35c5f15659bd897519) )
	ROM_LOAD( "r&f5.4", 0x0000, 0x010000, CRC(0441d833) SHA1(361910fd64bc7291f6200fe354c468d16e7d6c80) )
	ROM_LOAD( "r&f5.8t", 0x0000, 0x010000, CRC(525e2520) SHA1(84b2ff86d6a54ebb3bcf0138930b2619a8733161) )
	ROM_LOAD( "r&f55", 0x0000, 0x010000, CRC(6095a72b) SHA1(af25f7c2fb5241064ea995d35fe4fd2f242e3750) )


	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "rfamouss1.hex", 0x000000, 0x080000, CRC(b237c8b8) SHA1(b2322d68fe57cca0ed49b01ae0d3a0e93a623eac) )
	ROM_LOAD( "rfamouss2.hex", 0x080000, 0x080000, CRC(12c295d5) SHA1(0758354cfb5242b4ce3f5f25c3458d91f4b4a1ec) )
ROM_END

ROM_START( m4ringfr )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "rof03s.p1", 0x0000, 0x020000, CRC(4b4703fe) SHA1(853ce1f5932e09af2b5f3b5314709f13aa35cf19) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4rhog )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rr6s.p1", 0x0000, 0x010000, CRC(f978ca0b) SHA1(11eeac41f4c77b38b33baefb16dab7de1268d161) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "road hog 5p 6.bin", 0x0000, 0x010000, CRC(b365d1f0) SHA1(af3b4f5162af6c033039a1e004bc803175a4e996) )
	ROM_LOAD( "rdhogvkn", 0x0000, 0x010000, CRC(3db03ada) SHA1(9b26f466c1dc1d03edacf64cbe507e084edf5f90) )
	ROM_LOAD( "rh5p8.bin", 0x0000, 0x010000, CRC(35d56379) SHA1(ab70ef8151823c3157cf4cc4f9b29875c6ac81cc) )
	ROM_LOAD( "rh8c.p1", 0x0000, 0x010000, CRC(e36d7ca0) SHA1(73970761c5c7004669b02ba9f3a299f36f2d00e9) )
	ROM_LOAD( "rhog05_11", 0x0000, 0x010000, CRC(8e4b14aa) SHA1(8b67b34597c0d30b0b3cf2566536c02f880a74bc) )
	ROM_LOAD( "rhog10_11", 0x0000, 0x010000, CRC(83575be7) SHA1(2cb549554028f2fdc32ecfa58b786de375b8fa35) )
	ROM_LOAD( "rhog10c", 0x0000, 0x010000, CRC(308c6d4f) SHA1(f7f8063fe8dd4ef204f225d0aa5202732ead5fa0) )
	ROM_LOAD( "rhog20_11", 0x0000, 0x010000, CRC(63c80ee0) SHA1(22a3f11007acedd833af9e73e3038fb3542781fe) )
	ROM_LOAD( "rhog20c", 0x0000, 0x010000, CRC(74ec16f7) SHA1(995d75b3a4e88d8a34dc395b185f728c18e00a2b) )
	ROM_LOAD( "rhog55", 0x0000, 0x010000, CRC(29395082) SHA1(538434b82e31f7e40770a9b882e54a16195ee998) )
	ROM_LOAD( "rhog58c", 0x0000, 0x010000, CRC(e02b6da6) SHA1(7d329adcac594c98685dc5404f2b9e8f717cc47f) )
	ROM_LOAD( "rhog5p", 0x0000, 0x010000, CRC(49b11beb) SHA1(89c2320de4b3f2ff6ba28501f88147b659f1ee20) )
	ROM_LOAD( "ro_05sb_.2_1", 0x0000, 0x010000, CRC(4a33cfcf) SHA1(ac5d4873df74b521018d5eeac96fd7003ee093e8) )
	ROM_LOAD( "roadhog5p4std.bin", 0x0000, 0x010000, CRC(0ff60341) SHA1(c12d5b160d9e47a6f1aa6f378c2a70186be6bdff) )
	ROM_LOAD( "rr6ad.p1", 0x0000, 0x010000, CRC(f328204d) SHA1(057f28e7eaaa372b901a76250fb7ebf4403348ad) )
	ROM_LOAD( "rr6b.p1", 0x0000, 0x010000, CRC(ccacd58e) SHA1(64b67e54e5568378a18ba99017078fcd4e6bc749) )
	ROM_LOAD( "rr6c.p1", 0x0000, 0x010000, CRC(b5783c69) SHA1(38c122455bed904c9fd683be1a8508a69cbad03f) )
	ROM_LOAD( "rr6d.p1", 0x0000, 0x010000, CRC(b61115ea) SHA1(92b97cc8b71eb31e8377a59344faaf0d800d1bdc) )
	ROM_LOAD( "rr6dy.p1", 0x0000, 0x010000, CRC(0e540e0d) SHA1(a783e73822e436669c8cc1504619990725306df1) )
	ROM_LOAD( "rr6k.p1", 0x0000, 0x010000, CRC(121d29bf) SHA1(8a6dcf345012b2c499acd32c6bb76eb81ada6fa9) )
	ROM_LOAD( "rr6y.p1", 0x0000, 0x010000, CRC(56344b28) SHA1(7f6c740d0991a646393a47e2e85322a7c92bdd62) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "rh056c", 0x0000, 0x010000, CRC(073845e2) SHA1(5e6f3ccdfc346f95e5e7e955144332e727da1d9e) )
	ROM_LOAD( "rhog_05_.4", 0x0000, 0x010000, CRC(a75a2bd4) SHA1(d21505d27792acf8fa20a7cdc830efbe8756fe81) )
	ROM_LOAD( "rhog_05_.8", 0x0000, 0x010000, CRC(5476f9b4) SHA1(fbd038e8710a79ea697d5acb482bed2f307cefbb) )
	ROM_LOAD( "rhog_10_.4", 0x0000, 0x010000, CRC(8efa581c) SHA1(03c25b674cfb02792edc9ef8a76b16af31d80aae) )
	ROM_LOAD( "rhog_10_.8", 0x0000, 0x010000, CRC(84d1f95d) SHA1(33f10e0e1e5abe6011b05f32f55c7dd6d3298945) )
	ROM_LOAD( "rhog_20_.4", 0x0000, 0x010000, CRC(15e28457) SHA1(2a758a727a6956e3029b2026cd189f6249677c6a) )
	ROM_LOAD( "rhog_20_.8", 0x0000, 0x010000, CRC(3a82e4bf) SHA1(6582951c2afe14502c37460381bf4c28ec02f3c9) )
	ROM_LOAD( "ro_05a__.2_1", 0x0000, 0x010000, CRC(67450ed1) SHA1(84cab7bb2411eb47c1336159bd1941862da59db3) )
	ROM_LOAD( "ro_05s__.2_1", 0x0000, 0x010000, CRC(dc18f70f) SHA1(da81b8279e4f58b1447f51beb446a6007eb39df9) )
	ROM_LOAD( "ro_05sd_.2_1", 0x0000, 0x010000, CRC(f230ae7e) SHA1(5525ed33d115b01722186587de20013265ac19b2) )
	ROM_LOAD( "ro_05sk_.2_1", 0x0000, 0x010000, CRC(3e1dfedd) SHA1(a750663c96060b858e194445bc1e677b49da85b8) )
	ROM_LOAD( "ro_10a__.1_1", 0x0000, 0x010000, CRC(1772bce6) SHA1(c5d0cec8e5bcfcef5003325169522f1da066354b) )
	ROM_LOAD( "ro_10s__.1_1", 0x0000, 0x010000, CRC(d140597a) SHA1(0ddf898b5db2a1cbfda84e8a63e0be3de7582cbd) )
	ROM_LOAD( "ro_10sb_.1_1", 0x0000, 0x010000, CRC(34febd6f) SHA1(e1d5e178771714f9633dd9782c1f9d373a9ca5e1) )
	ROM_LOAD( "ro_10sd_.1_1", 0x0000, 0x010000, CRC(3f9152f3) SHA1(97e0c0461b8d4994515ac9e20d001dc7e74042ec) )
	ROM_LOAD( "ro_10sk_.1_1", 0x0000, 0x010000, CRC(5d5118d1) SHA1(c4abc5ccdeb711b6ec2a2c82bb2f8da9d824fe4e) )
	ROM_LOAD( "roi05___.2_1", 0x0000, 0x010000, CRC(85fbd24a) SHA1(653a3cf3e651d94611caacddbd0692111667424a) )
	ROM_LOAD( "roi10___.1_1", 0x0000, 0x010000, CRC(2f832f4b) SHA1(b9228e2585cff6d4d9df64048c77e0b9ad3e75d7) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "rhm.chr", 0x0000, 0x000048, CRC(e8417c98) SHA1(460c43327b41c95b7d091c04dbc9ce7b2e4773f6) )
	ROM_LOAD( "rr6s.chr", 0x0000, 0x000048, CRC(ca08d53a) SHA1(b419c45f46ee352cbdb0b38a8c3fd33383b61f3a) )

	ROM_REGION( 0x100000, "msm6376", 0 )

	ROM_LOAD( "rr6snd.p1", 0x000000, 0x080000, CRC(a5ec3f46) SHA1(2d6f1adbbd8ac931a99a7d3d9caa2a7a117ac3fa) )
	ROM_LOAD( "rr6snd.p2", 0x080000, 0x080000, CRC(e5b72ef2) SHA1(dcdfa162db8bf3f9610709b5a8f3b695f42b2371) )

	ROM_REGION( 0x100000, "msm6376_alt", 0 ) // these don't work?
	ROM_LOAD( "rr6sndiss61.bin", 0x000000, 0x080000, CRC(5df6c5c9) SHA1(8c182007bbd0bb0c0071aba09bddbd9c381408ae) )
	ROM_LOAD( "rr6sndiss62.bin", 0x080000, 0x080000, CRC(575f1123) SHA1(99c40606049a307904d190df5e8bcb78a99333a6) )
ROM_END


ROM_START( m4rhog2 )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "2rh06c.p1", 0x0000, 0x020000, CRC(62c312bc) SHA1(6b02345c97b130deabad58a238ba9045161b5a80) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "2rh06ad.p1", 0x0000, 0x020000, CRC(f44040d1) SHA1(685bbfe5f975c7e5b3efee17e1833f6f51b223af) )
	ROM_LOAD( "2rh06b.p1", 0x0000, 0x020000, CRC(5589afae) SHA1(15c9c65089cc2754d644dabfd6f5a32a2a788219) )
	ROM_LOAD( "2rh06bd.p1", 0x0000, 0x020000, CRC(795aee14) SHA1(7703c8456aaa2e27f71a7edbfa74fb2d7434a762) )
	ROM_LOAD( "2rh06d.p1", 0x0000, 0x020000, CRC(2892a4d8) SHA1(2b592552d5d45349b2df0d769bd24649f4da5680) )
	ROM_LOAD( "2rh06dh.p1", 0x0000, 0x020000, CRC(b7b6be57) SHA1(0b02eafe58e23ac50f2ae74137550868115d6f6f) )
	ROM_LOAD( "2rh06dk.p1", 0x0000, 0x020000, CRC(339d4642) SHA1(55301e716c235aab7ee88cc28ec7426bd24328c0) )
	ROM_LOAD( "2rh06dr.p1", 0x0000, 0x020000, CRC(8092fd73) SHA1(52c68ee7d02f7256b434110c4df2e926b528af16) )
	ROM_LOAD( "2rh06dy.p1", 0x0000, 0x020000, CRC(b47e66bc) SHA1(01ae45693fd4b81c9090a29264dfa1db58837dde) )
	ROM_LOAD( "2rh06h.p1", 0x0000, 0x020000, CRC(9b65ffed) SHA1(65ab62fe772bd54793c45cc1105a189f21bb5d25) )
	ROM_LOAD( "2rh06k.p1", 0x0000, 0x020000, CRC(1f4e07f8) SHA1(35459640bc215c465b84df073505e8fd6077a332) )
	ROM_LOAD( "2rh06r.p1", 0x0000, 0x020000, CRC(ac41bcc9) SHA1(0de0c0976ef5c58084f02310495b246dc7c23e60) )
	ROM_LOAD( "2rh06s.p1", 0x0000, 0x020000, CRC(2ea10eed) SHA1(825bd6a53100b389f7d67ec49e4535c1de0ece74) )
	ROM_LOAD( "2rh06y.p1", 0x0000, 0x020000, CRC(98ad2706) SHA1(862a725bad97d28580dad102a71750465c7b0f5d) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "rh.chr", 0x0000, 0x000048, CRC(5522383a) SHA1(4413b1d68500f21f10e7cff6b2d3de7258b1b614) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "2rhsnd02.p1", 0x000000, 0x080000, CRC(0f4630dc) SHA1(7235e53c74e113230a683de33763023e95090d39) )
	ROM_LOAD( "2rhsnd02.p2", 0x080000, 0x080000, CRC(c2d0540a) SHA1(160080b350d41b95a0c129f9189222d79734e7d0) )
ROM_END


ROM_START( m4rhogc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rhcf.p1", 0x0000, 0x010000, CRC(0b726e87) SHA1(12c334e7dd712b9e19e8241b1a8e278ff84110d4) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "rhcs.p1", 0x0000, 0x010000, CRC(d1541050) SHA1(ef1ee3b9319e2a357540cf0de902de439267c3e2) )
	ROM_LOAD( "rhcd.p1", 0x0000, 0x010000, CRC(7a7df536) SHA1(9c53e5c6a5f3a32de05a574e1c8dedc3e5be66eb) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "rhc.chr", 0x0000, 0x000048, CRC(6ceab6b0) SHA1(04f4238ea3fcf944c97bc11031e456b851ebe917) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "rhc.s1", 0x000000, 0x080000, CRC(8840737f) SHA1(eb4a4bedfdba1b33fa74b9c2000c0d09a4cca5d7) )
	ROM_LOAD( "rhc.s2", 0x080000, 0x080000, CRC(04eaa2da) SHA1(2c23bde76f6a9406b0cb30246ce8805b5181047f) )
ROM_END


ROM_START( m4roadrn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dro19", 0x0000, 0x010000, CRC(8b591766) SHA1(df156390b427e31cdda64826a6c1d2457c915f25) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "dro_1.snd", 0x000000, 0x080000, CRC(895cfe63) SHA1(02134e149cef3526bbdb6cb93ef3efa283b9d6a2) )
	ROM_LOAD( "dro_2.snd", 0x080000, 0x080000, CRC(1d5c8d4f) SHA1(15c18ae7286807cdc0feb825b958eae808445690) )
ROM_END


ROM_START( m4rockmn )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "rok06c.p1", 0x0000, 0x020000, CRC(8e3a628f) SHA1(3bedb095af710f0b6376a5d99c072f7b3d3de0af) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "rok06ad.p1", 0x0000, 0x020000, CRC(9daa1e35) SHA1(11e7a503c289813cc2ea4507bf5255957e92bc12) )
	ROM_LOAD( "rok06b.p1", 0x0000, 0x020000, CRC(b970df9d) SHA1(4230c3130a52502fb0a8aabf60fd33e90a7fa266) )
	ROM_LOAD( "rok06bd.p1", 0x0000, 0x020000, CRC(10b0b0f0) SHA1(a2f485e7578648aadb8ddb04ea885f4315b9ab82) )
	ROM_LOAD( "rok06d.p1", 0x0000, 0x020000, CRC(c46bd4eb) SHA1(cd07aebaf398eb4bcbc647d0fccdab2561b7aa1c) )
	ROM_LOAD( "rok06dk.p1", 0x0000, 0x020000, CRC(5a7718a6) SHA1(7c6cbdb4066e7664f70a2b584a074652f8baef2c) )
	ROM_LOAD( "rok06dr.p1", 0x0000, 0x020000, CRC(e978a397) SHA1(b48512e1ae332d6e3b6432e9caf247222df5d0a4) )
	ROM_LOAD( "rok06dy.p1", 0x0000, 0x020000, CRC(dd943858) SHA1(ac8e53f72f98b217f190a9d3a9822a41b3028adb) )
	ROM_LOAD( "rok06k.p1", 0x0000, 0x020000, CRC(f3b777cb) SHA1(2d5b67c69458712c370801702a813f81640ce184) )
	ROM_LOAD( "rok06r.p1", 0x0000, 0x020000, CRC(40b8ccfa) SHA1(8868a0ca622e5331204b2431bf64e723a1a79222) )
	ROM_LOAD( "rok06s.p1", 0x0000, 0x020000, CRC(e8b89551) SHA1(753828fd8631588c7725ee4f013f3c78d23f7038) )
	ROM_LOAD( "rok06y.p1", 0x0000, 0x020000, CRC(74545735) SHA1(79ee259656fb71c24382c6670150e49a5b8bc62f) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "rmchr.chr", 0x0000, 0x000048, CRC(94147e3d) SHA1(92e311096331deb54752cc158ece86fbe8ad2063) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "rokmsnd1",  0x000000, 0x080000, CRC(b51e5d7d) SHA1(71f36f866583d592d029cba47901cbfd17631b06) )
	ROM_LOAD( "roksnd.p1", 0x000000, 0x080000, CRC(462a690e) SHA1(5a82f63a9d03c89c8fdb0ead1fc40e480aedd787) )
	ROM_LOAD( "roksnd.p2", 0x080000, 0x080000, CRC(37786d14) SHA1(d6dc2d3dbe54ca943092938500d72081153b5a34) )
ROM_END


ROM_START( m4royjwl )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "rj.bin", 0x0000, 0x020000, CRC(3ffbe4a8) SHA1(47a0309cc9ff315ad9f64e6855863409443e94e2) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "rj_sound1.bin", 0x000000, 0x080000, CRC(443c4901) SHA1(7b3c6737b47dfe04c072f0e157d83c09340c3f9b) )
	ROM_LOAD( "rj_sound2.bin", 0x080000, 0x080000, CRC(9456523e) SHA1(ea1b6bf16b7d1015c188ad83760336d9851de391) )
ROM_END


ROM_START( m4rfym )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rund.p1", 0x0000, 0x010000, CRC(2be2a66d) SHA1(a66d74ccf1783912673cfcb6c1ae7fbb6d70ca0e) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ap1ad.p1", 0x0000, 0x010000, CRC(d1adbf80) SHA1(08801f38b8ba5034fd83b53b6cfff864104525b4) )
	ROM_LOAD( "ap1b.p1", 0x0000, 0x010000, CRC(4939f186) SHA1(389d46d603e75d3aaeeca990f4e1143c61f1565f) )
	ROM_LOAD( "ap1bd.p1", 0x0000, 0x010000, CRC(08a33b2c) SHA1(ef38e9cd0c9bc8393530e36060c803d1250c46a6) )
	ROM_LOAD( "ap1d.p1", 0x0000, 0x010000, CRC(edef44fe) SHA1(4907804c1bebc1f13aa3eb9dad0e9189de8e9601) )
	ROM_LOAD( "ap1dk.p1", 0x0000, 0x010000, CRC(873a402c) SHA1(1315a4ad18544ca5d65526ea0f620cac528e4cad) )
	ROM_LOAD( "ap1dy.p1", 0x0000, 0x010000, CRC(e8436c00) SHA1(1c2f171e55c3519d63d6c4dd0d56df4e1daad6af) )
	ROM_LOAD( "ap1k.p1", 0x0000, 0x010000, CRC(9afeb1e7) SHA1(5fc5d73a2c976d227a0598fb1dd802c6336415d1) )
	ROM_LOAD( "ap1s.p1", 0x0000, 0x010000, CRC(7474509c) SHA1(c87e20f10806ec87fd33f97b43b8378d304f7d67) )
	ROM_LOAD( "ap1y.p1", 0x0000, 0x010000, CRC(152bf7cb) SHA1(8dd8b621f9dac430c293b29ca03814fc21a148b9) )
	ROM_LOAD( "ap502ad.p1", 0x0000, 0x010000, CRC(ab059e57) SHA1(45ba91989b0fd1a44628f696b78eae2a349e3e4a) )
	ROM_LOAD( "ap502b.p1", 0x0000, 0x010000, CRC(9ed27a6e) SHA1(2d655305a178e4ebe43f3d429dfec5a2ef6b9873) )
	ROM_LOAD( "ap502bd.p1", 0x0000, 0x010000, CRC(48e83fcd) SHA1(3e2de0416722df5004f00baae2d3f6846ff596e5) )
	ROM_LOAD( "ap502d.p1", 0x0000, 0x010000, CRC(d0560301) SHA1(c35e97391c588f6567eeb253eb9de59bec9e1724) )
	ROM_LOAD( "ap502dk.p1", 0x0000, 0x010000, CRC(82aa8d80) SHA1(e42d10537dcc5aaae59472681b215b0eb0821c25) )
	ROM_LOAD( "ap502dr.p1", 0x0000, 0x010000, CRC(1cfb3102) SHA1(b1d3a533de0ff93e15f7c039e75af0ef6c8eec57) )
	ROM_LOAD( "ap502dy.p1", 0x0000, 0x010000, CRC(819019ec) SHA1(36d2093a7a592850533d4206e0c9dd28cdc17568) )
	ROM_LOAD( "ap502k.p1", 0x0000, 0x010000, CRC(5064a894) SHA1(3e67358fe5ed9bfac05f621d7e72e5be7aae67df) )
	ROM_LOAD( "ap502r.p1", 0x0000, 0x010000, CRC(2503c7da) SHA1(2478bab8b19ab68ff01be8fae2e86e47894b3d7c) )
	ROM_LOAD( "ap502s.p1", 0x0000, 0x010000, CRC(8502a09a) SHA1(e635552b7f0c7b2e142d7f4d0f1fd93edac6132d) )
	ROM_LOAD( "ap502y.p1", 0x0000, 0x010000, CRC(b868ef34) SHA1(a773503afd2f59b71e0b9a7e202d3e7120ec88ff) )
	ROM_LOAD( "aprad.p1", 0x0000, 0x010000, CRC(936f59ac) SHA1(325708d965d56a9a7482dbeaa089ca871d5c01b5) )
	ROM_LOAD( "aprb.p1", 0x0000, 0x010000, CRC(72ad662a) SHA1(11f1695e05ecf34a58f8df3ffbc72ab2dd7d02c9) )
	ROM_LOAD( "aprbd.p1", 0x0000, 0x010000, CRC(13af990d) SHA1(604d2173e3d6d25252b30b5bf386b53470c35581) )
	ROM_LOAD( "aprc.p1", 0x0000, 0x010000, CRC(fd3ece9a) SHA1(e11d1d258a415865f7477cdfddcd47e9bdb1c9b5) )
	ROM_LOAD( "aprd.p1", 0x0000, 0x010000, CRC(8c19b732) SHA1(e7aeea41cf649fe2a28414ddedacdf72f56d32fe) )
	ROM_LOAD( "aprdk.p1", 0x0000, 0x010000, CRC(58a41fcd) SHA1(e8c92dfb5c9662c90d363b5b7a7e0a4b4894d4cb) )
	ROM_LOAD( "aprdy.p1", 0x0000, 0x010000, CRC(9496cfad) SHA1(cb24779db99d283f1df86864886f21ad333cb98b) )
	ROM_LOAD( "aprk.p1", 0x0000, 0x010000, CRC(7277ef07) SHA1(dc509d125f8d377d4b2cb011d32be5bdba1daa17) )
	ROM_LOAD( "aprs.p1", 0x0000, 0x010000, CRC(a114a96a) SHA1(b0a9091cac86750329513a0927dd39b76995b2f2) )
	ROM_LOAD( "apry.p1", 0x0000, 0x010000, CRC(bf2120bc) SHA1(473374a9510dd53e39b94bfcf1369e13647239e6) )
	ROM_LOAD( "rfym20", 0x0000, 0x010000, CRC(5e1d70e2) SHA1(2da1b8033a77d367c4b5c3d83a0e5def4e5e5d78) )
	ROM_LOAD( "rfym2010", 0x0000, 0x010000, CRC(ec440e7e) SHA1(21f8d4708b5d779dcefcc1e921a5efe17dd6f8c7) )
	ROM_LOAD( "rfym510l", 0x0000, 0x010000, CRC(24af47f3) SHA1(3d1ec9b013f3f7b497cfb62b42fbb2fa914b24b6) )
	ROM_LOAD( "rfym55", 0x0000, 0x010000, CRC(b7d638d8) SHA1(6064ceffd94ff149d8bcb117fd823de52030ac64) )
	ROM_LOAD( "ru5ad.p1", 0x0000, 0x010000, CRC(1c3e1f39) SHA1(a45cdaaa875e52cf5cd5adf986c98f4a22a14785) )
	ROM_LOAD( "ru5b.p1", 0x0000, 0x010000, CRC(41e44d37) SHA1(8eb409b96864fb0f7c3bf5c66a20a63c8cbc68af) )
	ROM_LOAD( "ru5bd.p1", 0x0000, 0x010000, CRC(8d4db415) SHA1(b023a13f89b7e5c2f72fd213179f723621871faf) )
	ROM_LOAD( "ru5d.p1", 0x0000, 0x010000, CRC(fcb70a63) SHA1(df81c3c26c066c1326b20b9e0dda2863ee9635a6) )
	ROM_LOAD( "ru5dk.p1", 0x0000, 0x010000, CRC(b4d83863) SHA1(02aebf94773d0a9454119b4ad663b6d8475fc8d3) )
	ROM_LOAD( "ru5dy.p1", 0x0000, 0x010000, CRC(66375af5) SHA1(0a6d10357c163e5e27e7436f8190070e36e3ef90) )
	ROM_LOAD( "ru5k.p1", 0x0000, 0x010000, CRC(7871c141) SHA1(e1e9d2972c87d2835b1e5a62502160cb4abb7736) )
	ROM_LOAD( "ru5s.p1", 0x0000, 0x010000, CRC(41795ea3) SHA1(6bfb6da6c0f7e762d628ce8a9dcdcbc3c0326ca6) )
	ROM_LOAD( "ru5y.p1", 0x0000, 0x010000, CRC(ee217541) SHA1(68474c2e430d95ded2856183b9a02be917d092d6) )
	ROM_LOAD( "ru8c.p1", 0x0000, 0x010000, CRC(93290724) SHA1(37b17b08f77b308289d4392900576dc66a0377eb) )
	ROM_LOAD( "ru8d.p1", 0x0000, 0x010000, CRC(3e7d6ebb) SHA1(a836a52aef9fe4a9021835e99109b7fefb4ead76) )
	ROM_LOAD( "ru8dk.p1", 0x0000, 0x010000, CRC(b2983dc1) SHA1(412bf4a643c807371fa465fb5f9a85bc3e46623d) )
	ROM_LOAD( "ru8dy.p1", 0x0000, 0x010000, CRC(7d06cdcc) SHA1(d68f6ee59eb7689df30412288db4e9ee6c4bf178) )
	ROM_LOAD( "ru8k.p1", 0x0000, 0x010000, CRC(42f6226e) SHA1(c4bac8efd9c17f96dd9d973e9f64c85ceeacb36b) )
	ROM_LOAD( "ru8s.p1", 0x0000, 0x010000, CRC(d6ce5891) SHA1(c130e7bf614c67767c9af6f38e3cd41ce63d11ef) )
	ROM_LOAD( "ru8y.p1", 0x0000, 0x010000, CRC(f1fc1e75) SHA1(f6f1008349505ee0c494fcdde27db2a15147b6cb) )
	ROM_LOAD( "runc.p1", 0x0000, 0x010000, CRC(09f53ddf) SHA1(f46be95bfacac751102a5f4d4a0917a5e51a653e) )
	ROM_LOAD( "rundy.p1", 0x0000, 0x010000, CRC(a6f69a24) SHA1(8370287dcc890fcb7529d3d4c7a3c2e2e688f6a8) )
	ROM_LOAD( "runk.p1", 0x0000, 0x010000, CRC(a2828b82) SHA1(0ae371a441df679fd9c699771ae9f58ce960d4a1) )
	ROM_LOAD( "runs.p1", 0x0000, 0x010000, CRC(e20f5a06) SHA1(f0f71f8870db7003fce96f1dfe09804cf17c3ab3) )
	ROM_LOAD( "runy.p1", 0x0000, 0x010000, CRC(0e311ab4) SHA1(c98540c07e9cc23ec70ecfbcb2f4d66f2c716fc3) )
	ROM_LOAD( "rutad.p1", 0x0000, 0x010000, CRC(f27090c9) SHA1(28b7bb8046f67a3f8b90069de845b0b791b57078) )
	ROM_LOAD( "rutb.p1", 0x0000, 0x010000, CRC(cb7a74bf) SHA1(24274c7e3b40642d698f5c3a9a10cfeb23faaf1b) )
	ROM_LOAD( "rutbd.p1", 0x0000, 0x010000, CRC(19aba8f2) SHA1(cb726130837149c25adb5d87718b72259cb63a63) )
	ROM_LOAD( "rutd.p1", 0x0000, 0x010000, CRC(16a872bd) SHA1(47ad5eb9b473805e2eb86e0d4d9ef4b2e6e3c926) )
	ROM_LOAD( "rutdk.p1", 0x0000, 0x010000, CRC(a8259673) SHA1(443081395ea0c1b0a07e6cd4b17670b3e01bb50f) )
	ROM_LOAD( "rutdy.p1", 0x0000, 0x010000, CRC(6b799f68) SHA1(87482236f1116983e80a7f190710524d3809cd3a) )
	ROM_LOAD( "rutk.p1", 0x0000, 0x010000, CRC(20962e5e) SHA1(0be43050d403750b67c796a007b503e132014f4c) )
	ROM_LOAD( "ruts.p1", 0x0000, 0x010000, CRC(efaf4e03) SHA1(da19d6e28a6727eb9afb69c23fd5685f0dbcc31a) )
	ROM_LOAD( "ruty.p1", 0x0000, 0x010000, CRC(abb708c5) SHA1(6fe3b52a0ba484576fc83ed35aefeda01d275aec) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "rft20.10_chr", 0x0000, 0x000010, CRC(87cdda8d) SHA1(3d17d42d5eaf5dcc04b856ec07689cab6183b12d) )
	ROM_LOAD( "rfym20.10", 0x0000, 0x010000, CRC(947d00d2) SHA1(2c99da689541de247e35ac39eadfe070ac3196b5) )
	ROM_LOAD( "rfym5.10", 0x0000, 0x010000, CRC(c2ce2cc2) SHA1(d5633e01f669ee8772ed77befa90180c6aa0111c) )
	ROM_LOAD( "rfym5.4", 0x0000, 0x010000, CRC(fe613006) SHA1(898b90893bfcb121575952c22c16570a27948bce) )
	ROM_LOAD( "rfym5.8t", 0x0000, 0x010000, CRC(c600718a) SHA1(168fa558f1b5b91fb805d483f3f4351ac80f90ff) )


	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "runsnd.p1", 0x000000, 0x080000, CRC(a37a3a6d) SHA1(b82c7e90508795a53b91d7dab7938abf07e8ab4c) )
	ROM_LOAD( "runsnd.p2", 0x080000, 0x080000, CRC(1c03046f) SHA1(5235b2f60f12cbee11fb5e54e1858a11a755f460) )
ROM_END


ROM_START( m4runawy )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "r4t1_1.1", 0x8000, 0x008000, CRC(5c3c7276) SHA1(3e1e1e159f79524cb4eb69e4f5e81317598fcfd1) )
	ROM_LOAD( "r4t1_1.2", 0x6000, 0x002000, CRC(b25001ce) SHA1(73f11579094d38fc369805d18ebf73dee3c206ac) )
ROM_END


ROM_START( m4salsa )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dsa15.epr", 0x0000, 0x010000, CRC(22b60b0b) SHA1(4ad184d1557bfd01650684ea9d8ad794fded65f7) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "dsa_1#97c2.snd", 0x0000, 0x080000, CRC(0281a6dd) SHA1(a35a8cd0da32c51f77856ea3eeff7c58fd032333) )
ROM_END


ROM_START( m4samu )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dsm10.bin", 0x0000, 0x010000, CRC(c2a10732) SHA1(9a9dcd0445662d301320a7fb0f4e5da8a719a86b) )
ROM_END


ROM_START( m4sayno )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "say no more 425b.bin", 0x0000, 0x010000, CRC(2cf27394) SHA1(fb7688b7d9d2e68f0c84a57b66dd02dbbc6accc7) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "snm 5p.bin", 0x0000, 0x010000, CRC(4fba5c0a) SHA1(85438c531d4122bc31f59127a577dc6d71a4ba9d) )
	ROM_LOAD( "snm 6.bin", 0x0000, 0x010000, CRC(0d14730b) SHA1(2a35d72bdcc9402b00153621ec852f902720c104) )
	ROM_LOAD( "snms.p1", 0x0000, 0x010000, CRC(be1f2222) SHA1(7d8319796e1d45a3d0246bf13b6d818f20796db3) )
	ROM_LOAD( "snmx.p1", 0x0000, 0x010000, CRC(61a78035) SHA1(1d6c553c60fee0b80e06f8421b8a3806d1f3a587) )
ROM_END


ROM_START( m4showtm )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dsh13.bin", 0x0000, 0x010000, CRC(4ce40ff1) SHA1(f145d6c8e926ca4368d43dacda0fa38615988d84) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sdsh01s1.snd", 0x0000, 0x080000, CRC(f247ba83) SHA1(9b173503e63a4a861d1380b2ab1fe14af1a189bd) )
ROM_END


ROM_START( m4shocm )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "scms.p1", 0x0000, 0x020000, CRC(8cb17f49) SHA1(6c67d5d65567ba3677f51f9c636e1f8e253111de) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "scmad.p1", 0x0000, 0x020000, CRC(0960b887) SHA1(02b029760d141664a7c5860a29b158d8c2dec4e7) )
	ROM_LOAD( "scmb.p1", 0x0000, 0x020000, CRC(c96e88cd) SHA1(61abff544c979efabf5e53d2c53d7cbe90c1f265) )
	ROM_LOAD( "scmbd.p1", 0x0000, 0x020000, CRC(847a1642) SHA1(0bb6d2494888c5e45bf4bfd0f6ba123283346361) )
	ROM_LOAD( "scmd.p1", 0x0000, 0x020000, CRC(b47583bb) SHA1(ed7449764c03d1bd1a45a8891a7a4f9e73c1a9e6) )
	ROM_LOAD( "scmdj.p1", 0x0000, 0x020000, CRC(a0af8091) SHA1(3b6a5e8825cdff40051630d26ca5654173482692) )
	ROM_LOAD( "scmdk.p1", 0x0000, 0x020000, CRC(cebdbe14) SHA1(4da55b365d5eaf558c44eaee33b3ebdc6b6882fa) )
	ROM_LOAD( "scmdy.p1", 0x0000, 0x020000, CRC(495e9eea) SHA1(f10ff85e37538083919af1eec9dabfacd1ac524b) )
	ROM_LOAD( "scmj.p1", 0x0000, 0x020000, CRC(edbb1e1e) SHA1(9a6f4f17d138d7df76bc30a2b16e0bc6ee6149e6) )
	ROM_LOAD( "scmy.p1", 0x0000, 0x020000, CRC(044a0065) SHA1(e5deb75e7d05787f1e820352aec99abebd3530b6) )
	ROM_LOAD( "scmk.p1", 0x0000, 0x020000, CRC(83a9209b) SHA1(011ecd85c435c02b4868ed74012e16c73beb6e99) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4shodf )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sdfs.p1", 0x0000, 0x010000, CRC(5df9abdb) SHA1(0dce3a7ff4d2f11c370a3a2578c592910a9e7371) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sd8b.p1", 0x0000, 0x010000, CRC(79f7fea2) SHA1(5bfa695aef54c9621a91beac2e6c8a09d3b2974b) )
	ROM_LOAD( "sd8d.p1", 0x0000, 0x010000, CRC(060a1b37) SHA1(fb4fbc1164f97f13eb10edbd4e8a37502d716340) )
	ROM_LOAD( "sd8dk.p1", 0x0000, 0x010000, CRC(20982264) SHA1(178ce24ce21e865608133fe2ae281ba2adbdf1d4) )
	ROM_LOAD( "sd8dy.p1", 0x0000, 0x010000, CRC(3fb73b48) SHA1(328f827a92e6fb8ccfb3a82c52401b2d31e974bf) )
	ROM_LOAD( "sd8k.p1", 0x0000, 0x010000, CRC(0d8f2238) SHA1(55643a1f9fe136fb724b05efc0362b6295c9caf9) )
	ROM_LOAD( "sd8s.p1", 0x0000, 0x010000, CRC(59d696e4) SHA1(e51a9a0bc1348b44e77f85343463154ad680ef89) )
	ROM_LOAD( "sd8y.p1", 0x0000, 0x010000, CRC(f79c2e78) SHA1(f6c298b77a9c32378e3f219063daab17e551d083) )
	ROM_LOAD( "sdfb.p1", 0x0000, 0x010000, CRC(a15204bb) SHA1(c862822615e82e5f2f9f2f3cb7e31f804fd859be) )
	ROM_LOAD( "sdfd.p1", 0x0000, 0x010000, CRC(19913c83) SHA1(894da549e790b9062f36fdce90b8e8d284d513e6) )
	ROM_LOAD( "sdfdy.p1", 0x0000, 0x010000, CRC(df1325b1) SHA1(002780fcecf895d20a2a3c0c57fbe4dd675a1e42) )
	ROM_LOAD( "sdfk.p1", 0x0000, 0x010000, CRC(32def2fb) SHA1(45064f319cb5268745e8d5210ceed3a84a8e7f20) )
	ROM_LOAD( "sdfy.p1", 0x0000, 0x010000, CRC(dbb6aa80) SHA1(976f5811a0a578c7f2497ac654f7c416b6018a34) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sdfsnd.p1", 0x000000, 0x080000, CRC(a5829cec) SHA1(eb65c86125350a7f384f9033f6a217284b6ff3d1) )
	ROM_LOAD( "sdfsnd.p2", 0x080000, 0x080000, CRC(1e5d8407) SHA1(64ee6eba3fb7700a06b89a1e0489a0cd54bb89fd) )
ROM_END




ROM_START( m4silnud )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sn1_0.bin", 0x0000, 0x010000, CRC(b51ba096) SHA1(c280b8ba4aecd2256e268b9623f84070f38beeb8) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "ctnsnd.hex", 0x0000, 0x080000, CRC(150a4513) SHA1(97147e11b49d18225c527d8a0926118a83ee906c)) // == 2p Unlimited?
ROM_END

ROM_START( m4nud2p )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "classic 2p nudger v0-1 (27512)", 0x0000, 0x010000, CRC(9c22f5f8) SHA1(5391f5d5b3a0861b93702e476d9635ff304a02a2) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "ctnsnd.hex", 0x0000, 0x080000, CRC(150a4513) SHA1(97147e11b49d18225c527d8a0926118a83ee906c)) // == 2p Unlimited?
ROM_END

ROM_START( m4ctn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ctn0_3.hex", 0x0000, 0x010000, CRC(7573bbc7) SHA1(bc15b03c9fddbd0b93be259547c5420f0623fd58) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "ctnsnd.hex", 0x0000, 0x080000, CRC(150a4513) SHA1(97147e11b49d18225c527d8a0926118a83ee906c)) // == 2p Unlimited?
ROM_END





ROM_START( m4silshd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sh1-0.p1", 0x8000, 0x008000, CRC(35a298bc) SHA1(d64ccb8a1ecfbb9c01b66789ff3d0b5fe48bc6f9) )
	ROM_LOAD( "sh1-0.p2", 0x0000, 0x008000, CRC(81921f9b) SHA1(dcb1c6bf14eb7c41d6cb37e0a9f49c79f187ab94) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "shadow1.bin", 0x0000, 0x008000, CRC(8072340d) SHA1(7346e98421a2355bba490ccbafbf97250bf189de) )
	ROM_LOAD( "shadow2.bin", 0x0000, 0x008000, CRC(4f65cb62) SHA1(d8616fc07f2d42fac7ca50edf7c5c8596092761b) )
	ROM_LOAD( "sshad1.bin", 0x0000, 0x007f3b, CRC(88dbe1f5) SHA1(65b2ad2f6c40da9ad894d2cd6ac4a5bfd925f415) )
	ROM_LOAD( "sshad2.bin", 0x0000, 0x008000, CRC(c8c0f458) SHA1(a86866aec9412e623681e97ad0cea0f95a3e7427) )
ROM_END


ROM_START( m4sgrab )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "1b.bin", 0xe000, 0x002000, CRC(980e4e38) SHA1(0e1db8ec5ba21c97351e91e09e6daaeea7391f07) )
	ROM_LOAD( "2b.bin", 0xc000, 0x002000, CRC(8b407449) SHA1(99ab60a3acde4f5f785d33d1266d5dd28b43665e) )
	ROM_LOAD( "3b.bin", 0xa000, 0x002000, CRC(72c2735a) SHA1(afedc6bd0e270d0f40da3ae01b57698767fdb212) )
	ROM_LOAD( "4b.bin", 0x8000, 0x002000, CRC(92d9c12f) SHA1(e62646af5d4d53bf418629a29037c5a3df6304f7) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sag34.hex", 0x0000, 0x008000, CRC(e066ffdf) SHA1(3d906af27c13e1514225ae176e9e49e9b22552f4) )
	ROM_LOAD( "smashandgrabv1prom1", 0x0000, 0x004000, CRC(5f75b90d) SHA1(b8baa2a3cb70d102950c3caf21ab90be5befc3ca) )
	ROM_LOAD( "smashandgrabv1prom2", 0x0000, 0x004000, CRC(a91ebd37) SHA1(9db197ad81d888eaf829b278be2ddc62639337fe) )
ROM_END


ROM_START( m4solsil )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sos22p1.bin", 0x8000, 0x008000, CRC(7075a3bd) SHA1(9a797e1612d3c532d414efe1f826c13663b67bcb) )
	ROM_LOAD( "sos22p2.bin", 0x6000, 0x002000, CRC(ab196907) SHA1(a4b1f0d29286f4143ef980d5b4d927367871a334) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sos_2_1.p1", 0x0000, 0x008000, CRC(303706fa) SHA1(cf9df67358f3517a8b18d3b40839e839369786a9) )
	ROM_LOAD( "sos_2_1.p2", 0x0000, 0x002000, CRC(31f53d41) SHA1(41966952d18e4ee006d0a5b0992b5d908dc84adc) )
ROM_END


ROM_START( m4sss )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "spend6 10m.bin", 0x0000, 0x010000, CRC(a7519725) SHA1(cdab0ae00b865291ff7389122d174ef2e2676c6e) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sp_05a__.1o3", 0x0000, 0x010000, CRC(044a0133) SHA1(7cf85cf19f5c3f588daf5c0d7efe4204d67161a2) )
	ROM_LOAD( "sp_05s__.1o3", 0x0000, 0x010000, CRC(2e000a62) SHA1(e60390a383388e385bbde79ca14c63e5d69a8869) )
	ROM_LOAD( "sp_05sb_.1o3", 0x0000, 0x010000, CRC(c6380ef5) SHA1(673044aae9998dfe52205a5e4a3d26361f01c518) )
	ROM_LOAD( "sp_05sd_.1o3", 0x0000, 0x010000, CRC(31f818e1) SHA1(bbfa45ef63a73aa726a8223be234fb8ffba45e24) )
	ROM_LOAD( "sp_10a__.1o3", 0x0000, 0x010000, CRC(918c038c) SHA1(608062dc4e39c15967e16d95945b65ef7feabea2) )
	ROM_LOAD( "sp_10s__.1o3", 0x0000, 0x010000, CRC(1bc5780a) SHA1(df1b5d0d6f4751a480aef77be40fb2cfd153bf18) )
	ROM_LOAD( "sp_10sb_.1o3", 0x0000, 0x010000, CRC(2dfc3926) SHA1(b6b201c65c182f9b18a590910183ce88b245af2b) )
	ROM_LOAD( "sp_10sd_.1o3", 0x0000, 0x010000, CRC(fe5c7e3e) SHA1(f5066f1f0c2220da874cbac0ce510cbac6fff8e7) )
	ROM_LOAD( "sx_05a__.2_1", 0x0000, 0x010000, CRC(ceb830a1) SHA1(c9bef44d64a64872460ae3c450533fd14c92ca43) )
	ROM_LOAD( "sx_10a__.2_1", 0x0000, 0x010000, CRC(73e3bc13) SHA1(004097cc9cd62b8fa4c584fcb9874cf998c7b89d) )
	ROM_LOAD( "sxi05___.2_1", 0x0000, 0x010000, CRC(a804a20b) SHA1(477d2a750c0c252ffa215c3cf89916cb3a296b92) )
	ROM_LOAD( "sxi10___.2_1", 0x0000, 0x010000, CRC(bbb23438) SHA1(2cc4376f6393c69c1e18ad06be18933592b6bdae) )

ROM_END




ROM_START( m4squid )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "squidsin.bin", 0x0000, 0x020000, CRC(be369b43) SHA1(e5c7b7a858b264db2f8f726396ddeb42004d7cb9) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sq__x_dx.2_0", 0x0000, 0x020000, CRC(2eb6c814) SHA1(070ad5cb36220daf98043f175cf67d4d584c3d01) )
	ROM_LOAD( "sq__xa_x.2_0", 0x0000, 0x020000, CRC(196a6b34) SHA1(a044ba73b4cf04657ddfcf787dedcb151507ef15) )
	ROM_LOAD( "sq__xb_x.2_0", 0x0000, 0x020000, CRC(53adc362) SHA1(3920f08299bf284ee9f102ce1505d9e9cdc1d1f0) )


	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "squidsnd.p1", 0x000000, 0x080000, CRC(44cebe30) SHA1(a93f64897b4ba333d044649f28fa5dd68d3d2e94) )
	ROM_LOAD( "squidsnd.p2", 0x080000, 0x080000, CRC(d2a1b073) SHA1(d4931f18d369e89492fe72a7a1c511c8d3c23a71) )
ROM_END


ROM_START( m4stakeu )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "stakeup.hex", 0x6000, 0x00a000, CRC(a7ac8f19) SHA1(ec87512e16ff0252012067ad655c3fcee1d2e908) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "su4_4.p1", 0x0000, 0x004000, CRC(e534edba) SHA1(0295013e3e9271f7e5023b8de670a4903578bf05) )
	ROM_LOAD( "su4_4.p2", 0x0000, 0x004000, CRC(8c2e0872) SHA1(9b0f1195d740e51085007417041428b449b3ee51) )
	ROM_LOAD( "su4_4.p3", 0x0000, 0x002000, CRC(0c25955b) SHA1(71f0ebbf088abc3ad860f9a1c7a830348a979289) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "m400.chr", 0x0000, 0x000048, CRC(8f00f720) SHA1(ea59fa2a3b016a7ae83be3caf863de87ce7aeffa) )
ROM_END


ROM_START( m4str300 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "starplay.epr", 0x0000, 0x010000, CRC(79e1746c) SHA1(794317f3aba7b1a7994cde89d81abc2b687d0821) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "starpl.chr", 0x0000, 0x000048, CRC(ead61793) SHA1(f38a38601a67804111b8f8cf0a05d35ed79b7ed1) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "starplay.snd", 0x0000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END


ROM_START( m4stards )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dsd13.bin", 0x0000, 0x010000, CRC(a17fbe93) SHA1(f2a9e0c059f309f63e6da3e47740644ad4839fa8) )
ROM_END


ROM_START( m4starbr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dsb28j.bin", 0x0000, 0x010000, CRC(2f21b614) SHA1(9cc47b879ad8ff81147a5d66fe9a41118aa722f2) )
ROM_END


ROM_START( m4steptm )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dst11.bin", 0x0000, 0x010000, CRC(3960f210) SHA1(c7c4fe74cb9a53eaa9114a84240de3bce4ffe75e) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "sdun01.bin", 0x0000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END


ROM_START( m4stopcl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sc251.bin", 0xc000, 0x004000, CRC(a0205ecb) SHA1(e3f33c7d692dbe39669eb8121c42e1ca6024e611) )
	ROM_LOAD( "sc252.bin", 0x8000, 0x004000, CRC(84f4e19c) SHA1(c077ed83001b9dd29265941d173c939a7f815068) )
	ROM_LOAD( "sc253.bin", 0x6000, 0x002000, CRC(7e4d0526) SHA1(c5d4e6e2662157dd111337e619bee215b3514a91) )
ROM_END


ROM_START( m4sunset )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sunsetboulivard2p.bin", 0x0000, 0x010000, CRC(c83a0b84) SHA1(a345d1e4914ad08d40980022f63e10879e2ef32f) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "b2512s.p1", 0x0000, 0x010000, CRC(8c509538) SHA1(eab6a1e44e77cb48cf490616facc74932acc93c5) )
	ROM_LOAD( "b2512y.p1", 0x0000, 0x010000, CRC(65fa2cd9) SHA1(d2ab1ae25d5425a0788f86535a20d3ebe4a9db2b) )
	ROM_LOAD( "sunboul-5p3.bin", 0x0000, 0x010000, CRC(5ccbf062) SHA1(cf587018511d1a06624d271f2fde4e40f16ec87c) )
	ROM_LOAD( "sunsetb.hex", 0x0000, 0x010000, CRC(53e848a9) SHA1(62cd50003a8bd580b68128324ac98974470ce803) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sbc02__1.1", 0x0000, 0x010000, CRC(0c4cad79) SHA1(0595d149958ccc4e5bfeb67fb021c314232a7ca0) )
	ROM_LOAD( "sbc05__1.0", 0x0000, 0x010000, CRC(e70acd03) SHA1(96339f9e2011bd1f3ac1eab42ae55466526b48f8) )
	ROM_LOAD( "sbc10__1.0", 0x0000, 0x010000, CRC(c93b9185) SHA1(209307c7a2dec14e3d65d45c3eb4f1faa1881671) )
	ROM_LOAD( "sbc10d_1.0", 0x0000, 0x010000, CRC(ab1e65c5) SHA1(6a8e497765dc4f4a41e0128856447fcdf5319975) )
	ROM_LOAD( "sbc20__1.0", 0x0000, 0x010000, CRC(4d1651c5) SHA1(f8e2fd4e8afc068f23ef32f81b7eb20e8f33d787) )
	ROM_LOAD( "sbc20d_1.0", 0x0000, 0x010000, CRC(9c1dea06) SHA1(0433d892143e5d74c29b99917571af4675d0d9c4) )
	ROM_LOAD( "sbo02__1.1", 0x0000, 0x010000, CRC(3a2541ef) SHA1(fd5e84bed8fdb3e3cedf8bc5c8d8070c781d5b44) )
	ROM_LOAD( "sbo05__1.0", 0x0000, 0x010000, CRC(87e2ff53) SHA1(02b4f87aaf5786a3f0f0eb4bac4bcc756960885c) )
	ROM_LOAD( "sbo10__1.0", 0x0000, 0x010000, CRC(5bb9c3ff) SHA1(ea2d30713e351de72f76bb642d5a778899c56634) )
	ROM_LOAD( "sbo10d_1.0", 0x0000, 0x010000, CRC(50b5dd43) SHA1(1a8e2725b688c989695bdb71fa8ee3c7a5fa035b) )
	ROM_LOAD( "sbo10dy1.0", 0x0000, 0x010000, CRC(8e7508c0) SHA1(e3000e7f8dc806392faab235b50f3584c8ce61eb) )
	ROM_LOAD( "sbo20__1.0", 0x0000, 0x010000, CRC(6a652ff2) SHA1(b51d6e59948cf8b1f5ca926d0930b3f8c18cec18) )
	ROM_LOAD( "sbo20c_1.0", 0x0000, 0x010000, CRC(d6ba347a) SHA1(0ac93bad181b462036c5c28ea871716c1d393b49) )
	ROM_LOAD( "sbo20d_1.0", 0x0000, 0x010000, CRC(ffe841a8) SHA1(50ab96c9c28d11f17316bd386d7a83cd5198a583) )
	ROM_LOAD( "sbo20dy1.0", 0x0000, 0x010000, CRC(7d0023ef) SHA1(efb0202082acdbb37d5ed8af5222cfac3b4bbaca) )
	ROM_LOAD( "sbo20y_1.0", 0x0000, 0x010000, CRC(9b25ba02) SHA1(0b6b7c53c136724d74837dd5bcdf544e4a2696b0) )



	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "sunsetb.chr", 0x0000, 0x000048, CRC(f166963b) SHA1(5cc6ada61036d8dbeca470e9548f9f5d2bd545a8) )
ROM_END


ROM_START( m4supslt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ss", 0x8000, 0x008000, CRC(6995e373) SHA1(2b121ff53da66d9febeba65e96cf2da4058c3b45) )
	ROM_LOAD( "ss2", 0x6000, 0x002000, CRC(3e53e684) SHA1(89de2ccd21c5c7e41586e737dcdaf56056bbd98c) )
ROM_END


ROM_START( m4suptrn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dsu21", 0x0000, 0x010000, CRC(b09f63e3) SHA1(8dba0731e1ed5e7056ec6ad1fa269b5b77629745) )
ROM_END


ROM_START( m4supbj )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sbj31.hex", 0x0000, 0x010000, CRC(f7fb2b99) SHA1(c860d3f95ee3fde02bf00b2e20eeee0ebaf01912) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sbjackv3", 0x0000, 0x002000, CRC(13b799d6) SHA1(510f9b60dbcdbd5ce115f103aeee414443ca1d96) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "sbj.chr", 0x0000, 0x000048, CRC(cc4b7911) SHA1(9f8a96a1f8b0f9b33b852e93483ce5c684703349) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sbjsnd1.hex", 0x000000, 0x080000, CRC(70388bec) SHA1(256fa01b57049d73e88b0bb270fccb555b12dfb7) )
	ROM_LOAD( "sbjsnd2.hex", 0x080000, 0x080000, CRC(1d588554) SHA1(48c092ce83d2f881fc217a3d566e896718ad6f24) )
ROM_END


ROM_START( m4supbjc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sbj31.hex", 0x0000, 0x010000, CRC(f7fb2b99) SHA1(c860d3f95ee3fde02bf00b2e20eeee0ebaf01912) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sbjd.p1", 0x0000, 0x010000, CRC(555361f4) SHA1(f5327b811ab3421307dc59d209a216798cd54393) )
	ROM_LOAD( "sbjf.p1", 0x0000, 0x010000, CRC(7966deff) SHA1(5cdb6c80ef56b27878eb1fffd6fdf31060e56291) )
	ROM_LOAD( "sbjl.p1", 0x0000, 0x010000, CRC(fc47ed74) SHA1(f29b2caac8168410e534e2f224c98dd4bbb9a7f7) )
	ROM_LOAD( "sbjs.p1", 0x0000, 0x010000, CRC(f7fb2b99) SHA1(c860d3f95ee3fde02bf00b2e20eeee0ebaf01912) )
	ROM_LOAD( "superbjclub.bin", 0x0000, 0x010000, CRC(68d11d27) SHA1(a0303f845fb5f5b396a7be3ca17a9eaf1a7baef4) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "sbj.chr", 0x0000, 0x000048, CRC(cc4b7911) SHA1(9f8a96a1f8b0f9b33b852e93483ce5c684703349) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sbjsnd1.hex", 0x000000, 0x080000, CRC(70388bec) SHA1(256fa01b57049d73e88b0bb270fccb555b12dfb7) )
	ROM_LOAD( "sbjsnd2.hex", 0x080000, 0x080000, CRC(1d588554) SHA1(48c092ce83d2f881fc217a3d566e896718ad6f24) )

	ROM_LOAD( "sbj.s1", 0x000000, 0x080000, CRC(9bcba966) SHA1(5ced282aca9d39ebf0828aa19357026d5298e955) )
	ROM_LOAD( "sbj.s2", 0x080000, 0x080000, CRC(1d588554) SHA1(48c092ce83d2f881fc217a3d566e896718ad6f24) )
ROM_END


ROM_START( m4supbf )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sbff.p1", 0x0000, 0x010000, CRC(f27feba0) SHA1(157bf28e2d5fc2fa58bed11b3285cf56ae18abb8) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sbfs.p1", 0x0000, 0x010000, CRC(c8c52d5e) SHA1(d53513b9faabc307623a7c2f5be0225fb812beeb) )
ROM_END


ROM_START( m4suphv )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "hyperviper.bin", 0x0000, 0x010000, CRC(8373f6a3) SHA1(79bff20ab80ffe11447595c6fe8e5ab90d432e17) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "hv_05___.3h3", 0x0000, 0x010000, CRC(13bfa891) SHA1(ffddd14a019d52029bf8d4f680d8d05413a9f0b7) )
	ROM_LOAD( "hv_05___.3o3", 0x0000, 0x010000, CRC(9ae86366) SHA1(614ae0ab184645c9f568796783f29a177eda3208) )
	ROM_LOAD( "hv_05___.4n3", 0x0000, 0x010000, CRC(f607f351) SHA1(d7b779b80fa964a27b106bd9d5ca3be16a11d5e9) )
	ROM_LOAD( "hv_05_d_.3h3", 0x0000, 0x010000, CRC(50c66ce8) SHA1(ef12525fc3ac82caf80326edaac81bb9fbc3245c) )
	ROM_LOAD( "hv_05_d_.3o3", 0x0000, 0x010000, CRC(87dfca0e) SHA1(3ab4105680acc46d3633a722f40ff1af0a520a7f) )
	ROM_LOAD( "hv_05_d_.4n3", 0x0000, 0x010000, CRC(f4d702d7) SHA1(268c7f6443c7ae587caf5b227fcd438530a06bcc) )
	ROM_LOAD( "hv_10___.3h3", 0x0000, 0x010000, CRC(627caac7) SHA1(4851ce2441850743ea68ecbf89bde3f4cd6c2b4c) )
	ROM_LOAD( "hv_10___.3o3", 0x0000, 0x010000, CRC(02e4d86a) SHA1(47aa83e8bcd85e8ba7fb972cdd1ead7fe21e0418) )
	ROM_LOAD( "hv_10_d_.3h3", 0x0000, 0x010000, CRC(15cfa26e) SHA1(6bc3feaba65d1797b9945f23a89e983f56b13f79) )
	ROM_LOAD( "hv_10_d_.3n3", 0x0000, 0x010000, CRC(b81f1d0a) SHA1(5fd293be2b75393069c9f5e099b4700ff930f081) )
	ROM_LOAD( "hv_10_d_.3o3", 0x0000, 0x010000, CRC(85f176b9) SHA1(30380d58bf2834829764cbdbdc7d950632e61e6d) )
	ROM_LOAD( "hvi05___.3h3", 0x0000, 0x010000, CRC(6959332e) SHA1(edaa5f86ad4389b0a3bc2e6679fe8f62520be3ae) )
	ROM_LOAD( "hvi05___.3o3", 0x0000, 0x010000, CRC(cdba80a5) SHA1(6c9fac7e5ee324b18922cc7a053495f1977bcb6d) )
	ROM_LOAD( "hvi05___.4n3", 0x0000, 0x010000, CRC(38a33c2b) SHA1(21004092b81e08146291fd3a025652f0edbe47dc) )
	ROM_LOAD( "hvi10___.3h3", 0x0000, 0x010000, CRC(6c1b4b89) SHA1(e8eb4e689d43c5b9e8354aa7375ca3ba12ed1160) )
	ROM_LOAD( "hvi10___.3n3", 0x0000, 0x010000, CRC(9d95cf8c) SHA1(26daf3975e1e3a605bc4392700c5470b52450d6e) )
	ROM_LOAD( "hviper.bin", 0x0000, 0x010000, CRC(13bfa891) SHA1(ffddd14a019d52029bf8d4f680d8d05413a9f0b7) )
	ROM_LOAD( "hypv_05_.4", 0x0000, 0x010000, CRC(246f171c) SHA1(7bbefb0cae57cf8097aa6d033df1a428e8bfe744) )
	ROM_LOAD( "hypv_10_.4", 0x0000, 0x010000, CRC(f85d21a1) SHA1(55ed92147335a1471b7b443f68dd700f579d21f3) )
	ROM_LOAD( "hypv_20_.4", 0x0000, 0x010000, CRC(27a0162b) SHA1(2d1342edbfa29c4f2ee1f1a825f3eeb0489fbaf5) )
ROM_END


ROM_START( m4supst )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cs4b.p1", 0x0000, 0x010000, CRC(fb0aac20) SHA1(3a40be78f7add7905afa8d1226ad41bf0041a2ec) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cs4ad.p1", 0x0000, 0x010000, CRC(c0e81dfd) SHA1(2da922df6c102f8d0f1678e974df9e4d356e5133) )
	ROM_LOAD( "cs4bd.p1", 0x0000, 0x010000, CRC(dafc7ed6) SHA1(3e92d5557d2f587132f4b3b633978ab7d4333fcc) )
	ROM_LOAD( "cs4d.p1", 0x0000, 0x010000, CRC(c1fcda65) SHA1(11f2a45f3f821eac6b98b1988824d77aada3d759) )
	ROM_LOAD( "cs4dk.p1", 0x0000, 0x010000, CRC(30a46171) SHA1(ef1f2951b478ba2b2d42dfb0ec4ed59f28d79972) )
	ROM_LOAD( "cs4dy.p1", 0x0000, 0x010000, CRC(72b15ce7) SHA1(c451ac552ffe9bcde1990b97a60b0ed8918bf8c8) )
	ROM_LOAD( "cs4k.p1", 0x0000, 0x010000, CRC(f252f9ea) SHA1(251998ea752deb4f4a05c833b19e89d334334fac) )
	ROM_LOAD( "cs4s.p1", 0x0000, 0x010000, CRC(10f7b88d) SHA1(0aac0ebbe0ce04db49fc7de4325eea9abdfd74b5) )
	ROM_LOAD( "cs4y.p1", 0x0000, 0x010000, CRC(a464d09d) SHA1(d38c0f8c7c9b7f560b685781a7dcf82bc031a191) )
	ROM_LOAD( "csp02ad.p1", 0x0000, 0x010000, CRC(96bbbc26) SHA1(ca127151c771963c07f0f368102ede8095d11863) )
	ROM_LOAD( "csp02b.p1", 0x0000, 0x010000, CRC(913ea9ff) SHA1(182bcc007d007a1c7f57767358600d2de7d1e3cf) )
	ROM_LOAD( "csp02bd.p1", 0x0000, 0x010000, CRC(ad0137a1) SHA1(d043372ba09081dd4e807f009a6460b4b30e6453) )
	ROM_LOAD( "csp02c.p1", 0x0000, 0x010000, CRC(fdad4b22) SHA1(4f19922821a9d1663bd9355447209384272e7542) )
	ROM_LOAD( "csp02d.p1", 0x0000, 0x010000, CRC(9717a58d) SHA1(8bc495dc4db0041718ae2db14a01a789616c8764) )
	ROM_LOAD( "csp02dk.p1", 0x0000, 0x010000, CRC(cd8aa547) SHA1(a13dcb75507878cb133b9ef739fb41d932d4eed5) )
	ROM_LOAD( "csp02dr.p1", 0x0000, 0x010000, CRC(6656e588) SHA1(4001ec0d1145ef0107e62ccda61e22ba8b0cdc92) )
	ROM_LOAD( "csp02dy.p1", 0x0000, 0x010000, CRC(14ff7e1d) SHA1(455b6ff93a5f25dc5f43c62a6c1d9a18de1ce94b) )
	ROM_LOAD( "csp02k.p1", 0x0000, 0x010000, CRC(c438c754) SHA1(c1d2e664091c1eaf1e4d964a3bfd446b11d7ba41) )
	ROM_LOAD( "csp02r.p1", 0x0000, 0x010000, CRC(4abe0f80) SHA1(67f7f9946a26b5097b6ce719dbd599790078f365) )
	ROM_LOAD( "csp02s.p1", 0x0000, 0x010000, CRC(47c0068d) SHA1(5480a519a6e6df2757e66cfcf904dd6c2873cc43) )
	ROM_LOAD( "csp02y.p1", 0x0000, 0x010000, CRC(d51d18d8) SHA1(a65fd4326872775364d2d7a886e98a1ee07596b7) )
	ROM_LOAD( "cst04ad.p1", 0x0000, 0x010000, CRC(b946d40d) SHA1(c03fa48f8b64c3cf4504f472f21a38f8a55f12e6) )
	ROM_LOAD( "cst04b.p1", 0x0000, 0x010000, CRC(45333d45) SHA1(d6ccb39ee9b316772052f856f79424c34ff273c5) )
	ROM_LOAD( "cst04bd.p1", 0x0000, 0x010000, CRC(03b56b07) SHA1(903b24ab93f9584f228278729b5a99451b8e81f7) )
	ROM_LOAD( "cst04c.p1", 0x0000, 0x010000, CRC(9c000883) SHA1(da0a9f1afc218c14a57a46fe2ea63e166f4e3739) )
	ROM_LOAD( "cst04d.p1", 0x0000, 0x010000, CRC(32281bec) SHA1(a043fb615c2a66d23d85ae80cb0b1705523f411c) )
	ROM_LOAD( "cst04dk.p1", 0x0000, 0x010000, CRC(9345e7b7) SHA1(8bff80d2b847fbae050f77215efe3e55b98a4657) )
	ROM_LOAD( "cst04dr.p1", 0x0000, 0x010000, CRC(8d397063) SHA1(45642de2629e89e2495d1cbd5aed90cf2a4cf1c1) )
	ROM_LOAD( "cst04dy.p1", 0x0000, 0x010000, CRC(4a303ced) SHA1(6c12b956358753c8bf99bd3316646721c9ec2585) )
	ROM_LOAD( "cst04k.p1", 0x0000, 0x010000, CRC(a59584f5) SHA1(8cfcf069ad905277f1925e682602e129e97e619b) )
	ROM_LOAD( "cst04r.p1", 0x0000, 0x010000, CRC(c9771997) SHA1(ed98650c0d73f2db0fe380777d10404ccabced31) )
	ROM_LOAD( "cst04s.p1", 0x0000, 0x010000, CRC(cd5b848d) SHA1(4dd3dd1c883552c7b5c475156308604b12eff75a) )
	ROM_LOAD( "cst04y.p1", 0x0000, 0x010000, CRC(7adc00ae) SHA1(5688f0876c18faf474a6d8487fdd85f20f9fc144) )
	ROM_LOAD( "csu03ad.p1", 0x0000, 0x010000, CRC(5d7b6393) SHA1(19c24f4113efb6a1499936e5f89a8ad859ff8df0) )
	ROM_LOAD( "csu03b.p1", 0x0000, 0x010000, CRC(57826c2a) SHA1(b835eb3066fec468ab55851d1dd023484e2d57e3) )
	ROM_LOAD( "csu03bd.p1", 0x0000, 0x010000, CRC(092e7039) SHA1(36a7c18872e4012e3acce0d01d2cc2c201a3c867) )
	ROM_LOAD( "csu03c.p1", 0x0000, 0x010000, CRC(b30a3c00) SHA1(066b0007092720a6f89edf8eafffe2f8fd83edbc) )
	ROM_LOAD( "csu03d.p1", 0x0000, 0x010000, CRC(03ff9d99) SHA1(390087c136e4c314de9086adb7b020e8adabe34a) )
	ROM_LOAD( "csu03dk.p1", 0x0000, 0x010000, CRC(cf7e61ff) SHA1(0e328ce5ff86770fabaf91d48a8de039323d112a) )
	ROM_LOAD( "csu03dr.p1", 0x0000, 0x010000, CRC(00d700d1) SHA1(8bcc3c470c42780b1f1404fc6ff53e6ec7d89ad0) )
	ROM_LOAD( "csu03dy.p1", 0x0000, 0x010000, CRC(8ec77c04) SHA1(64708460439a7e124f90eef6b9628e57f7d78ebc) )
	ROM_LOAD( "csu03k.p1", 0x0000, 0x010000, CRC(701a0837) SHA1(31237fd108b354fb2afc449efa3a53dee2cf7be8) )
	ROM_LOAD( "csu03r.p1", 0x0000, 0x010000, CRC(d86a6895) SHA1(2c42bcf5de739f01e18bd1b766eec26a6da5aa52) )
	ROM_LOAD( "csu03s.p1", 0x0000, 0x010000, CRC(197bb032) SHA1(06e98713ff5fc72bffccde1cc92fc8cb63665fad) )
	ROM_LOAD( "csu03y.p1", 0x0000, 0x010000, CRC(bee0e7e1) SHA1(6a1ab766af9147f0d4a7c1d2a95c9a6e3e3f4986) )
	ROM_LOAD( "eeh02ad.p1", 0x0000, 0x010000, CRC(25874a6d) SHA1(12e4fb36d231c3104df3613dd3851f411a876eb0) )
	ROM_LOAD( "eeh02b.p1", 0x0000, 0x010000, CRC(ef280a8a) SHA1(912a825e69482a540cf0cadfc49a37a2822f3ecb) )
	ROM_LOAD( "eeh02bd.p1", 0x0000, 0x010000, CRC(5f126810) SHA1(8fe1cbc7d93e2db35225388ee0773f6a98762ca1) )
	ROM_LOAD( "eeh02c.p1", 0x0000, 0x010000, CRC(3f49b936) SHA1(a0d07e0101f8cc38ebc28cfc1b239793b961f5ab) )
	ROM_LOAD( "eeh02d.p1", 0x0000, 0x010000, CRC(14dcfe63) SHA1(3ac77c9aa9b3b77fb1df98d2b427564be41dca78) )
	ROM_LOAD( "eeh02dk.p1", 0x0000, 0x010000, CRC(81a39421) SHA1(6fa43e8cb83e7fb940cc224eed5ee3f254c18c4d) )
	ROM_LOAD( "eeh02dr.p1", 0x0000, 0x010000, CRC(c7755823) SHA1(05626ed49a2f800555f3f404273fa910b68de75c) )
	ROM_LOAD( "eeh02dy.p1", 0x0000, 0x010000, CRC(5a1e70cd) SHA1(88bb29fd52d2331b72bb04652f9578f2c2f5a9ac) )
	ROM_LOAD( "eeh02k.p1", 0x0000, 0x010000, CRC(b78882ec) SHA1(79c6a6d2cfe113743d3a93eb825fccab2b025933) )
	ROM_LOAD( "eeh02r.p1", 0x0000, 0x010000, CRC(ff54884e) SHA1(2783f0e562e946597288ddbec4dcd1101e188d1d) )
	ROM_LOAD( "eeh02s.p1", 0x0000, 0x010000, CRC(c5856c3c) SHA1(5a0e5a7188913e1c36eac894bbeeae47a4f3589c) )
	ROM_LOAD( "eeh02y.p1", 0x0000, 0x010000, CRC(623fa0a0) SHA1(5a49cea5e94afccbf965cbda7a8d9a74f9734a6e) )
	ROM_LOAD( "sp8b.p1", 0x0000, 0x010000, CRC(3b12d7e8) SHA1(92a15e5f8391d74c192e8386abdb8853a76bff05) )
	ROM_LOAD( "sp8bd.p1", 0x0000, 0x010000, CRC(e0d7f789) SHA1(f6157469e43059adb44e7f2eff5bf73861d5636c) )
	ROM_LOAD( "sp8c.p1", 0x0000, 0x010000, CRC(da0af8ae) SHA1(91042506050967c508b30c3dc2bfa6f6a6e8b532) )
	ROM_LOAD( "sp8dk.p1", 0x0000, 0x010000, CRC(92432e8f) SHA1(5e6df963ccf92a89c71ae1edd7b71ec1e3f97522) )
	ROM_LOAD( "sp8k.p1", 0x0000, 0x010000, CRC(e39f74d8) SHA1(9d776e7d67859f4514c69fc4f9f43160da9a2ca1) )
	ROM_LOAD( "sp8s.p1", 0x0000, 0x010000, CRC(fab99461) SHA1(82f8ca06bb04396f86124dfe4de46265b2edc393) )
	ROM_LOAD( "spsbd.p1", 0x0000, 0x010000, CRC(b621b32d) SHA1(9aab0e074c120cb12beac585f9c513053502955c) )
	ROM_LOAD( "spsc.p1", 0x0000, 0x010000, CRC(8c7a24f5) SHA1(f86be164e05235281fb275e950cedaf6f630d29a) )
	ROM_LOAD( "spsd.p1", 0x0000, 0x010000, CRC(d34d3617) SHA1(5373335557e4bbb21264bbd9d0fbaf3640f9ab35) )
	ROM_LOAD( "spsdk.p1", 0x0000, 0x010000, CRC(cf2fd3e7) SHA1(50d3c0851bec90037cd65a5c55654b0e688b96ca) )
	ROM_LOAD( "spsk.p1", 0x0000, 0x010000, CRC(873a1414) SHA1(47b2bbef168382112cd12ace2d6a58695f4b0254) )
	ROM_LOAD( "spss.p1", 0x0000, 0x010000, CRC(5e28bdb7) SHA1(3865c891178feb744ad11b2dea491350efc48bea) )
	ROM_LOAD( "stc02ad.p1", 0x0000, 0x010000, CRC(d9a2b4d1) SHA1(9a6862a44817b3ec465f126fd2a5d2c9825d846e) )
	ROM_LOAD( "stc02b.p1", 0x0000, 0x010000, CRC(bd2e8e6c) SHA1(71670dccedc2f47888c1205de59a81677ffeabaa) )
	ROM_LOAD( "stc02bd.p1", 0x0000, 0x010000, CRC(efbed99b) SHA1(62d80248bb666bfb49ed7546936da744e43fa870) )
	ROM_LOAD( "stc02c.p1", 0x0000, 0x010000, CRC(9d342386) SHA1(b50f64d66d89dbd3dee1ff2cb430a2caa050e7c8) )
	ROM_LOAD( "stc02d.p1", 0x0000, 0x010000, CRC(c43f6e65) SHA1(0278cf389f8289d7b819125ae0a612c81ea75fab) )
	ROM_LOAD( "stc02dk.p1", 0x0000, 0x010000, CRC(36576570) SHA1(214a57344d8e161b3dbd07457291ed9bce011842) )
	ROM_LOAD( "stc02dr.p1", 0x0000, 0x010000, CRC(450c553f) SHA1(46050285eeb10dc368ad501c61d41351c4e2fcde) )
	ROM_LOAD( "stc02dy.p1", 0x0000, 0x010000, CRC(d8677dd1) SHA1(18abc0a1d28458c3b26a0d1dbf6ca8aba3f3e240) )
	ROM_LOAD( "stc02k.p1", 0x0000, 0x010000, CRC(c6e8d110) SHA1(9e05961b9bba502f52a03de27e608afc52f6c025) )
	ROM_LOAD( "stc02r.p1", 0x0000, 0x010000, CRC(918d769f) SHA1(2a4438828d9e7efd3a94eaebe56585e7ae23d9d1) )
	ROM_LOAD( "stc02s.p1", 0x0000, 0x010000, CRC(9c50fff7) SHA1(3468340d2d04cbdecd669817f8a9c4028e301eeb) )
	ROM_LOAD( "stc02y.p1", 0x0000, 0x010000, CRC(0ce65e71) SHA1(02ae1fd5a41ab5a96ddcfe1cf3e8567561291961) )
	ROM_LOAD( "sttad.p1", 0x0000, 0x010000, CRC(af615f05) SHA1(b2c1b8ba086a4d33f1269c28d4caa7286a27f085) )
	ROM_LOAD( "sttb.p1", 0x0000, 0x010000, CRC(3119149f) SHA1(e749fcc5f95ccd29f42bfd0b140cf3cbb84d9599) )
	ROM_LOAD( "sttbd.p1", 0x0000, 0x010000, CRC(cfddaf39) SHA1(0f24b5e691e1d43f6604087f0b3bc2571d2c4002) )
	ROM_LOAD( "sttd.p1", 0x0000, 0x010000, CRC(8bc2498c) SHA1(a9cd3a6968186818a8c4033b1f304eac152244cf) )
	ROM_LOAD( "sttdk.p1", 0x0000, 0x010000, CRC(39903dde) SHA1(f92c4380051ada7bbc5739550c8dfdd6ddaaa3fe) )
	ROM_LOAD( "sttdr.p1", 0x0000, 0x010000, CRC(866f69f0) SHA1(ef9717f89b9718f1bcf8d3592f240ec9cf48bca3) )
	ROM_LOAD( "sttdy.p1", 0x0000, 0x010000, CRC(74ebd933) SHA1(b308c8cae2c74e4e07c6e4afb505068220714824) )
	ROM_LOAD( "sttk.p1", 0x0000, 0x010000, CRC(461db2f5) SHA1(8b97342d7ebfb33aa6aff246e8d799f4435788b7) )
	ROM_LOAD( "sttr.p1", 0x0000, 0x010000, CRC(2591f6ec) SHA1(3d83d930e41e164e71d67b529967320e1eee8354) )
	ROM_LOAD( "stts.p1", 0x0000, 0x010000, CRC(a5e29c32) SHA1(8ba2f76505c2f40493c918b9d9524fa67999f7c1) )
	ROM_LOAD( "stty.p1", 0x0000, 0x010000, CRC(7306fab9) SHA1(0da1612490fcff9b7a17f97190b6b561016c3b18) )
	ROM_LOAD( "stuad.p1", 0x0000, 0x010000, CRC(e7a01b7b) SHA1(3db08800a35d440f012ca69d84c30465818b4993) )
	ROM_LOAD( "stub.p1", 0x0000, 0x010000, CRC(9044badf) SHA1(af8e218e3dc457bb5f24e3f2d74a8639466c3f11) )
	ROM_LOAD( "stubd.p1", 0x0000, 0x010000, CRC(438e1687) SHA1(5e0f27e95bf861d4edc55709efc79496c7353e8b) )
	ROM_LOAD( "stud.p1", 0x0000, 0x010000, CRC(1cbe3bec) SHA1(005dde84e57c5517fc6d6b975cc882dae11cbf63) )
	ROM_LOAD( "studk.p1", 0x0000, 0x010000, CRC(0931d501) SHA1(afa078248230cbc0acc9d3af641ec63ed0424a75) )
	ROM_LOAD( "studr.p1", 0x0000, 0x010000, CRC(e06e1c59) SHA1(f4454f640335dbf6f9b8154d7805102253f605b4) )
	ROM_LOAD( "study.p1", 0x0000, 0x010000, CRC(8b4275e0) SHA1(267a9d2eddf41b8838eeaee06bba45f0a8b8451f) )
	ROM_LOAD( "stuk.p1", 0x0000, 0x010000, CRC(a66fb54f) SHA1(4351edbf6c5de817cf6972885ff1f6c7df837c37) )
	ROM_LOAD( "stur.p1", 0x0000, 0x010000, CRC(eeb3bfed) SHA1(87a753511fb384a505d3cc69ca67fe4e288cf3bb) )
	ROM_LOAD( "stus.p1", 0x0000, 0x010000, CRC(19aca6ad) SHA1(1583e76a4e1058fa97efdd9a7e6f7c4fe806b2f4) )
	ROM_LOAD( "stuy.p1", 0x0000, 0x010000, CRC(e6b2b76f) SHA1(bf251b751e6a8d2764c63e92d48e1a64666b9a47) )
	ROM_LOAD( "superstreak1deb.bin", 0x0000, 0x010000, CRC(892ccad9) SHA1(c88daadd9778e363e154b674b57ccd07cea59836) )
	ROM_LOAD( "supst2515", 0x0000, 0x010000, CRC(c073a249) SHA1(4ae37eb61dd5e50687f433fb89f65b97926b7358) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "ss.chr", 0x0000, 0x000048, CRC(bd206d57) SHA1(ecfe38d9b4823ae6bc2fc440c243e6ae5e2edaa4) )
ROM_END


ROM_START( m4suptub )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "s4t10p1.bin", 0x8000, 0x008000, CRC(3c49a2a1) SHA1(9715584f8aa5bf8c97ebeecd011476a603aea0d3) )
	ROM_LOAD( "s4t10p2.bin", 0x6000, 0x002000, CRC(6e22096a) SHA1(f9923f272497fb7438eac2b46ca5e60babb01a0b) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "st1.bin", 0x0000, 0x008000, CRC(ef0ca8bc) SHA1(464d18df3d3519f4b3f1cc160cca766ad18802cc) )
ROM_END


ROM_START( m4suptwo )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "suts.p1", 0x0000, 0x010000, CRC(0476a1f9) SHA1(5dfd26cf7307c9fb98852e3b2daa65f81f13380d) )
ROM_END


ROM_START( m4taj )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tajmahal.bin", 0x0000, 0x010000, CRC(c2db162a) SHA1(358e7bb858f0a34d39f43494cea13bf00a67e48e) )
ROM_END


ROM_START( m4take5 )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "take5.bin", 0x0000, 0x020000, CRC(24beb7d6) SHA1(746beccaf57fd0c54c8cf8d742b8ef50563a40fd) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "tfive1.hex", 0x000000, 0x080000, CRC(70f16892) SHA1(e6448831d3ce7fa251b40023bc7d5d6dee9d6793) )
	ROM_LOAD( "tfive2.hex", 0x080000, 0x080000, CRC(5fc888b0) SHA1(8d50ee4f36bd36aed5d0e7a77f76bd6caffc6376) )
ROM_END


ROM_START( m4take2 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "take220p", 0x0000, 0x010000, CRC(b536311a) SHA1(234945d2419c8391307db5b5d01d228894441faf) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ttos.p1", 0x0000, 0x010000, CRC(d7943729) SHA1(76fcaf7dbfa7863a4dfe2804e2d472dcfec13124) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "taketwo.chr", 0x0000, 0x000048, CRC(61c0b52b) SHA1(9defb30a186dc237625fc2476dd8650e7fe289af) )
ROM_END


ROM_START( m4takepk )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "tap.hex", 0x0000, 0x020000, CRC(f21f6dc8) SHA1(d421bee2564d3aaa389c35601adc23ad3fda5aa0) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "tapad.p1", 0x0000, 0x020000, CRC(162448c4) SHA1(1f77d053fb5dfddeba1248e9e2a05536ab1bc66a) )
	ROM_LOAD( "tapb.p1", 0x0000, 0x020000, CRC(3f3be560) SHA1(a60d66c5de33747d19ae43bbc15da104cc3e7390) )
	ROM_LOAD( "tapbd.p1", 0x0000, 0x020000, CRC(9b3ee601) SHA1(95f11641d9e3ca0bcfa11a17fb0971b1e2598c7b) )
	ROM_LOAD( "tapc.p1", 0x0000, 0x020000, CRC(99bf563d) SHA1(23672e4519911033f607566b27217c75b8b8651e) )
	ROM_LOAD( "tapd.p1", 0x0000, 0x020000, CRC(4220ee16) SHA1(6c677b24f40e481bb3e61fc3bccccde39b088f8d) )
	ROM_LOAD( "tapdk.p1", 0x0000, 0x020000, CRC(d1f94e57) SHA1(b1dde10ce45c668b5ba5a05462d41716779206a6) )
	ROM_LOAD( "tapdy.p1", 0x0000, 0x020000, CRC(561a6ea9) SHA1(736af1fed00a4df540b8f83da677583dee950b50) )
	ROM_LOAD( "tapk.p1", 0x0000, 0x020000, CRC(75fc4d36) SHA1(18a0e33af69c32a69416612f639a7c8601010177) )
	ROM_LOAD( "tapr.p1", 0x0000, 0x020000, CRC(c6f3f607) SHA1(cee0b59a45ebb50d51bc26b2b5c37fe7ed299bc7) )
	ROM_LOAD( "taps.p1", 0x0000, 0x020000, CRC(01956f25) SHA1(895cd30023b689b61d5ced0cf477f555faf786af) )
	ROM_LOAD( "tapy.p1", 0x0000, 0x020000, CRC(f21f6dc8) SHA1(d421bee2564d3aaa389c35601adc23ad3fda5aa0) )
	ROM_LOAD( "tphad.p1", 0x0000, 0x020000, CRC(51a2f147) SHA1(442d4adc92c6a9215c7655c2c4b955f974420a26) )
	ROM_LOAD( "tphb.p1", 0x0000, 0x020000, CRC(391214a6) SHA1(21f6f29ff1a60b7a646d49bef2c29008f2dc501c) )
	ROM_LOAD( "tphbd.p1", 0x0000, 0x020000, CRC(dcb85f82) SHA1(eb83c8d829d88e940c11f6d0d1c4607a4aa9cba2) )
	ROM_LOAD( "tphd.p1", 0x0000, 0x020000, CRC(44091fd0) SHA1(a05814bc27da6a452802a253060efd9d04956d45) )
	ROM_LOAD( "tphdk.p1", 0x0000, 0x020000, CRC(967ff7d4) SHA1(403865583aabc9983b8c6fa4f0c06ba53696df3b) )
	ROM_LOAD( "tphdy.p1", 0x0000, 0x020000, CRC(119cd72a) SHA1(326f5a548234ec5cd780f22c96b9151222684ed0) )
	ROM_LOAD( "tphk.p1", 0x0000, 0x020000, CRC(73d5bcf0) SHA1(22c620c7d6fc8bc51bda4dd3ae5ec3e38056ce82) )
	ROM_LOAD( "tphs.p1", 0x0000, 0x020000, CRC(e9231738) SHA1(066b1ffd02238783931452b7a1dff05c293a6abe) )
	ROM_LOAD( "tphy.p1", 0x0000, 0x020000, CRC(f4369c0e) SHA1(d8c1fc2ede48673a1e8efaf004e3d76b62594de1) )
	ROM_LOAD( "typ15f", 0x0000, 0x020000, CRC(65c44b06) SHA1(629e7ac4149c66fc1dc33a103e1a4ff5aaecdcfd) )
	ROM_LOAD( "typ15r", 0x0000, 0x020000, CRC(8138c70b) SHA1(aafc805a8a56cf1722ebe0f3eba0a47f15c9049a) )
	ROM_LOAD( "typ510", 0x0000, 0x010000, CRC(ebf0c71c) SHA1(6c759144aecce83f82ded8aae7c61ecec2d92fb3) )
	ROM_LOAD( "typ510s", 0x0000, 0x010000, CRC(4cc6032d) SHA1(e6eaff56e39555393156aa2e56bf1c17e548bdc9) )
	ROM_LOAD( "typ55", 0x0000, 0x010000, CRC(6837344f) SHA1(4d5c6ea005d0916f27a7f445b37ce9252549c61f) )
	ROM_LOAD( "typ55s", 0x0000, 0x010000, CRC(05dc9b07) SHA1(9fc2c7575a704ca1252bb5c6a638e28b0324f2a6) )
	ROM_LOAD( "typ58s", 0x0000, 0x010000, CRC(56e26a42) SHA1(7add260212d3fbc8b356b58e85df8cafbef151e3) )
	ROM_LOAD( "typ58t", 0x0000, 0x010000, CRC(3fbbbbc8) SHA1(9f097cbce3710a51c19ef7961f91ee6e77fc843f) )
	ROM_LOAD( "typ5p10p.bin", 0x0000, 0x010000, CRC(45ddeaf4) SHA1(6db822aac402cb6772718015420c14875e74b13d) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "m441.chr", 0x0000, 0x000048, CRC(3ec3a5fa) SHA1(ea8c831da9944506393dd5d5f380a084fd6543b6) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "tapsnd1.hex", 0x000000, 0x080000, CRC(8dc408e3) SHA1(48a9ffc5cf4fd04ed1320619ca915bbfa2406750) )
	ROM_LOAD( "tapsnd2.hex", 0x080000, 0x080000, CRC(6034e17a) SHA1(11e044c87b5fc6461b0c6cfac5c419daee930d7b) )
	ROM_LOAD( "typkp2", 0x080000, 0x080000, CRC(753d9bc1) SHA1(c27c8b7cfba7ad67685f637ee3f68a3edb7986e7) ) // alt copy of tapsnd2
ROM_END


ROM_START( m4tpcl )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "ctp12s.p1", 0x0000, 0x020000, CRC(5f0bbd2a) SHA1(ba1fa09ea7b4713a99b2033bdbbf6b15f973dcca) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
//  ROM_LOAD( "ctp1.2", 0x0000, 0x00df8f, CRC(5b32661f) SHA1(905eed53c50e54d46ead04277072dde5cbb40510) ) // just a zipped version of other file
//  ROM_LOAD( "ctp1.2s", 0x0000, 0x00e131, CRC(bca78748) SHA1(a6f03131dfaa4d7d7ee2f32b6fda7aa11dcf5dc9) ) // just a zipped version of other file
	ROM_LOAD( "ctp13d.p1", 0x0000, 0x020000, CRC(a0f081b9) SHA1(794bba6ed86c3f332165c4b3224315256c939926) )
	ROM_LOAD( "ctp13f.p1", 0x0000, 0x020000, CRC(dd7d94fd) SHA1(127ef8159facf647dff37109bcbb94311a8343f1) )
	ROM_LOAD( "ctp13s.p1", 0x0000, 0x020000, CRC(f0a69f92) SHA1(cd34fb26ecbe6a6e8602a8549c5f331a525567df) )
//  ROM_LOAD( "ntp0.2", 0x0000, 0x00d3b9, CRC(dfd82d90) SHA1(0c591780aed258627135c583c700b6d9a76908b5) )  // just a zipped version of other file
	ROM_LOAD( "ntp02.p1", 0x0000, 0x020000, CRC(6063e27d) SHA1(c99599fbc7146d8fcf62432994098dd51250b17b) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "ctp13s.chr", 0x0000, 0x000048, CRC(6b8772a9) SHA1(8b92686e675b00d2c2541dd7b8055c3145283bec) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "ctpsnd02.p1", 0x000000, 0x080000, CRC(6fdd5051) SHA1(3f713314b303d6e1f78e3ca050bed7a45f43d5b3) )
	ROM_LOAD( "ctpsnd02.p2", 0x080000, 0x080000, CRC(994bfb3a) SHA1(3cebfbbe77c4bbb5fb73e6d9b23f721b07c6435e) )
ROM_END


ROM_START( m4techno )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dte13.bin", 0x0000, 0x010000, CRC(cf661d06) SHA1(316b2c42e7253a03b2c12b713821045d9f95a8a7) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "dte13hack.bin", 0x0000, 0x010000, CRC(8b8eafe3) SHA1(93a7714eb4c749b7b19f4f844cf88da9443b0bb7) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "techno.bin", 0x0000, 0x080000, CRC(3e80f8bd) SHA1(2e3a195b49448da11cc0c089a8a9b462894c766b) )
ROM_END


ROM_START( m4toot )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "toc03c.p1", 0x0000, 0x020000, CRC(752ffa3f) SHA1(6cbe521ff85173159b6d34cc3e29a4192cd66394) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "toc03ad.p1", 0x0000, 0x020000, CRC(f67e53c1) SHA1(07a50fb649c5085a33f0a1a9b3d65b0b61a3f152) )
	ROM_LOAD( "toc03b.p1", 0x0000, 0x020000, CRC(4265472d) SHA1(01d5eb4e0a30abd1efed45658dcd8455494aabc4) )
	ROM_LOAD( "toc03bd.p1", 0x0000, 0x020000, CRC(7b64fd04) SHA1(377af32317d8356f06b19de553a13e8558993c34) )
	ROM_LOAD( "toc03d.p1", 0x0000, 0x020000, CRC(61f6b566) SHA1(2abd8092fe387c474ed10885a23ac242fa1462fa) )
	ROM_LOAD( "toc03dk.p1", 0x0000, 0x020000, CRC(31a35552) SHA1(3765fe6209d7b33afa1805ba56376e83e825165f) )
	ROM_LOAD( "toc03dr.p1", 0x0000, 0x020000, CRC(82acee63) SHA1(5a95425d6175c6496d745011d0ca3d744a027579) )
	ROM_LOAD( "toc03dy.p1", 0x0000, 0x020000, CRC(b64075ac) SHA1(ffdf8e45c2eab593570e15efd5161b67de5e4ecf) )
	ROM_LOAD( "toc03k.p1", 0x0000, 0x020000, CRC(08a2ef7b) SHA1(a4181db6280c7cc37b54baaf9cce1e61f61f3274) )
	ROM_LOAD( "toc03r.p1", 0x0000, 0x020000, CRC(bbad544a) SHA1(5fb31e5641a9e85147f5b61c5aba5a1ee7470f9c) )
	ROM_LOAD( "toc03s.p1", 0x0000, 0x020000, CRC(30feff92) SHA1(14397768ebd7469b4d1cff22ca9727f63608a98a) )
	ROM_LOAD( "toc03y.p1", 0x0000, 0x020000, CRC(8f41cf85) SHA1(315b359d6d1a9f6ad939be1fc5e4d8f21f998fb8) )
	ROM_LOAD( "toc04ad.p1", 0x0000, 0x020000, CRC(59075e2e) SHA1(a3ad5c642fb9cebcce2fb6c1e65514f2414948e0) )
	ROM_LOAD( "toc04b.p1", 0x0000, 0x020000, CRC(b2d54721) SHA1(4c72d434c0f4f37b9a3f08a760c7fe3851717059) )
	ROM_LOAD( "toc04bd.p1", 0x0000, 0x020000, CRC(d41df0eb) SHA1(e5a04c728a2893073ff8b5f6efd7cffd433a2985) )
	ROM_LOAD( "toc04c.p1", 0x0000, 0x020000, CRC(859ffa33) SHA1(05b7bd3b87a0ebcc78de751766cfcdc4276035ac) )
	ROM_LOAD( "toc04d.p1", 0x0000, 0x020000, CRC(9146b56a) SHA1(04bcd265d83e3554aef2de05aab9c3869bb966ea) )
	ROM_LOAD( "toc04dk.p1", 0x0000, 0x020000, CRC(9eda58bd) SHA1(5e38f87a162d1cb37e74850af6a00ae81619ecbe) )
	ROM_LOAD( "toc04dr.p1", 0x0000, 0x020000, CRC(2dd5e38c) SHA1(a1b0e8d48e164ab91b277a7efedf5b9fc73fc266) )
	ROM_LOAD( "toc04dy.p1", 0x0000, 0x020000, CRC(19397843) SHA1(e7e3299d8e46c79d3cd0ea7fd639a1d649a806df) )
	ROM_LOAD( "toc04k.p1", 0x0000, 0x020000, CRC(f812ef77) SHA1(d465b771efed27a9f616052d3fcabdbeb7c2d151) )
	ROM_LOAD( "toc04r.p1", 0x0000, 0x020000, CRC(4b1d5446) SHA1(f4bac0c8257add41295679b3541d2064d8c772c2) )
	ROM_LOAD( "toc04s.p1", 0x0000, 0x020000, CRC(295e6fff) SHA1(a21d991f00f144e12de60b891e3e2e5dd7d08d71) )
	ROM_LOAD( "toc04y.p1", 0x0000, 0x020000, CRC(7ff1cf89) SHA1(d4ab56b2b5b05643cd077b8d596b6cddf8a25134) )
	ROM_LOAD( "tot05ad.p1", 0x0000, 0x020000, CRC(fce00fcc) SHA1(7d10c0b83d782a9e603522ed039089866d931474) )
	ROM_LOAD( "tot05b.p1", 0x0000, 0x020000, CRC(594d551c) SHA1(3fcec5f41cfbea497aa53af4570664265774d1aa) )
	ROM_LOAD( "tot05bd.p1", 0x0000, 0x020000, CRC(71faa109) SHA1(8bec4a03e2dc43656c910652cf10d1afdf0bab33) )
	ROM_LOAD( "tot05c.p1", 0x0000, 0x020000, CRC(6e07e80e) SHA1(087a51da5578c326d7d9716c61b454be5e091761) )
	ROM_LOAD( "tot05d.p1", 0x0000, 0x020000, CRC(24565e6a) SHA1(0f2c19e54e5a78ae10e786d49ebcd7c16e41850c) )
	ROM_LOAD( "tot05dk.p1", 0x0000, 0x020000, CRC(3b3d095f) SHA1(326574adaa285a479abb5ae7515ef7d6bdd64126) )
	ROM_LOAD( "tot05dr.p1", 0x0000, 0x020000, CRC(8832b26e) SHA1(d5414560245bbb2c070f7a035e5e3416617c2cf3) )
	ROM_LOAD( "tot05dy.p1", 0x0000, 0x020000, CRC(bcde29a1) SHA1(cf8af40faf81a2a3141d3addd6b28c917beeda49) )
	ROM_LOAD( "tot05k.p1", 0x0000, 0x020000, CRC(138afd4a) SHA1(bc53d71d926da7aca74c79f45c47610e62e347b6) )
	ROM_LOAD( "tot05r.p1", 0x0000, 0x020000, CRC(a085467b) SHA1(de2deda7635c9565db0f69aa6f375216ed36b7bb) )
	ROM_LOAD( "tot05s.p1", 0x0000, 0x020000, CRC(7dd1cfa8) SHA1(3bd0eeb621cc81ac462a6981e081837985f6635b) )
	ROM_LOAD( "tot05y.p1", 0x0000, 0x020000, CRC(9469ddb4) SHA1(553812e3ece921d31c585b6a412c00ea5095b1b0) )
	ROM_LOAD( "tot06ad.p1", 0x0000, 0x020000, CRC(ebe50569) SHA1(1906f1a8d47cc9ee3fa703ad57b180f8a4cdcf89) )
	ROM_LOAD( "tot06b.p1", 0x0000, 0x020000, CRC(4d5a8ebe) SHA1(d30da9ce729fed7ad42b30d522b2b6d65a462b84) )
	ROM_LOAD( "tot06bd.p1", 0x0000, 0x020000, CRC(66ffabac) SHA1(25822e42a58173b8f51dcbbd98d041b261e675e4) )
	ROM_LOAD( "tot06c.p1", 0x0000, 0x020000, CRC(7a1033ac) SHA1(9d3c2f521574f405e1da81b605581bc4b6f011a4) )
	ROM_LOAD( "tot06d.p1", 0x0000, 0x020000, CRC(304185c8) SHA1(f7ef4fce3ce9a455f39766ae97ebfbf93418e019) )
	ROM_LOAD( "tot06dk.p1", 0x0000, 0x020000, CRC(2c3803fa) SHA1(68c83ccdd1f776376608918d9a1257fe64ce3a9b) )
	ROM_LOAD( "tot06dr.p1", 0x0000, 0x020000, CRC(9f37b8cb) SHA1(3eb2f843dc4f87b7bfe16da1d133750ad7075a71) )
	ROM_LOAD( "tot06dy.p1", 0x0000, 0x020000, CRC(abdb2304) SHA1(1c29f4176306f472323388f8c34b102930fb9f5f) )
	ROM_LOAD( "tot06k.p1", 0x0000, 0x020000, CRC(079d26e8) SHA1(60e3d02d62f6fde6bdf4b9e77702549d493ccf09) )
	ROM_LOAD( "tot06r.p1", 0x0000, 0x020000, CRC(b4929dd9) SHA1(fa3d99b8f6344c9511ecc864d4fff4629b105b5f) )
	ROM_LOAD( "tot06s.p1", 0x0000, 0x020000, CRC(c6140fea) SHA1(c2257dd84bf97b71580e8b873fc745dfa456ddd9) )
	ROM_LOAD( "tot06y.p1", 0x0000, 0x020000, CRC(807e0616) SHA1(2a3f89239a7fa43dfde90dd7ad929747e888074b) )
	ROM_LOAD( "tten2010", 0x0000, 0x020000, CRC(28373e9a) SHA1(496df7b511b950b5affe9d65c80037f3ecddc5f8) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tenoutten.chr", 0x0000, 0x000048, CRC(f94b32a4) SHA1(7d7fdf7612dab684b549c6fc99f11185056b8c3e) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "totsnd.p1", 0x0000, 0x080000, CRC(684e9eb1) SHA1(8af28de879ae41efa07dfb07ecbd6c72201749a7) )

	ROM_LOAD( "tocsnd.p1", 0x000000, 0x080000, CRC(b9527b0e) SHA1(4dc5f6794c3e63c8faced34e166dcc748ffb4941) )
	ROM_LOAD( "tocsnd.p2", 0x080000, 0x080000, CRC(f684a488) SHA1(7c93cda3d3b55d9818625f696798c7c2cde79fa8) )

ROM_END


ROM_START( m4ttdia )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "tda04s.p1", 0x0000, 0x020000, CRC(1240642e) SHA1(7eaf02d5c00707a0a6d98d247c293cad1ca87108) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "tda04ad.p1", 0x0000, 0x020000, CRC(79d804ba) SHA1(0616a2718aea85692ce5c5086f18e54a531efb19) )
	ROM_LOAD( "tda04b.p1", 0x0000, 0x020000, CRC(dc755e6a) SHA1(386a1baf7d86d73dff1d6034f60094a55255d6bc) )
	ROM_LOAD( "tda04bd.p1", 0x0000, 0x020000, CRC(f4c2aa7f) SHA1(aae114e9ab813809a8f8e2e12773a9f6379f535d) )
	ROM_LOAD( "tda04c.p1", 0x0000, 0x020000, CRC(eb3fe378) SHA1(d805cc596dc5c4dd9d0ee5d7c741736698f7db36) )
	ROM_LOAD( "tda04d.p1", 0x0000, 0x020000, CRC(a16e551c) SHA1(7aaf5355e3f454523363f78d32b7ff80bac45268) )
	ROM_LOAD( "tda04dh.p1", 0x0000, 0x020000, CRC(3a2efa3c) SHA1(e7826f2cc7e7b853cbb9a216212e60be29a89a90) )
	ROM_LOAD( "tda04dk.p1", 0x0000, 0x020000, CRC(be050229) SHA1(a4a66f87430a03b1f3b1638d3e45786a64f8db0f) )
	ROM_LOAD( "tda04dr.p1", 0x0000, 0x020000, CRC(0d0ab918) SHA1(41f8ac5a59b2600f71f2d5498a20d2ac0f6c7ffa) )
	ROM_LOAD( "tda04dy.p1", 0x0000, 0x020000, CRC(39e622d7) SHA1(b8525ffa91b29fe71c452e5642a5eb9432c0fa63) )
	ROM_LOAD( "tda04h.p1", 0x0000, 0x020000, CRC(12990e29) SHA1(b61a5efa34e3d3bff80272f2b3008d15ada24179) )
	ROM_LOAD( "tda04k.p1", 0x0000, 0x020000, CRC(96b2f63c) SHA1(7cb76b7dfc34d5967ba82c01d8d0b0f8a62a2c93) )
	ROM_LOAD( "tda04r.p1", 0x0000, 0x020000, CRC(25bd4d0d) SHA1(967949e7ce73ba7064f3d75333091140236629c7) )
	ROM_LOAD( "tda04y.p1", 0x0000, 0x020000, CRC(1151d6c2) SHA1(0048447537061d15c4173366ac1b431e4eef4d57) )

	ROM_REGION( 0x100000, "msm6376", 0 )
//  ROM_LOAD( "tda0.1 snd p1.bin", 0x0000, 0x080000, CRC(6e0bf4ab) SHA1(0cbcdc11d2d64a5fda2cf40bdde850f5c7b56d12) )
	ROM_LOAD( "tdasnd.p1", 0x000000, 0x080000, CRC(6e0bf4ab) SHA1(0cbcdc11d2d64a5fda2cf40bdde850f5c7b56d12) )
	ROM_LOAD( "tdasnd.p2", 0x080000, 0x080000, CRC(66cc2f87) SHA1(6d8af6090b2ab29039aa89a125512190e7e34a03) )
ROM_END




ROM_START( m4tiktak )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tictak.bin", 0x0000, 0x010000, CRC(00c15733) SHA1(e8e7ef0de7266f910e96248ea923030e7b507fec) )
ROM_END


ROM_START( m4toma )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dtk23.bin", 0x0000, 0x010000, CRC(ffba2b96) SHA1(c7635023ac5181e661e808c6b44ac1add58f4f56) )
ROM_END


ROM_START( m4topact )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dta22p.bin", 0x0000, 0x010000, CRC(e90697d6) SHA1(e92d94f30ecb474d2e04b81ed78dbe99f8ae32f5) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "dta22j.bin", 0x0000, 0x010000, CRC(42375128) SHA1(82cb16cd64726b7eca5a9cad29d8f630a5af6a6b) )
ROM_END


ROM_START( m4topdk )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dtd26pj.bin", 0x0000, 0x010000, CRC(1f84d995) SHA1(7412632cf79008b980e48f14aea89c3f8d742ed2) )
ROM_END


ROM_START( m4topgr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "topgear.hex", 0x6000, 0x00a000, CRC(539cc3d7) SHA1(7d5c9eccd2d929189e8d82783fc630b2f3cacd24) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "topgear.chr", 0x0000, 0x000048, CRC(88578212) SHA1(c6c451b835c465e13de22bbe0dd472dbd2f8f504) )
ROM_END


ROM_START( m4toprn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "toprun_v1_1.bin", 0xc000, 0x004000, CRC(9b924324) SHA1(7b155467f30cc22f7cda301ae770fb2a889c9c66) )
	ROM_LOAD( "toprun_v1_2.bin", 0x8000, 0x004000, CRC(940fafa9) SHA1(2a8b669c51c8df50710bd8b552ab30a5d1a136ab) )
ROM_END


ROM_START( m4topst )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tsp0.5.bin", 0x0000, 0x010000, CRC(d77dc5c5) SHA1(cadec7070e6e7fcca769b6c94a60c711e445eba1) )
ROM_END


ROM_START( m4toptak )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "toptake.bin", 0x8000, 0x008000, CRC(52e74d55) SHA1(72c14474b9c799d682ccd8bc20af86ce7dd6be64) )
ROM_END


ROM_START( m4topten )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "tts04s.p1", 0x0000, 0x020000, CRC(5e53f04f) SHA1(d49377966ed787cc3571eadff8c4c16fac74434c) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "topt15g", 0x0000, 0x020000, CRC(4bd34f0b) SHA1(e513582ec0579e2d030a82735284ca62ee8fedf9) )
	ROM_LOAD( "topt15t", 0x0000, 0x020000, CRC(5c0f6549) SHA1(c20c0beccf23d49633127da37536f81186861c28) )
	ROM_LOAD( "tot15g", 0x0000, 0x020000, CRC(1f9508ad) SHA1(33089ea05f6adecf8f4004aa1e4d626969b6ac3a) )
	ROM_LOAD( "tot15t", 0x0000, 0x020000, CRC(1ce7f467) SHA1(cf47a126500680107a2f31743c3fff8290b595b8) )
	ROM_LOAD( "tth10ad.p1", 0x0000, 0x020000, CRC(cd15be85) SHA1(fc2aeabbb8524a7225322c17c4f1a32d2a17bcd0) )
	ROM_LOAD( "tth10b.p1", 0x0000, 0x020000, CRC(52d17d33) SHA1(0da24e685b1a7c8ee8899a17a87a55c3d7916608) )
	ROM_LOAD( "tth10bd.p1", 0x0000, 0x020000, CRC(400f1040) SHA1(4485791bb962d8f13bcc87a10dccf1fd096f31d5) )
	ROM_LOAD( "tth10c.p1", 0x0000, 0x020000, CRC(659bc021) SHA1(12e99ef185ac75ecab0604659085fee344e2d798) )
	ROM_LOAD( "tth10d.p1", 0x0000, 0x020000, CRC(2fca7645) SHA1(84237e9d72180f39f273540e1df711f9cb282afd) )
	ROM_LOAD( "tth10dh.p1", 0x0000, 0x020000, CRC(29cf8d8c) SHA1(b5fe04e5dd1417cda8665d2593809ae1b49bf4be) )
	ROM_LOAD( "tth10dk.p1", 0x0000, 0x020000, CRC(0ac8b816) SHA1(d8b7d4e467b48b4998cbc7bf02a7adb8307d1594) )
	ROM_LOAD( "tth10dr.p1", 0x0000, 0x020000, CRC(b9c70327) SHA1(5c4ba29b11f317677bea0b588975e1ce8f9c66f2) )
	ROM_LOAD( "tth10dy.p1", 0x0000, 0x020000, CRC(8d2b98e8) SHA1(3dc25a4d90407b3b38a1717d4a67d5ef5f815417) )
	ROM_LOAD( "tth10h.p1", 0x0000, 0x020000, CRC(3b11e0ff) SHA1(9aba4f17235876aeab71fcc46a4f15e58c87aa42) )
	ROM_LOAD( "tth10k.p1", 0x0000, 0x020000, CRC(1816d565) SHA1(78d65f8cd5dfba8bfca732295fab12d408a4a8a0) )
	ROM_LOAD( "tth10r.p1", 0x0000, 0x020000, CRC(ab196e54) SHA1(d55b8cb3acd961e4fd8ace7590be7ce0ae5babfc) )
	ROM_LOAD( "tth10s.p1", 0x0000, 0x020000, CRC(046f5357) SHA1(ddcf7ff7d113b2bbf2106095c9166a678b00ad06) )
	ROM_LOAD( "tth10y.p1", 0x0000, 0x020000, CRC(9ff5f59b) SHA1(a51f5f9bc90bfe89efd2cb39f32a626831c22056) )
	ROM_LOAD( "tth12ad.p1", 0x0000, 0x020000, CRC(08e54740) SHA1(c86e36eb16d6031017a9a309ae0ae627c855b75a) )
	ROM_LOAD( "tth12b.p1", 0x0000, 0x020000, CRC(dc787847) SHA1(0729350c65f4363b04aedbae214aca9f54b22b36) )
	ROM_LOAD( "tth12bd.p1", 0x0000, 0x020000, CRC(85ffe985) SHA1(bf0eba40b0f74f77b7625fbbe6fa0382f089fd9d) )
	ROM_LOAD( "tth12c.p1", 0x0000, 0x020000, CRC(eb32c555) SHA1(aa5c36d1c6d3f5f198d38705742a0a6a44b745bb) )
	ROM_LOAD( "tth12d.p1", 0x0000, 0x020000, CRC(a1637331) SHA1(b5787e5f099375db689f8815493d1b6c9de5ee1e) )
	ROM_LOAD( "tth12dh.p1", 0x0000, 0x020000, CRC(ec3f7449) SHA1(1db5e83734342c4c431614001fe06a8d8632242b) )
	ROM_LOAD( "tth12dk.p1", 0x0000, 0x020000, CRC(cf3841d3) SHA1(e2629b9f06c39b2c7e1b321ab262e61d64c706b1) )
	ROM_LOAD( "tth12dr.p1", 0x0000, 0x020000, CRC(7c37fae2) SHA1(e16eb1297ec725a879c3339d17ae2d8029646375) )
	ROM_LOAD( "tth12dy.p1", 0x0000, 0x020000, CRC(48db612d) SHA1(6ab67832ad61c4b0192b8e3a282238981a730aba) )
	ROM_LOAD( "tth12h.p1", 0x0000, 0x020000, CRC(b5b8e58b) SHA1(d1dd2ce68c657089267f78622ab5ecc34d7c306e) )
	ROM_LOAD( "tth12k.p1", 0x0000, 0x020000, CRC(96bfd011) SHA1(38a1473d61eaec3d9d2bbb6486a8cb1d81d03b9f) )
	ROM_LOAD( "tth12r.p1", 0x0000, 0x020000, CRC(25b06b20) SHA1(21b1566424fd68ab90a0ba11cb2b61fc6131256c) )
	ROM_LOAD( "tth12s.p1", 0x0000, 0x020000, CRC(d204097c) SHA1(d06c0e1c8d3da373772723c580977aefdd7224b3) )
	ROM_LOAD( "tth12y.p1", 0x0000, 0x020000, CRC(115cf0ef) SHA1(b13122a69d16200a587c8cb6328fe7cd89897261) )
	ROM_LOAD( "tts02ad.p1", 0x0000, 0x020000, CRC(afba21a4) SHA1(6394014f5d46df96d6c7cd840fec996a6d5ffee5) )
	ROM_LOAD( "tts02b.p1", 0x0000, 0x020000, CRC(ef4e080d) SHA1(a82940e58537d0c40f97c43aec470d68e9b344e8) )
	ROM_LOAD( "tts02bd.p1", 0x0000, 0x020000, CRC(22a08f61) SHA1(5a28d4f3cf89368a1cfa0cf5df1a9050f27f7e05) )
	ROM_LOAD( "tts02c.p1", 0x0000, 0x020000, CRC(d804b51f) SHA1(b6f18c7855f11978c408de3ec799859d8a534c93) )
	ROM_LOAD( "tts02d.p1", 0x0000, 0x020000, CRC(9255037b) SHA1(f470f5d089a598bc5da6329caa38f87974bd2984) )
	ROM_LOAD( "tts02dh.p1", 0x0000, 0x020000, CRC(ec4cdf22) SHA1(d3af8a72ce2740461c4528328b04084924e12832) )
	ROM_LOAD( "tts02dk.p1", 0x0000, 0x020000, CRC(68672737) SHA1(79e5d15a62a6fcb90c3949e3676276b49167e353) )
	ROM_LOAD( "tts02dr.p1", 0x0000, 0x020000, CRC(db689c06) SHA1(6c333c1fce6ccd0a34f11c11e3b826488e3ea663) )
	ROM_LOAD( "tts02dy.p1", 0x0000, 0x020000, CRC(ef8407c9) SHA1(ecd3704d995d797d2c4a8c3aa729850ae4ccde56) )
	ROM_LOAD( "tts02h.p1", 0x0000, 0x020000, CRC(21a2584e) SHA1(b098abf763af86ac691ccd1dcc3e0a4d92b0073d) )
	ROM_LOAD( "tts02k.p1", 0x0000, 0x020000, CRC(a589a05b) SHA1(30e1b3a7baddd7a69be7a9a01ee4a84937eaedbd) )
	ROM_LOAD( "tts02r.p1", 0x0000, 0x020000, CRC(16861b6a) SHA1(8275099a3abfa388e9568be5465abe0e31db320d) )
	ROM_LOAD( "tts02s.p1", 0x0000, 0x020000, CRC(3cd87ce5) SHA1(b6214953e5b9f655b413b008d61624acbc39d419) )
	ROM_LOAD( "tts02y.p1", 0x0000, 0x020000, CRC(226a80a5) SHA1(23b25dba28af1225a75e6f7c428c8576df4e8cb9) )
	ROM_LOAD( "tts04ad.p1", 0x0000, 0x020000, CRC(cdcc3d18) SHA1(4e9ccb8bfbe5b86731a24631cc60819919bb3ce8) )
	ROM_LOAD( "tts04b.p1", 0x0000, 0x020000, CRC(d0280881) SHA1(c2e416a224a7ed4cd9010a8e10b0aa5e808fbbb9) )
	ROM_LOAD( "tts04bd.p1", 0x0000, 0x020000, CRC(40d693dd) SHA1(fecbf86d6b533dd0721497cc689ab978c75d67e5) )
	ROM_LOAD( "tts04c.p1", 0x0000, 0x020000, CRC(e762b593) SHA1(7bcd65b747d12801430e783ead01c746fee3f371) )
	ROM_LOAD( "tts04d.p1", 0x0000, 0x020000, CRC(ad3303f7) SHA1(5df231e7d20bf21da56ce912b736fc570707a10f) )
	ROM_LOAD( "tts04dh.p1", 0x0000, 0x020000, CRC(8e3ac39e) SHA1(27ed795953247075089c1df8e577aa61ae64f59e) )
	ROM_LOAD( "tts04dk.p1", 0x0000, 0x020000, CRC(0a113b8b) SHA1(1282ad75537040fa84620f0871050762546b5a28) )
	ROM_LOAD( "tts04dr.p1", 0x0000, 0x020000, CRC(b91e80ba) SHA1(b7e082ea29d8558967564d057dfd4a48d0a997cc) )
	ROM_LOAD( "tts04dy.p1", 0x0000, 0x020000, CRC(8df21b75) SHA1(2de6bc76bae324e149fb9003eb8327f4a2db269b) )
	ROM_LOAD( "tts04h.p1", 0x0000, 0x020000, CRC(1ec458c2) SHA1(49a8e39a6506c8c5af3ad9eac47871d828611338) )
	ROM_LOAD( "tts04k.p1", 0x0000, 0x020000, CRC(9aefa0d7) SHA1(f90be825c58ac6e443822f7c8f5da74dcf18c652) )
	ROM_LOAD( "tts04r.p1", 0x0000, 0x020000, CRC(29e01be6) SHA1(59ee4baf1f48dbd703e94c7a8e45d841f196ec54) )
	ROM_LOAD( "tts04y.p1", 0x0000, 0x020000, CRC(1d0c8029) SHA1(9ddc7a3d92715bfd4b24470f3d5ba2d9047be967) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "tops1.hex", 0x000000, 0x080000, CRC(70f16892) SHA1(e6448831d3ce7fa251b40023bc7d5d6dee9d6793) )
	ROM_LOAD( "tops2.hex", 0x080000, 0x080000, CRC(5fc888b0) SHA1(8d50ee4f36bd36aed5d0e7a77f76bd6caffc6376) )
ROM_END

ROM_START( m4toptena )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "toptenner2_1.bin", 0x8000, 0x008000, CRC(0ffa15d3) SHA1(4c2e5762ddfc34e18d82019b6edfaf9a612719fb) )
	ROM_LOAD( "toptenner2_2.bin", 0x4000, 0x004000, CRC(b3b9a66b) SHA1(ca744e6481c47619abc3e133fafac6457df5b746) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "tp2_1.bin", 0x8000, 0x008000, CRC(0ffa15d3) SHA1(4c2e5762ddfc34e18d82019b6edfaf9a612719fb) )
	ROM_LOAD( "tp2_2.bin", 0x4000, 0x004000, CRC(b3b9a66b) SHA1(ca744e6481c47619abc3e133fafac6457df5b746) )
ROM_END

ROM_START( m4toplot )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "topthelot.bin", 0x8000, 0x008000, CRC(52051209) SHA1(e34720521bc9e391ffb3bcdae18ff9d6449d83bd) )
ROM_END


ROM_START( m4toptim )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "toptimer.bin", 0x0000, 0x010000, CRC(74804012) SHA1(0d9460ba6b1d359d358483c4e8bfd5518f364518) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "dtt2-1.bin", 0x0000, 0x010000, CRC(f9c84a34) SHA1(ad654442f580d6a49658f0e4e39bacbd9d0d0018) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "ttimer.chr", 0x0000, 0x000048, CRC(f694224e) SHA1(936ab5e349fa59accbb37959cce9519fd97f3978) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "toptimer-snd.bin", 0x0000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END


ROM_START( m4tricol )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dtc25.bin", 0x0000, 0x010000, CRC(d3318dde) SHA1(19194d206deee920a1b0122dddc3d5bc0a7a48c5) )
ROM_END


ROM_START( m4tribnk )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "triplebank_1.bin", 0x0000, 0x010000, CRC(efeb5810) SHA1(e882f3ce028dcd4705365e0c17dab8d5839e4901) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "trip.chr", 0x0000, 0x000048, CRC(7d9348ea) SHA1(c0b32a5c964e5d0a9e715a282c92f2ca61547c25) )
ROM_END


ROM_START( m4tridic )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "trp.bin", 0x0000, 0x010000, CRC(d91fb9b2) SHA1(a06a868a17f84e2a012b0fe28025458e4f899c1d) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tdice.chr", 0x0000, 0x000048, CRC(6d28754a) SHA1(beb7724e9f621d315b2f16abfd3bbc6a99077a05) )
ROM_END


ROM_START( m4tropcl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tro20.bin", 0x0000, 0x010000, CRC(5e86c3fc) SHA1(ce2419991559839a8875060c1afe0f030190010a) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "tr2d.p1", 0x0000, 0x010000, CRC(0cc23f89) SHA1(a66c8c28073f53381c43e3e597f15f81c5c61479) )
	ROM_LOAD( "tr2f.p1", 0x0000, 0x010000, CRC(fbdcd06f) SHA1(27ccdc83e60a62227d33d8cf3d516fc43908ab99) )
	ROM_LOAD( "tr2s.p1", 0x0000, 0x010000, CRC(6d43375c) SHA1(5be1dc85374c6a1235e0b137b46ebd7a2d7d922a) )
	ROM_LOAD( "trod.p1", 0x0000, 0x010000, CRC(60c84612) SHA1(84dc8b34e41436331832c1a32ddac0fce269488a) )
	ROM_LOAD( "tros.p1", 0x0000, 0x010000, CRC(5e86c3fc) SHA1(ce2419991559839a8875060c1afe0f030190010a) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tro20.chr", 0x0000, 0x000048, CRC(97618d38) SHA1(7958e99684d50b9bdb56c97f7fcfe161f0824578) )
ROM_END


ROM_START( m4tupen )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ctn.p1", 0xc000, 0x004000, CRC(bd44cf68) SHA1(6efc88987f330acfc44c248e71ec8acf1bc37228) )
	ROM_LOAD( "ctn.p2", 0x8000, 0x004000, CRC(afb93378) SHA1(974a2fe8ecac264d23cd7f4d8cb3fe77152485a0) )
ROM_END


ROM_START( m4tbplay )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dtp13", 0x0000, 0x010000, CRC(de424bc3) SHA1(c82dd56a0b3ccea78325cd90ed8e72ed68a1af77) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "rmtp4b", 0x0000, 0x010000, CRC(33a1764e) SHA1(7475f460dee015a2cd78fc3e0d1d14fd96fdbb9c) )
	ROM_LOAD( "trmyid", 0x0000, 0x010000, CRC(e7af5944) SHA1(64559c97375a3536f7929d7f4d8d19c30527a3ec) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "dtpchr.chr", 0x0000, 0x000048, CRC(7743df66) SHA1(69b1943837ccf8671861ac8ef690138b41de0e5b) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "dtps10_1", 0x000000, 0x080000, CRC(d1d2c981) SHA1(6a4940248b0bc8df0a9de0d60e98cfebf1962504) )
	ROM_LOAD( "dtps20_1", 0x080000, 0x080000, CRC(f77c4f39) SHA1(dc0e056f4d8c00824b3e672a02da64613bbf204e) )
ROM_END


ROM_START( m4tbreel )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dtr12.bin", 0x0000, 0x010000, CRC(cdb63ef5) SHA1(748cc06e6a274b125d189dd66f2adad8bd2fb166) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "dtr31.dat", 0x0000, 0x010000, CRC(cdb63ef5) SHA1(748cc06e6a274b125d189dd66f2adad8bd2fb166) )
	ROM_LOAD( "dtv30.dat", 0x0000, 0x010000, CRC(c314846c) SHA1(bfa6539b204477a04a5bbc8d13c3a666c52b597b) )
ROM_END


ROM_START( m4tbrldx )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dtv30.dat", 0x0000, 0x010000, CRC(c314846c) SHA1(bfa6539b204477a04a5bbc8d13c3a666c52b597b) )
ROM_END


ROM_START( m4tutfrt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tft04s.p1", 0x0000, 0x010000, CRC(c20c3589) SHA1(55d1bc5d5f4ae14acafb36bd640faaf4ffccc6eb) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ctuad.p1", 0x0000, 0x010000, CRC(0ec1661b) SHA1(162ddc30c341fd8eda8ce57a60edf06b4e39a24f) )
	ROM_LOAD( "ctub.p1", 0x0000, 0x010000, CRC(f4289621) SHA1(a4078552146c88c05845cbdcd551e4564840fea4) )
	ROM_LOAD( "ctubd.p1", 0x0000, 0x010000, CRC(38dd0b51) SHA1(04df9511f366cc575a1a06d3a5d60ec0245f64a7) )
	ROM_LOAD( "ctud.p1", 0x0000, 0x010000, CRC(6033fae5) SHA1(f5bdd1821344d4546eea8caa52d76e3bd509810e) )
	ROM_LOAD( "ctudk.p1", 0x0000, 0x010000, CRC(36dd1e41) SHA1(ad5ad7cae12634149d38e286e6873b81bda52871) )
	ROM_LOAD( "ctudy.p1", 0x0000, 0x010000, CRC(58c02db6) SHA1(faf85caeaa0678b5771d801cf3d9645d7767767c) )
	ROM_LOAD( "ctuk.p1", 0x0000, 0x010000, CRC(4c247447) SHA1(f5aebb4a75632c9a74dca1f3e9559399c89ac679) )
	ROM_LOAD( "ctur.p1", 0x0000, 0x010000, CRC(705a2b52) SHA1(40b0738146d073f93877a15f63830ff3e07814c1) )
	ROM_LOAD( "ctus.p1", 0x0000, 0x010000, CRC(1b282170) SHA1(e3082aed6e96587de56c5593d32d0129c47fe667) )
	ROM_LOAD( "ctuy.p1", 0x0000, 0x010000, CRC(ed3103bc) SHA1(eefb72728e026fad3dd031665510ee0aba23e14b) )
	ROM_LOAD( "f1u01ad.p1", 0x0000, 0x010000, CRC(7573d8cf) SHA1(fe1553ca8f588554fdd495dc2f048e50e00590bb) )
	ROM_LOAD( "f1u01b.p1", 0x0000, 0x010000, CRC(158d1a3a) SHA1(da80334e9982f778a908a6fe89a593863e7c763e) )
	ROM_LOAD( "f1u01bd.p1", 0x0000, 0x010000, CRC(9844e568) SHA1(a580176338cdeed5fb4d1744b537bde1f499293e) )
	ROM_LOAD( "f1u01c.p1", 0x0000, 0x010000, CRC(4709bd66) SHA1(c15f64767315ea0434a57b9e494a9e8090f1e05a) )
	ROM_LOAD( "f1u01d.p1", 0x0000, 0x010000, CRC(3a3c6745) SHA1(f270bccb4bdedb5cfaf0130da6e480dc31889682) )
	ROM_LOAD( "f1u01dk.p1", 0x0000, 0x010000, CRC(4fa79f23) SHA1(ce9a0815d96a94d564edf5a775af94ea10070ff5) )
	ROM_LOAD( "f1u01dr.p1", 0x0000, 0x010000, CRC(6fcc4d76) SHA1(27d8fdd5965ba565cb5b6113b7cba5e820650419) )
	ROM_LOAD( "f1u01dy.p1", 0x0000, 0x010000, CRC(cdd43fc2) SHA1(6f4da20de3040675592b4338a1d72654800c20eb) )
	ROM_LOAD( "f1u01k.p1", 0x0000, 0x010000, CRC(7e9c3110) SHA1(56ab6e5362ce8795c65d0cf11742e3ddb6d8b8a3) )
	ROM_LOAD( "f1u01r.p1", 0x0000, 0x010000, CRC(0e6b2132) SHA1(8757713677e2eb0400c69d3cdde6506662e0ef0b) )
	ROM_LOAD( "f1u01s.p1", 0x0000, 0x010000, CRC(d69668d2) SHA1(86ea656a3a4d4e6701c70b5e730ae8402cd70342) )
	ROM_LOAD( "f1u01y.p1", 0x0000, 0x010000, CRC(33e7d5fd) SHA1(96f53fbb228e98ce3a848b2c72bdb8876c9de160) )
	ROM_LOAD( "f3u01ad.p1", 0x0000, 0x010000, CRC(acb1bfb3) SHA1(8aa22c45d98ecec324fa031b46689496f9a2842c) )
	ROM_LOAD( "f3u01b.p1", 0x0000, 0x010000, CRC(a0d14e25) SHA1(16f2444334608702748a3b0b2556ac1a7760615a) )
	ROM_LOAD( "f3u01bd.p1", 0x0000, 0x010000, CRC(9aadd2f9) SHA1(4dbff4f6fd4d02778733eb846a354177f0e204a5) )
	ROM_LOAD( "f3u01c.p1", 0x0000, 0x010000, CRC(a3ad34d5) SHA1(e8c435f80b4fd3f7af16f341e107a85a33f1fe1c) )
	ROM_LOAD( "f3u01d.p1", 0x0000, 0x010000, CRC(c6790301) SHA1(fb0b619e75e1227f4d293b613e80d8d653517eec) )
	ROM_LOAD( "f3u01dk.p1", 0x0000, 0x010000, CRC(ee0554fe) SHA1(12cd26d6205fec35590fd23682c578f06466eb01) )
	ROM_LOAD( "f3u01dr.p1", 0x0000, 0x010000, CRC(32d761eb) SHA1(aa1098629d2a1c98c606a71a7cf0ae97f381aebe) )
	ROM_LOAD( "f3u01dy.p1", 0x0000, 0x010000, CRC(3ad66969) SHA1(4c79edc52095cfa1fae8215caaaaf434cd38199d) )
	ROM_LOAD( "f3u01k.p1", 0x0000, 0x010000, CRC(2b6c0f0f) SHA1(64e50adc6656225c9cdaaee64ae59cafcd1623ee) )
	ROM_LOAD( "f3u01r.p1", 0x0000, 0x010000, CRC(93cb1bfb) SHA1(e29439caed4a2f4512e50ff158427b61b5a9c4a9) )
	ROM_LOAD( "f3u01s.p1", 0x0000, 0x010000, CRC(dce2e5be) SHA1(3c218cdb939d5b7cc650c820737ae3ac653435ce) )
	ROM_LOAD( "f3u01y.p1", 0x0000, 0x010000, CRC(9aae0ca2) SHA1(83192225d886848ee0320973fb9dbd85cf9045b8) )
	ROM_LOAD( "tf4ad.p1", 0x0000, 0x010000, CRC(6ddc90a9) SHA1(76dd22c5e65fc46360123e200016d11a8946d2f3) )
	ROM_LOAD( "tf4b.p1", 0x0000, 0x010000, CRC(c3a70eac) SHA1(ea5a39e33af96e84ce0ea184850d5f580dbf19ce) )
	ROM_LOAD( "tf4bd.p1", 0x0000, 0x010000, CRC(54ae2498) SHA1(54a63a0de794eb2ce321f79b09a56485d9e77715) )
	ROM_LOAD( "tf4d.p1", 0x0000, 0x010000, CRC(d8ff9045) SHA1(ae7307212614c6f1b4e3d72d3a1ae68ca1d0b470) )
	ROM_LOAD( "tf4dk.p1", 0x0000, 0x010000, CRC(a2e3b67f) SHA1(dea9958caba08b5cdec6eec9e4c17038ecb0ca55) )
	ROM_LOAD( "tf4dy.p1", 0x0000, 0x010000, CRC(ff4f26c4) SHA1(21ef226bf92deeab15c9368d707bf75b7104e7c3) )
	ROM_LOAD( "tf4k.p1", 0x0000, 0x010000, CRC(1a4eb247) SHA1(f6b4c85dd8b155b672bd96ea7ee6630df773c6ca) )
	ROM_LOAD( "tf4s.p1", 0x0000, 0x010000, CRC(2d298c58) SHA1(568c2babdb002da871df7a36d16e4f7810cac265) )
	ROM_LOAD( "tf4y.p1", 0x0000, 0x010000, CRC(06cd8b06) SHA1(92205e9edd42f80de67d5d6652de8ea80bc60af7) )
	ROM_LOAD( "tfruity.hex", 0x0000, 0x010000, CRC(dce2e5be) SHA1(3c218cdb939d5b7cc650c820737ae3ac653435ce) )
	ROM_LOAD( "tft04ad.p1", 0x0000, 0x010000, CRC(2994aa14) SHA1(af0e618f24cdedd14e3a347701313360d9fc73d1) )
	ROM_LOAD( "tft04b.p1", 0x0000, 0x010000, CRC(e95eab06) SHA1(70e85e38493ac1fd30a79582bab45af5227d835a) )
	ROM_LOAD( "tft04bd.p1", 0x0000, 0x010000, CRC(060d3572) SHA1(e78b6248d3aef6cd08f4b30e0b00bd4cf254e630) )
	ROM_LOAD( "tft04c.p1", 0x0000, 0x010000, CRC(3499fe77) SHA1(3f82ca6d856bddf82581790c46abf725963335a0) )
	ROM_LOAD( "tft04d.p1", 0x0000, 0x010000, CRC(10626059) SHA1(c7b2fd2b65946fe82950ff506a56bd08b7c2ef71) )
	ROM_LOAD( "tft04dk.p1", 0x0000, 0x010000, CRC(40700fe2) SHA1(1f121adae094c2d11a66b5e8ae4b026e85fc7f73) )
	ROM_LOAD( "tft04dr.p1", 0x0000, 0x010000, CRC(feeb4417) SHA1(e2f2c55c48067ad67188ff5a75caa08d8726cb77) )
	ROM_LOAD( "tft04dy.p1", 0x0000, 0x010000, CRC(63806cf9) SHA1(850c707c65b8dba6b6914389d573a8b7b7b12cdb) )
	ROM_LOAD( "tft04k.p1", 0x0000, 0x010000, CRC(ffbf53e1) SHA1(a003bb5d94b43d6ae9b45c599cccb0006bd8a89a) )
	ROM_LOAD( "tft04r.p1", 0x0000, 0x010000, CRC(cbf79555) SHA1(0aacb3f28984637919294a18f40858e8f46a18b3) )
	ROM_LOAD( "tft04y.p1", 0x0000, 0x010000, CRC(569cbdbb) SHA1(8a978dfba876e5a2e12226f5fe55c29b5f079fad) )
	ROM_LOAD( "tut25.bin", 0x0000, 0x010000, CRC(c98fb5bb) SHA1(1a3bc343a38b5978a919b454e9a2e806dce7a78a) )
	ROM_LOAD( "tut25patched.bin", 0x0000, 0x010000, CRC(b4443cf5) SHA1(e79ec52730146f1591140555b814cbd20b5dfe78) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "tu_05___.1a3", 0x0000, 0x010000, CRC(97acc82d) SHA1(be53e60cb8a33b91a7f5556715ab4befe7170dd2) )
	ROM_LOAD( "tu_05_d_.1a3", 0x0000, 0x010000, CRC(33bb3018) SHA1(2c2f49c31919682ac03e61a665ce15d835e22467) )
	ROM_LOAD( "tu_10___.1a3", 0x0000, 0x010000, CRC(7878827f) SHA1(ac692ae50e63e632d45e7240c2520df83d2baaf5) )
	ROM_LOAD( "tu_20___.1a3", 0x0000, 0x010000, CRC(cada1c42) SHA1(6a4048da89a0bffeebfd21549c2d9812cc275bd5) )
	ROM_LOAD( "tu_20_b_.1a3", 0x0000, 0x010000, CRC(a8f1bc11) SHA1(03596171540e6490133f374cca69f4fd0359952e) )
	ROM_LOAD( "tu_20_d_.1a3", 0x0000, 0x010000, CRC(6ecde477) SHA1(694296eb226c59069800d6936c9dee2623105db0) )
	ROM_LOAD( "tu_20_k_.1a3", 0x0000, 0x010000, CRC(0ce64424) SHA1(7415c9de9982aa7f15f71ef791cbd8ad5a9331d3) )
	ROM_LOAD( "tu_20bg_.1a3", 0x0000, 0x010000, CRC(31a6196d) SHA1(1113737dd3b209afda14ec273d923e2057ea7d99) )
	ROM_LOAD( "tuf20__1.0", 0x0000, 0x010000, CRC(ddadbcb6) SHA1(2d2934ec73d979de45d0998f8975361d33358dd3) )
	ROM_LOAD( "tuf20ad1.0", 0x0000, 0x010000, CRC(5a74ead3) SHA1(3216c8d0c67aaeb18f791a6e1f3f6e30145d6beb) )
	ROM_LOAD( "tui05___.1a3", 0x0000, 0x010000, CRC(42e3d400) SHA1(4cf914141dfc1f88704403b467176da77369da06) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "tri98.chr", 0x0000, 0x000048, CRC(8a4532a8) SHA1(c128fd513bbcba68a1c75a11e09a54ba1d23d6f4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "tftsnd02.p1", 0x000000, 0x080000, CRC(9789e60f) SHA1(7299eb4b6bb2fc90e8a36859102aad5daf66b163) )
	ROM_LOAD( "tftsnd02.p2", 0x080000, 0x080000, CRC(0bdc1dc9) SHA1(909af8ff4d0e3e36e280e9553a73bb1dfdb62144) )

	ROM_LOAD( "tfsnd1.hex", 0x000000, 0x080000, CRC(a5b623fa) SHA1(eb4d84a7d3977ddea42c4995dddaabace73e6f8a) )
	ROM_LOAD( "tfsnd2.hex", 0x080000, 0x080000, CRC(1275e528) SHA1(0c3c901cb2be1e84dba123677205108cf0388343) )
ROM_END


ROM_START( m4tutcl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "f2u01ad.p1", 0x0000, 0x010000, CRC(65537552) SHA1(b0a761dcc6e0a9f01cfb934b570356ca67fdd099) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "f2u01b.p1", 0x0000, 0x010000, CRC(2cae37df) SHA1(5aed985476b7b747a99a4046b846ee4a359776af) )
	ROM_LOAD( "f2u01bd.p1", 0x0000, 0x010000, CRC(0dd91ccf) SHA1(bcdfc39025d02e7a51f69757238dfa44fe9d3655) )
	ROM_LOAD( "f2u01c.p1", 0x0000, 0x010000, CRC(6b6d9bb9) SHA1(140e9cbb8b484116e5fb9a7670d41fb0bcb37ec0) )
	ROM_LOAD( "f2u01d.p1", 0x0000, 0x010000, CRC(b477a20d) SHA1(51daf5e61a2ebcb3cb9884421b9e8f32df51ec07) )
	ROM_LOAD( "f2u01dk.p1", 0x0000, 0x010000, CRC(ccd14dd3) SHA1(c93ff69e0534e8190c10e0c819ed439d4e61a472) )
	ROM_LOAD( "f2u01dr.p1", 0x0000, 0x010000, CRC(d4918506) SHA1(2081ead45ff744cafcf3c4164c86acf609e54632) )
	ROM_LOAD( "f2u01dy.p1", 0x0000, 0x010000, CRC(24dd0a73) SHA1(a75129e414dd8cbe5f6f44e39b1d3dc3d7dfafb2) )
	ROM_LOAD( "f2u01k.p1", 0x0000, 0x010000, CRC(b9cec403) SHA1(90a1f49202ea9b79e2ab097cf95cf94088c52926) )
	ROM_LOAD( "f2u01r.p1", 0x0000, 0x010000, CRC(471e39d7) SHA1(874db6f2d04ed0b2c6756efba5fa1140d2fbfc58) )
	ROM_LOAD( "f2u01s.p1", 0x0000, 0x010000, CRC(25b68f22) SHA1(7f484dbc841e1e87d9f5e322cf497b6b68e4a096) )
	ROM_LOAD( "f2u01y.p1", 0x0000, 0x010000, CRC(5a583a6f) SHA1(0421d079de12a7379c13832108e8608c9a01f41d) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? in above set? */
ROM_END


ROM_START( m421 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "twentyone.bin", 0x0000, 0x010000, CRC(243f3bc1) SHA1(141df3dcdd8d70ad26a76ec071e0cd927357ee6e) )
ROM_END


ROM_START( m4twilgt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dtl22.bin", 0x0000, 0x010000, CRC(ed4c8b6a) SHA1(7644046a273304104eaa6260f6cc75950592d4b6) )
ROM_END


ROM_START( m4twintm )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "d2t11.bin", 0x0000, 0x010000, CRC(6a76ac6f) SHA1(824912ff1fc3155d11d32b597be53481532fdf5e) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "m533.chr", 0x0000, 0x000048, CRC(b1d7e29b) SHA1(e8ef07f85780e24b5f406478de50287b740379d9) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "sdun01.bin", 0x0000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END


ROM_START( m4twist )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "twist_again_mk29-6", 0x8000, 0x008000, CRC(cb331bee) SHA1(a88099a3f35caf02925f1a3f548fbf65c11e3ec9) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "twistagain-98-mkii.bin", 0x0000, 0x008000, CRC(1cbc7b58) SHA1(eda998a64272fe6796243c2db48ef988b9668c35) )
	ROM_LOAD( "twistagain-mki-27.bin", 0x0000, 0x008000, CRC(357f7072) SHA1(8a23509fff79a83a819b27eff8de8db08c679e3f) )
ROM_END




ROM_START( m4univ )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dun20", 0x0000, 0x010000, CRC(6a845d4d) SHA1(82bfc3f3a0ede76a4d482efc71b0390610db7acf) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "dunchr.chr", 0x0000, 0x000048, CRC(f694224e) SHA1(936ab5e349fa59accbb37959cce9519fd97f3978) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "sdun01.bin", 0x0000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END


ROM_START( m4uuaw )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "uua21h.p1", 0x0000, 0x020000, CRC(199e6dae) SHA1(ecd95ba2c2255afbaa8df96d625a8bfc97e4d3bc) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "upa15g", 0x0000, 0x020000, CRC(d20b8b92) SHA1(6fcddc781c204dfd34de2c4e4ce0ec35fb3ec4e0) )
	ROM_LOAD( "upa15t", 0x0000, 0x020000, CRC(85e3e82a) SHA1(e90183fab082f159d76ea14da794d52ee6ab8200) )
	ROM_LOAD( "ups21ad.p1", 0x0000, 0x020000, CRC(c19fa891) SHA1(c2772ec20a65ce999d901e8c873ec687113b18d4) )
	ROM_LOAD( "ups21b.p1", 0x0000, 0x020000, CRC(01320407) SHA1(a3273c59733e42013c3448b2a5c7c575ec0182b9) )
	ROM_LOAD( "ups21bd.p1", 0x0000, 0x020000, CRC(2fa3cb8a) SHA1(8df994ce93fc6f0df27a6ee73676d9ee73593091) )
	ROM_LOAD( "ups21d.p1", 0x0000, 0x020000, CRC(2a8db1a6) SHA1(873ab3757920c9153c1542748a74b36ce5e190c2) )
	ROM_LOAD( "ups21dh.p1", 0x0000, 0x020000, CRC(7bcfbb46) SHA1(b93dfa71e3ec0ea96eaf2db4cd382b0a2852a1ff) )
	ROM_LOAD( "ups21dk.p1", 0x0000, 0x020000, CRC(0642ae02) SHA1(8898341f8dc4f4c8c45ce6d04a01bd919cb0548a) )
	ROM_LOAD( "ups21dr.p1", 0x0000, 0x020000, CRC(b54d1533) SHA1(1f9220342dcab675b04895f69a7ca75579ba729f) )
	ROM_LOAD( "ups21dy.p1", 0x0000, 0x020000, CRC(4c850654) SHA1(21f386060301adef646fe469c1fcfb002d3e3424) )
	ROM_LOAD( "ups21h.p1", 0x0000, 0x020000, CRC(555e74cb) SHA1(14246b54839eb334576a119d7c87901f3b2f25ad) )
	ROM_LOAD( "ups21k.p1", 0x0000, 0x020000, CRC(28d3618f) SHA1(186337119e4b663dadc129533ce8a913013390a9) )
	ROM_LOAD( "ups21r.p1", 0x0000, 0x020000, CRC(9bdcdabe) SHA1(db0bb90705abec92a220a3dbe0ea69266d5e0558) )
	ROM_LOAD( "ups21s.p1", 0x0000, 0x020000, CRC(c4a8a542) SHA1(61063d55c6017cf17d704df576cb62da5bd75820) )
	ROM_LOAD( "ups21y.p1", 0x0000, 0x020000, CRC(6214c9d9) SHA1(d25fecc9798e342207d358a54efad1908c0e2247) )
	ROM_LOAD( "ups22ad.p1", 0x0000, 0x020000, CRC(ee0f53a6) SHA1(eabe58efa82015eb2266a793853e8ade546d6da1) )
	ROM_LOAD( "ups22b.p1", 0x0000, 0x020000, CRC(e7dbf5ae) SHA1(0fbbc3da1af8b60993a7f6082bd5e96da21cd0b8) )
	ROM_LOAD( "ups22bd.p1", 0x0000, 0x020000, CRC(003330bd) SHA1(42ad6ddfd7639909151dcee5e40e82a23074fd59) )
	ROM_LOAD( "ups22d.p1", 0x0000, 0x020000, CRC(cc64400f) SHA1(0ff7858c637fbb43a7cd1313bbf046177e4b7761) )
	ROM_LOAD( "ups22dh.p1", 0x0000, 0x020000, CRC(545f4071) SHA1(3947499d78d31fb0b269a63a518790b503a97685) )
	ROM_LOAD( "ups22dk.p1", 0x0000, 0x020000, CRC(29d25535) SHA1(7f053741d12cce467dd437ea998064e13d1ca52b) )
	ROM_LOAD( "ups22dr.p1", 0x0000, 0x020000, CRC(9addee04) SHA1(45c15536c8846da825a994a667b6e46598c1642e) )
	ROM_LOAD( "ups22dy.p1", 0x0000, 0x020000, CRC(6315fd63) SHA1(9a5fcab51d4e94b96669149285dda28cd41020b8) )
	ROM_LOAD( "ups22h.p1", 0x0000, 0x020000, CRC(b3b78562) SHA1(3e75fa20156faa3d38c2b5ac824bffe47e72b7bc) )
	ROM_LOAD( "ups22k.p1", 0x0000, 0x020000, CRC(ce3a9026) SHA1(80977176c5bae809a564f4fc0e3d6370f91f829b) )
	ROM_LOAD( "ups22r.p1", 0x0000, 0x020000, CRC(7d352b17) SHA1(d2d1d016a587be318e9018eb1953e68fe83620df) )
	ROM_LOAD( "ups22s.p1", 0x0000, 0x020000, CRC(ac990aa9) SHA1(396c9eded9c18ab2bcb0f4066a890f6e239830f1) )
	ROM_LOAD( "ups22y.p1", 0x0000, 0x020000, CRC(84fd3870) SHA1(8d294ae1a92d1e99c4c3f17a2d77fe1d994b2c33) )
	ROM_LOAD( "uua21ad.p1", 0x0000, 0x020000, CRC(2a18c292) SHA1(5853cb069eb5caa23372e5dedd33868103125780) )
	ROM_LOAD( "uua21b.p1", 0x0000, 0x020000, CRC(d71cc3db) SHA1(7d783110341237769165a08fd86f597225f8d90c) )
	ROM_LOAD( "uua21bd.p1", 0x0000, 0x020000, CRC(f04323be) SHA1(194c996d4c8e2fed2bcb02e29423e68b53900a1f) )
	ROM_LOAD( "uua21d.p1", 0x0000, 0x020000, CRC(664da8c3) SHA1(c93b0b5e796fcde0850e2b3054f1db0417f4e9ed) )
	ROM_LOAD( "uua21dh.p1", 0x0000, 0x020000, CRC(3ec18dcb) SHA1(fb5fe8ba0b59a21401cf091f43ad9f2a4df3447c) )
	ROM_LOAD( "uua21dk.p1", 0x0000, 0x020000, CRC(84919e1c) SHA1(c7315a3d1985180ec5ae1f4e5c7f0c99c1e0bac4) )
	ROM_LOAD( "uua21dr.p1", 0x0000, 0x020000, CRC(434c988f) SHA1(eb6126048df5bc4ff98d9838a3bb6cf24a9ab895) )
	ROM_LOAD( "uua21dy.p1", 0x0000, 0x020000, CRC(098b30d9) SHA1(48daa77f3aafdcd52d7291cdda533e8a9428de0e) )
	ROM_LOAD( "uua21k.p1", 0x0000, 0x020000, CRC(a3ce7e79) SHA1(8670d2cb7281ccabc15c5288a3e0dd99cfc1ae36) )
	ROM_LOAD( "uua21r.p1", 0x0000, 0x020000, CRC(641378ea) SHA1(de0282af6a17c7fc16c7eca10e81ffb208675779) )
	ROM_LOAD( "uua21s.p1", 0x0000, 0x020000, CRC(27c46fcc) SHA1(68a03fcce5d8155d6c0115d813c17217c4120375) )
	ROM_LOAD( "uua21y.p1", 0x0000, 0x020000, CRC(2ed4d0bc) SHA1(ffb0585e729b389855d24015e1ef7582eab88d3e) )
	ROM_LOAD( "uua22ad.p1", 0x0000, 0x020000, CRC(b2ace4d5) SHA1(da02abe111fea3fbfb9495e9b447139cd67a61e0) )
	ROM_LOAD( "uua22b.p1", 0x0000, 0x020000, CRC(71a6374a) SHA1(c14ed22fceb83b5ac72021322c9b8bb3d5afeffb) )
	ROM_LOAD( "uua22bd.p1", 0x0000, 0x020000, CRC(68f705f9) SHA1(678ba97241f3dede96239265eed418d4717637a6) )
	ROM_LOAD( "uua22d.p1", 0x0000, 0x020000, CRC(c0f75c52) SHA1(a4e1e496b0cbb24767f017fbe228fbb8ab2bb907) )
	ROM_LOAD( "uua22dh.p1", 0x0000, 0x020000, CRC(a675ab8c) SHA1(37fb437b95f9ff50fe41ebce825e3dd1b361925e) )
	ROM_LOAD( "uua22dk.p1", 0x0000, 0x020000, CRC(1c25b85b) SHA1(b42cdae2e2c00644376eb5f0c5b7567d3811b162) )
	ROM_LOAD( "uua22dr.p1", 0x0000, 0x020000, CRC(dbf8bec8) SHA1(b3ac5ed5b8cbc0457e5dfadefcab563e3197b045) )
	ROM_LOAD( "uua22dy.p1", 0x0000, 0x020000, CRC(913f169e) SHA1(9f82f5d868a9be046ced838f8b53730140ed50b2) )
	ROM_LOAD( "uua22h.p1", 0x0000, 0x020000, CRC(bf24993f) SHA1(618d6d2f2b762d61eb58087a3597ffb709658631) )
	ROM_LOAD( "uua22k.p1", 0x0000, 0x020000, CRC(05748ae8) SHA1(d9aeee26c8471bb6ee58a4a838e5c9930da92725) )
	ROM_LOAD( "uua22r.p1", 0x0000, 0x020000, CRC(c2a98c7b) SHA1(115f7c7a4b9eab5f3270f43a2db7a320dfc4e223) )
	ROM_LOAD( "uua22s.p1", 0x0000, 0x020000, CRC(65f57c0c) SHA1(7b2526cdd1ec973a91bc7ade116e16e03b32596a) )
	ROM_LOAD( "uua22y.p1", 0x0000, 0x020000, CRC(886e242d) SHA1(4b49b70fc2635fcf7b538b35b42a358cf4dd60b3) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "uuasnd.p1", 0x00000, 0x080000, CRC(be1a1131) SHA1(b7f50d8db6b7d134757e0746e7d9faf9fd3a2c7e) )
	ROM_LOAD( "uuasnd.p2", 0x080000, 0x080000, CRC(c8492b3a) SHA1(d390e37f4a62869079bb38395a2e86a5ad24392f) )
ROM_END


ROM_START( m4vegastg )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "vs.p1", 0x0000, 0x020000, CRC(4099d572) SHA1(91a7c1575013e61c754b2c2cb841e7687b76d7f9) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "vssound.bin", 0x0000, 0x16ee37, CRC(456da6be) SHA1(f0e293f0a383878b581326f869c2e49bec61d0c5) )
ROM_END


ROM_START( m4vegast )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "uvsad.p1", 0x0000, 0x020000, CRC(f26d7fa8) SHA1(bb37be4a189bd38bd71afd836e94a55f9ef84ad4) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "uvsb.p1", 0x0000, 0x020000, CRC(32e017ff) SHA1(3e8aa863b85164ee9d535244bafb82b14ee19528) )
	ROM_LOAD( "uvsbd.p1", 0x0000, 0x020000, CRC(7f77d16d) SHA1(7f34a687877ca1d9257ee1c39ca5b3c44a42782e) )
	ROM_LOAD( "uvsc.p1", 0x0000, 0x020000, CRC(05aaaaed) SHA1(7eee93204467b9ecdff4b742a6e16306b83778ba) )
	ROM_LOAD( "uvsd.p1", 0x0000, 0x020000, CRC(4ffb1c89) SHA1(bff002aa62684de9bfe4a445cc6e72d58c0e29ee) )
	ROM_LOAD( "uvsdk.p1", 0x0000, 0x020000, CRC(35b0793b) SHA1(90ef897fcd9cfb48007e5788a4df02053e38430c) )
	ROM_LOAD( "uvsdy.p1", 0x0000, 0x020000, CRC(b25359c5) SHA1(da4aa9b5069db222e22f24cd78f641c70a015166) )
	ROM_LOAD( "uvsk.p1", 0x0000, 0x020000, CRC(7827bfa9) SHA1(720d9793e97f2e11c1c9b18e3b4fa6ec7e29250a) )
	ROM_LOAD( "uvss.p1", 0x0000, 0x020000, CRC(8b5b120f) SHA1(90749c4f986a248252661b8e4157871330673ecd) )
	ROM_LOAD( "uvsy.p1", 0x0000, 0x020000, CRC(ffc49f57) SHA1(fb64afa2fefb3ff1c0f9b71aa3d00e1a17903e84) )
	ROM_LOAD( "vsg04ad.p1", 0x0000, 0x020000, CRC(d63f8f24) SHA1(f3dcd908bceb5a508927a83d23e82577e8684240) )
	ROM_LOAD( "vsg04b.p1", 0x0000, 0x020000, CRC(4211e2bf) SHA1(5f634d074d0f95673f734c5600ac990fb7510bdc) )
	ROM_LOAD( "vsg04bd.p1", 0x0000, 0x020000, CRC(5b2521e1) SHA1(67d2496e7a52f9aa984d57a5b76f995506051a8c) )
	ROM_LOAD( "vsg04c.p1", 0x0000, 0x020000, CRC(755b5fad) SHA1(fd76ae19e3ed7ea8c138655bc45e35ab5e4947a9) )
	ROM_LOAD( "vsg04d.p1", 0x0000, 0x020000, CRC(3f0ae9c9) SHA1(ca3ce4651fe07559d64a4a15c987ba6a5d06cc2f) )
	ROM_LOAD( "vsg04dk.p1", 0x0000, 0x020000, CRC(11e289b7) SHA1(19a4498a85038d14c062843b86027b5bd587b750) )
	ROM_LOAD( "vsg04dr.p1", 0x0000, 0x020000, CRC(a2ed3286) SHA1(4a8260625281bb400e35365f34d9fc59cac53740) )
	ROM_LOAD( "vsg04dy.p1", 0x0000, 0x020000, CRC(9601a949) SHA1(39a06f671b8f817039b9861887dd9521e7f3acdd) )
	ROM_LOAD( "vsg04k.p1", 0x0000, 0x020000, CRC(08d64ae9) SHA1(5cfe1b2fe0933d06618a2c88e1a63224686e972f) )
	ROM_LOAD( "vsg04r.p1", 0x0000, 0x020000, CRC(bbd9f1d8) SHA1(22312ff72d5b2fbe6416a7e84435e1df456a3547) )
	ROM_LOAD( "vsg04s.p1", 0x0000, 0x020000, CRC(aff47295) SHA1(d249f280b721c96b7c36329e2c2bb955fa91aa59) )
	ROM_LOAD( "vsg04y.p1", 0x0000, 0x020000, CRC(8f356a17) SHA1(33ac5e8a455175471466f7c7f35c66f795067bf2) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "uvssnd.p1", 0x000000, 0x080000, CRC(04a47007) SHA1(cfe1f4aa9d29c784b2034c2daa09b8bd7181562e) )
	ROM_LOAD( "uvssnd.p2", 0x080000, 0x080000, CRC(3b35d824) SHA1(e4007d5d13898ed0f91cd270c75b5df8cc62e003) )
ROM_END


ROM_START( m4vivaes )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "5p5vivaespana6-0.bin", 0x0000, 0x010000, CRC(adf02a7b) SHA1(2c61e175b920a67098503eb4d80b07b828c9f91d) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ep8ad.p1", 0x0000, 0x010000, CRC(1591cc9b) SHA1(b7574b71955d7780f3f127670e458befad951383) )
	ROM_LOAD( "ep8b.p1", 0x0000, 0x010000, CRC(33b085b3) SHA1(5fc22ee8ae2d597392c82b09a830893bb04e1014) )
	ROM_LOAD( "ep8bd.p1", 0x0000, 0x010000, CRC(d1eedaac) SHA1(9773fbb9b15dbbe313d76b0746698fbc12e26dd2) )
	ROM_LOAD( "ep8c.p1", 0x0000, 0x010000, CRC(d2a8aaf5) SHA1(7aabe3e0522877700453068c30c74cbe2c058e9a) )
	ROM_LOAD( "ep8d.p1", 0x0000, 0x010000, CRC(06f87010) SHA1(636707d4077bee0ea2f221904fa0e187ea4a1e31) )
	ROM_LOAD( "ep8dk.p1", 0x0000, 0x010000, CRC(e87b56da) SHA1(f3de0ab0badc9bd14505822c63f110b9b2521d55) )
	ROM_LOAD( "ep8dy.p1", 0x0000, 0x010000, CRC(d20ec7ed) SHA1(dffd4fcaf360b2b9f4b7241fe80bb6ee983b6d57) )
	ROM_LOAD( "ep8k.p1", 0x0000, 0x010000, CRC(0a2509c5) SHA1(d0fd30953cbc36363a6d4941b4a0805f9663aebb) )
	ROM_LOAD( "ep8s.p1", 0x0000, 0x010000, CRC(51537f2d) SHA1(a837a525cd7da724f338c47e716be175c37070b0) )
	ROM_LOAD( "ep8y.p1", 0x0000, 0x010000, CRC(4cc454e4) SHA1(a08ec2a4a17600eba86300dcb6b150b1b5a7fc74) )
	ROM_LOAD( "espc.p1", 0x0000, 0x010000, CRC(9534d0d0) SHA1(8e4a1081821d472eb4d9aa01e38b6956a1388d28) )
	ROM_LOAD( "espd.p1", 0x0000, 0x010000, CRC(012fbc14) SHA1(5e4a1cd7989f804ac52c7cbf46d7f9c1d7200336) )
	ROM_LOAD( "espdy.p1", 0x0000, 0x010000, CRC(90efbb8e) SHA1(a7338c5d71719b86f524f35d7edd176f41383f15) )
	ROM_LOAD( "espk.p1", 0x0000, 0x010000, CRC(775a56d6) SHA1(b0e47b56315948a7162ae00c3f5197fbb7b81ec5) )
	ROM_LOAD( "esps.p1", 0x0000, 0x010000, CRC(0c83b014) SHA1(e7cc513b66534b4fec89170d7b739c99a1ba3831) )
	ROM_LOAD( "espy.p1", 0x0000, 0x010000, CRC(020aa8bb) SHA1(497dae13fe9f9eba624db907e9f4a5bef1584a64) )
	ROM_LOAD( "ve5ad.p1", 0x0000, 0x010000, CRC(c545d5f0) SHA1(6ad168d2c1f2da2fff85fe0e21a3191cba8f5838) )
	ROM_LOAD( "ve5b.p1", 0x0000, 0x010000, CRC(ed02fa94) SHA1(9980b2f78ea8f40715e77fd8fafe883739ac1165) )
	ROM_LOAD( "ve5bd.p1", 0x0000, 0x010000, CRC(fce73b5c) SHA1(35e635ade9b4a7a992c568e317190d12576f78c9) )
	ROM_LOAD( "ve5d.p1", 0x0000, 0x010000, CRC(e739556d) SHA1(0816aa256cf8ac253ff37999595e981e90874d39) )
	ROM_LOAD( "ve5dk.p1", 0x0000, 0x010000, CRC(64f174d0) SHA1(f51b28607715931a9d4c1c14fc71b4f8bb8e56fb) )
	ROM_LOAD( "ve5dy.p1", 0x0000, 0x010000, CRC(fe6339c6) SHA1(82f14d80e96b65eeea08f1029ffaebf2e505091e) )
	ROM_LOAD( "ve5k.p1", 0x0000, 0x010000, CRC(05428018) SHA1(b6884a1bfd2cf8268258d3d9a8d2c482ba92e5af) )
	ROM_LOAD( "ve5s.p1", 0x0000, 0x010000, CRC(65df6cf1) SHA1(26eadbad30b93df6dfd37f984be2dec77f1d6442) )
	ROM_LOAD( "ve5y.p1", 0x0000, 0x010000, CRC(2fe06579) SHA1(9e11b371edd8fab78e9594ed864f8eb487112150) )
	ROM_LOAD( "vesp05_11", 0x0000, 0x010000, CRC(32100a2e) SHA1(bb7324267708a0c0850fb77885df9868954d86cd) )
	ROM_LOAD( "vesp10_11", 0x0000, 0x010000, CRC(2a1dfcb2) SHA1(7d4ef072c41779554a2b8046688957585821e356) )
	ROM_LOAD( "vesp20_11", 0x0000, 0x010000, CRC(06233420) SHA1(06101dbe871617ae6ff098e070316ec98a15b704) )
	ROM_LOAD( "vetad.p1", 0x0000, 0x010000, CRC(fb9564dc) SHA1(9782d04eaec7d9c19138abf4f2dd3daa6c745c2a) )
	ROM_LOAD( "vetb.p1", 0x0000, 0x010000, CRC(2a8d7beb) SHA1(e503bdc388c2ab7551cc84dd9e45b85bd2420ef8) )
	ROM_LOAD( "vetbd.p1", 0x0000, 0x010000, CRC(ebaffb7d) SHA1(b54a581927fc28ce14ab9efe6fe62e074831a42a) )
	ROM_LOAD( "vetd.p1", 0x0000, 0x010000, CRC(365dff45) SHA1(6ce756f1d6133e05c46e8e7b7ad554f9f512b722) )
	ROM_LOAD( "vetdk.p1", 0x0000, 0x010000, CRC(5fb1ba90) SHA1(57a7f225d7bd8ed78c2ebf5d363e06b7694efc5f) )
	ROM_LOAD( "vetdy.p1", 0x0000, 0x010000, CRC(100261cb) SHA1(f834c5b848059673b9e9824854e6600dae6c4499) )
	ROM_LOAD( "vetk.p1", 0x0000, 0x010000, CRC(db48f34b) SHA1(013d84b27c4ea6d7b538011c22a3cd573f1d12cc) )
	ROM_LOAD( "vets.p1", 0x0000, 0x010000, CRC(d7e00f9d) SHA1(df2d85ff9eae7adf662b7d8a9c6f874ec8c07183) )
	ROM_LOAD( "vety.p1", 0x0000, 0x010000, CRC(ba3b19c7) SHA1(6e9ee238ec6a272ef16ebfba0dc49bc076e741de) )
	ROM_LOAD( "viva.bin", 0x0000, 0x010000, CRC(51537f2d) SHA1(a837a525cd7da724f338c47e716be175c37070b0) )
	ROM_LOAD( "viva206", 0x0000, 0x010000, CRC(76ab9a5d) SHA1(455699cbc05f744eafe58881a8fb120b24cfe5c8) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ve_05a__.3_1", 0x0000, 0x010000, CRC(92e0e121) SHA1(f32c8f1c8008794283bd32f9440e0a580f77b5b3) )
	ROM_LOAD( "ve_10a__.5_1", 0x0000, 0x010000, CRC(afdc0a2f) SHA1(ab8fec2c48db07c0aba31930893fe7211b306468) )
	ROM_LOAD( "vei05___.4_1", 0x0000, 0x010000, CRC(687a511b) SHA1(362e1d5557b6b7d551c9b9c5ef70d7944b44a3ce) )
	ROM_LOAD( "vei10___.4_1", 0x0000, 0x010000, CRC(b9e2471f) SHA1(3fa561466332ed14e233d97bf9170ec08a019bd0) )
	ROM_LOAD( "vesp5.8c", 0x0000, 0x010000, CRC(266d42cf) SHA1(b1e583652d6184db2a5f03cb7ae3f694627591c8) )
	ROM_LOAD( "vesp5.8t", 0x0000, 0x010000, CRC(bf8c9dfa) SHA1(69f28d3ce04efdb89db688dbc2341d19c27c5ba8) )
	ROM_LOAD( "vesp510l", 0x0000, 0x010000, CRC(15c33530) SHA1(888625c383e52825c06cbf1e7022cd8b02bf549c) )
	ROM_LOAD( "vesp55", 0x0000, 0x010000, CRC(9cc395ef) SHA1(d62cb55664246e3fada3d971ee317eef51739018) )
	ROM_LOAD( "vesp58c", 0x0000, 0x010000, CRC(d8cc868d) SHA1(0b9fa8b61998badbd870827e32af4937548b583e) )
	ROM_LOAD( "vesp_10 (2).4", 0x0000, 0x010000, CRC(95e95339) SHA1(59633b7c01da25237342bce7e989259bf723ba6f) )
	ROM_LOAD( "vesp_10 (2).8", 0x0000, 0x010000, CRC(8054766d) SHA1(8e7fd6f8cd74d2760e2923af32813ca93fbf98e6) )
	ROM_LOAD( "vesp_10.4", 0x0000, 0x010000, CRC(95e95339) SHA1(59633b7c01da25237342bce7e989259bf723ba6f) )
	ROM_LOAD( "vesp_10.8", 0x0000, 0x010000, CRC(8054766d) SHA1(8e7fd6f8cd74d2760e2923af32813ca93fbf98e6) )
	ROM_LOAD( "vesp_20_.8", 0x0000, 0x010000, CRC(35f90f05) SHA1(0013ff32c809603efdad782306140bd7086be965) )
	ROM_LOAD( "vesp_5 (2).4", 0x0000, 0x010000, CRC(3b6762ce) SHA1(9dc53dce453a7b124ea2b65a590aff6c7d05831f) )
	ROM_LOAD( "vesp_5 (2).8", 0x0000, 0x010000, CRC(63abf642) SHA1(6b585147a771e4bd445b525aafc25293845f660b) )
	ROM_LOAD( "vesp_5.4", 0x0000, 0x010000, CRC(3b6762ce) SHA1(9dc53dce453a7b124ea2b65a590aff6c7d05831f) )
	ROM_LOAD( "vesp_5.8", 0x0000, 0x010000, CRC(63abf642) SHA1(6b585147a771e4bd445b525aafc25293845f660b) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "viva.chr", 0x0000, 0x000048, CRC(4662e1fb) SHA1(54074bcc67adedb3dc6df80bdc60e0222f934156) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "vivasnd1.bin", 0x000000, 0x080000, CRC(e7975c75) SHA1(407c3bcff29f4b6599de2c35d96f62c72a897bd1) )
	ROM_LOAD( "vivasnd2.bin", 0x080000, 0x080000, CRC(9f22f32d) SHA1(af64f6bde0b825d474c42c56f6e2253b28d4f90f) )
ROM_END


ROM_START( m4vivess )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "se8ad.p1", 0x0000, 0x010000, CRC(4f799dfe) SHA1(e85108ab0aad92a64eabf5c7562068caf22f8d5b) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "se8b.p1", 0x0000, 0x010000, CRC(876efabb) SHA1(6ca1d37416b5401ba10977dad6a5881bdc7246ed) )
	ROM_LOAD( "se8bd.p1", 0x0000, 0x010000, CRC(39fe1c08) SHA1(99a04561555c819fc2954897e7831cf2c38db702) )
	ROM_LOAD( "se8d.p1", 0x0000, 0x010000, CRC(86cfec8e) SHA1(22e66ab075148c084db703358554b5496837d936) )
	ROM_LOAD( "se8dj.p1", 0x0000, 0x010000, CRC(86cfec8e) SHA1(22e66ab075148c084db703358554b5496837d936) )
	ROM_LOAD( "se8dk.p1", 0x0000, 0x010000, CRC(006b907e) SHA1(368915ec502bff70c1bdb0724ba6e32a9892aa5e) )
	ROM_LOAD( "se8dy.p1", 0x0000, 0x010000, CRC(36dcf85f) SHA1(e635501e6ba7dc4e56f1e00b472b32c030aa6592) )
	ROM_LOAD( "se8j.p1", 0x0000, 0x010000, CRC(d5c261de) SHA1(5f70944ffe03109ad16f162370fd3653d131034d) )
	ROM_LOAD( "se8k.p1", 0x0000, 0x010000, CRC(befb76cd) SHA1(f60e17538acd6f5b20e786f8a51a0471ee3246c8) )
	ROM_LOAD( "se8s.p1", 0x0000, 0x010000, CRC(d5c261de) SHA1(5f70944ffe03109ad16f162370fd3653d131034d) )
	ROM_LOAD( "se8y.p1", 0x0000, 0x010000, CRC(8be03e81) SHA1(f51024036f56b2009905e9c08bb292f2a280c0f6) )
	ROM_LOAD( "sesb.p1", 0x0000, 0x010000, CRC(0e3dc285) SHA1(53cf28228192b6e83d0ff95c8de2fb978720d363) )
	ROM_LOAD( "sesd.p1", 0x0000, 0x010000, CRC(549aaf0b) SHA1(084aca4429e27ce2642991aae8738d85c0157e54) )
	ROM_LOAD( "sesdy.p1", 0x0000, 0x010000, CRC(1869edd8) SHA1(b76dfa439eef641817a9bdf9c737cb06ac54efea) )
	ROM_LOAD( "sesk.p1", 0x0000, 0x010000, CRC(ebdb5ec4) SHA1(f7ec6e8c0142a0885fda066f379e7bd22f5844e5) )
	ROM_LOAD( "sess.p1", 0x0000, 0x010000, CRC(0e8d5c05) SHA1(bf05e4e83d6d4fb7c471e8ca22df21b357d8ed9b) )
	ROM_LOAD( "sesy.p1", 0x0000, 0x010000, CRC(126472e9) SHA1(3bebd273debbc9b71fce83cdc1031927698f7775) )

	ROM_REGION( 0x080000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4vivalvd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dlv11.bin", 0x0000, 0x010000, CRC(a890184c) SHA1(26d9952bf2eb4b55d21cdb934ffc73ff1a1cfbac) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "vegssnd.bin", 0x0000, 0x080000, CRC(93fd4253) SHA1(69feda7ffc56defd515c9cd1ce204af3d9731a3f) )
ROM_END


ROM_START( m4vivalv )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "5p5vivalasvegas6.bin", 0x0000, 0x010000, CRC(4d365b57) SHA1(69ff75ccc91f1f7b867a0914d350d1649834a48e) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "v.las vegas 6 10p 10m.bin", 0x0000, 0x010000, CRC(f09d5a2c) SHA1(6f9df58767e88a1ca7fc7dd17c618d30ab97067d) )
	ROM_LOAD( "vegas15g", 0x0000, 0x020000, CRC(5b804e4d) SHA1(5967b9f4e28e5a5e5e7005a29ecb30fc661800e0) )
	ROM_LOAD( "vegas15t", 0x0000, 0x020000, CRC(9d15f44f) SHA1(3a3f2034de9ba56cb2cb18b4898f2829a2019d4e) )
	ROM_LOAD( "viv55", 0x0000, 0x010000, CRC(4d365b57) SHA1(69ff75ccc91f1f7b867a0914d350d1649834a48e) )
	ROM_LOAD( "viva05_11", 0x0000, 0x010000, CRC(1e6ea483) SHA1(e6a53eb1bf3b8e661287c0d57fc6ab5ed41755a3) )
	ROM_LOAD( "viva10_11", 0x0000, 0x010000, CRC(246a39b7) SHA1(c0f5c21374e43b42df5df0ada0967a34ecefbdb4) )
	ROM_LOAD( "viva20_1.1", 0x0000, 0x010000, CRC(80ea2429) SHA1(e5d258967340fe85dd5baf6ba16f82ce83307b68) )
	ROM_LOAD( "viva20_11", 0x0000, 0x010000, CRC(51b93018) SHA1(fc13179e3e1939839c3b90d7600a7eb301ea03da) )
	ROM_LOAD( "viva58c", 0x0000, 0x010000, CRC(719d0802) SHA1(ba6bd5fbf49f0ada383cb2e8faa037b78f6af587) )
	ROM_LOAD( "viva_05_.4", 0x0000, 0x010000, CRC(b094914f) SHA1(8217b4bb7a8d55fb8e86018ffc520a63f41a79b8) )
	ROM_LOAD( "viva_05_.8", 0x0000, 0x010000, CRC(c5c09c10) SHA1(47890d0ba1c2ca53231ac148a02f046452dce1b4) )
	ROM_LOAD( "viva_10_.4", 0x0000, 0x010000, CRC(b1d5e820) SHA1(68012216d7e82168c7468d1e54c527c15d268917) )
	ROM_LOAD( "viva_10_.8", 0x0000, 0x010000, CRC(f392c81c) SHA1(cb3320b688b315dbc226f45b78490fed439ee9a2) )
	ROM_LOAD( "viva_20_.4", 0x0000, 0x010000, CRC(e1efc846) SHA1(a4bf7f5c4febe5a71a09e23876387328e1bba87b) )
	ROM_LOAD( "viva_20_.8", 0x0000, 0x010000, CRC(f538a1fc) SHA1(d0dbd22a1cb4b7ec5bfa304ba544806e01150662) )
	ROM_LOAD( "vlv208ac", 0x0000, 0x010000, CRC(416535ee) SHA1(f2b0177fecd5076d9d89c819fe9402fc944c8d77) )
	ROM_LOAD( "vlvad.p1", 0x0000, 0x010000, CRC(88262812) SHA1(f0a31d510c1b06af122df493585c04a49177f06d) )
	ROM_LOAD( "vlvb.p1", 0x0000, 0x010000, CRC(c4caec15) SHA1(d88c6e081a6bbdd80f773713b038293cabdeee8c) )
	ROM_LOAD( "vlvc.p1", 0x0000, 0x010000, CRC(4d651ba4) SHA1(7746656f0a9f8af8e265568f7479edef9a2247d9) )
	ROM_LOAD( "vlvd.p1", 0x0000, 0x010000, CRC(cce926c7) SHA1(8e3a0cef0cbee66d264da5d6dfc7ec2fbdcd9584) )
	ROM_LOAD( "vlvdy.p1", 0x0000, 0x010000, CRC(6e17cbc8) SHA1(5c69eda0ff6a01d9d0d434ff7ce1ac1e67b16362) )
	ROM_LOAD( "vlvk.p1", 0x0000, 0x010000, CRC(b5f2157e) SHA1(574f3e2890ac5479790ea92760c6500d37e6637d) )
	ROM_LOAD( "vlvs.p1", 0x0000, 0x010000, CRC(b7fb3e19) SHA1(c6cc4175f8c100fc37e6e7014b0744054b4e547a) )
	ROM_LOAD( "vlvy.p1", 0x0000, 0x010000, CRC(3211caf3) SHA1(3634ef11099c2f4938529bb262cc2556ad96a675) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "vv_05___.3_3", 0x0000, 0x010000, CRC(bb8361f6) SHA1(d5f651a66be1cab3662798751a290a65c29bba64) )
	ROM_LOAD( "vv_05_b_.3_3", 0x0000, 0x010000, CRC(12079321) SHA1(5b5dd55080c04393a45d3ef9c63b6fef5de9b7cd) )
	ROM_LOAD( "vv_05_d_.3_3", 0x0000, 0x010000, CRC(b758df52) SHA1(f4d47a93fa1b1deb84654bb2272767093f3463c2) )
	ROM_LOAD( "vv_05_k_.3_3", 0x0000, 0x010000, CRC(9875c59c) SHA1(c31a7fc5df8af9d931353bc095a59befe808434b) )
	ROM_LOAD( "vv_05a__.3_3", 0x0000, 0x010000, CRC(0f416e47) SHA1(54338fbef5f227c440c04448b51e8f0ec04a4cc7) )
	ROM_LOAD( "vv_10___.3_3", 0x0000, 0x010000, CRC(dc8db002) SHA1(305547b4f0b1e1bde9354e5ed9f18f99c6829cab) )
	ROM_LOAD( "vv_10_b_.3_3", 0x0000, 0x010000, CRC(e1c4b292) SHA1(4516c7d918935862824e206626a5a24f936ec514) )
	ROM_LOAD( "vv_10_d_.3_3", 0x0000, 0x010000, CRC(e9dda1ee) SHA1(6363b5b26be22cb1f5aac71e98c5e5a5064839f4) )
	ROM_LOAD( "vv_10_k_.3_3", 0x0000, 0x010000, CRC(70fc4c56) SHA1(02cbaadd3575ef0d9dc192aabbe39a735893a662) )
	ROM_LOAD( "vv_10a__.3_3", 0x0000, 0x010000, CRC(c908d65a) SHA1(5af180e697c22c27380e275d76708103e298cf41) )
	ROM_LOAD( "vvi05___.3_3", 0x0000, 0x010000, CRC(a5829d5c) SHA1(4cd1a2185579898db7be75f8c3f565043f0691b6) )


	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "vivalasvegas4.bin", 0x0000, 0x080000, CRC(76971425) SHA1(0974a9dce51cc3dd4e26cec11a948c9c8021fde4) )
ROM_END


ROM_START( m4vivasx )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "vlvs.p1", 0x0000, 0x010000, CRC(b7fb3e19) SHA1(c6cc4175f8c100fc37e6e7014b0744054b4e547a) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "vlvs.chr", 0x0000, 0x000048, CRC(31acf949) SHA1(d622ea1caee968b786f3183ca44355f9db190081) )
ROM_END


ROM_START( m4viz )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "viz208c", 0x0000, 0x010000, CRC(00a65029) SHA1(8dfb68d1a9f4cd00f239ed87a1d330ccb655c35b) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "viz20_101", 0x0000, 0x010000, CRC(0847b812) SHA1(6de9e9dad272932a22ebe457ac50da1126d931ea) )
	ROM_LOAD( "viz20pv2", 0x0000, 0x010000, CRC(7e56ff95) SHA1(83679b64881adbe547b43255374de061859e17ef) )
	ROM_LOAD( "viz58c", 0x0000, 0x010000, CRC(95b8918b) SHA1(4ad4ff9098e98c2076e7058493c181da705acb52) )
	ROM_LOAD( "vizb.p1", 0x0000, 0x010000, CRC(afdc6306) SHA1(4d35703267b3742dd7008c00ec525689c56bf227) )
	ROM_LOAD( "vizc.p1", 0x0000, 0x010000, CRC(876c30fc) SHA1(f126496e87d7e84ca39d2921bf9f2be0fa2c7586) )
	ROM_LOAD( "vizd.p1", 0x0000, 0x010000, CRC(46bee8cd) SHA1(4094651fd8954ca2f5cfc2bba4fc51d865c86098) )
	ROM_LOAD( "vizdk.p1", 0x0000, 0x010000, CRC(24476360) SHA1(b5141a40f8c1ed3b3fbaf43ae539ae2f1aedbcca) )
	ROM_LOAD( "vizdy.p1", 0x0000, 0x010000, CRC(88807a1f) SHA1(dc1539a5e69b5f0b3f68ccf7360ff4f240f6b7c7) )
	ROM_LOAD( "vizk.p1", 0x0000, 0x010000, CRC(6647f592) SHA1(2ce7222bd9e173480ddc901f84859ca3ad7aded1) )
	ROM_LOAD( "vizs.p1", 0x0000, 0x010000, CRC(86b487dc) SHA1(62215752e1da1ca923e6b9e410c8445577be34dd) )
	ROM_LOAD( "vizy.p1", 0x0000, 0x010000, CRC(0e12112d) SHA1(4a34832dd95246e80e616affe3eab3c8794ca769) )
	ROM_LOAD( "vizzzvkn", 0x0000, 0x010000, CRC(cf5c41f5) SHA1(c9b7de0e73141833e5f8d23f0cb641b1c6094178) )

	ROM_REGION( 0x10000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "vi_05a__.1_1", 0x0000, 0x010000, CRC(56e0ea7a) SHA1(cbe979cdfceb2c1c7be5adaf8163b96bebbc4bb6) )
	ROM_LOAD( "vi_05s__.1_1", 0x0000, 0x010000, CRC(c6896e33) SHA1(7db1a5e08f1a307aac0818424fab274cd8141474) )
	ROM_LOAD( "vi_05sb_.1_1", 0x0000, 0x010000, CRC(12fecbdf) SHA1(c0137aac536ec17c3b2ffa405f8400308f759590) )
	ROM_LOAD( "vi_05sd_.1_1", 0x0000, 0x010000, CRC(9241fd92) SHA1(f3e58273089ee9b828e431a043802d4ec3948a64) )
	ROM_LOAD( "vi_10a__.1_1", 0x0000, 0x010000, CRC(e7c4e4d9) SHA1(9ac3bd60e6000e36cd2229284c48e009ea22cfdb) )
	ROM_LOAD( "vi_10s__.1_1", 0x0000, 0x010000, CRC(039a4620) SHA1(097335ba8846c8c8b28bf85f836ba76d22bc763d) )
	ROM_LOAD( "vi_10sb_.1_1", 0x0000, 0x010000, CRC(4b7e6686) SHA1(97985f1ecd3a8e77f07a91c5171810e6aff13f4c) )
	ROM_LOAD( "vi_10sd_.1_1", 0x0000, 0x010000, CRC(84da6fca) SHA1(8a42855b161619a56435da52dd24e8e60fb56bd8) )
	ROM_LOAD( "vii05___.1_1", 0x0000, 0x010000, CRC(22a10f78) SHA1(83411b77e5de441b0f5fa02f2b1dbc40755f41cb) )
	ROM_LOAD( "vii10___.1_1", 0x0000, 0x010000, CRC(92e11e00) SHA1(2ebae74a39434269333ea0067163e9607926646d) )
	ROM_LOAD( "viz_20_.8", 0x0000, 0x010000, CRC(b4fbc43b) SHA1(4cce5e3a0c32a402b81dfd16e66d12e98704c4d2) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "vizs.chr", 0x0000, 0x000048, CRC(2365ca6b) SHA1(b964420eb1df4065b2a6f3f934135d435b52af2b) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "vizsnd.p1", 0x000000, 0x080000, CRC(2b39a622) SHA1(77916650fe19f18025e10fb25de704f7bb733295) )
	ROM_LOAD( "vizsnd.p2", 0x080000, 0x080000, CRC(e309bede) SHA1(a4615436fcfd5f31293f887b8bc972f0d2d6b0cb) )
ROM_END


ROM_START( m4voodoo )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ddo32", 0x0000, 0x010000, CRC(260dfef1) SHA1(2b4918e40808963a86d289cd251740a9b0bed70a) )
ROM_END


ROM_START( m4wayin )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "wi1.bin", 0xc000, 0x004000, CRC(d59d351a) SHA1(815ff595a74dc165e1996a46de29095174c80442) )
	ROM_LOAD( "wi2.bin", 0x8000, 0x004000, CRC(cba2bcc5) SHA1(6412fa614ac8eef4eda6213afa35aabaf14ba6ce) )
	ROM_LOAD( "wi3.bin", 0x6000, 0x002000, CRC(1ba06ef1) SHA1(495c4e21a5b34cba0859e4e8fc842036c97ec063) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ws1.0p1128k.bin", 0xc000, 0x004000, CRC(ba5dea02) SHA1(12ae92c802b085550dc5fc2d2024c73a834bf0b8) )
	ROM_LOAD( "ws1.0p2128k.bin", 0x8000, 0x004000, CRC(3fd3bdb4) SHA1(79b748d613db3132d08ae0a4e805e5494ea56ea0) )
	ROM_LOAD( "ws1.0p364k.bin", 0x6000, 0x002000, CRC(c0ec66e5) SHA1(703a6265f305d600aeb062981c39e09f1d059443) )
ROM_END


ROM_START( m4whaton )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pwos.p1", 0x0000, 0x010000, CRC(6a87aa68) SHA1(3dc8c006de3adcada43c3581be0ff921081ecff0) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "pwos.chr", 0x0000, 0x000048, CRC(352b86c4) SHA1(59c26a1948ffd6ecea08d8ca8e62735ec9732c0f) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "pwo.s1", 0x000000, 0x080000, CRC(1dbd8a33) SHA1(37bd71688475591232422eb0841e23aff58e3800) )
	ROM_LOAD( "pwo.s2", 0x080000, 0x080000, CRC(6c7badef) SHA1(416c36fe2b4253bf7944b3ba412561bd0d21cbe5) )
ROM_END


ROM_START( m4wildms )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "wild.bin", 0x0000, 0x010000, CRC(33519799) SHA1(d5154fa5307f25f6a3ee8759520907eb4c06fdf9) )
ROM_END


ROM_START( m4wildtm )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "wildtimer.bin", 0x0000, 0x010000, CRC(5bd54924) SHA1(23fcf13c52ee7b9b39f30f999a9102171fffd642) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "charter.chr", 0x0000, 0x000048, CRC(4ff4eda2) SHA1(092435e34d79775910316a7bed0f90c4f086e5c4) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "wildtimer-snd.bin", 0x0000, 0x080000, CRC(50450909) SHA1(181659b0594ba8d196b7130c5999c91676a363c0) )
ROM_END


ROM_START( m4wta )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "wta55", 0x0000, 0x010000, CRC(df3e66cd) SHA1(68e769816cb1a71dea8a3ccf4636414c45c01646) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pwnk.p1", 0x0000, 0x020000, CRC(7c6e21e3) SHA1(d6aeb5948e0800050193575a3b5c06c11f46eed8) )
	ROM_LOAD( "windy.p1", 0x0000, 0x010000, CRC(d8b78c2d) SHA1(d8c2a2ac30a9b876acfbe99e3c540ba0e82cde33) )
	ROM_LOAD( "winner takes all wn80.1c 20p 8 idm.bin", 0x0000, 0x010000, CRC(471ba65a) SHA1(6ede860bcf323ee75dd7f75a51e5d1166ee72abc) )
	ROM_LOAD( "wins.p1", 0x0000, 0x010000, CRC(d79d1e5b) SHA1(722657423a605d6d272d61e4e00b4055ed05f98d) )
	ROM_LOAD( "winy.p1", 0x0000, 0x010000, CRC(5ff8ed08) SHA1(9567db64e8ebf25ecb22236598cc88a3106f0e36) )
	ROM_LOAD( "wn5ad.p1", 0x0000, 0x010000, CRC(0eb0845d) SHA1(57a2ca27672119e71af3b990cedcf52dd89e24cc) )
	ROM_LOAD( "wn5b.p1", 0x0000, 0x010000, CRC(82cefba2) SHA1(07753a5f0d455422f33495a6f050c8e16a92e087) )
	ROM_LOAD( "wn5bd.p1", 0x0000, 0x010000, CRC(19d25b26) SHA1(91459c87e95d9800c5f77fd0c7f72f8a1488dc37) )
	ROM_LOAD( "wn5d.p1", 0x0000, 0x010000, CRC(8a3d6bed) SHA1(a20f24cd5216976913c0405f54883d6080986867) )
	ROM_LOAD( "wn5dk.p1", 0x0000, 0x010000, CRC(1dfcb2bc) SHA1(b1a73a7758c3126f7b13156835c91a4900cbe6e0) )
	ROM_LOAD( "wn5dy.p1", 0x0000, 0x010000, CRC(d45e1db0) SHA1(2524c4b60a89ea0ca15cf999fbd1f8d9029dfbb6) )
	ROM_LOAD( "wn5k.p1", 0x0000, 0x010000, CRC(71c34cb4) SHA1(e1b96dd30d8ab680128d76886691d06fcd2d48c0) )
	ROM_LOAD( "wn5s.p1", 0x0000, 0x010000, CRC(f6e925c1) SHA1(963f06462c73300757aad2371df4ebe28afca521) )
	ROM_LOAD( "wn5y.p1", 0x0000, 0x010000, CRC(7155f8b5) SHA1(f55f88fd7b0144cb7b64640d529b179dd056f5ec) )
	ROM_LOAD( "wn8b.p1", 0x0000, 0x010000, CRC(7e84f99c) SHA1(bef41b3e7906bdaadfa5741e9ae40028f4fd360f) )
	ROM_LOAD( "wn8c.p1", 0x0000, 0x010000, CRC(471ba65a) SHA1(6ede860bcf323ee75dd7f75a51e5d1166ee72abc) )
	ROM_LOAD( "wn8d.p1", 0x0000, 0x010000, CRC(eb2bd01e) SHA1(df74f8eb8fa411bab20ab522fd7c511a1370fe90) )
	ROM_LOAD( "wn8dk.p1", 0x0000, 0x010000, CRC(ec20a0bc) SHA1(61b615165a6e77cd85e1fa6aeb955307ec48d1b6) )
	ROM_LOAD( "wn8dy.p1", 0x0000, 0x010000, CRC(d2a1513c) SHA1(e4d2ad88846cbb6b393d3615bf10e1dea01de219) )
	ROM_LOAD( "wn8k.p1", 0x0000, 0x010000, CRC(3e15c690) SHA1(2fc1cca91ac5cc9abeac112e4d60e8fd57b07b94) )
	ROM_LOAD( "wn8s.p1", 0x0000, 0x010000, CRC(5c5a0f31) SHA1(301e595141dd6eb9250d71e591780e15a7d36423) )
	ROM_LOAD( "wn8y.p1", 0x0000, 0x010000, CRC(993cee6a) SHA1(26b2d5d3aa3465f90fe74960f183b8580ea2fbb1) )
	ROM_LOAD( "wnta2010", 0x0000, 0x010000, CRC(5b08faf8) SHA1(f4657041562044e17febfe77ad1f849545dcdaec) )
	ROM_LOAD( "wntad.p1", 0x0000, 0x010000, CRC(8502766e) SHA1(2a47c8f8ce8711b30962c5e8ef9093bdd3543551) )
	ROM_LOAD( "wntb.p1", 0x0000, 0x010000, CRC(1e3159f0) SHA1(ab9d0e9e6731b40c66c358d98c6481f31d9a0b0c) )
	ROM_LOAD( "wntbd.p1", 0x0000, 0x010000, CRC(91cc8978) SHA1(570ad4092bb148106fb2600f1e22b6cb6f57002a) )
	ROM_LOAD( "wntd.p1", 0x0000, 0x010000, CRC(ad68d804) SHA1(f301d0d267dd0020903f06b67ee6494b71258c68) )
	ROM_LOAD( "wntdk.p1", 0x0000, 0x010000, CRC(3a6b65b8) SHA1(1da0448e53a45fa249c14b5655cd0dc957ebb646) )
	ROM_LOAD( "wntdy.p1", 0x0000, 0x010000, CRC(2420634f) SHA1(5c6e891c34a6e2b3a6acb3856c1554145bb24d0d) )
	ROM_LOAD( "wntk.p1", 0x0000, 0x010000, CRC(3d8d07c7) SHA1(4659e2459d956bbcf5ef2a605527317ccdafcccb) )
	ROM_LOAD( "wnts.p1", 0x0000, 0x010000, CRC(3a9b0878) SHA1(85e86cca1a3a079746cd4401767ba1d9fc31a938) )
	ROM_LOAD( "wnty.p1", 0x0000, 0x010000, CRC(edaa5ae7) SHA1(d24b9f37d75f13f16718374e48e6c003b0b3333f) )
	ROM_LOAD( "wta20p10.bin", 0x0000, 0x010000, CRC(c7f235b8) SHA1(a25f6f755140d70b0392985839b1729640cf5d5d) )
	ROM_LOAD( "wta510l", 0x0000, 0x010000, CRC(9ce140ae) SHA1(01d53a5da0161ac4ecc861309f645d6eb47b4af5) )
	ROM_LOAD( "wta58tl", 0x0000, 0x010000, CRC(7275e865) SHA1(d5550646a062609cfc052fab81c533ca69171875) )
	ROM_LOAD( "wta_5p_4c.bin", 0x0000, 0x010000, CRC(54c51976) SHA1(70cae1f931615b993ac6a9e7ce2e529ad6d27da8) )
	ROM_LOAD( "wtall20a", 0x0000, 0x010000, CRC(b53c951e) SHA1(24f96d16852a4fbaf49fbdf29a26d15877f07b18) )

	ROM_REGION( 0x20000, "altbwb", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "wnt20.10", 0x0000, 0x000010, CRC(e77d5458) SHA1(c5ace2bca74370f7825da66907972a3da167e2c7) )
	ROM_LOAD( "wt_05__4.1_1", 0x0000, 0x010000, CRC(5e05485e) SHA1(062f16ca92518f746f5410a2b9b551542e1a68e3) )
	ROM_LOAD( "wt_05__5.3_1", 0x0000, 0x010000, CRC(8a289bbd) SHA1(8ae0858716ed6aa02f6b4f93fd367c7cee85d13a) )
	ROM_LOAD( "wta5.10", 0x0000, 0x010000, CRC(c1ae8e9a) SHA1(66c0b200202386a10b96b7141517a52921266950) )
	ROM_LOAD( "wta5.4", 0x0000, 0x010000, CRC(00c64637) SHA1(54214edb107b28852a1bd3e095787bf9241e4fe3) )
	ROM_LOAD( "wta5.5n", 0x0000, 0x010000, CRC(85eed9b5) SHA1(6a11ff6a031b788524d23018e3af44767176246a) )
	ROM_LOAD( "wta5.8t", 0x0000, 0x010000, CRC(548122ab) SHA1(c611084e8a08d5556e458daf9cc721c0e5ba1948) )


	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "wn5s.chr", 0x0000, 0x000048, CRC(b90e5068) SHA1(14c57dcd7242104eb48a9be36192170b97bc5110) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	//ROM_LOAD( "wta_5p4c.bin", 0x0000, 0x080000, CRC(0516acad) SHA1(cfecd089c7250cb19c9e4ca251591f820acefd88) ) // no 1st part? this appears to belong to wacky weekend club (global)

	ROM_LOAD( "winsnd.p1", 0x000000, 0x080000, CRC(a913ad0d) SHA1(5f39b661912da903ce8d6658b7848081b191ea56) )
	ROM_LOAD( "winsnd.p2", 0x080000, 0x080000, CRC(6a22b39f) SHA1(0e0dbeac4310e03490b665fff514392481ad265f) )
ROM_END


ROM_START( m4ch30 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ch301s", 0x0000, 0x010000, CRC(d31c9081) SHA1(21d1f4cc3de2343d830e3ee02e3a53abd12b6b9d) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing */
ROM_END



ROM_START( m4sb5 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sb5pcz", 0x0000, 0x010000, CRC(331bfd68) SHA1(7436994de8617c31cb044540da7a2867daa5f829) )
ROM_END



ROM_START( m4stc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "stc01s", 0x0000, 0x010000, CRC(8371bb8f) SHA1(bd60825b3f5011c218b34f00886b6b54afe61b9f) )
ROM_END





ROM_START( m4acechs )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ae_05a__.2_3", 0x0000, 0x010000, CRC(c9a03623) SHA1(8daf7e71057528c481915eb8506e03ce9cf372c8) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ae_05a__.3_1", 0x0000, 0x010000, CRC(900e1789) SHA1(dbb13f1728d8527a7de5d257c866732adb0a95b5) )
	ROM_LOAD( "ae_05s__.2_3", 0x0000, 0x010000, CRC(eb64ab0a) SHA1(4d4c6908c8ca8b1d3c39c8973c8386da079cbd39) )
	ROM_LOAD( "ae_05sb_.2_3", 0x0000, 0x010000, CRC(5d67c6f6) SHA1(213225405defb3be7f564459d71aeca6f5856f8f) )
	ROM_LOAD( "ae_05sd_.2_3", 0x0000, 0x010000, CRC(2bdbe356) SHA1(a328a8f50847cbb199b31672ca50e1e95a474e4b) )
	ROM_LOAD( "ae_10a__.2_3", 0x0000, 0x010000, CRC(d718d498) SHA1(d13970b0ca86b988bcc91cd3c2dbee4c637944ca) )
	ROM_LOAD( "ae_10a__.3_1", 0x0000, 0x010000, CRC(e20c2513) SHA1(857ed8a6b155863c769ee9c3aca5e4702c1372b6) )
	ROM_LOAD( "ae_10bg_.2_3", 0x0000, 0x010000, CRC(7ed7fcee) SHA1(7b2b0c47dc8a75d11f49f09441a4320815d838ac) )
	ROM_LOAD( "ae_10s__.2_3", 0x0000, 0x010000, CRC(31932d3f) SHA1(a1809c7baaea22d24491829a8638f232e2d75849) )
	ROM_LOAD( "ae_10sb_.2_3", 0x0000, 0x010000, CRC(d6bcd1fd) SHA1(664ec7e7821c09bddfd1996892ae3f9fbdbc6809) )
	ROM_LOAD( "ae_10sd_.2_3", 0x0000, 0x010000, CRC(5920b9ad) SHA1(fb8de53e7877505fe53ff874b396707ee8e01e5e) )
	ROM_LOAD( "ae_20a__.3_1", 0x0000, 0x010000, CRC(43f6cc19) SHA1(3eda49477b141c649a4ba7a4ecc021694d9830db) )
	ROM_LOAD( "ae_20b__.3_1", 0x0000, 0x010000, CRC(30060ac4) SHA1(488263a1d3cfe067d43de29c57e58fe55024437c) )
	ROM_LOAD( "ae_20bd_.3_1", 0x0000, 0x010000, CRC(f9b922c2) SHA1(fc0deb79fc6c33732872da8925a6729f3d11bcaf) )
	ROM_LOAD( "ae_20bg_.3_1", 0x0000, 0x010000, CRC(02706741) SHA1(8388d91091945d1f73aa5e68a86f930f5d9dafa2) )
	ROM_LOAD( "ae_20bt_.3_1", 0x0000, 0x010000, CRC(3b313958) SHA1(9fe4cb99dc30d1305816f9a27079d97c4d07cb15) )
	ROM_LOAD( "ae_20sb_.3_1", 0x0000, 0x010000, CRC(471f2ba4) SHA1(baaf8339d8ee15365886cea2ecb36ad298975633) )
	ROM_LOAD( "aei05___.2_3", 0x0000, 0x010000, CRC(f035ba55) SHA1(d13bebec00650018a9236cc18df73b06c970cfd0) )
	ROM_LOAD( "aei05___.3_1", 0x0000, 0x010000, CRC(bb84d01f) SHA1(f1653590e8cd642faf09a16c5c1b0a4b267d42e7) )
	ROM_LOAD( "aei10___.2_3", 0x0000, 0x010000, CRC(96edf44f) SHA1(8abcb5d4018e0a4c879eb1a1550af09f55f75135) )
	ROM_LOAD( "aei10___.3_1", 0x0000, 0x010000, CRC(db99a965) SHA1(1fb200b30e10d502af39bcd2e58d3e36e13f3695) )
	ROM_LOAD( "aei20___.3_1", 0x0000, 0x010000, CRC(1744e7f4) SHA1(bf2f1b720a1a2610aff46a1de5c789a17828eae0) )
ROM_END


ROM_START( m4bigmt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bigm1320", 0x0000, 0x010000, CRC(a5085347) SHA1(93a7f7656e53461270e04190ff538959d6c917c1) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "tb_20___.7_1", 0x0000, 0x010000, CRC(22fae0f0) SHA1(a875adccf96fbbff69f5fe76720514767cdcae66) )
	ROM_LOAD( "tb_20_b_.7_1", 0x0000, 0x010000, CRC(40d140a3) SHA1(fd4de8dd827db933481f671e4f10684c3b7a363a) )
	ROM_LOAD( "tb_20_d_.7_1", 0x0000, 0x010000, CRC(86ed18c5) SHA1(1699645532134bd830e3fc2c3ff4b67b5d67ba3e) )
	ROM_LOAD( "tb_20_k_.7_1", 0x0000, 0x010000, CRC(e4c6b896) SHA1(ccf656d14ad68edb0aea99d4d3621540b899cdb7) )
	ROM_LOAD( "tb______.1_2", 0x0000, 0x004000, CRC(be2a3989) SHA1(5ef857101335f90cd9f6147ec330ae281bb97b2b) )
	ROM_LOAD( "tbi20___.7_1", 0x0000, 0x010000, CRC(104f45cc) SHA1(c28c08b44c46db9d0eade1f60aeda120c3981a03) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "bigmsnd", 0x0000, 0x07db60, CRC(876c53ae) SHA1(ea2511ec9ba4ff67879212c6e2ba908873130a4e) )
	ROM_LOAD( "tbmsnd.hex", 0x0000, 0x080000, CRC(e98da8de) SHA1(36668f2b82778f441224c94831f5b95efb9fa92b) )
ROM_END


ROM_START( m4bingbl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bb_20a__.8_1", 0x0000, 0x010000, CRC(10f29ba3) SHA1(739b413f35676834ebafeb121c6059759586ec72) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "bb_20bg_.8_1", 0x0000, 0x010000, CRC(9969c9ce) SHA1(8c754335e7ff75bc46f02095c2c7d57df046db47) )
	ROM_LOAD( "bb_20bt_.8_1", 0x0000, 0x010000, CRC(3a06f518) SHA1(79a9b1e63517f6436ee4743dfc7aa63a9cf585b8) )
	ROM_LOAD( "bb_20s__.8_1", 0x0000, 0x010000, CRC(c5bd878e) SHA1(dc6048cf25b3a62b23761c338fe6398724b24159) )
	ROM_LOAD( "bb_20sb_.8_1", 0x0000, 0x010000, CRC(a79627dd) SHA1(c9a6262a0e52a4d1ff073465ab6b489626ae6a2a) )
	ROM_LOAD( "bb_20sd_.8_1", 0x0000, 0x010000, CRC(61aa7fbb) SHA1(0bf54a562d04c0b41fc35c5c84f529efa2ae9e10) )
	ROM_LOAD( "bb_20sk_.8_1", 0x0000, 0x010000, CRC(0381dfe8) SHA1(8d3d232c10f9ec683a82464ee901f0963d9ffcd6) )
	ROM_LOAD( "bbi20___.8_1", 0x0000, 0x010000, CRC(eae04eac) SHA1(bcc619868a065d92d09fa3b2411958b373b3edde) )
ROM_END


ROM_START( m4bingbs )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bp_20a__.2_1", 0x0000, 0x010000, CRC(ca005003) SHA1(271ff0dbee529ca15c79c9aa1047efa8993ea073) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "bp_20bg_.2_1", 0x0000, 0x010000, CRC(1b59c32f) SHA1(0c7df33f921639bfedbddd969dcbcd62e38ff912) )
	ROM_LOAD( "bp_20bt_.2_1", 0x0000, 0x010000, CRC(bc7a1830) SHA1(3c16432562ebaef3f17e51feebb4c35d911e90b9) )
	ROM_LOAD( "bp_20s__.2_1", 0x0000, 0x010000, CRC(1f4f4c2e) SHA1(28f6361c7ee693635c718ea485687bf9fbf8532b) )
	ROM_LOAD( "bp_20sb_.2_1", 0x0000, 0x010000, CRC(7d64ec7d) SHA1(bc279010642edadcba19cdfad222a783e35039b9) )
	ROM_LOAD( "bp_20sd_.2_1", 0x0000, 0x010000, CRC(bb58b41b) SHA1(253859ab6d8033eeb94ad5c4586eadc8792bd436) )
	ROM_LOAD( "bp_20sk_.2_1", 0x0000, 0x010000, CRC(d9731448) SHA1(2f054a99c5d1570b6e0307542c50421408a610dc) )
ROM_END


ROM_START( m4bingcl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bc_xe___.2_1", 0x0000, 0x010000, CRC(3abbc215) SHA1(b5e59b30c07c4ffef69c5729f1a28d7ee55636bd) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "bc_xe_b_.2_1", 0x0000, 0x010000, CRC(3e11c5c0) SHA1(2d9bc987fed040664f211bb9d13984b6cba5e25f) )
	ROM_LOAD( "bc_xe_d_.2_1", 0x0000, 0x010000, CRC(0a3a162f) SHA1(1b0857039b0442269b8fd4c063fc99b3b50cc312) )
ROM_END


ROM_START( m4blflsh )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bfl20__0.3t", 0x0000, 0x010000, CRC(3cf3aa54) SHA1(98472e72cf48f938b756617d71f3860b34c05c3a) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "bflxc__0.3", 0x0000, 0x010000, CRC(04149b9e) SHA1(9d2d783acccf36e5ad85bb7b6ccdfae88201748b) )
	ROM_LOAD( "bflxcb_0.3", 0x0000, 0x010000, CRC(0f4538ec) SHA1(a1bf9513e5018a38548691338dfe53ff0a382625) )
	ROM_LOAD( "bflxcbd0.3", 0x0000, 0x010000, CRC(49d59b00) SHA1(18759ab3eae9956d2b20c608e66d9c01b8647817) )
	ROM_LOAD( "bflxcc_0.3", 0x0000, 0x010000, CRC(eb13ee16) SHA1(eabedab917bbc9ea2d71f868e9a7fa8c594e36ab) )
	ROM_LOAD( "bflxcd_0.3", 0x0000, 0x010000, CRC(4eae4984) SHA1(4f4626fc4f71a9ccad7f6d99d2b28c079e20fa41) )
ROM_END



ROM_START( m4cshenc )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "ca_sj__c.5_1", 0x0000, 0x020000, CRC(d9131b39) SHA1(4af89a7bc10de1406f401bede41e1bc452dbb159) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ca_sj_bc.5_1", 0x0000, 0x020000, CRC(30d1fb6d) SHA1(f845bef4ad7f2f48077eed74840916e87abb24b2) )
	ROM_LOAD( "ca_sj_dc.5_1", 0x0000, 0x020000, CRC(ac3ec716) SHA1(4ff8c26c46ec6e1321249b4d6d0c5194ed917f33) )
	ROM_LOAD( "ca_sja_c.5_1", 0x0000, 0x020000, CRC(c56a9d0b) SHA1(b0298c2e03097ab8ba5f99892e732ff1ab784c9b) )
	ROM_LOAD( "ca_sjb_c.5_1", 0x0000, 0x020000, CRC(8fad355d) SHA1(2ac16ad85ab8239a3e961abb06f9f71d17e5832a) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "cesnd.p1", 0x000000, 0x080000, CRC(2a10dc1a) SHA1(f6803f6e1fee2b58fe4831f59ddc08ec02792823) )
	ROM_LOAD( "cesnd.p2", 0x080000, 0x080000, CRC(6f0b75c0) SHA1(33898d75a1e51b49950d7843069066d17c4736c5) )
ROM_END


ROM_START( m4czne )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "czone 6.bin", 0x0000, 0x010000, CRC(e5b2b64e) SHA1(b73a2aed7b04184bc7c5c3d0a11d44e624a47428) )
ROM_END

ROM_START( m4csoc )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "chsoc8ac", 0x0000, 0x040000, CRC(8e0471ba) SHA1(3b7e6edbb3490e99af148c0cfe8d39c13c282880) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sg_sj___.1_0", 0x0000, 0x040000, CRC(f21cd1aa) SHA1(dc010a315a8d738ad9e5e384197499e08a8d5ef6) )
	ROM_LOAD( "sg_sj___.2_0", 0x0000, 0x040000, CRC(5513f2a3) SHA1(e9e59461a007be02beae6cd1610b8582d367c15e) )
	ROM_LOAD( "sg_sj_d_.2_0", 0x0000, 0x040000, CRC(b0058d0f) SHA1(635c4f729c27c5cb4356f62dcc13127043ea0e5c) )
	ROM_LOAD( "so_sj___.b_0", 0x0000, 0x040000, CRC(65d5bb2d) SHA1(61c6896d97ed79e2f31b37b9d8998980ceac4fc5) )
	ROM_LOAD( "so_sj_d_.b_0", 0x0000, 0x040000, CRC(5e79ba34) SHA1(cb8c689319b5f94ce0385b3b7846a49589358ccc) )
	ROM_LOAD( "so_sjs__.b_0", 0x0000, 0x040000, CRC(2f50675e) SHA1(baed8e3a455ec5bfa810e64dc4c66996d6746bbc) )
	ROM_LOAD( "so_vc___.c_0", 0x0000, 0x040000, CRC(d683b202) SHA1(95803008a50229bc85ed177b587fdf05cb152df3) )
	ROM_LOAD( "so_vc_d_.c_0", 0x0000, 0x040000, CRC(ed2fb31b) SHA1(de72d8abbb4a22125ed312e6ccfcab6b3e591ec2) )
	ROM_LOAD( "ch_socc", 0x0000, 0x040000, CRC(ea9af5bd) SHA1(99319995ee886196ddd540bf37960a4e5b9d4f34) )
	ROM_LOAD( "ch_socc.5", 0x0000, 0x040000, CRC(1b2ea78d) SHA1(209534ccd537c0ca9d02301830a52ebc29b93cb7) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "ch_socc.s1", 0x000000, 0x080000, CRC(abaea3f3) SHA1(cf3b6e4ee99680726efd2a839b49b4d86e2bd270) )
	ROM_LOAD( "ch_socc.s2", 0x080000, 0x080000, CRC(2048f5b2) SHA1(b07addfd9d861b1d19d4db248e16c597cf79b159) )
	ROM_LOAD( "ch_socc.s3", 0x100000, 0x080000, CRC(064224b0) SHA1(99a8bacfd3a42f72e40b93d1f7eeea633c3cf366) )
ROM_END




ROM_START( m4cpycat )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "co_20_bc.1_1", 0x0000, 0x010000, CRC(c9d3cdc1) SHA1(28265b0f95a8829efc4e346269a7af17a6abe345) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "co_20_dc.1_1", 0x0000, 0x010000, CRC(c6552f9a) SHA1(ae7ad183d2cd89bc9748dcbb3ea26832bed30009) )
	ROM_LOAD( "co_20_kc.1_1", 0x0000, 0x010000, CRC(b5260e35) SHA1(6cbf4ca426fd47b0db49e188a7a7fe72f6c99aef) )
	ROM_LOAD( "co_20a_c.1_1", 0x0000, 0x010000, CRC(486d42af) SHA1(327fca9604845ec37c2212413105f48a7b0e2836) )
	ROM_LOAD( "co_20b_c.1_1", 0x0000, 0x010000, CRC(90e0e19f) SHA1(c7e73faa4c3e853dbaa6b14303ab454a09eb36d7) )
	ROM_LOAD( "co_20bgc.1_1", 0x0000, 0x010000, CRC(f99d6ae2) SHA1(2106412caa7d3dfc262dc2b1d3e258bb33605912) )
	ROM_LOAD( "co_20bgp.4_1", 0x0000, 0x010000, CRC(fdc6753a) SHA1(4dd39fa995ee9fa7d153d64dd163d5482aa490d2) )
	ROM_LOAD( "co_20btc.1_1", 0x0000, 0x010000, CRC(3780d767) SHA1(4fb4354b02eed754ca1caef2b56eccc76524ae1e) )
	ROM_LOAD( "co_25_bc.1_1", 0x0000, 0x010000, CRC(13c82730) SHA1(992a6d04ed357548bb6cea6505316013048a2e57) )
	ROM_LOAD( "co_25_bp.2_1", 0x0000, 0x010000, CRC(a5983412) SHA1(7daa3028355fb0e85dce1629477a8efae625f86d) )
	ROM_LOAD( "co_25_bp.3_1", 0x0000, 0x010000, CRC(4060da30) SHA1(34a93e0550992e7510d1eaf2d5109da3c3fa2f75) )
	ROM_LOAD( "co_25_dc.1_1", 0x0000, 0x010000, CRC(a3f34f3f) SHA1(4c4842cc668b4c3abd9d2896cc50bbc3d9643b75) )
	ROM_LOAD( "co_25_dp.2_1", 0x0000, 0x010000, CRC(1f192638) SHA1(16a86242281281ca7b994dee06910d7f107c4743) )
	ROM_LOAD( "co_25_kc.1_1", 0x0000, 0x010000, CRC(6b2a7f2b) SHA1(5d558431a6b83214cc0dc33d999eeb72f3c53e85) )
	ROM_LOAD( "co_25_kp.2_1", 0x0000, 0x010000, CRC(73d3aaba) SHA1(5ef9118462bfdf93182ec539d3b80a72c09fa032) )
	ROM_LOAD( "co_25_kp.3_1", 0x0000, 0x010000, CRC(2e8fa3ee) SHA1(18f4e4eae7d7ac14486ed731bb67cab22d0b287d) )
	ROM_LOAD( "co_25a_c.1_1", 0x0000, 0x010000, CRC(e753196a) SHA1(79fc9b567dc946f81d40c3b215035cf2adcf94af) )
	ROM_LOAD( "co_25a_p.2_1", 0x0000, 0x010000, CRC(a058d7f7) SHA1(6e9116ce757503ef5b1822473a310513dd2973e3) )
	ROM_LOAD( "co_25a_p.3_1", 0x0000, 0x010000, CRC(ba172858) SHA1(d0a025c339f886802b7491448b6b2a4e1f5a3451) )
	ROM_LOAD( "co_25b_c.1_1", 0x0000, 0x010000, CRC(d82a9080) SHA1(9b9091f867f0b5d75f2bd5f9d62a4419073da357) )
	ROM_LOAD( "co_25b_p.2_1", 0x0000, 0x010000, CRC(48c8f7e6) SHA1(ff651e4c88b81bb816ab92e7f1d1fbd2c2920db1) )
	ROM_LOAD( "co_25bgc.1_1", 0x0000, 0x010000, CRC(947a5465) SHA1(c99c0e8ca515fbad8801e07e5936256cca8e7af1) )
	ROM_LOAD( "co_25bgp.2_1", 0x0000, 0x010000, CRC(84ab9c9d) SHA1(4c1043fb7ff6cd4e681f18e2dd0ddd29d5ce6d09) )
	ROM_LOAD( "co_25bgp.3_1", 0x0000, 0x010000, CRC(7fa4089e) SHA1(f73aff58c7a627f993e65527bb551c23640b22ed) )
	ROM_LOAD( "co_25btc.1_1", 0x0000, 0x010000, CRC(dbbae1b6) SHA1(2ee1d53872774d4b80f8c1a4e8a6ceb9e79ed6f5) )
	ROM_LOAD( "co_25btp.2_1", 0x0000, 0x010000, CRC(cea10ed8) SHA1(2b10193824afb50d561b2307a3189d14e2f5d47a) )
	ROM_LOAD( "co_30_bc.2_1", 0x0000, 0x010000, CRC(2d39f5fa) SHA1(20075621085765150a57233eccd61f16dbbae9b1) )
	ROM_LOAD( "co_30_bp.4_1", 0x0000, 0x010000, CRC(0826ffa7) SHA1(05a12d68acf69d8a582fc7fee91a282280380420) )
	ROM_LOAD( "co_30_dc.2_1", 0x0000, 0x010000, CRC(b028d639) SHA1(9393b82d41e7f8a7e2dba33545477ae13a8d6804) )
	ROM_LOAD( "co_30_dp.4_1", 0x0000, 0x010000, CRC(f40f09ca) SHA1(11f7af5bf78768759c3eba50ab1a906e81ce1100) )
	ROM_LOAD( "co_30_kp.4_1", 0x0000, 0x010000, CRC(97ab5c33) SHA1(dc6b9705de4731a5cbc35557ca26c80b20cc6518) )
	ROM_LOAD( "co_30a_c.2_1", 0x0000, 0x010000, CRC(eea1522d) SHA1(fdfe797b8cf2fa10f24e89c3047290ac63acebc7) )
	ROM_LOAD( "co_30b_c.2_1", 0x0000, 0x010000, CRC(61e873b0) SHA1(00037fde263fe9e9cb227ef2945e8b90feee0d6e) )
	ROM_LOAD( "co_30bdc.2_1", 0x0000, 0x010000, CRC(6f261bdc) SHA1(df7ab51c984b20665fdb327d17ca6ec32109ec2d) )
	ROM_LOAD( "co_30bgc.2_1", 0x0000, 0x010000, CRC(85cd1c27) SHA1(e0c250bf2848b6991cf33c07b43c2704ae906e47) )
	ROM_LOAD( "co_30btc.2_1", 0x0000, 0x010000, CRC(3a940326) SHA1(f1a0eca5ceccbf979ac7a2c51bfdc1de6f0aa40e) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "copycatsnd.bin", 0x0000, 0x080000, CRC(cd27a3ce) SHA1(d061fae0ef8584d2e349e91e53f41718128c61e2) )
ROM_END




ROM_START( m4cpfinl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cu_10___.5_1", 0x0000, 0x010000, CRC(47a85443) SHA1(d308b9a6dcb0200f72d5c5b380907d2d55f3e40d) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cu_10_b_.5_1", 0x0000, 0x010000, CRC(2583f410) SHA1(447a2316e3c3da6f835699602834f7ca5bafbdf9) )
	ROM_LOAD( "cu_10_d_.5_1", 0x0000, 0x010000, CRC(e3bfac76) SHA1(bf1bfa7a995dc4198de890d307718ddb2d9e0092) )
	ROM_LOAD( "cu_10_k_.5_1", 0x0000, 0x010000, CRC(81940c25) SHA1(7bb98b0f6355c6cea12c2ffb2751c4dc35042120) )
	ROM_LOAD( "cu_20___.5_1", 0x0000, 0x010000, CRC(69a38622) SHA1(65d83c9590bf200077046d564778e7fc7d146030) )
	ROM_LOAD( "cu_20_b_.5_1", 0x0000, 0x010000, CRC(0b882671) SHA1(0f3382385c9065eb275d59cb17045a592bdc11cc) )
	ROM_LOAD( "cu_20_d_.5_1", 0x0000, 0x010000, CRC(cdb47e17) SHA1(f491049cbd2f5a1f803a3f22ef12f4d41020a928) )
	ROM_LOAD( "cu_20_k_.5_1", 0x0000, 0x010000, CRC(af9fde44) SHA1(916295e999ea1017a6f24a73436d97cf89f827c8) )
	ROM_LOAD( "cui20___.5_1", 0x0000, 0x010000, CRC(5b16231e) SHA1(1bf37843c6e757d7ffe9932eab72b7f49a9ef107) )
	ROM_LOAD( "cui20_b_.5_1", 0x0000, 0x010000, CRC(393d834d) SHA1(12766cdc1c75ed0d967d7937b9fea2cfb3b6c2c3) )
	ROM_LOAD( "cui20_d_.5_1", 0x0000, 0x010000, CRC(ff01db2b) SHA1(bd4f826f235e0c0bf53584abc0dc1929ac157d99) )
	ROM_LOAD( "cui20_k_.5_1", 0x0000, 0x010000, CRC(9d2a7b78) SHA1(d7c26a47dcbb836650f3021733d09a426ff3a390) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "cupsnd_1.0_2", 0x000000, 0x080000, CRC(54384ce8) SHA1(ff78c4ea16722662a480bff1f85af7efe84b01e5) )
	ROM_LOAD( "cupsnd_1.0_3", 0x080000, 0x080000, CRC(24d3d848) SHA1(64287c3cbe2e9693954bc880d6edf2bc17b0ed65) )
ROM_END


ROM_START( m4danced )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "dd_22bg_.2_1", 0x0000, 0x020000, CRC(f79525a1) SHA1(babfbf8beae423626057235bcad5eae18531160e) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "dd_22bg_.4_1", 0x0000, 0x020000, CRC(e50ffa46) SHA1(b42d806422f85573bcbe284b4192f393e3e57306) )
	ROM_LOAD( "dd_22bt_.4_1", 0x0000, 0x020000, CRC(11e910a2) SHA1(6e35ae37dbd12169ccd9cf5b32a3f08f9e3d1899) )
	ROM_LOAD( "dd_25__c.2_1", 0x0000, 0x020000, CRC(1b072e2b) SHA1(cd40d0b7ef29979d65bfabdffee9cf686c6d6f49) )
	ROM_LOAD( "dd_25_bc.2_1", 0x0000, 0x020000, CRC(41f544ec) SHA1(5676f3ca6abc71fdb9286a3b47cb789990298548) )
	ROM_LOAD( "dd_25_dc.2_1", 0x0000, 0x020000, CRC(28b4b2bc) SHA1(8d6294fa4c10fa26322130e6ac1d227b777e8028) )
	ROM_LOAD( "dd_25a_c.2_1", 0x0000, 0x020000, CRC(e243b150) SHA1(8828b73921d49fa9e61ac56325bdc35e60b06d1e) )
	ROM_LOAD( "dd_25b_c.2_1", 0x0000, 0x020000, CRC(4fb0abb1) SHA1(bb8099c90312eb4509b15f098636d6c8e1db5a1d) )
	ROM_LOAD( "dd_25bgc.2_1", 0x0000, 0x020000, CRC(7e8c62bf) SHA1(63515ec61ff0e8fb4d62e14059001e2a33906bd6) )
	ROM_LOAD( "dd_25btc.2_1", 0x0000, 0x020000, CRC(10f3d837) SHA1(71958987e481fba5c5a43495d0c2f823e21e234c) )
	ROM_LOAD( "dd_26a__.5_1", 0x0000, 0x020000, CRC(e854ceb3) SHA1(1bb2eea5d353f62558993f3845bd3d11611045d1) )
	ROM_LOAD( "dd_26b__.4_1", 0x0000, 0x009712, CRC(3f974b8a) SHA1(9563c204e290756494f1efae67d2cfafefe1163e) )
	ROM_LOAD( "dd_26bg_.4_1", 0x0000, 0x009712, CRC(0ac7f8e4) SHA1(a8ebdcd281b46b3546f58f726f4d57d4ed41777b) )
	ROM_LOAD( "dd_26bt_.4_1", 0x0000, 0x009715, CRC(aab6371c) SHA1(cb0eb4fb1630ff36c872d17cb2984cc43e691f7e) )
	ROM_LOAD( "dd_26s__.4_1", 0x0000, 0x020000, CRC(ba1e2ec8) SHA1(c4f5a46a684219bdf531234c3dc06fda4b7c7a3e) )
	ROM_LOAD( "dd_26sb_.4_1", 0x0000, 0x00970e, CRC(b0b04ab6) SHA1(52d47bd159a26c745c3c61daae1ed68099f5d297) )
	ROM_LOAD( "dd_32a__.3_1", 0x0000, 0x020000, CRC(b6f7a984) SHA1(70323926114855071981f3b8f00064b0149824d1) )
	ROM_LOAD( "dd_32b__.3_1", 0x0000, 0x020000, CRC(013c05d2) SHA1(a332ddb36c137cc111a629adee60d2d89c5f2d88) )
	ROM_LOAD( "dd_32bg_.3_1", 0x0000, 0x020000, CRC(a949d679) SHA1(a282c498b263c6a85323f42d41ac706f44047adf) )
	ROM_LOAD( "dd_32bt_.3_1", 0x0000, 0x020000, CRC(f7935bd2) SHA1(cfa65404fa091f445a6b2f30a359739087b7d57b) )
	ROM_LOAD( "dd_32s__.3_1", 0x0000, 0x020000, CRC(46fc506e) SHA1(87f2a201e39e78d33ce87b9be44ddf5d5e62106d) )
	ROM_LOAD( "dd_32sb_.3_1", 0x0000, 0x020000, CRC(9155deb5) SHA1(5b3ee18fd003e882f80a6c14d01215b4eeb8831e) )
	ROM_LOAD( "dd_32sd_.3_1", 0x0000, 0x0096b3, CRC(4135b5d9) SHA1(568563d3057717754a348b9a8870f41441a3c6a7) )
	ROM_LOAD( "dd_sja__.2_1", 0x0000, 0x020000, CRC(45db5106) SHA1(b12a5c2c3f61cc78a7ff040e1ffff82c225e6d9e) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "dd______.1_2", 0x0000, 0x080000, CRC(b9043a08) SHA1(5d87a30f23e8b5e3eaa0584d0d49efc08209882b) )
ROM_END


ROM_START( m4daytn )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "da_78___.1_0", 0x0000, 0x040000, CRC(50beafdd) SHA1(0ef6dd4fc9c8cda596fd383e47b9c7976b5d15f0) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "da_78_d_.1_0", 0x0000, 0x040000, CRC(d55d3f9a) SHA1(a145379237947601f2ecb84138c113b71842cd34) )
	ROM_LOAD( "da_80___.1_0", 0x0000, 0x040000, CRC(60f31edd) SHA1(ee016e5001cc80f6796b3a00ceebf14b5fb38ae7) )
	ROM_LOAD( "da_80_d_.1_0", 0x0000, 0x040000, CRC(e5108e9a) SHA1(4d0d530803bd349f57a2a3216fe284e7490a0ae0) )
	ROM_LOAD( "da_82___.1_0", 0x0000, 0x040000, CRC(93a22c97) SHA1(bcb0089c8d2febedd84207851a2c7997e31f8a27) )
	ROM_LOAD( "da_82_d_.1_0", 0x0000, 0x040000, CRC(1641bcd0) SHA1(66e4bc49d6dc8dc1926f3c2faac21498df6082d1) )
	ROM_LOAD( "da_84___.1_0", 0x0000, 0x040000, CRC(5d207c08) SHA1(57d6d5947c611badb1573739995037beb8017774) )
	ROM_LOAD( "da_84_d_.1_0", 0x0000, 0x040000, CRC(d8c3ec4f) SHA1(f3571c36114809dd440cee5d406f3da704797b01) )
	ROM_LOAD( "da_86___.1_0", 0x0000, 0x040000, CRC(ae714e42) SHA1(17ec05b35c28968c4319a71b1485052e04c23c0a) )
	ROM_LOAD( "da_86_d_.1_0", 0x0000, 0x040000, CRC(2b92de05) SHA1(f59e66c486c51bc0e968fc90009884e6ac851b93) )
	ROM_LOAD( "da_88___.1_0", 0x0000, 0x040000, CRC(1b55db77) SHA1(147a36ea76ba30eb465df3e2ad5795d8e3f96d99) )
	ROM_LOAD( "da_88_d_.1_0", 0x0000, 0x040000, CRC(9eb64b30) SHA1(f527ef38f9965774f59b5d36f45f801c7d7ce714) )
	ROM_LOAD( "da_90___.1_0", 0x0000, 0x040000, CRC(a54d2e81) SHA1(65f08c83dcff2934938a7aa1b56e02ba18ba7898) )
	ROM_LOAD( "da_90_d_.1_0", 0x0000, 0x040000, CRC(20aebec6) SHA1(bcf4ca9fa5723fcae0ea661b7cfa005cd0046cb1) )
	ROM_LOAD( "da_92_d_.1_0", 0x0000, 0x040000, CRC(9e99647d) SHA1(34cf734808ffbfa9bc920ad1c93c0a9f7bbba791) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END



ROM_START( m4excal )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "ex_05a__.6_1", 0x0000, 0x020000, CRC(317fa289) SHA1(8a0e83a764e2a04285367e0f7ebb814fedc81400) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ex_20a_6.6_1", 0x0000, 0x020000, CRC(284937c8) SHA1(3be8bf21ab0ff97f67ce170cee48cd08ea325571) )
	ROM_LOAD( "ex_20a_8.6_1", 0x0000, 0x020000, CRC(45fb3559) SHA1(9824d2e6bc8c2249de66aea2422b7a6efd0e37b3) )
	ROM_LOAD( "ex_20a_c.7_1", 0x0000, 0x020000, CRC(ab773d70) SHA1(3a5e3aa3978bf0bc9a4924d3225665b583c90abc) )
	ROM_LOAD( "ex_20atc.7_1", 0x0000, 0x020000, CRC(ec5c7bc7) SHA1(c57eb69044858b7ea43726d0f49e1c224aeb1377) )
	ROM_LOAD( "ex_20s_6.6_1", 0x0000, 0x020000, CRC(44176959) SHA1(fcaad1732176330b382f1690293ed43fa66de084) )
	ROM_LOAD( "ex_20s_8.6_1", 0x0000, 0x020000, CRC(29a56bc8) SHA1(7345b8bd4e3cf64ca222c59857ef7df9b1db45c1) )
	ROM_LOAD( "ex_20sb6.6_1", 0x0000, 0x020000, CRC(e387c4b9) SHA1(cc83de53a1ffe8d75acb9c2f2ceca9bbff516df7) )
	ROM_LOAD( "ex_20sb8.6_1", 0x0000, 0x020000, CRC(8e35c628) SHA1(000811c50189cae0f7327b4e7484eb64091a989c) )
	ROM_LOAD( "ex_20sd6.6_1", 0x0000, 0x020000, CRC(7adbd752) SHA1(d758e202ed75ce3a99f0666c405090107cb0feca) )
	ROM_LOAD( "ex_20sd8.6_1", 0x0000, 0x020000, CRC(1769d5c3) SHA1(3e5ff338364781e99685cde9e45707dd88d1da11) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "ex______.1_2", 0x000000, 0x080000, CRC(9305e516) SHA1(5a5c67f97761fe2e042ba31594d1881238d3227b) )
	ROM_LOAD( "ex______.1_3", 0x080000, 0x080000, CRC(29e3709a) SHA1(2e2f089aa2a938158930f235bf821685932d698b) )
	ROM_LOAD( "ex______.1_4", 0x100000, 0x080000, CRC(dd747003) SHA1(cf0a2936c897e3b833984c55f4825c358b723ab8) )
ROM_END



ROM_START( m4exotic )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "eo_49bg_.2_0", 0x0000, 0x020000, CRC(c3bf2286) SHA1(74090fd0a103a6c311d426f4aae8e7af8b1d3bc0) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "eo_49bm_.2_0", 0x0000, 0x020000, CRC(c748c4ca) SHA1(7d0d498f9edd792ed861c8bf9cf1bb03698d144d) )
	ROM_LOAD( "eo_49bmd.2_0", 0x0000, 0x020000, CRC(98436c04) SHA1(1db7c95f7a0297aa3da7f1ce27c790ffa1fa4ebe) )
	ROM_LOAD( "eo_49bmd.2g0", 0x0000, 0x020000, CRC(425a0152) SHA1(6a235c613f52c4b8985a589f89542eebd3574fde) )
	ROM_LOAD( "eo_s9bt_.2g0", 0x0000, 0x020000, CRC(c527d333) SHA1(083d7be95d73d259fe8ec1d87a3a41089a4c44df) )
	ROM_LOAD( "eo_sja__.2_0", 0x0000, 0x020000, CRC(5ca9557f) SHA1(5fa42c56c67b505272d358a54ebe911fdb0b905e) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4firice )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "fi_20__d.5_0", 0x0000, 0x040000, CRC(ab46574c) SHA1(d233b137f8f42b9b644b34a627fbcc5b662e8ae1) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "fi_20_bd.5_0", 0x0000, 0x040000, CRC(9b2bc052) SHA1(34b970659218fde097238b852dadedcb928f69fd) )
	ROM_LOAD( "fi_20_dd.5_0", 0x0000, 0x040000, CRC(2bbc9855) SHA1(84d51eeadc01ac74d630a05b933343f01f04b2af) )
	ROM_LOAD( "fi_20_kd.5_0", 0x0000, 0x040000, CRC(529b43b1) SHA1(80d4d928918fdc869b1693e21a5c25045e5c9449) )
	ROM_LOAD( "fi_20a_d.5_0", 0x0000, 0x040000, CRC(8d6ce79d) SHA1(05954ed4b34af73c065b6203a50da9af7d8373fe) )
	ROM_LOAD( "fi_20s_d.5_0", 0x0000, 0x040000, CRC(d0aa53af) SHA1(f71801344c17a759ec4eb8958377bbcf4b4cae65) )
	ROM_LOAD( "fi_sj___.e_0", 0x0000, 0x040000, CRC(7f12e37a) SHA1(fb09ff782f66972b8bdeff105c5f3d1f9f676809) )
	ROM_LOAD( "fi_sj_b_.e_0", 0x0000, 0x040000, CRC(5aef48d2) SHA1(73f410951a737f75f3e7c14e704eca9c26cfa750) )
	ROM_LOAD( "fi_sj_d_.e_0", 0x0000, 0x040000, CRC(61822af2) SHA1(8c721229a5ce9f491cbc638b8c5fa5c0c3032700) )
	ROM_LOAD( "fi_sj_k_.e_0", 0x0000, 0x040000, CRC(2b57036b) SHA1(60cec130770ff643af1148f16a3afe3b102e94e2) )
	ROM_LOAD( "fi_sja__.e_0", 0x0000, 0x040000, CRC(da5e0eff) SHA1(9f5ddce366786bdf898c9410be417c8028cebeb4) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "fire_ice.s1", 0x000000, 0x080000, CRC(74ee37c1) SHA1(bc9b419dd1fd1c66090f8946247e754c0267baa3) )
	ROM_LOAD( "fire_ice.s2", 0x080000, 0x080000, CRC(b86bafeb) SHA1(ee237f601b970dc5be8096a4018cb6a3edac500f) )
	ROM_LOAD( "fire_ice.s3", 0x100000, 0x080000, CRC(75f349b3) SHA1(1505bec7b69e1eabd679b70d95ae58fd264ca698) )
ROM_END

ROM_START( m4flshlt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "flt02__1.0", 0x0000, 0x010000, CRC(3e060051) SHA1(1ef132ecf514d3ad0b5f2a4d04062fcc95713a46) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "flt05__0.5", 0x0000, 0x010000, CRC(8b290639) SHA1(ec75be98fc1afd95d5ffff0ece288c4f51b9f43c) )
	ROM_LOAD( "flt05__1.0", 0x0000, 0x010000, CRC(b6c0b96e) SHA1(d1d526671444e96115439270c48632d626e5439f) )
	ROM_LOAD( "flt10__1.0", 0x0000, 0x010000, CRC(df16930c) SHA1(d230a524b1757ea52d2e00cf01a97a6ce762a7c2) )
	ROM_LOAD( "flt10d_1.0", 0x0000, 0x010000, CRC(6908ddb1) SHA1(2aaae24340b2ddc9571d2e1bca64597cf9836ef9) )
	ROM_LOAD( "flt20__1.0", 0x0000, 0x010000, CRC(fdac16fa) SHA1(615cae707b3994d4935a92b3991b966d6a528fbc) )
	ROM_LOAD( "flt20d_1.0", 0x0000, 0x010000, CRC(d1d88302) SHA1(b0234897170ae535e4571d61a73cf34183c73aab) )
	ROM_LOAD( "flt20dy1.0", 0x0000, 0x010000, CRC(a9962461) SHA1(22fc22125846ca73b0ccb1ff5338600f52184493) )
ROM_END


ROM_START( m4fourmr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "frmr5p26.bin", 0x8000, 0x008000, CRC(f0c5bd8a) SHA1(39026459008ed5b5bd3a10841799227fef70e5b5) )
ROM_END





ROM_START( m4harle )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "hq_20a__.1_1", 0x0000, 0x010000, CRC(b8ae3025) SHA1(94a449eff103bf6ba1fc6e85b03061b9ce658ae0) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "hq_20bg_.1_1", 0x0000, 0x010000, CRC(31356248) SHA1(d8791b1c861ed4388660bbe78f2589db7f1e779e) )
	ROM_LOAD( "hq_20bg_.2a1", 0x0000, 0x010000, CRC(f50898af) SHA1(ba4470abb85b92d647ff8da48dba571cec5f594e) )
	ROM_LOAD( "hq_20bt_.1_1", 0x0000, 0x010000, CRC(925a5e9e) SHA1(e8a4f75d4f7e6825894e19a915db21ee21206b5b) )
	ROM_LOAD( "hq_20s__.1_1", 0x0000, 0x010000, CRC(6de12c08) SHA1(df279a07ccd448b9a6948ccb059aff6306ddcc99) )
	ROM_LOAD( "hq_20sb_.1_1", 0x0000, 0x010000, CRC(0fca8c5b) SHA1(f430f2d1ff71513426bb5cfed883797de6afd3a7) )
	ROM_LOAD( "hq_20sd_.1_1", 0x0000, 0x010000, CRC(c9f6d43d) SHA1(4eef4ec6bff20fdf5b20b266ddd1646045390d42) )
	ROM_LOAD( "hq_20sk_.1_1", 0x0000, 0x010000, CRC(abdd746e) SHA1(5bb7043a1a790e5c784c7a51fd3c023b3099439f) )
	ROM_LOAD( "hq_xea__.2f1", 0x0000, 0x010000, CRC(4ef7c19c) SHA1(bd8e5c69ab31c5a9c89a7f2020a61bde5e00b6b2) )
	ROM_LOAD( "hq_xea__.2s1", 0x0000, 0x010000, CRC(05d42107) SHA1(e1dc4e2f84ebadead994a09c53c2d9a1ed8e29aa) )
	ROM_LOAD( "hq_xes__.2f1", 0x0000, 0x010000, CRC(9bb8ddb1) SHA1(c8bba9bdfd7a0bbcbe89cd422d37909f7ebcaa82) )
	ROM_LOAD( "hq_xes__.2s1", 0x0000, 0x010000, CRC(d09b3d2a) SHA1(4de366ed5eae89f2bb39abc052d88cb59e846307) )
	ROM_LOAD( "hq_xesd_.2a1", 0x0000, 0x010000, CRC(b402aa45) SHA1(6c6eed7b172604112a2c00df9fd00476d07cc971) )
	ROM_LOAD( "hq_xesd_.2f1", 0x0000, 0x010000, CRC(3faf2584) SHA1(01b049b6ae44771f37b298dc525e16b6e9a182f2) )
	ROM_LOAD( "hq_xesd_.2s1", 0x0000, 0x010000, CRC(748cc51f) SHA1(d2ac7c237be40564809b006bf68d09560817f97d) )
	ROM_LOAD( "ph_20a__.2s1", 0x0000, 0x010000, CRC(559bf168) SHA1(3c6a47bba52481af3f987d284a2102a8ee2cc7e6) )
	ROM_LOAD( "ph_20bg_.1_1", 0x0000, 0x010000, CRC(4c96d2fa) SHA1(1ff8c8c5dc6a67ac187ba0b96cd93c786884aa3b) )
	ROM_LOAD( "ph_20bt_.1_1", 0x0000, 0x010000, CRC(eff9ee2c) SHA1(ae15ee4bbc3028580b3c6cbc8a078fbe3291283e) )
	ROM_LOAD( "ph_20s__.1_1", 0x0000, 0x010000, CRC(4cccba32) SHA1(6d2ec2324866555dfa3f3fd5b79ff3883e1b2ebe) )
	ROM_LOAD( "ph_20s__.2s1", 0x0000, 0x010000, CRC(80d4ed45) SHA1(c818f3e154cfa1efcd11efb71061aa5bfc2e668f) )
	ROM_LOAD( "ph_20sb_.1_1", 0x0000, 0x010000, CRC(2ee71a61) SHA1(0e91cc5b22899f9765cabb9416fcafed790951ae) )
	ROM_LOAD( "ph_20sb_.2f1", 0x0000, 0x010000, CRC(1f56841a) SHA1(dfd90d8af765bc981d0dcb4b0ffb5ce613bfcbed) )
	ROM_LOAD( "ph_20sd_.1_1", 0x0000, 0x010000, CRC(e8db4207) SHA1(bd806e5b04207b3121284c485a6ab8a385231504) )
	ROM_LOAD( "ph_20sd_.2s1", 0x0000, 0x010000, CRC(24c31570) SHA1(a6da77ebd80dc234add5da880e3472f2f1e2ca3d) )
	ROM_LOAD( "ph_20sk_.1_1", 0x0000, 0x010000, CRC(8af0e254) SHA1(b7d80ab84684bdabe169623864b9efd0d3881f2e) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "hq______.1_2", 0x000000, 0x080000, CRC(0c8c14be) SHA1(a1268e71ea43772a532b63327f24c64fabd7e715) )
	ROM_LOAD( "hq______.1_3", 0x080000, 0x080000, CRC(5a07e514) SHA1(6e589756c0fc4b0458ca856e918fa3b7cd396c39) )
ROM_END


ROM_START( m4hvhel )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "hh_20__d.2_0", 0x0000, 0x040000, CRC(801de788) SHA1(417b985714d8f0ebed93b65a3f865e03474ce9e5) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "hh_20a_d.2_0", 0x0000, 0x040000, CRC(ea4e7876) SHA1(5bf711c2bdff50fe745edefa0eebf719824d9e5b) )
	ROM_LOAD( "hh_20s_d.2_0", 0x0000, 0x040000, CRC(a519a441) SHA1(f3c19d316c82d1ebbcfdabb6d4eaa6cfa369d287) )
	ROM_LOAD( "hh_sj___", 0x0000, 0x040000, CRC(04577b99) SHA1(48689c3a96bc42ad64dc4d363dad38c967f0cdcc) )
	ROM_LOAD( "hh_sj___.f_0", 0x0000, 0x040000, CRC(8ab33720) SHA1(0c9283a20c3f008baa8ce027d1266e4ef49ca56b) )
	ROM_LOAD( "hh_sjs__.f_0", 0x0000, 0x040000, CRC(8854763d) SHA1(323bd76a014e52e3b12427998b0e2851463246c8) )
	ROM_LOAD( "hh_vc___.g_0", 0x0000, 0x040000, CRC(db338fb7) SHA1(e7e92293374721e7360493e9ef189991dad0a1ee) )
	ROM_LOAD( "hh_vc_d_.g_0", 0x0000, 0x040000, CRC(292468bd) SHA1(f9b19f57a49c1afd670c68b7acd85d4141adfce1) )
	ROM_LOAD( "h_hell._pound5", 0x0000, 0x040000, CRC(cd59c0d0) SHA1(8caad9043a277fa39a3ad2d5ec3388c121e7f697) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "hh___snd.1_1", 0x000000, 0x080000, CRC(afa7ba60) SHA1(25278046252e49364d4a51de79295b87baf6018e) )
	ROM_LOAD( "hh___snd.1_2", 0x080000, 0x080000, CRC(ec1ec822) SHA1(3fdee0526cb70f4951b7bbced74e32641ded9b7b) )
	ROM_LOAD( "hh___snd.1_3", 0x100000, 0x080000, CRC(d4119155) SHA1(b61c71e1ee0dbfc0bb9eff1a8c019cf11731ee11) )
ROM_END


ROM_START( m4holywd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "hollywood 5p.bin", 0x0000, 0x010000, CRC(fb4ebb6e) SHA1(1ccfa81c173011ce70640097c85b532fd44f5a6e) )
ROM_END



ROM_START( m4indycr )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "ic_sj___.1_0", 0x0000, 0x040000, CRC(4dea0d17) SHA1(4fa19896dbb5e8f21ac7e74efc56de5cadd5bf54) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ic___.g", 0x0000, 0x000657, CRC(25cc2b32) SHA1(a065276fe4cc1e55f366ac216b5bc8cf934da9dd) )
	ROM_LOAD( "ic___.p", 0x0000, 0x00058b, CRC(26388ec4) SHA1(e33183d0c3e75fe23ab6df01d561f5b65923d1db) )
	ROM_LOAD( "ic_sj___.2_0", 0x0000, 0x040000, CRC(6d0ddf54) SHA1(0985aa9fddb71a499d266c12893aabbab8755319) )
	ROM_LOAD( "ic_sj_b_.1_0", 0x0000, 0x040000, CRC(4bc0cb73) SHA1(d4c048ba9578add0104f0c529f20356c3502ea71) )
	ROM_LOAD( "ic_sj_d_.1_0", 0x0000, 0x040000, CRC(165ad977) SHA1(daa444e0d128859832094d3b07026483cd3466ce) )
	ROM_LOAD( "ic_sj_d_.2_0", 0x0000, 0x040000, CRC(36bd0b34) SHA1(306d6e6536a4137353f9b895e64c7e9a5c79561a) )
	ROM_LOAD( "ic_sj_k_.1_0", 0x0000, 0x040000, CRC(857fda64) SHA1(3eb230ea1adf9acb4cf83422c4bb1cde40756310) )
	ROM_LOAD( "ic_sjs__.1_0", 0x0000, 0x040000, CRC(6310b904) SHA1(0f2cd7ed83f77423bcfb2a71144fab2047dfea13) )
	ROM_LOAD( "icsj___.1_0", 0x0000, 0x01215c, CRC(a8217e6c) SHA1(df419dc92196f1b7cd3c473df05d3b1429aecb05) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "da______.1_1", 0x000000, 0x080000, CRC(99d36c12) SHA1(f8848a28b5546649d6a3f8599dbc4ca84bdac77c) )
	ROM_LOAD( "da______.1_2", 0x080000, 0x080000, CRC(32b40094) SHA1(f02c3b088d76116f817b536cf7cec5188b2f73bf) )
	ROM_LOAD( "da______.1_3", 0x100000, 0x080000, CRC(2df33d18) SHA1(40afa32d6c72c6a76e3e2e61db19a16003f4e176) )
	ROM_LOAD( "da______.1_4", 0x180000, 0x080000, CRC(8e254a3b) SHA1(bc3643ea5878bbde110ee6971c5149b3320bcffc) )
ROM_END


ROM_START( m4jakjok )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "jj_sj___.6_0", 0x0000, 0x040000, CRC(7bc45b0e) SHA1(f30fef8fccdac04859f1ff93198a497eff723020) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "jj_sj_k_.3_0", 0x0000, 0x040000, CRC(c33dd82f) SHA1(c1f3f6ca1c45503b7f71e897e5c27368f5efb439) )
	ROM_LOAD( "jj_sjs__.6_0", 0x0000, 0x040000, CRC(4bcac6f5) SHA1(7dc07a7a61a6ba044020d6c2496143168c103a70) )
	ROM_LOAD( "jj_vc___.7_0", 0x0000, 0x040000, CRC(4cdca8da) SHA1(ee7448b12380416a3bea2713ed5feca7473be8aa) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "jj_____.1_1", 0x000000, 0x080000, CRC(e759a958) SHA1(b107f4ef5a2805e56d4024940bfc632155de1eb1) )
	ROM_LOAD( "jj_____.1_2", 0x080000, 0x080000, CRC(aa215ff0) SHA1(4bf2c6f8153730cc3ca86f78ec14063ece7d8700) )
	ROM_LOAD( "jj_____.1_3", 0x100000, 0x080000, CRC(03c0ffc3) SHA1(2572f62362325df8b235b487d4a764218e7f1589) )
ROM_END




ROM_START( m4jflash )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "jf_25__c.2_1", 0x0000, 0x020000, CRC(4d5f1a12) SHA1(c25b6d899b74231da505bde7b671be001bdcea5d) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "jf_25a_c.2_1", 0x0000, 0x020000, CRC(76722e15) SHA1(4bd107049ad98b848cdaba3a1318373bbd06ab9f) )
	ROM_LOAD( "jf_25b_c.2_1", 0x0000, 0x020000, CRC(35a927c6) SHA1(6776fe77ad8a85feecdedfad0eac89f9cb826fbf) )
	ROM_LOAD( "jf_25bdc.2_1", 0x0000, 0x020000, CRC(d372689e) SHA1(ecad53022c7786f387586484a3e679afbf0bac37) )
	ROM_LOAD( "jf_25bgc.2_1", 0x0000, 0x020000, CRC(421084bc) SHA1(b6f847468c20a3f85d9e77b633dc48adb43c970f) )
	ROM_LOAD( "jf_25btc.2_1", 0x0000, 0x020000, CRC(1c701822) SHA1(559b4d269a3af388aa86f28ccedd22505fcdb355) )
	ROM_LOAD( "jf_25d_c.2_1", 0x0000, 0x020000, CRC(9cde7bfc) SHA1(a0b840f00c487e963f4dd9f58e3abca7b0cea31b) )
	ROM_LOAD( "jf_25dkc.2_1", 0x0000, 0x020000, CRC(ef583053) SHA1(23b33f0e49d8efceb4a6690ac58da1ccf6576a1a) )
	ROM_LOAD( "jf_25k_c.2_1", 0x0000, 0x020000, CRC(d82701b8) SHA1(a784fa11877e05f2219f1452463ec3348c84e879) )
	ROM_LOAD( "jf_25sbc.2_1", 0x0000, 0x020000, CRC(c6bce1c6) SHA1(708002059d307a02fec32a3cdb6eff995a438631) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4kingq )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ee_05a_4.2_1", 0x0000, 0x010000, CRC(8dd842b6) SHA1(1c1bcaae355ceee4d7b1572b0fa1a8b23a8afdbf) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ee_05a__.2_1", 0x0000, 0x010000, CRC(36aa5fb9) SHA1(b4aaf647713e33e79be7927e5eeef240d3beedf7) )
	ROM_LOAD( "ee_20a__.2_1", 0x0000, 0x010000, CRC(2c61341f) SHA1(76d68ae2a44087414be8be12b3824c62311721dd) )
	ROM_LOAD( "ee_20a_c.1_1", 0x0000, 0x010000, CRC(948140ac) SHA1(d43f1f2903ecd809dee191087fa075c638728a5b) )
	ROM_LOAD( "ee_20b__.2_1", 0x0000, 0x010000, CRC(2fc7c7c2) SHA1(3b8736a582009d7b1455769374342ff72026d2fa) )
	ROM_LOAD( "ee_20b_c.1_1", 0x0000, 0x010000, CRC(70d399ab) SHA1(ca2c593151f4f852c7cb66859a12e832e53cd31f) )
	ROM_LOAD( "ee_20bd_.2_1", 0x0000, 0x010000, CRC(239de2dd) SHA1(c8021ba5bfdc10f59fec27c364035225093328d8) )
	ROM_LOAD( "ee_20bdc.1_1", 0x0000, 0x010000, CRC(cbb8c57b) SHA1(ea165199213f95128aec95ae40799faa8c457dd3) )
	ROM_LOAD( "ee_20bg_.2_1", 0x0000, 0x010000, CRC(ddc4d832) SHA1(031f987e9fced1df4acc57eb4b60911d52e1dbf6) )
	ROM_LOAD( "ee_20bt_.2_1", 0x0000, 0x010000, CRC(6f278771) SHA1(4459c9490be14bcbc139eebe6542325c80937ff3) )
	ROM_LOAD( "ee_20s_c.1_1", 0x0000, 0x010000, CRC(a0c1e313) SHA1(8a088a33e51a31ff0abdb554aa4d8ce61eaf4b7d) )
	ROM_LOAD( "ee_20sb_.2_1", 0x0000, 0x010000, CRC(307ad157) SHA1(32b6187e907bfbdb87a9ad2d9ca5870b09de5e4a) )
	ROM_LOAD( "ee_25a_c.3_1", 0x0000, 0x010000, CRC(4dc25083) SHA1(b754b4003f73bd74d1670a36a70985ce5e48794d) )
	ROM_LOAD( "ee_25b_c.3_1", 0x0000, 0x010000, CRC(a6fe50ff) SHA1(011602d9624f232ba8484e57f5f33ff06091809f) )
	ROM_LOAD( "ee_25bdc.3_1", 0x0000, 0x010000, CRC(d0088a97) SHA1(aacc1a86bd4b321d0ee21d14147e1d135b3a5bae) )
	ROM_LOAD( "ee_25bgc.3_1", 0x0000, 0x010000, CRC(e4dcd86b) SHA1(b8f8ec317bf9f18e3d0ae9a9fd59349fee24530d) )
	ROM_LOAD( "ee_25btc.3_1", 0x0000, 0x010000, CRC(8f44347a) SHA1(09815a6e1d3a91cd2e69578bbcfef3203ddb33d6) )
	ROM_LOAD( "ee_25s_c.3_1", 0x0000, 0x010000, CRC(a6fe50ff) SHA1(011602d9624f232ba8484e57f5f33ff06091809f) )
	ROM_LOAD( "ee_25sbc.3_1", 0x0000, 0x010000, CRC(0f4bdd7c) SHA1(5c5cb3a9d6a96afc6e29149d2a8adf19aae0bc41) )
	ROM_LOAD( "eei20___.2_1", 0x0000, 0x010000, CRC(15f4b869) SHA1(5be6f660321cb47900dda986ef44eb5c1c324013) )
	ROM_LOAD( "knq2pprog.bin", 0x0000, 0x010000, CRC(23b22f79) SHA1(3d8b9cbffb9b427897548981ddacf724215336a4) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "kingsnqueenssnd.bin", 0x0000, 0x080000, CRC(31d722d4) SHA1(efb7079a1036cad8d9c08106f97c70a248b31898) )
	//ROM_LOAD( "knqsnd.bin", 0x0000, 0x080000, CRC(13012f48) SHA1(392b3bcf6f8e3e01082087637f9d378302d046c4) )
	ROM_LOAD( "ee______.1_2", 0x0000, 0x080000, CRC(13012f48) SHA1(392b3bcf6f8e3e01082087637f9d378302d046c4) )
ROM_END


ROM_START( m4kingqc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cn_20_b4.6_1", 0x0000, 0x010000, CRC(22d0b20c) SHA1(a7a4f60017cf62247339c9b23420d29845657895) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cn_20_bc.3_1", 0x0000, 0x010000, CRC(dfb0eb80) SHA1(ad973125681db0aae8ef1cf57b1c280e7f0e5803) )
	ROM_LOAD( "cn_20_dc.3_1", 0x0000, 0x010000, CRC(56e919ad) SHA1(c3c6f522574b287f7ed4dc4d1d8a32f68369dd5c) )
	ROM_LOAD( "cn_20a_c.2_1", 0x0000, 0x010000, CRC(0df15fd9) SHA1(e7c5e2277aac1c71d27710ea71d09d1005c5b8f9) )
	ROM_LOAD( "cn_20a_c.3_1", 0x0000, 0x010000, CRC(68e1dcef) SHA1(a0b15344b900226052633703e935c0ec0f718936) )
	ROM_LOAD( "cn_20a_c.5_1", 0x0000, 0x010000, CRC(4975d39e) SHA1(243b8af1a12c3538e826bfd5f6feb6927c1467b0) )
	ROM_LOAD( "cn_20b_c.2_1", 0x0000, 0x010000, CRC(66e074f3) SHA1(1f5381a41dd1402ee344e228635b35521e9377c8) )
	ROM_LOAD( "cn_20b_c.3_1", 0x0000, 0x010000, CRC(bcae86c9) SHA1(f03b136d82fe7b93350f0ca5dc36e78e98aecfa9) )
	ROM_LOAD( "cn_20b_c.5_1", 0x0000, 0x010000, CRC(6a19d734) SHA1(e0d5f5020e7997d3927b42336ab18757bd9f1ed0) )
	ROM_LOAD( "cn_20bg4.6_1", 0x0000, 0x010000, CRC(6d4158fe) SHA1(9c12264a415601d6f28f23c1e1f6a3d97fadddba) )
	ROM_LOAD( "cn_20bgc.3_1", 0x0000, 0x010000, CRC(3ecc1bf3) SHA1(fb191749f920aa4ac0d9809c6c59b695afdf6594) )
	ROM_LOAD( "cn_20bgc.5_1", 0x0000, 0x010000, CRC(24743f7e) SHA1(c90d95df2357bc00aba2bb21c0c77082b8c32463) )
	ROM_LOAD( "cn_20btc.2_1", 0x0000, 0x010000, CRC(b8f0ade0) SHA1(5b5344f799b27833f6456ae852eb5085afb3dbe5) )
	ROM_LOAD( "cn_20btc.3_1", 0x0000, 0x010000, CRC(b92f3787) SHA1(7efd815cf1a9a738ffae2c3ce19149f47d465c72) )
	ROM_LOAD( "cn_20btc.5_1", 0x0000, 0x010000, CRC(e2b8baf0) SHA1(23e966a6cc94c26903bfe943160a327529d7e21b) )
	ROM_LOAD( "cn_20s_c.2_1", 0x0000, 0x010000, CRC(66e074f3) SHA1(1f5381a41dd1402ee344e228635b35521e9377c8) )
	ROM_LOAD( "cn_20s_c.5_1", 0x0000, 0x010000, CRC(6a19d734) SHA1(e0d5f5020e7997d3927b42336ab18757bd9f1ed0) )
	ROM_LOAD( "cn_20sbc.5_1", 0x0000, 0x007e02, CRC(a20d7293) SHA1(2e83b73907b867aec20edacb9b49250f746faa3e) )
	ROM_LOAD( "cn_25_bc.2_1", 0x0000, 0x010000, CRC(3e2b2d7b) SHA1(9a68cf4902ca210e8fb52a35b4c507708c7f6d2a) )
	ROM_LOAD( "cn_25_dc.2_1", 0x0000, 0x010000, CRC(eb384ef6) SHA1(489c59d8e1e6296ec2b05fb0aa307c48f3486aa2) )
	ROM_LOAD( "cn_25_kc.2_1", 0x0000, 0x010000, CRC(bd11e742) SHA1(0c3b290e3010bc3f904f9087ee89efe63072b8c3) )
	ROM_LOAD( "cn_25a_c.2_1", 0x0000, 0x010000, CRC(1994efd5) SHA1(d7c3d692737138b30244d2d51eb535b88c87e401) )
	ROM_LOAD( "cn_25b_c.2_1", 0x0000, 0x010000, CRC(24255989) SHA1(017d0dc811b5c82d5b8785022169929c94f3f18a) )
	ROM_LOAD( "cn_25bdc.2_1", 0x0000, 0x010000, CRC(503ccd3c) SHA1(936b77b33373624e6bd80d168bcd48dc2ebcb2fe) )
	ROM_LOAD( "cn_25bgc.2a1", 0x0000, 0x010000, CRC(89e2130d) SHA1(22f97030e6f4cb94f62215a0c653d170ba3e0efd) )
	ROM_LOAD( "cn_25btc.2_1", 0x0000, 0x010000, CRC(876bc126) SHA1(debe36a082493cdeba26a0808f205a19e9e897d5) )
	ROM_LOAD( "cn_25s_c.1_1", 0x0000, 0x010000, CRC(84d1a32b) SHA1(f6e76a2bf1bd7b31eb360dea8b453d235c365e64) )
	ROM_LOAD( "cn_25sbc.1_1", 0x0000, 0x010000, CRC(e53b672c) SHA1(2aea2a243817857df31b6f7b767e380bd003fafa) )
	ROM_LOAD( "cn_30_dc.1_1", 0x0000, 0x010000, CRC(aeb21904) SHA1(32bd505e738b8826c6ab138f30831b7a53b700cf) )
	ROM_LOAD( "cn_30a_c.1_1", 0x0000, 0x010000, CRC(be7aed91) SHA1(7dac1281bbc9da8924657b13ec4aa86aa6ff9de4) )
	ROM_LOAD( "cn_30b_c.1_1", 0x0000, 0x010000, CRC(232c87ec) SHA1(2c2bf1c273ab88c0ab27a672d53cd6184a24a8d1) )
	ROM_LOAD( "cn_30bgc.1_1", 0x0000, 0x010000, CRC(40afaa86) SHA1(edb8f55abf66e3e1cc7e353c520a93fc42073585) )
	ROM_LOAD( "cn_30btc.1_1", 0x0000, 0x010000, CRC(1920cc67) SHA1(55a3ad78d68d635faff98390e2feeea29dd10664) )
	ROM_LOAD( "knq2pprog.bin", 0x0000, 0x010000, CRC(23b22f79) SHA1(3d8b9cbffb9b427897548981ddacf724215336a4) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "knqsnd.bin", 0x0000, 0x080000, CRC(13012f48) SHA1(392b3bcf6f8e3e01082087637f9d378302d046c4) )
	ROM_LOAD( "cn______.5_a", 0x0000, 0x080000, CRC(7f82f113) SHA1(98851f8820cb39b45d477151982c80fc91b15e56) )
ROM_END


ROM_START( m4lazy )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "lb_sj___.1_0", 0x0000, 0x020000, CRC(8628dcf1) SHA1(80cb9348e2704d0f72a44b4aa74b24fe03e279bc) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "lb_sj___.1_2", 0x0000, 0x020000, CRC(2b906f52) SHA1(802bcf6b3679e135308026752a55e55f00f21e85) )
	ROM_LOAD( "lb_sj_d_.1_2", 0x0000, 0x020000, CRC(a7691bad) SHA1(6cda3f3c18c13c04dbe0e4c1e4c817eedc34aa92) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4lvlcl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ll__x__x.1_1", 0x0000, 0x010000, CRC(1ef1c5b4) SHA1(455c147f158f8a36a9add9b984abc22af78258cf) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ll__x__x.3_1", 0x0000, 0x010000, CRC(42b85ebc) SHA1(a352d8389674fcfd90dc4e8155e6f4a78c9ec70d) )
	ROM_LOAD( "ll__x_dx.3_1", 0x0000, 0x010000, CRC(7753c8f0) SHA1(9600fee08529f29716697c4630730f15ef8a457b) )
	ROM_LOAD( "ll__xa_x.3_1", 0x0000, 0x010000, CRC(79468e93) SHA1(4beaa6fe2ad095b4674473ab99a7216513923077) )
	ROM_LOAD( "ll__xb_x.3_1", 0x0000, 0x010000, CRC(73b2fb34) SHA1(c127bc0954f8d01e9d8365a4506dde6f17da33fd) )
	ROM_LOAD( "ll__xgdx.1_1", 0x0000, 0x010000, CRC(65824c4f) SHA1(a514e48ac0f9d4a8d7506bf6932aeee88ca17104) )
	ROM_LOAD( "ll__xgdx.3_1", 0x0000, 0x010000, CRC(ba5b951a) SHA1(9ee36d3d42ce68f5797208633be87ddbbe605cf1) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END



ROM_START( m4ln7 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "l7_20a__.1_1", 0x0000, 0x010000, CRC(bfe82d2a) SHA1(4477d737a2326602a355758d8fc06220312fc085) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "l7_20s__.1_1", 0x0000, 0x010000, CRC(0037cd57) SHA1(b5882027269cf71878a73009bc3e40d9fcfac60d) )
	ROM_LOAD( "l7_20sb_.1_1", 0x0000, 0x010000, CRC(d53bcd66) SHA1(7f5c65d5ca3dbb8a0c38f169585fc78d512166af) )
	ROM_LOAD( "l7_20sd_.1_1", 0x0000, 0x010000, CRC(a82d04cb) SHA1(1abe9e22f6f526ab076163ce79cff841d1b38b0a) )
	ROM_LOAD( "l7_20sk_.1_1", 0x0000, 0x010000, CRC(7d2104fa) SHA1(a0a65042f4db8ea5184d41c68ffcf7608580d928) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "l7______.1_2", 0x000000, 0x080000, CRC(216209e3) SHA1(af274a7f27ba0e7ac03400e9919537ab36464e64) )
	ROM_LOAD( "l7______.1_3", 0x080000, 0x080000, CRC(e909c3ec) SHA1(68ce743729aaefd6c20ee447af40d99e0f4c072b) )
ROM_END




ROM_START( m4madmon )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "mm_20a__.7_1", 0x0000, 0x020000, CRC(7df66388) SHA1(4e5bcbcb2fb08b23989c83f11751400f666bbdc2) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "mm_20b__.7_1", 0x0000, 0x020000, CRC(7f592e44) SHA1(05e78347cd09d1e58f0a50a724e0563490ec5185) )
	ROM_LOAD( "mm_20bg_.7_1", 0x0000, 0x020000, CRC(2cd8dcc2) SHA1(c4a2a423a55c6b0668739429c24c69b25e3824cf) )
	ROM_LOAD( "mm_20bt_.7_1", 0x0000, 0x020000, CRC(6e929612) SHA1(6b1d06c3cfce440c4cefb8dd04e7a5da673d545a) )
	ROM_LOAD( "mm_20s__.7_1", 0x0000, 0x020000, CRC(c23b338c) SHA1(0d3bc801132e68564c2bc01b810e71362047ade1) )
	ROM_LOAD( "mm_20sb_.7_1", 0x0000, 0x020000, CRC(51be206b) SHA1(913defb24bdb7551acefefd9673f5663129edbec) )
	ROM_LOAD( "mm_25__c.3_1", 0x0000, 0x020000, CRC(e1879caf) SHA1(4d0a804a8d81aab5bb9dec611654325e6a3fb741) )
	ROM_LOAD( "mm_25_dc.3_1", 0x0000, 0x020000, CRC(9bc81854) SHA1(b4c42a3da0ab03a0f43846e0c3a4a0b5f3c7e65a) )
	ROM_LOAD( "mm_25_gc.3_1", 0x0000, 0x020000, CRC(8d2f1259) SHA1(239c2430bd4ce7b53615b00fac79fb7eceecabf1) )
	ROM_LOAD( "mm_25a_c.3_1", 0x0000, 0x020000, CRC(b45b88c2) SHA1(56ed8c83c68f410fcc5ac342abac6b1f4419cccd) )
	ROM_LOAD( "mm_25b_c.3_1", 0x0000, 0x020000, CRC(fc33804b) SHA1(f817a6dd691739fcf7d7d622da265f63f60503f6) )
	ROM_LOAD( "mm_25bdc.3_1", 0x0000, 0x020000, CRC(188666c0) SHA1(e8a61c327c73aac2a6b0dc674dee7bc2aa358b27) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4madmnc )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cm_25__c.3_1", 0x0000, 0x020000, CRC(3d9ff5fe) SHA1(b918bb15251514f50a669216c7d00ecf23e64d1b) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cm_25_bc.3_1", 0x0000, 0x020000, CRC(65a7b870) SHA1(58b910d7e002164cbbe1aa32c5e17dfe7cfb507d) )
	ROM_LOAD( "cm_25_dc.3_1", 0x0000, 0x020000, CRC(fcae8cf3) SHA1(0f1e86e2f02be2e1870f0f70509bc4a2ada6d3a5) )
	ROM_LOAD( "cm_25_kc.3_1", 0x0000, 0x020000, CRC(f66cf97b) SHA1(9b6c4da3a9d64ed2581b04ca82a945dc295931bc) )
	ROM_LOAD( "cm_25a_c.3_1", 0x0000, 0x020000, CRC(94e21dc0) SHA1(0d35a467fdcd19909d5540c8a4461364bf7e17f3) )
	ROM_LOAD( "cm_25b_c.3_1", 0x0000, 0x020000, CRC(95a1eb43) SHA1(a44d5c8fadf547187834c4686f6001e6e7df83f7) )
	ROM_LOAD( "cm_25bgc.3_1", 0x0000, 0x020000, CRC(3241cdf5) SHA1(9ac4b415fff3ee3422dc6e4df7f626cddcab6c38) )
	ROM_LOAD( "cm_25btc.3_1", 0x0000, 0x020000, CRC(b8c0f623) SHA1(62d489d82955ac492f78be77a3f2558ad080b375) )
	ROM_LOAD( "cm_29_dc.4_1", 0x0000, 0x020000, CRC(72542e93) SHA1(f9aad1e290b345ebd93dee81a10ad3ebb61b8228) )
	ROM_LOAD( "cm_29a_c.4_1", 0x0000, 0x020000, CRC(aa7c11ce) SHA1(3ae11b5279f975c0f6f6462d9e23180b47cbe280) )
	ROM_LOAD( "cm_29b_c.4_1", 0x0000, 0x020000, CRC(b5d749d8) SHA1(91f318a193a7ad841ac9a7f2114385ad97555f4d) )
	ROM_LOAD( "cm_29bgc.4_1", 0x0000, 0x020000, CRC(82f06ed1) SHA1(3f982d8fb1f689cd774f407585a8dce5d8c031a7) )
	ROM_LOAD( "cm_29btc.4_1", 0x0000, 0x020000, CRC(dd96bbeb) SHA1(1cf461dc36a8086b3f438922e1675fbd929af771) )
	ROM_LOAD( "cm_39_dc.4_1", 0x0000, 0x020000, CRC(c81c575e) SHA1(8c1f5151412da267381434259ca2c2d307668a74) )
	ROM_LOAD( "cm_39a_c.4_1", 0x0000, 0x020000, CRC(10346803) SHA1(2d7b0ca2c30a24c4779c44872fc86d6ae269b51f) )
	ROM_LOAD( "cm_39a_c.5_1", 0x0000, 0x020000, CRC(a235932f) SHA1(b6875c1119c8ed3a77d7f358298c119afac17dea) )
	ROM_LOAD( "cm_39b_c.4_1", 0x0000, 0x020000, CRC(0f9f3015) SHA1(8a02b6a22a96c7e310a7428d36b306e8af618e9a) )
	ROM_LOAD( "cm_39bgc.4_1", 0x0000, 0x020000, CRC(38b8171c) SHA1(5e33189d6b2d72a045c98c2bb8492961ab3d978f) )
	ROM_LOAD( "cm_39btc.4_1", 0x0000, 0x020000, CRC(67dec226) SHA1(68bfe25ff693ea7b4469d2e7d1b6972cc421d1cc) )
	ROM_LOAD( "cm_49_dc.4_1", 0x0000, 0x020000, CRC(e0ab38ee) SHA1(a324a1bb9f709b72de6fdd4c896326ca0004fdef) )
	ROM_LOAD( "cm_49a_c.4_1", 0x0000, 0x020000, CRC(388307b3) SHA1(070d431171ce325f6935f5bc1fd8db3acf79b13c) )
	ROM_LOAD( "cm_49b_c.4_1", 0x0000, 0x020000, CRC(27285fa5) SHA1(5f08bc751d7a0875b1b879d7d96aa9803fb942c6) )
	ROM_LOAD( "cm_49bgc.4_1", 0x0000, 0x020000, CRC(100f78ac) SHA1(c5d30fa46508b00b163dbf05e572f8c23fdb6cc3) )
	ROM_LOAD( "cm_49btc.4_1", 0x0000, 0x020000, CRC(4f69ad96) SHA1(50948166fee3cd1e6f0e378076046ee305204d61) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END



ROM_START( m4mmm )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "mu_sj___.3_0", 0x0000, 0x040000, CRC(abdf9d1c) SHA1(e8c6a056025b44e4ec995b42b2720e6366a97283) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "mu_sja__.3_0", 0x0000, 0x040000, CRC(3d2a9ea4) SHA1(f2ec904c8cef84affaad603edf26a864bd34be29) )
	ROM_LOAD( "mu_sjk__.3_0", 0x0000, 0x040000, CRC(34e4f8ba) SHA1(606d607faeb43190f5167aa3d10c55d9986b7e58) )
	ROM_LOAD( "mu_sjs__.3_0", 0x0000, 0x040000, CRC(26fb12b3) SHA1(d341181be75c87b44e4066653225911ce3460ed8) )
	ROM_LOAD( "mu_ssj__.2_0", 0x0000, 0x040000, CRC(935b6602) SHA1(d5fa5688895fe3c2ae3ad7dbbc35d9b12574c93d) )
	ROM_LOAD( "mu_ssja_.2_0", 0x0000, 0x040000, CRC(ff97814c) SHA1(8d9d74e6b0096cdc3226cfa91d7b653855600d5a) )
	ROM_LOAD( "mu_ssjb_.2_0", 0x0000, 0x040000, CRC(5728973a) SHA1(2cd9c866fcc33150fb8d456f741ac809e0bd2b15) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "mu___snd.1_1", 0x000000, 0x080000, CRC(570cf5f8) SHA1(48b3703bf385d037e4e870dfb671b75e9bab84a7) )
	ROM_LOAD( "mu___snd.1_2", 0x080000, 0x080000, CRC(6ec1910b) SHA1(4920fe0b7c7f4ddb14d56f50598aaf62e5867014) )
	ROM_LOAD( "mu___snd.1_3", 0x100000, 0x080000, CRC(8699378c) SHA1(55c3e310cfde8046e58bf21a8788e697c8275b8d) )
	ROM_LOAD( "mu___snd.1_4", 0x180000, 0x080000, CRC(54b193d8) SHA1(ab24624ce69d352a14f6bd3db127fa1c8c5f07db) )

ROM_END



ROM_START( m4oadrac )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "dr__x__x.2_0", 0x0000, 0x020000, CRC(4ca65bd9) SHA1(deb0a7d3596647210061b69a10fc6cdfc066538e) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "dr__x__x.2_1", 0x0000, 0x020000, CRC(d91773af) SHA1(3d8dda0f409f55bce9c4d4e2a8377e43fe2f1f7d) )
	ROM_LOAD( "dr__x_dx.2_0", 0x0000, 0x020000, CRC(47f3ac5a) SHA1(e0413c55b897e96e32c3332dac041bc94da6dea3) )
	ROM_LOAD( "dr__x_dx.2_1", 0x0000, 0x020000, CRC(f8c36b67) SHA1(c765d7a5eb4d7cd74295da26a7c6f5341a1ca257) )
	ROM_LOAD( "dr__xa_x.2_0", 0x0000, 0x020000, CRC(702f0f7a) SHA1(8529c3eaa33cb972cc38067d176c7c8af0674147) )
	ROM_LOAD( "dr__xa_x.2_1", 0x0000, 0x020000, CRC(cf1fc847) SHA1(6b09c0de15a380da1783a387569d83328f5b29a0) )
	ROM_LOAD( "dr__xb_x.2_0", 0x0000, 0x020000, CRC(3ae8a72c) SHA1(a27faba69430b1d16abf62e0ef37182ab302bbbd) )
	ROM_LOAD( "dr__xb_x.2_1", 0x0000, 0x020000, CRC(85d86011) SHA1(81f8624908299aa37e75fc5d12059b3600212d35) )
	ROM_LOAD( "dri_xa_x.2_0", 0x0000, 0x020000, CRC(849d2a80) SHA1(c9ff0a5a543b62ca5b885f93a35b5f40e88db8c3) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "oad.chr", 0x0000, 0x000048, CRC(910b09db) SHA1(d54399660b1bf1a89712b25292ac99b740442e5c) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "oadsnd1.bin", 0x000000, 0x080000, CRC(b9a9b49b) SHA1(261e939da031768e2a2b5b171cbba55c87d1a758) )
	ROM_LOAD( "oadsnd2.bin", 0x080000, 0x080000, CRC(94e34646) SHA1(8787d6757e4ed86417aafac0e042091189974d3b) )
ROM_END

ROM_START( m4orland )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "or 05a v2-1(27c010)", 0x0000, 0x020000, CRC(a33c22ee) SHA1(3598a2940f05622405fdef16426f3f5f30dfef29) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "or_05a__.1_1", 0x0000, 0x020000, CRC(3e7fe3ac) SHA1(9f4c0d5b7ba10726376b0654c8ddbc62b62c9eed) )
	ROM_LOAD( "or_20a__.7_1", 0x0000, 0x020000, CRC(ae524299) SHA1(3bb2bfe1c0ca0660aca148d6f17b730b7bdc8183) )
	ROM_LOAD( "or_20b__.7_1", 0x0000, 0x020000, CRC(c8a30e2e) SHA1(8abc5437751faf61c12709c963a6819cb0b2b43f) )
	ROM_LOAD( "or_20bg_.7_1", 0x0000, 0x020000, CRC(552561e5) SHA1(c92c4b21182511e4880b90313a673045e20b01e8) )
	ROM_LOAD( "or_20bt_.7_1", 0x0000, 0x020000, CRC(7db2e12a) SHA1(c10989d5d6bbc8f87ad21364f6c64495dc4a3047) )
	ROM_LOAD( "or_20s__.7_1", 0x0000, 0x020000, CRC(9ce4b650) SHA1(26a3337526d398ce265d735cfbe6d0e69c1f5cab) )
	ROM_LOAD( "or_20sb_.7_1", 0x0000, 0x020000, CRC(90f59de7) SHA1(2bb6c0680c654265c8669a5f13346ae6afb72fb5) )
	ROM_LOAD( "or_20sd_.7_1", 0x0000, 0x020000, CRC(242c552c) SHA1(dce5f0d38c8c6c101337028f30c05a7eb629e703) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "orlandosnd.p1", 0x000000, 0x080000, CRC(2735649d) SHA1(7b27bf2d4091ab581d399679b03f538f449f180c) )
	ROM_LOAD( "orlandosnd.p2", 0x080000, 0x080000, CRC(0741e2ff) SHA1(c49a2809073dd058ba85ae14c888e19d3eb2b133) )
ROM_END


ROM_START( m4pzbing )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pb_20a__.4_1", 0x0000, 0x010000, CRC(52aa92e5) SHA1(3dc20e521677e829967e1d689c9905fb96aee639) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pb_20ad_.4_1", 0x0000, 0x010000, CRC(f6bd6ad0) SHA1(092cb895c576ed2e995b62aba21851af6fb90959) )
	ROM_LOAD( "pb_20bg_.4_1", 0x0000, 0x010000, CRC(593e89f4) SHA1(4ed79c889370eb5de20b434cd83b2ee3fae31ed8) )
	ROM_LOAD( "pb_20bt_.4_1", 0x0000, 0x010000, CRC(fa51b522) SHA1(1fdebe63b871f700a664053251b02c8cf47fbb92) )
	ROM_LOAD( "pb_20s__.4_1", 0x0000, 0x010000, CRC(601f37d9) SHA1(0971c5c5321f5ce1508a0dd8abd989939224a779) )
	ROM_LOAD( "pb_20sb_.4_1", 0x0000, 0x010000, CRC(0234978a) SHA1(f24069883efb69de1024b6efbeb3d6a100ac5b9a) )
ROM_END


ROM_START( m4quidin )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "qi_20a__.3_1", 0x0000, 0x010000, CRC(88873c45) SHA1(70fa65402dbbe716a089497a8ccb06e0ba2aac6d) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "qi_20s__.3_1", 0x0000, 0x010000, CRC(3758dc38) SHA1(d22a379975e948d465e13233a796e0fb07e3c04f) )
	ROM_LOAD( "qi_20sb_.3_1", 0x0000, 0x010000, CRC(e254dc09) SHA1(ad5853c854f628de6203be8d6c3cbaa6a600e340) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "qi_____.1_2", 0x000000, 0x080000, CRC(216209e3) SHA1(af274a7f27ba0e7ac03400e9919537ab36464e64) )
	ROM_LOAD( "qi_____.1_3", 0x080000, 0x080000, CRC(e909c3ec) SHA1(68ce743729aaefd6c20ee447af40d99e0f4c072b) )
ROM_END


ROM_START( m4quidis )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pq_20a__.3_1", 0x0000, 0x010000, CRC(7eb762a1) SHA1(4546a7bf43f8ab6eb9713348e3f919de7532eed2) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "pq_20s__.3_1", 0x0000, 0x010000, CRC(71360992) SHA1(0b64f27f0edfdebca41552181ff0f2b5491ec308) )
	ROM_LOAD( "pq_20sb_.3_1", 0x0000, 0x010000, CRC(a43a09a3) SHA1(46d83465d1026620af2f59dd4b638444ca834ad1) )
	ROM_LOAD( "pq_20sd_.3_1", 0x0000, 0x010000, CRC(d92cc00e) SHA1(bff2b5da08cc34040b1d4d750ea6a654f9b77959) )
	ROM_LOAD( "pq_20sk_.3_1", 0x0000, 0x010000, CRC(0c20c03f) SHA1(f802daa8ff2c159ba4831ed048e0ddd8469448da) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4rackem )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "re_sj___.2_0", 0x0000, 0x040000, CRC(e36d3f86) SHA1(a5f522c86482517b8dc735b1012f8f7668c2f18d) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "re_sj___.3_0", 0x0000, 0x040000, CRC(2f463d2f) SHA1(3410cc8a6d097a4edfcb4c57c237d1d514b507ba) )
	ROM_LOAD( "re_sj_d_.2_0", 0x0000, 0x040000, CRC(7a31658c) SHA1(4fade421b3a1a732a99f7cb6346279ad82f55362) )
	ROM_LOAD( "re_sjs__.2_0", 0x0000, 0x040000, CRC(d7c499c8) SHA1(73542f54322f5ffb87d16f5f66cc3a22c2849f20) )
	ROM_LOAD( "re_sjsw_.2_0", 0x0000, 0x040000, CRC(66355370) SHA1(d54aab7403e64a67edf2baeaf1321ee5c4aa553d) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "re_snd.p1", 0x000000, 0x080000, CRC(aea88892) SHA1(457dab5cddfb9762f7e0bd61187b8052aee71c28) )
	ROM_LOAD( "re_snd.p2", 0x080000, 0x080000, CRC(57394ec6) SHA1(cba7abebd3ab165e9531017168f51ada6cf35991) )
	ROM_LOAD( "re_snd.p3", 0x100000, 0x080000, CRC(5d5d5309) SHA1(402615633976410225a1ee50c454391dc69a68cb) )
ROM_END


ROM_START( m4rbgold )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rb_20a_p.2a1", 0x0000, 0x010000, CRC(d7e6e514) SHA1(25645b69e86335622df43113908ed88a21f27e30) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "rb_20a_p.2f1", 0x0000, 0x010000, CRC(62af6db6) SHA1(0dcb679c05f090f8dab7228009a700c31f0179d8) )
	ROM_LOAD( "rb_20sbp.2a1", 0x0000, 0x010000, CRC(ba4c2e74) SHA1(fcc325754f96e742998373c6c5c13a8509f48cd5) )
	ROM_LOAD( "rb_20sbp.2f1", 0x0000, 0x010000, CRC(0f05a6d6) SHA1(1ba7ccbb3d78196d4b98a0a1173bf41c9dd62c1f) )
	ROM_LOAD( "rb_20sbp.2s1", 0x0000, 0x010000, CRC(d8278b82) SHA1(8e924096e238359fd2a7f81198c8af1515dc8a19) )
	ROM_LOAD( "rb_xea__.2a1", 0x0000, 0x010000, CRC(1bc266ff) SHA1(fcdb63613dbcf0b15744317a69bd8a1e3cf92526) )
	ROM_LOAD( "rb_xea__.2f1", 0x0000, 0x010000, CRC(ae8bee5d) SHA1(bae49779a009a29b27c99f5e5a4b2aeca39b5626) )
	ROM_LOAD( "rb_xea__.2s1", 0x0000, 0x010000, CRC(79a9c309) SHA1(8ad87ec153ed8a89e612011679a172fc66bae711) )
	ROM_LOAD( "rb_xes__.2a1", 0x0000, 0x010000, CRC(14430dcc) SHA1(7e514a1857f6911a57aff7900af297412ef0d905) )
	ROM_LOAD( "rb_xes__.2f1", 0x0000, 0x010000, CRC(a10a856e) SHA1(a8536d576b3698ccc539c8ac1c9136222b1cf297) )
	ROM_LOAD( "rb_xes__.2s1", 0x0000, 0x010000, CRC(7628a83a) SHA1(da1833df2e88480dafd2410fb24f0fbffbdeb679) )
	ROM_LOAD( "rb_xesb_.2f1", 0x0000, 0x010000, CRC(c321253d) SHA1(0d59b0cf7118c7932e9d89eca823ce200c5030f5) )
	ROM_LOAD( "rb_xesb_.2s1", 0x0000, 0x010000, CRC(14030869) SHA1(283e7ca543a37a60f2d3a8c6d5473b591bb20e62) )
	ROM_LOAD( "rb_xesd_.2a1", 0x0000, 0x010000, CRC(b054f5f9) SHA1(fdcca5375ff8f26f6889c5556216ff0fdf2bce94) )
	ROM_LOAD( "rb_xesd_.2f1", 0x0000, 0x010000, CRC(051d7d5b) SHA1(b8216e505de00802e5e34d11eb3e18e0736fa772) )
	ROM_LOAD( "rb_xesd_.2s1", 0x0000, 0x010000, CRC(d23f500f) SHA1(fe8d2825c8fb24c3885013c046a15ddec5cb3a1f) )
	ROM_LOAD( "rbixe___.2a1", 0x0000, 0x010000, CRC(349fafdd) SHA1(da68e210c8c0a716c1ef62e7f404f6985903b00a) )
	ROM_LOAD( "rbixe___.2s1", 0x0000, 0x010000, CRC(56f40a2b) SHA1(0c6c035d2a3dbef70b1bc95fa38ed62a70770739) )
ROM_END


ROM_START( m4rhfev )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "rt_sj___.7_0", 0x0000, 0x040000, CRC(3dd895ef) SHA1(433ecc268956c94c51dbccefd006b72e0ad8567b) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "rt_sja__.7_0", 0x0000, 0x040000, CRC(0ab59402) SHA1(485b4d2efd8f99085ed6ce5b7e07ede001c982c4) )
	ROM_LOAD( "rt_sjs__.7_0", 0x0000, 0x040000, CRC(1a8feafb) SHA1(83151f63b7ebe1c538f9334e9c3d6889d0730144) )
	ROM_LOAD( "rt_vc___.1_0", 0x0000, 0x040000, CRC(2a8df147) SHA1(df0e7021e9d169575a1297f9851b5a64e20d1a40) )
	ROM_LOAD( "rt_vc_d_.1_0", 0x0000, 0x040000, CRC(7adef22b) SHA1(d6a584581745c0ce64f646ef0b49cb68343990d0) )
	ROM_LOAD( "rtv.p1", 0x0000, 0x008000, CRC(7b78f3f2) SHA1(07ef8e6a08fd70ee48e4463672a1230ecc669532) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "rf_____.1_1", 0x000000, 0x080000, CRC(ac8d539d) SHA1(8baf14bece50774f93ae9eaf3effabb6882d2c43) )
	ROM_LOAD( "rf_____.1_2", 0x080000, 0x080000, CRC(cc2fadd8) SHA1(681850e2e6164cf8af8e7501ac44f475cc07b742) )
	ROM_LOAD( "rf_____.1_3", 0x100000, 0x080000, CRC(165aaf9f) SHA1(815224fe94a77628cef1dd0d8a238edcb4813006) )
	ROM_LOAD( "rf_____.1_4", 0x180000, 0x080000, CRC(4f7e7b49) SHA1(f9d421eeab73e0c795a08cf166c8807e0b14ec82) )
ROM_END



ROM_START( m4rhs )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "rh_sj___.4s1", 0x0000, 0x020000, CRC(be6179cd) SHA1(8aefffdffb25bc4dd7d083c7027be746181c2ff9) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "rh_sj__c.6_1", 0x0000, 0x020000, CRC(476f3cf2) SHA1(18ce990e28ca8565ade5eec9a62f0b243121af73) )
	ROM_LOAD( "rh_sj_b_.4s1", 0x0000, 0x020000, CRC(58a4480e) SHA1(f4ecfa1debbfa9dba75263bce2c9f66741c3466f) )
	ROM_LOAD( "rh_sj_bc.6_1", 0x0000, 0x020000, CRC(2e37a58c) SHA1(a48c96384aa81f98bfa980c93e93523ecef3d43c) )
	ROM_LOAD( "rh_sj_d_.4s1", 0x0000, 0x020000, CRC(8f1176db) SHA1(283ef0b9515eac342a02489118bd30016ba85399) )
	ROM_LOAD( "rh_sj_k_.4s1", 0x0000, 0x020000, CRC(3f2ef505) SHA1(28c3806bc48af21a2b7ea27d42ea9f6b4346f3b8) )
	ROM_LOAD( "rh_sja__.4s1", 0x0000, 0x020000, CRC(b8cdd5fb) SHA1(4e336dd3d61f4fdba731951c56e440766ea8efeb) )
	ROM_LOAD( "rh_sja_c.6_1", 0x0000, 0x020000, CRC(b7b790e5) SHA1(e2b34dc2f6ede4f4c22b11123dfaed46f2c5c45e) )
	ROM_LOAD( "rh_sjab_.4s1", 0x0000, 0x020000, CRC(c8468d4c) SHA1(6a9f8fe10949712ecacca3bfcd7d5ab4860682e2) )
	ROM_LOAD( "rh_sjad_.4s1", 0x0000, 0x020000, CRC(df4768f0) SHA1(74894b232b27e65058d59acf174172da86def95a) )
	ROM_LOAD( "rh_sjak_.4s1", 0x0000, 0x020000, CRC(6f78eb2e) SHA1(a9fec7a7ad9334c3d8760e1982ac00651858cee8) )
	ROM_LOAD( "rocky15g", 0x0000, 0x020000, CRC(05f4f333) SHA1(a1b917f6c91d751fb2433e46c4c60840b47eed9e) )
	ROM_LOAD( "rocky15t", 0x0000, 0x020000, CRC(3fbad6de) SHA1(e8d76b3878794c769187d92d2834018a84e764ac) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "rh.chr", 0x0000, 0x000048, CRC(6e246d8f) SHA1(c6b0dff1b918c578b76f020ff70a24ea48dbae2e) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "rhp2snd", 0x0000, 0x080000, CRC(18112293) SHA1(b2bf838849ad1a9931c294ccc291ba2f5c5f45e9) )

	ROM_LOAD( "rh___snd.1_1", 0x000000, 0x080000, CRC(ceebd8f4) SHA1(fe9f62034aae7d2ec097d80dc471a7fd27ddec8a) )
	ROM_LOAD( "rh___snd.1_2", 0x080000, 0x080000, CRC(1f24cfb6) SHA1(cf1dc9d2a1c1cfb8718c89e245e9bf375fef8bfd) )
	ROM_LOAD( "rh___snd.1_3", 0x100000, 0x080000, CRC(726958d8) SHA1(6373765b80971dd7ff5c8eaeee83966335db4d27) )
ROM_END



ROM_START( m4sinbd )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "sd_20__c.1_1", 0x0000, 0x020000, CRC(28cd336e) SHA1(45bdf5403c04b7d3a3645b6b44ac3d12e6463a55) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sd_20a__.4_1", 0x0000, 0x020000, CRC(12b8f629) SHA1(c8540ecb217cf0615d7a8d080136926646ca8497) )
	ROM_LOAD( "sd_20a__.5_1", 0x0000, 0x020000, CRC(68a3d155) SHA1(5ae47f1ca860af30c77beebe3acf615958ed59e9) )
	ROM_LOAD( "sd_20a_c.1_1", 0x0000, 0x020000, CRC(5a5eef7c) SHA1(32d801cea0593e220ac67c2c17782fc144d02cc4) )
	ROM_LOAD( "sd_20ad_.5_1", 0x0000, 0x020000, CRC(dc9dd7b3) SHA1(e26c8a814cdb268026e31acb668f34160ea9ce7b) )
	ROM_LOAD( "sd_20s__.1v1", 0x0000, 0x020000, CRC(acb5aa9a) SHA1(4186058f83e241f06de842d0ae79f26a2a0e6cf0) )
	ROM_LOAD( "sd_20s__.4_1", 0x0000, 0x020000, CRC(bce21ced) SHA1(9e089fbca1dd39e356f38d0c301162affea9316c) )
	ROM_LOAD( "sd_20s__.5_1", 0x0000, 0x020000, CRC(939ecca5) SHA1(a9cd465fbe742b910602f861ae6ac7cedc7d9be9) )
	ROM_LOAD( "sd_20s_p.4_1", 0x0000, 0x020000, CRC(5ae716a9) SHA1(4c9ceab423a1480f8257fad2d62b65a17788a472) )
	ROM_LOAD( "sd_20s_s.5_1", 0x0000, 0x020000, CRC(e47c869a) SHA1(2735613aafe71e9b2a3aabb433fbd98a83a546c7) )
	ROM_LOAD( "sd_20sb_.4_1", 0x0000, 0x020000, CRC(811efef2) SHA1(280c6d6697735faca58317925c58b687c5988b87) )
	ROM_LOAD( "sd_20sb_.5_1", 0x0000, 0x020000, CRC(594920ec) SHA1(03ac253ef35b8fbccbfe4b2c5a7d906a3001b3ec) )
	ROM_LOAD( "sd_20sbc.1_1", 0x0000, 0x020000, CRC(3720cf0d) SHA1(8f0e63985badbff6ac39fcf956ebff3b0655c2b9) )
	ROM_LOAD( "sd_20sbp.4_1", 0x0000, 0x020000, CRC(671bf4b6) SHA1(ac928b47c1392e8a82404776068233e7d1dd5d28) )
	ROM_LOAD( "sd_20sbs.5_1", 0x0000, 0x020000, CRC(2eab6ad3) SHA1(66261328999a139389198e772a69dcd994792439) )
	ROM_LOAD( "sd_20sdc.1_1", 0x0000, 0x020000, CRC(fc386926) SHA1(6e64aeb82e62ded75e48f0faab932b63732bcf08) )
	ROM_LOAD( "sd_25__c.1_1", 0x0000, 0x020000, CRC(348e846d) SHA1(ca0fffd59076e6e60d37f677d0c7e7f182a41b9e) )
	ROM_LOAD( "sd_25a_c.1_1", 0x0000, 0x020000, CRC(f50c55e9) SHA1(df72b196fb21b6359282c06b960fa53117a95fee) )
	ROM_LOAD( "sd_25sbc.1_1", 0x0000, 0x020000, CRC(0773632a) SHA1(34294e32243b903a6c1c54b8718ca7cddbd3316e) )
	ROM_LOAD( "sd_25sdc.1_1", 0x0000, 0x020000, CRC(4917a542) SHA1(23de449d536c799032afab678b9001fa8541fb8a) )
	ROM_LOAD( "sdi20___.1v1", 0x0000, 0x020000, CRC(c50b3555) SHA1(c2d89126b9122f48ecce52d50cd7a03cc2bf1829) )
	ROM_LOAD( "sv_20a__.4_1", 0x0000, 0x020000, CRC(0e790ae8) SHA1(48f055f3f1f5d3392b7fba1c5c30624c1f230327) )
	ROM_LOAD( "sv_20sb_.4_1", 0x0000, 0x020000, CRC(e88c1c29) SHA1(49d180068e4ae9cb65a58e65b4e1ac4d6657ae1d) )
	ROM_LOAD( "svi20___.4_1", 0x0000, 0x020000, CRC(643037ed) SHA1(d4063faba3069625474dd761f9ad2dcf2f710a19) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4sky )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "sk_s____.3_1", 0x0000, 0x040000, CRC(749af008) SHA1(036514f2bcb84193cfa84313f0617f3196aea73e) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sk_sj___.5_0", 0x0000, 0x040000, CRC(45ae0423) SHA1(94d5b3d4aacb69a18ff3f45681eb5f7fba7657e8) )
	ROM_LOAD( "sk_sj_k_.5_0", 0x0000, 0x040000, CRC(e1bab980) SHA1(1c8b127809422ab0baf1875ca907f18269a0cc17) )
	ROM_LOAD( "sk_sja__.5_0", 0x0000, 0x040000, CRC(b2a16ef7) SHA1(9012dcc320e8af8fef53e0dc91d3bcd6cbafa5ee) )
	ROM_LOAD( "sk_sjs__.5_0", 0x0000, 0x040000, CRC(d176431f) SHA1(8ca90ef61486fc5a5b6527f913cd05b42ceabe3e) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4souls )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "ss_06a__.4_1", 0x0000, 0x020000, CRC(00390a21) SHA1(d31d1307301fa4e8cf0ce3677e68a4c1723e4404) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ss_16a__.4_1", 0x0000, 0x020000, CRC(b9ab9612) SHA1(ad30916a0f2cc745741c99d23c23192ae4088daf) )
	ROM_LOAD( "ss_26a__.2_1", 0x0000, 0x020000, CRC(bf9acf05) SHA1(13698b453e975a1801631163d06468f07c181b48) )
	ROM_LOAD( "ss_26ad_.2_1", 0x0000, 0x020000, CRC(3e867df2) SHA1(0c7d57c05952cb4f737ee07f139a3593803d3d60) )
	ROM_LOAD( "ss_26sb_.2_1", 0x0000, 0x020000, CRC(2010749e) SHA1(4220b12db7870a2efd7a7ab573e4e08ff0643e70) )
	ROM_LOAD( "ss_26sd_.2_1", 0x0000, 0x020000, CRC(773db916) SHA1(a7d168db22d5adeb3eaf64786bbe744ce787ff68) )
	ROM_LOAD( "ss_26sk_.2_1", 0x0000, 0x020000, CRC(9ae1672e) SHA1(79bcd12fae38dd1b0035e956148ffeaee33b9c71) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "ss______.1_2", 0x000000, 0x080000, CRC(ddea9d75) SHA1(fe5f88d49434109d0f51425e790e179dc1a02767) )
	ROM_LOAD( "ss______.1_3", 0x080000, 0x080000, CRC(23d1e57a) SHA1(b17afdaa95522fd7ea6c12f513fa338e1fcb06f6) )
	ROM_LOAD( "ss______.1_4", 0x100000, 0x080000, CRC(0ba3046a) SHA1(ec21fa328669bc7a5baf1ce8b9ac05f38f98e360) )
ROM_END


ROM_START( m4specu )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "speculator.bin", 0x8000, 0x008000, CRC(4035d20c) SHA1(4a534294c5c7332eacd09ca44f351d6a6850cc29) )
ROM_END




ROM_START( m4spinbt )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "sn_37ad_.5_0", 0x0000, 0x040000, CRC(42d6faaa) SHA1(3789e85981b33ffae7c50ccca3278ae62974972d) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sn_37b__.5_0", 0x0000, 0x040000, CRC(3a259a6f) SHA1(1acabb9e725ae1374b87808c4b3d06a329c824d0) )
	ROM_LOAD( "sn_37bd_.5_0", 0x0000, 0x040000, CRC(f4e3f395) SHA1(545b2ea1cf4231ba1663bea0e6770976b0797cf3) )
	ROM_LOAD( "sn_37bg_.5_0", 0x0000, 0x040000, CRC(c3ad8312) SHA1(9463903fe65462e5e9b09a992dc4625dba8452f0) )
	ROM_LOAD( "sn_37bt_.5_0", 0x0000, 0x040000, CRC(9fd1df18) SHA1(4c647bc887cdaa2415d0308787c710c83ce1c0d3) )
	ROM_LOAD( "sn_s7a__.5_0", 0x0000, 0x040000, CRC(69d39933) SHA1(ec2a9fb7c4977532a7a431745eb2d82d4e282159) )
	ROM_LOAD( "sn_s7ad_.5_0", 0x0000, 0x040000, CRC(a715f0c9) SHA1(bec188edcd0580465859876f8bff2ff5a392a9e1) )
	ROM_LOAD( "sn_s7s__.5_0", 0x0000, 0x040000, CRC(13288cc5) SHA1(2c47dcd8b57d10e0768729ad91dba12521c0d98a) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4starst )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "sr_20__d.3_0", 0x0000, 0x040000, CRC(98f6619b) SHA1(fc0a568e6695c9ad0fda7bc6703c752af26a7777) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sr_20_bd.3_0", 0x0000, 0x040000, CRC(ff8209de) SHA1(41a4c20c89b3a04612ad6298276472b888915c89) )
	ROM_LOAD( "sr_20_kd.3_0", 0x0000, 0x040000, CRC(4c9a53d5) SHA1(43ebf6c06db58de9c3934e2dbba0d8126f3e2dda) )
	ROM_LOAD( "sr_20a_d.3_0", 0x0000, 0x040000, CRC(e9eebb4c) SHA1(60d8010140d9debe8f12d7f810de223d9abd02a4) )
	ROM_LOAD( "sr_20s_d.3_0", 0x0000, 0x040000, CRC(725b50e6) SHA1(3efde346022e37b09df08b8188ac76dcdfac8a4e) )
	ROM_LOAD( "sr_sj___.5_0", 0x0000, 0x040000, CRC(7964bd86) SHA1(7078de5a61b52dedb776993643f7edd8a2c863c3) )
	ROM_LOAD( "sr_sj_b_.5_0", 0x0000, 0x040000, CRC(4ee1f95b) SHA1(1e3d52afd19a9489608d5446ef2118561c6411b0) )
	ROM_LOAD( "sr_sj_d_.5_0", 0x0000, 0x040000, CRC(b4d78711) SHA1(c864c944b3fa74aa1fed22afe656a37413b024ce) )
	ROM_LOAD( "sr_sj_k_.5_0", 0x0000, 0x040000, CRC(c7681e28) SHA1(a8c1c75df33c85301257147c97d6af8808dad0d2) )
	ROM_LOAD( "sr_sja__.5_0", 0x0000, 0x040000, CRC(aa86c4f2) SHA1(e90fd91f1d14b89714e3fb8236ac9e8a641e4c71) )
	ROM_LOAD( "sr_sjs__.5_0", 0x0000, 0x040000, CRC(89e405e4) SHA1(5aa9053e08c27570731f65502c7fb31f0ea0a678) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4thestr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "thestreak.bin", 0x0000, 0x010000, CRC(cb79f9e5) SHA1(6cbdc5327e81b51f1060fd91efa3d061b9748b49) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ts_20_b4.3_1", 0x0000, 0x010000, CRC(17726c7c) SHA1(193b572b9f859f1018f1be398b35a5103622faf8) )
	ROM_LOAD( "ts_20_bc.3_1", 0x0000, 0x010000, CRC(b03b3f11) SHA1(9116ac608ab5574d5912550b988fc319d0a38444) )
	ROM_LOAD( "ts_20_d4.3_1", 0x0000, 0x010000, CRC(7bfae07a) SHA1(9414ad510ca9a183181a30d98858278c375c185d) )
	ROM_LOAD( "ts_20_dc.3_1", 0x0000, 0x010000, CRC(7196b317) SHA1(c124ed3d030b77870b7851b3da104f8fc5393a31) )
	ROM_LOAD( "ts_20a_4.3_1", 0x0000, 0x010000, CRC(921b8cc3) SHA1(74143888de21aba4374d016cb4c08ae59dfa59ef) )
	ROM_LOAD( "ts_20a_c.3_1", 0x0000, 0x010000, CRC(c8eb1dd9) SHA1(7b7520467cd32295e6324d350d1f2bed829555e0) )
	ROM_LOAD( "ts_20b_4.3_1", 0x0000, 0x010000, CRC(2221f704) SHA1(8459b658d3ad84bb86250518d0403970f881323d) )
	ROM_LOAD( "ts_20b_c.3_1", 0x0000, 0x010000, CRC(ecdf59a9) SHA1(7c2141e336ba3f1865bbf422aaa0b78cb1a27a4c) )
	ROM_LOAD( "ts_20bg4.3_1", 0x0000, 0x010000, CRC(b2a419ea) SHA1(bbc565ce8e79d39e1b1a7cd1685fa8c7ce00d7b9) )
	ROM_LOAD( "ts_20bgc.3_1", 0x0000, 0x010000, CRC(3b2d7b50) SHA1(a8560b0894783a398aaf0510493583b8cb826947) )
	ROM_LOAD( "ts_20bt4.3_1", 0x0000, 0x010000, CRC(d10a6c5a) SHA1(07fdf3797c87e35468ef859bb67753f11c5fbded) )
	ROM_LOAD( "ts_20btc.3_1", 0x0000, 0x010000, CRC(58830ee0) SHA1(b2798e0f8e03870c77892a32654263575e9aaafa) )
	ROM_LOAD( "ts_25_bc.3_1", 0x0000, 0x010000, CRC(43877de6) SHA1(b006ae97139c9bd66a32884b92fdbdf4f10db58a) )
	ROM_LOAD( "ts_25_dc.3_1", 0x0000, 0x010000, CRC(60ab675c) SHA1(763b66d7731489abdec84d2f8e3c186ad95c7349) )
	ROM_LOAD( "ts_25_kc.3_1", 0x0000, 0x010000, CRC(418cbb32) SHA1(e19a0c7fd88a82983ec33b99f3819a2a238c68a5) )
	ROM_LOAD( "ts_25a_c.3_1", 0x0000, 0x010000, CRC(448a705f) SHA1(35a7cc480c376eaef7439d5c96cec490aec9fc4b) )
	ROM_LOAD( "ts_25b_c.3_1", 0x0000, 0x010000, CRC(5b390ec2) SHA1(db7719ab8021e0b75e9419d2a05f3139fbab8e61) )
	ROM_LOAD( "ts_25bgc.3_1", 0x0000, 0x010000, CRC(612463fc) SHA1(085d8faf91c8ef6520ca971d249322f336464856) )
	ROM_LOAD( "ts_25btc.3_1", 0x0000, 0x010000, CRC(6d658e61) SHA1(619dbacad424e5db82c6ee19d1e3358c18cfe783) )
	ROM_LOAD( "ts_30a_c.1_1", 0x0000, 0x010000, CRC(8636e700) SHA1(f11c20da6c3bfe1842ea8f9eac8c831d49f42c32) )
	ROM_LOAD( "ts_30b_c.1_1", 0x0000, 0x010000, CRC(2577c8fc) SHA1(6d22bd1a93f423862f5466f99690eeced9090420) )
	ROM_LOAD( "ts_30bgc.1_1", 0x0000, 0x010000, CRC(2c582ba6) SHA1(dbcae0ef90105a7f6c720156711f73bb3c237b8a) )
	ROM_LOAD( "ts_39_dc.1_1", 0x0000, 0x010000, CRC(84d338b8) SHA1(847e6b7808b6d5d361414a4aaa5d5cf6a5863a70) )
	ROM_LOAD( "ts_39a_c.1_1", 0x0000, 0x010000, CRC(9ee56a3a) SHA1(365ec4c90abbe4b352bdd2d6aed5eec4cdaf35ff) )
	ROM_LOAD( "ts_39b_c.1_1", 0x0000, 0x010000, CRC(470cd6d1) SHA1(c9c3c9c23c596e79f1b6495d4706b1da6cbd1b2e) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "thestreaksnd.bin", 0x0000, 0x080000, CRC(fdbd0f88) SHA1(8d0eaa9aa8d505affeb8bd12d7cb13337aa2e2c2) )
ROM_END


ROM_START( m4sunclb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sucxe__0.2", 0x0000, 0x010000, CRC(fd702a6f) SHA1(0f6d553fcb096ca4874bb971425dabfbe18db31d) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sucxed_0.2", 0x0000, 0x010000, CRC(70802bc3) SHA1(69b36f716cb608931f933cb58e47232b18064f9d) )
ROM_END



ROM_START( m4sunscl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sc_xe___.3_3", 0x0000, 0x010000, CRC(e3732cc6) SHA1(77f0368bb29ad00030f83af794a52df92fe97e5d) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sc_xe_d_.3_3", 0x0000, 0x010000, CRC(b8627c4a) SHA1(ad616d38773cbd82376b518aa15dc3d7027237c5) )
	ROM_LOAD( "sc_xef__.3_3", 0x0000, 0x010000, CRC(8e7e1100) SHA1(7648ea860a546081388a213845e27312730f46d9) )
ROM_END


ROM_START( m4supleg )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "sl_sj.hex", 0x0000, 0x040000, CRC(254835f7) SHA1(2fafaa3da747edd27d393ad106008e898e465283) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sl_sjs.hex", 0x0000, 0x040000, CRC(98942cd3) SHA1(858fde0a350159d089c6a0e0cc2e2eed6ab2092c) )
	ROM_LOAD( "sl_vc.hex", 0x0000, 0x040000, CRC(1940d117) SHA1(ae7338483ac39e9e1973dde5eb837443512630dd) )
	ROM_LOAD( "sl_vcd.hex", 0x0000, 0x040000, CRC(7aab16d1) SHA1(6da4e0d9883a48937d00bfc5929b3557de51f60e) )
	ROM_LOAD( "sls.hex", 0x0000, 0x040000, CRC(5ad6dbb9) SHA1(ff6f9dcf14df22c7bb2b949fcd5c70f31d4c1928) )
	ROM_LOAD( "s_leag._pound5", 0x0000, 0x040000, CRC(4c6bd78e) SHA1(f67793a2a16adacc8d92b57050f02cffa50a1283) ) //Whitbread?
	ROM_REGION( 0x80000, "altsnd", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "s_leag.s1", 0x0000, 0x080000, CRC(e8d90896) SHA1(4c67507f18b5dc966e2df3685dc6c257f5053e61) ) //alt first sound rom, connected to whitbread set

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "sls1.hex", 0x000000, 0x080000, CRC(341e3d3e) SHA1(b42ec737e95e766b1b26d6225416ee0c5cad2663) )
	ROM_LOAD( "sls2.hex", 0x080000, 0x080000, CRC(f4ab6f0d) SHA1(4b59608ca16c9d158d4d1ac532e7fbe6ff0da959) )
	ROM_LOAD( "sls3.hex", 0x100000, 0x080000, CRC(dcba96a1) SHA1(d474c63b37cb18a0b3b1299b5cacadfd8cd5458b) )
ROM_END



ROM_START( m4supscr )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "multistakesoccer.bin", 0x0000, 0x040000, CRC(ce27b6a7) SHA1(f9038336137b0642da4d1520b5d71a047d8fbe12) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "sm_78___.6_0", 0x0000, 0x040000, CRC(e7022c44) SHA1(da3a5b9954f7e50dce73aeb9c46bd4631c8350d5) )
	ROM_LOAD( "sm_78_d_.6_0", 0x0000, 0x040000, CRC(4dbe6a87) SHA1(fe2ce1fca7105afbf459ee6558744f8fee417169) )
	ROM_LOAD( "sm_80___.6_0", 0x0000, 0x040000, CRC(7e9831ec) SHA1(d0e645ea1f6d34c507103550c248200313d033ef) )
	ROM_LOAD( "sm_80_d_.6_0", 0x0000, 0x040000, CRC(d424772f) SHA1(938eb7a35e59db704b0737d309b7989b35ed43e0) )
	ROM_LOAD( "sm_82___.6_0", 0x0000, 0x040000, CRC(019b4d42) SHA1(b27894f99dc2945fccb0de66f34eacc5b02a3463) )
	ROM_LOAD( "sm_82_d_.6_0", 0x0000, 0x040000, CRC(ab270b81) SHA1(6af522e78b9b263c96cb350cbc8585be206dcd0a) )
	ROM_LOAD( "sm_84___.6_0", 0x0000, 0x040000, CRC(809ec8b0) SHA1(965edf407a1fd7dfda2817b884ef65b3d57d51e5) )
	ROM_LOAD( "sm_84_d_.6_0", 0x0000, 0x040000, CRC(2a228e73) SHA1(04c8d17b2f2800dac9aca81e926e6e48eb02c5ac) )
	ROM_LOAD( "sm_86___.6_0", 0x0000, 0x040000, CRC(ff9db41e) SHA1(5996438e801534ebf5d9755b340fada67cadc942) )
	ROM_LOAD( "sm_86_d_.6_0", 0x0000, 0x040000, CRC(5521f2dd) SHA1(add1b70a6cddc4e176697660aeff331535d92898) )
	ROM_LOAD( "sm_88___.6_0", 0x0000, 0x040000, CRC(59e4c515) SHA1(5a76962dc6530d326bfe6ef6498c8f2ad481d6f1) )
	ROM_LOAD( "sm_88_d_.6_0", 0x0000, 0x040000, CRC(f35883d6) SHA1(1948e06e302cdb01fbd32d711374957e9e6bd64a) )
	ROM_LOAD( "sm_90___.6_0", 0x0000, 0x040000, CRC(cdc7c594) SHA1(acb829257472bc4420c141932b6f4c708ea04f1b) )
	ROM_LOAD( "sm_90_d_.6_0", 0x0000, 0x040000, CRC(677b8357) SHA1(4ec54f8b7d28c0459152309a58bd5c0db1a2f036) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4ssclas )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cs__x__x.6_0", 0x0000, 0x010000, CRC(3230284d) SHA1(bca3b4c43859ed424956c4119fa6a91a2e7d6eec) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "cs__x_dx.2_0", 0x0000, 0x010000, CRC(ea004a13) SHA1(db9a187b0672c69a6a149ec6d1025bd6da9beccd) )
	ROM_LOAD( "cs__x_dx.6_0", 0x0000, 0x010000, CRC(6dd2d11f) SHA1(8c7e60d3e5a0d4fccb024b5c0aa21fd2b9a5ada9) )
	ROM_LOAD( "cs__xa_x.6_0", 0x0000, 0x010000, CRC(6657e810) SHA1(0860076cf01c732f419483876991fb42a838622a) )
	ROM_LOAD( "cs__xb_x.5_0", 0x0000, 0x010000, CRC(a5f46ff5) SHA1(a068029f774bc6ed2e76acc2eb509bc6e2490945) )
	ROM_LOAD( "cs__xb_x.6_0", 0x0000, 0x010000, CRC(801d543c) SHA1(f0905947312fb2a526765d17cde01af5095ef923) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "ssbwb.chr", 0x0000, 0x000048, CRC(910b09db) SHA1(d54399660b1bf1a89712b25292ac99b740442e5c) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "css_____.1_1", 0x0000, 0x080000, CRC(e738fa1e) SHA1(7a1125320e0d488729aec66e658d418b96228fd0) )
ROM_END


ROM_START( m4sure )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "su_xf___.3_1", 0x0000, 0x010000, CRC(f85dae5c) SHA1(4c761c355fb6651f1e0cb041342f8a2ff510dfd2) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "su_xf_b_.3_1", 0x0000, 0x010000, CRC(9a760e0f) SHA1(fdacdae0e2322daa004b2385616dd34626814d42) )
	ROM_LOAD( "su_xf_d_.3_1", 0x0000, 0x010000, CRC(5c4a5669) SHA1(55e1e853fdfdbb43e7b61b59ab642fb013a0db0e) )
	ROM_LOAD( "suixf___.3_1", 0x0000, 0x010000, CRC(cae80b60) SHA1(23545aaf1cc3a0c8868beafb56eccedbbb6099de) )
ROM_END

ROM_START( m4tic )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tt_20a__.2_1", 0x0000, 0x010000, CRC(b923ac0d) SHA1(1237962af43c2c3f4ed0ad5bed21f24decfeae02) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "tt_20a_c.1_1", 0x0000, 0x010000, CRC(18a68ea0) SHA1(37783121ff5540e264d89069101d991acb66b982) )
	ROM_LOAD( "tt_20b__.2_1", 0x0000, 0x010000, CRC(b5eb86ab) SHA1(99ddb80941c67bd271e22af17405457d32676484) )
	ROM_LOAD( "tt_20b_c.1_1", 0x0000, 0x010000, CRC(d35079ab) SHA1(d109af8ef6f4d26b505f63df10d5850ddc0c0b65) )
	ROM_LOAD( "tt_20bd_.2_1", 0x0000, 0x010000, CRC(0889a699) SHA1(c96d135b9248e9bab78af438b97e6cb854b2c771) )
	ROM_LOAD( "tt_20bdc.1_1", 0x0000, 0x010000, CRC(2a43efd4) SHA1(9f6e568ca95a5f4e1a4e82eda2d15dfa225e65ea) )
	ROM_LOAD( "tt_20bg_.2_1", 0x0000, 0x010000, CRC(128896c1) SHA1(af37645b88116cde57fcc42ed58d69bf9c11ff8a) )
	ROM_LOAD( "tt_20bt_.2_1", 0x0000, 0x010000, CRC(362046f9) SHA1(6cb5a986517158d63e7403891bb749eaccb63acb) )
	ROM_LOAD( "tt_20s__.2_1", 0x0000, 0x010000, CRC(53dfefe9) SHA1(0f9fc1d65ebd7e370de6001f594616b79b2aa57e) )
	ROM_LOAD( "tt_20s_c.1_1", 0x0000, 0x010000, CRC(65a38960) SHA1(48ffdda1c5c98742124418429c510de9f5b90270) )
	ROM_LOAD( "tt_20sb_.2_1", 0x0000, 0x010000, CRC(c9174384) SHA1(f694a6a7f78b8a062fd26371fa6758ec4252352a) )
	ROM_LOAD( "tt_20sk_.2_1", 0x0000, 0x010000, CRC(dca42636) SHA1(c6e9aaf402c2fc7eec6e9b07aa4c33312bc0af0e) )
	ROM_LOAD( "tt_25a_c.3_1", 0x0000, 0x010000, CRC(2e44c6db) SHA1(ffc96dafbcfae719c3971882e066971540fafe78) )
	ROM_LOAD( "tt_25b_c.3_1", 0x0000, 0x010000, CRC(d393edf0) SHA1(66f17a88018fee71f3e0c7996371c9b6832ef23a) )
	ROM_LOAD( "tt_25bdc.3_1", 0x0000, 0x010000, CRC(2ce71772) SHA1(a2f36d0d11826a7be7f8cc04f21a77facb4ce188) )
	ROM_LOAD( "tt_25bgc.3_1", 0x0000, 0x010000, CRC(2dbeb9c3) SHA1(8288a9d17932582c7536563e34e2150a85c7a822) )
	ROM_LOAD( "tt_25btc.3_1", 0x0000, 0x010000, CRC(d5702abf) SHA1(6115f39d70dfdf1a00bcfc5f0fe257dd1e0ff968) )
	ROM_LOAD( "tt_25s_c.3_1", 0x0000, 0x010000, CRC(d393edf0) SHA1(66f17a88018fee71f3e0c7996371c9b6832ef23a) )
	ROM_LOAD( "tt_25sbc.3_1", 0x0000, 0x010000, CRC(11c0152f) SHA1(d46b0a6774da35cf9d3a352b9fe7cb574880b210) )
	ROM_LOAD( "tti20___.2_1", 0x0000, 0x010000, CRC(91054bf6) SHA1(68cc6c9b47849149a574e3af97bd0e8255fc5c43) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4ticcla )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ct_20_b4.7_1", 0x0000, 0x010000, CRC(48b9a162) SHA1(2d19a5d6379dc93a56c920b3cd61a0d1a8c6b303) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ct_20_bc.4_1", 0x0000, 0x010000, CRC(fb40b5ff) SHA1(723a07a2b6b08483aa75ecdd4fd9720a66201fc3) )
	ROM_LOAD( "ct_20_d4.7_1", 0x0000, 0x007f17, CRC(016f1ff2) SHA1(316aff73e787bf7a1371afd4373eb1cc453ad536) )
	ROM_LOAD( "ct_20_dc.4_1", 0x0000, 0x010000, CRC(0f20a790) SHA1(02876178f0af64154d490cc048a7bc1c9a6f521b) )
	ROM_LOAD( "ct_20a_4.7_1", 0x0000, 0x010000, CRC(35318095) SHA1(888105a674c9ea8ccad33e24c05ef42936f5f4cf) )
	ROM_LOAD( "ct_20a_c.4_1", 0x0000, 0x010000, CRC(e409f49f) SHA1(8774015ec20ed9fe54e812013dfc12d408276c31) )
	ROM_LOAD( "ct_20b_c.4_1", 0x0000, 0x010000, CRC(864a59cf) SHA1(abd9b7a47c791ce4f91abbd3bf97bdcd9d8296ee) )
	ROM_LOAD( "ct_20bg4.7_1", 0x0000, 0x010000, CRC(7f200f42) SHA1(0ea6aa0de88982737d818c9dac9f2605cea7bc11) )
	ROM_LOAD( "ct_20bgc.4_1", 0x0000, 0x010000, CRC(215b8965) SHA1(883735066a1425b502e89d1234575294ac83746c) )
	ROM_LOAD( "ct_20bt4.7_1", 0x0000, 0x010000, CRC(7c7280a4) SHA1(3dbdc53a3474f4147427ed4fa8a161a3b364d43b) )
	ROM_LOAD( "ct_25_bc.3_1", 0x0000, 0x010000, CRC(9d6fb3b0) SHA1(a6278579d217b5544d9f0b942a7a344596153950) )
	ROM_LOAD( "ct_25_dc.2_1", 0x0000, 0x010000, CRC(b49af435) SHA1(e5f92f114931e554eb8eb5fe89f50298783d541c) )
	ROM_LOAD( "ct_25_dc.3_1", 0x0000, 0x010000, CRC(eb359c82) SHA1(c137768461b859d5277b08c8783b0c8625f9b1be) )
	ROM_LOAD( "ct_25_kc.2_1", 0x0000, 0x010000, CRC(43309e7b) SHA1(d8f6ecbea618da7f54309f2a6e93210c51b68b81) )
	ROM_LOAD( "ct_25a_c.2_1", 0x0000, 0x010000, CRC(717396ed) SHA1(6cdb0f99b40096178f6e85a0966182e704d1b99a) )
	ROM_LOAD( "ct_25a_c.3_1", 0x0000, 0x010000, CRC(28e0a15b) SHA1(b3678ba3d1f392665cc6ec9c24c2c506a41cd4fa) )
	ROM_LOAD( "ct_25b_c.2_1", 0x0000, 0x010000, CRC(b3d7e79c) SHA1(86c0b419c3ca054f8a2ed785cffeb03e6c5b69f2) )
	ROM_LOAD( "ct_25b_c.3_1", 0x0000, 0x010000, CRC(e0ba763d) SHA1(453d8a0dbe616c5a8c4313b918fcfe21fed473e0) )
	ROM_LOAD( "ct_25bgc.2_1", 0x0000, 0x010000, CRC(0869d04c) SHA1(0f0fd3982ac376c66d139655a50639f48bf740b4) )
	ROM_LOAD( "ct_25bgc.3_1", 0x0000, 0x010000, CRC(1e0ca1d1) SHA1(0b1023cdd5cd3db657cea53c85e31ed83c2e5524) )
	ROM_LOAD( "ct_25btc.2_1", 0x0000, 0x010000, CRC(032ec96d) SHA1(c5cef956bc0e3eb45cf128c8d0b4e1d6e5b01afe) )
	ROM_LOAD( "ct_25btc.3_1", 0x0000, 0x010000, CRC(f656897a) SHA1(92ad5c6ce2a696298bbfc8c1750825db4e3bc80b) )
	ROM_LOAD( "ct_30_dc.2_1", 0x0000, 0x010000, CRC(57fabdfb) SHA1(ad86621e4bc8141508c691e148a66e74fc070a88) )
	ROM_LOAD( "ct_30a_c.2_1", 0x0000, 0x010000, CRC(800c94c3) SHA1(c78497899ea9cf27e66f6e8526b95d51215053b2) )
	ROM_LOAD( "ct_30b_c.2_1", 0x0000, 0x010000, CRC(3036ef04) SHA1(de514a85d45d11a880ed147aebe211ffb5bee146) )
	ROM_LOAD( "ct_30bdc.2_1", 0x0000, 0x010000, CRC(9852c9d4) SHA1(37bb20d63fa70ea99e18a16a8f11c461a377a07a) )
	ROM_LOAD( "ct_30bgc.2_1", 0x0000, 0x010000, CRC(a1bc89b4) SHA1(4c82ce8fe78768443823e868f7cc49a06e7cc441) )
	ROM_LOAD( "ct_30btc.2_1", 0x0000, 0x010000, CRC(cde0d12e) SHA1(5427ad700311c30cc86eccc7f1ff36cf0da3b980) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "ct______.5_a", 0x0000, 0x080000, CRC(9a936f50) SHA1(f3f66d6093a939220d24aee985e210cdfd214db4) )
ROM_END


ROM_START( m4ticgld )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tg_25a_c.3_1", 0x0000, 0x010000, CRC(44b2b6b0) SHA1(c2caadd68659bd474df534101e3bc13b15a43694) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "ct______.5_a", 0x0000, 0x080000, CRC(9a936f50) SHA1(f3f66d6093a939220d24aee985e210cdfd214db4) )
ROM_END


ROM_START( m4ticglc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tg_25a_c.3_1", 0x0000, 0x010000, CRC(44b2b6b0) SHA1(c2caadd68659bd474df534101e3bc13b15a43694) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "tg_30_dc.4_1", 0x0000, 0x010000, CRC(19c0fb1e) SHA1(955da095df56f28ace6839c9b6df5669f576730c) )
	ROM_LOAD( "tg_30a_c.4_1", 0x0000, 0x010000, CRC(3e4dcc70) SHA1(c4ad3a8633e19015d4d2b08a653119e9e4c5dcbb) )
	ROM_LOAD( "tg_30b_c.4_1", 0x0000, 0x010000, CRC(83d1517a) SHA1(38a9269dac53ca701e4b621d5e77696142f429cd) )
	ROM_LOAD( "tg_30bgc.4_1", 0x0000, 0x010000, CRC(a366c32d) SHA1(8d86778411ef07e06d99c12147a211d7620af9bf) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4topdog )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "td_20_b4.7_1", 0x0000, 0x010000, CRC(fe864f25) SHA1(b9f97aaf0425b4987b5bfa0b793e9226fdffe58f) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "td_20_bc.7_1", 0x0000, 0x010000, CRC(3af18a9f) SHA1(0db7427d934363d021265fcac811505867f20d47) )
	ROM_LOAD( "td_20_d4.7_1", 0x0000, 0x010000, CRC(35da9e2d) SHA1(a2d1efd7c9cbe4bb5ce7574c6bea2edf55f3e08f) )
	ROM_LOAD( "td_20_dc.7_1", 0x0000, 0x010000, CRC(b90dfbce) SHA1(b9eb9393fbd33725d372b3b6648c261cf0ae486f) )
	ROM_LOAD( "td_20_k4.7_1", 0x0000, 0x010000, CRC(44618034) SHA1(0fce08e279a16d94422155c695b9b5f124b657ea) )
	ROM_LOAD( "td_20_kc.7_1", 0x0000, 0x010000, CRC(8ec10cf7) SHA1(cdc479f7f41f2205285a9db6539dce83feef6af4) )
	ROM_LOAD( "td_20a_4.7_1", 0x0000, 0x010000, CRC(e7bcc879) SHA1(6c963d059867bdd506af1826fe038daa560a3623) )
	ROM_LOAD( "td_20a_c.7_1", 0x0000, 0x010000, CRC(ea229917) SHA1(3e42c1eca1a89b2d536498156beddddcba9899b2) )
	ROM_LOAD( "td_20b_4.7_1", 0x0000, 0x010000, CRC(79468269) SHA1(709f34a0ebea816cb268b5dc36c3d02939cd6224) )
	ROM_LOAD( "td_20b_c.7_1", 0x0000, 0x010000, CRC(1301d28b) SHA1(b0fc0c73dedd89bbdb5845ec9f91530959fabeb6) )
	ROM_LOAD( "td_20bg4.7_1", 0x0000, 0x010000, CRC(4cb61b04) SHA1(6bb56cd06240c1bbb73406fe132e302822dec0df) )
	ROM_LOAD( "td_20bgc.7_1", 0x0000, 0x010000, CRC(8ce831d0) SHA1(e58ca3b38e8dc7196c27cf00123a6e7122bd7f58) )
	ROM_LOAD( "td_20bt4.7_1", 0x0000, 0x010000, CRC(2cdd5be2) SHA1(bc1afe70268eb7e3cb8fe1a43d262201faec0613) )
	ROM_LOAD( "td_20btc.7_1", 0x0000, 0x010000, CRC(67e96c75) SHA1(da9dd06f5d4773fa8e3945cf89cfdde4c465acb9) )
	ROM_LOAD( "td_25_bc.8_1", 0x0000, 0x010000, CRC(ac324184) SHA1(d6743c8cbbe719b12f47792a07ec2e898630591b) )
	ROM_LOAD( "td_25_dc.8_1", 0x0000, 0x010000, CRC(6ea8077c) SHA1(672976af1fad0257be7a15b839ec261653704be8) )
	ROM_LOAD( "td_25_kc.8_1", 0x0000, 0x010000, CRC(e006de48) SHA1(2c09e04d2dc3ec369c4c01eb1ff1af57156d05c1) )
	ROM_LOAD( "td_25a_c.8_1", 0x0000, 0x010000, CRC(84e54ba8) SHA1(dd09094854463f4b7033773be77d4a2d7f06b650) )
	ROM_LOAD( "td_25b_c.8_1", 0x0000, 0x010000, CRC(314f4f03) SHA1(a7c399ddf453305d0dbe2a63e57427b261c48c2c) )
	ROM_LOAD( "td_25bgc.8_1", 0x0000, 0x010000, CRC(efc0899c) SHA1(0d0e5a006d260a1bfcde7966c06360386c949f29) )
	ROM_LOAD( "td_25bgp.2_1", 0x0000, 0x010000, CRC(f0894f48) SHA1(63056dd434d18bb9a052db25cc6ce29d0c3f9f82) )
	ROM_LOAD( "td_25btc.8_1", 0x0000, 0x010000, CRC(f5dec7d9) SHA1(ffb361745aebb3c7d6bf4925d95904e8ced13a35) )
	ROM_LOAD( "td_30a_c.1_1", 0x0000, 0x010000, CRC(f0986895) SHA1(65c24de42a3009959c9bb7f5b42536aa6fd70c2b) )
	ROM_LOAD( "td_30b_c.1_1", 0x0000, 0x010000, CRC(7683cf72) SHA1(4319954b833ef6b0d88b8d22c5e700a9df96dc65) )
	ROM_LOAD( "td_30bdc.1_1", 0x0000, 0x010000, CRC(f5a4481b) SHA1(75b32b0996315b8ce833fd695377716dbeb0b7e4) )
	ROM_LOAD( "td_30bgc.1_1", 0x0000, 0x010000, CRC(1ffe440f) SHA1(adc1909fbbfe7e63bb89b29878bda5a6df776a6a) )
	ROM_LOAD( "td_30btc.1_1", 0x0000, 0x010000, CRC(5109516c) SHA1(a4919465286be9e1f0e7970a91a89738f8fcad4e) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "topdogsnd.bin", 0x0000, 0x080000, CRC(a29047c6) SHA1(5956674e6b895bd46b99f4d04d5797b53ccc6668) )
ROM_END


ROM_START( m4trex )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tr_20a__.2_1", 0x0000, 0x010000, CRC(21150b8e) SHA1(1531bc6fdb8b787fed6f4f98c6463313c55efc3c) )

	ROM_REGION( 0x10000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "tr_20a_p.2_1", 0x0000, 0x010000, CRC(ec1b35bc) SHA1(944959c6d1f8e9b0bb33c659b7c515cb7585fed0) )
	ROM_LOAD( "tr_20ab_.2_1", 0x0000, 0x010000, CRC(f4190bbf) SHA1(45c20c5e56f0bc39e3af5817eb6d705caef14b40) )
	ROM_LOAD( "tr_20bg_.2_1", 0x0000, 0x010000, CRC(14d8d80c) SHA1(6db190550b18401a067eeb26890af39612fc3012) )
	ROM_LOAD( "tr_20bt_.2_1", 0x0000, 0x010000, CRC(3ad19bf3) SHA1(4808f8bbb963be30eace75228d4d1decde3282b0) )
	ROM_LOAD( "tr_20s__.2_1", 0x0000, 0x010000, CRC(2e9460bd) SHA1(8e2ac58cb1686aadad38356a4d47e9502ddbaa52) )
	ROM_LOAD( "tr_20s_p.2_1", 0x0000, 0x010000, CRC(e39a5e8f) SHA1(d1d507040fefa959d71856395639778e898272aa) )
	ROM_LOAD( "tr_20sb_.2_1", 0x0000, 0x010000, CRC(fb98608c) SHA1(48b4fbdc289552131b621c870d7af082b6d8916e) )
	ROM_LOAD( "tr_20sbp.2_1", 0x0000, 0x010000, CRC(36965ebe) SHA1(f2adce7cc97b30b0bf67fd5698b867603973e87a) )
	ROM_LOAD( "tr_20sd_.2_1", 0x0000, 0x010000, CRC(868ea921) SHA1(3055816747b4773ec67669403a81420fabbe327e) )
	ROM_LOAD( "tr_20sdp.2_1", 0x0000, 0x010000, CRC(4b809713) SHA1(d70581ff669d19cf5a91b1546f5c02f27aeda2e4) )
	ROM_LOAD( "tr_20sk_.2_1", 0x0000, 0x010000, CRC(5382a910) SHA1(c8b2811081ec31fecd1b435e775d29e2e6406111) )
	ROM_LOAD( "tr_20skp.2_1", 0x0000, 0x010000, CRC(9e8c9722) SHA1(7aee25966e6d2107f8a8f89acf6af62a73ff05c9) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "tr______.1_2", 0x000000, 0x080000, CRC(75687514) SHA1(dc8f5f1db7da164175c241187cf3f0db1dd71fc9) )
	ROM_LOAD( "tr______.1_3", 0x080000, 0x080000, CRC(1e30d4ed) SHA1(8cd916d28f5060d74a0d795f9b75ab597de1cd60) )
ROM_END







ROM_START( m4volcan )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "vo_sj___.5_0", 0x0000, 0x040000, CRC(78096ebf) SHA1(96915bc2eca00fbd82fab8b3f62e697da118acdd) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "vo_sj_d_.5_0", 0x0000, 0x040000, CRC(87e0347d) SHA1(be5d5b90739fa8ac10f6504290aa58fcf147f323) )
	ROM_LOAD( "vo_sj_k_.5_0", 0x0000, 0x040000, CRC(8604d102) SHA1(34c7df0257ba02ace4a74ffd5b0eed11eea0c333) )
	ROM_LOAD( "vo_sja__.5_0", 0x0000, 0x040000, CRC(d73bade2) SHA1(7e02493ec0710f109ae45e523ef3d7d275aaefab) )
	ROM_LOAD( "vo_sjs__.5_0", 0x0000, 0x040000, CRC(51eff796) SHA1(d0efb1eb4be176906726a438fcffb50cf5ddd217) )
	ROM_LOAD( "vo_vc___.2_0", 0x0000, 0x040000, CRC(24a9e5d6) SHA1(dd4223c3b5c024eb9d56bb45426e327b49f78dde) )
	ROM_LOAD( "vo_vc_d_.2_0", 0x0000, 0x040000, CRC(7f7341b6) SHA1(23d46ca0eed1e942b2a0d33d6ada2434ded5819b) )
	ROM_LOAD( "volcano_bwb_2-0.bin", 0x0000, 0x040000, CRC(20688684) SHA1(fe533341417a3a0b16f485351cb635f4e7d823db) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "vo___snd.1_1", 0x000000, 0x080000, CRC(62bb0166) SHA1(17e5557cce4e7841cbcf5d67783fe78452aacc63) )
	ROM_LOAD( "vo___snd.1_2", 0x080000, 0x080000, CRC(1eded545) SHA1(0010833e42b33fb0fd621a1059e1cf9a123c3fbd) )
	ROM_LOAD( "vo___snd.1_3", 0x100000, 0x080000, CRC(915f4adf) SHA1(fac6644329ee6ef0026d65d8b94c971e01770d45) )
	ROM_LOAD( "vo___snd.1_4", 0x180000, 0x080000, CRC(fec0fbe9) SHA1(26f651c5558a80e88666403d01cf916c3a13d948) )
ROM_END


ROM_START( m4vdexpr )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "vd_sj___.2_0", 0x0000, 0x040000, CRC(03efd2a5) SHA1(4fc3695c24335aef11ba168f660fb519d8c9d473) )

	ROM_REGION( 0x40000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "vd_sj_d_.2_0", 0x0000, 0x040000, CRC(5073b98e) SHA1(66b020b8c096e78e1c9694f1cbc139e97314ab48) )
	ROM_LOAD( "vd_sja__.2_0", 0x0000, 0x040000, CRC(c53dbf48) SHA1(ceee2de3ea8cb511540d90b87bc67bec3309de35) )
	ROM_LOAD( "vd_sjs__.2_0", 0x0000, 0x040000, CRC(036157b3) SHA1(b575751006c3ee59bf0404fa0e177fee9ef9c5db) )
	ROM_LOAD( "vd_vc___.1_0", 0x0000, 0x040000, CRC(6326e14a) SHA1(3dbfbb1cfb60dc10c6972aa2fda89c8e3c3107ea) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "vd___snd.1_1", 0x000000, 0x080000, CRC(d66b43a9) SHA1(087cf1571a9afb8c1c7cac13640fa453b614fd53) )
	ROM_LOAD( "vd___snd.1_2", 0x080000, 0x080000, CRC(a501c887) SHA1(c56a05fd8196afb86e665fec3fe7d02b9bf94c1a) )
	ROM_LOAD( "vd___snd.1_3", 0x100000, 0x080000, CRC(70c6bd96) SHA1(ecdd4276ff72939433630e04bba5be3df569e17e) )
	ROM_LOAD( "vd___snd.1_4", 0x180000, 0x080000, CRC(a6753f41) SHA1(b4af3054b62c3f00f2b5a19b816507fc3a62bef4) )

ROM_END



ROM_START( m4xch )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "ec_25b__.b_0", 0x0000, 0x020000, CRC(cec9e836) SHA1(460ec38566d7608e51b62f1ffebc18a395002ed4) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "ec_36bg_.bv0", 0x0000, 0x020000, CRC(c5d1523a) SHA1(813916008d7e7576e4594a6eb79a76c514470f31) )
	ROM_LOAD( "ec_36bgn.bv0", 0x0000, 0x020000, CRC(4be33ee1) SHA1(888009e09c59f30649eac3238e0b70dec258cb3c) )
	ROM_LOAD( "ec_39b__.b_0", 0x0000, 0x020000, CRC(e5d5961d) SHA1(ac3916cba91a13d1e0820982a1cefabd378647c9) )
	ROM_LOAD( "ec_39bg_.b_0", 0x0000, 0x020000, CRC(49b2be01) SHA1(dbf808d0949a9658d23e57e7eaaa520891a4f0e0) )
	ROM_LOAD( "ec_39bm_.b_0", 0x0000, 0x020000, CRC(29c86cf0) SHA1(384a475d34ad5d15b68d019c3771aba471f89b4d) )
	ROM_LOAD( "ec_49bd_.b_0", 0x0000, 0x020000, CRC(02854189) SHA1(beab97ccd9889cfdb24e1fb6fb373bf9fd114eab) )
	ROM_LOAD( "ec_49bmd.b_0", 0x0000, 0x020000, CRC(2a5578e3) SHA1(d8f100bc83721b6b0f365be7a962249af79d6162) )
	ROM_LOAD( "ec_s9bt_.b_0", 0x0000, 0x020000, CRC(7e0a27d1) SHA1(d4a23a6c358e38a1a66a06b82af85c844f684830) )
	ROM_LOAD( "ec_sja__.a_0", 0x0000, 0x020000, CRC(1f923f89) SHA1(84486287d55591c7e81c59a10e8cc722ec21e8f9) )
	ROM_LOAD( "ec_sja__.b_0", 0x0000, 0x020000, CRC(16d7b8bb) SHA1(be8ab98a64aa976e25cb302b68323c6781034f2b) )
	ROM_LOAD( "xchange.bin", 0x0000, 0x010000, CRC(c96cd014) SHA1(6e32d10c18b6b34dbcb21e75925a77e810ffe892) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "xchasnd.bin", 0x0000, 0x080000, CRC(32c44cd5) SHA1(baafb48e6f95ba152942d1e1c273ffb3c95afa82) )
ROM_END


ROM_START( m4xs )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "es_39b__.3_0", 0x0000, 0x020000, CRC(ba478372) SHA1(c13f9cc4261e91119aa694ec3ac81d94d9f32d22) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "es_39bg_.3_0", 0x0000, 0x020000, CRC(b689f14f) SHA1(0c3253e1f747a979f55d53fe637fc61cf50e01a3) )
	ROM_LOAD( "es_39bm_.3_0", 0x0000, 0x020000, CRC(934f5d1e) SHA1(1ffd462d561d4a16f2392cc90a139499b74a234a) )
	ROM_LOAD( "es_39bmd.3_0", 0x0000, 0x020000, CRC(cf663dcc) SHA1(620e65528687eb5fd6fd879e305e8b7da9b95253) )
	ROM_LOAD( "es_49bg_.3_0", 0x0000, 0x020000, CRC(b76f1d7d) SHA1(9b43a9e847db3d4024f978b6f996534b8d52368b) )
	ROM_LOAD( "es_49bmd.3_0", 0x0000, 0x020000, CRC(1150e499) SHA1(25d2c37e5287f73d2b11608c50f21072422850f0) )
	ROM_LOAD( "es_sja__.3_0", 0x0000, 0x020000, CRC(5909092d) SHA1(64df6ad5ba5ac74592b525af2f4cab8a092a5766) )
	ROM_LOAD( "xs.bin", 0x0000, 0x020000, CRC(1150e499) SHA1(25d2c37e5287f73d2b11608c50f21072422850f0) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4xtrm )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "et_39bg_.2_0", 0x0000, 0x020000, CRC(db1a3c3c) SHA1(081c934ebfc0a9dfa195bb20f51e025e53d9c4b9) )

	ROM_REGION( 0x20000, "altrevs", 0 ) /* alternate revisions - to be sorted / split into clones in the future */
	ROM_LOAD( "et_49bg_.2_0", 0x0000, 0x020000, CRC(f858d927) SHA1(e7ab84c8898a95075a41fb0249e4b103d60e7d85) )
	ROM_LOAD( "et_sja__.2_0", 0x0000, 0x020000, CRC(8ee2602b) SHA1(b9a779b900ac71ec842dd7eb1643f7a2f1cb6a38) )

	ROM_REGION( 0x180000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END



ROM_START( m42punlm )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "2pun0-0.bin", 0x0000, 0x020000, CRC(f8fd7b92) SHA1(400a66d0b401b2df2e2fb0f70eae6da7e547a50b) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "ctnsnd.hex", 0x0000, 0x080000, CRC(150a4513) SHA1(97147e11b49d18225c527d8a0926118a83ee906c))
ROM_END


ROM_START( m4aao )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "aao2_8.bin", 0x0000, 0x010000, CRC(94ce4016) SHA1(2aecb6dbe798b7bbfb3d27f4d115b6611c7d990f) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "aaosnd.bin", 0x0000, 0x080000, CRC(7bf30b96) SHA1(f0086ae239b1d973018a3ea04e816a87f8f20bad) )
ROM_END


ROM_START( m4apachg )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "ag0_3x.bin", 0x0000, 0x020000, CRC(b521b3fd) SHA1(ffdfd4a67f0eb1665f14274f2abc7f59d0050fe5) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "agsnd1.bin", 0x000000, 0x080000, CRC(4fcc8cf1) SHA1(339684ad1bf9f58782bb1ec0d1767fc98bb86b0f) )
	ROM_LOAD( "agsnd2.bin", 0x080000, 0x080000, CRC(d2824ef6) SHA1(32bf329c87a8ea7416cfc217519cd963d4d2430d) )
	ROM_LOAD( "agsnd3.bin", 0x100000, 0x080000, CRC(316549d6) SHA1(72fc19cbee363ba7c71801c480cb87ebf9e64e86) )
ROM_END

ROM_START( m4apachga )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "ag0_4.bin", 0x0000, 0x020000, CRC(99415578) SHA1(73f9947ecee575a4f284a2e3837ec6b87ac2c007) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "agsnd1.bin", 0x000000, 0x080000, CRC(4fcc8cf1) SHA1(339684ad1bf9f58782bb1ec0d1767fc98bb86b0f) )
	ROM_LOAD( "agsnd2.bin", 0x080000, 0x080000, CRC(d2824ef6) SHA1(32bf329c87a8ea7416cfc217519cd963d4d2430d) )
	ROM_LOAD( "agsnd3.bin", 0x100000, 0x080000, CRC(316549d6) SHA1(72fc19cbee363ba7c71801c480cb87ebf9e64e86) )
ROM_END

ROM_START( m4apachgb )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "ag0_4i.bin", 0x0000, 0x020000, CRC(4671a784) SHA1(ea95e82192ad6b53d19cc3c4166b872cd94396d1) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "agsnd1.bin", 0x000000, 0x080000, CRC(4fcc8cf1) SHA1(339684ad1bf9f58782bb1ec0d1767fc98bb86b0f) )
	ROM_LOAD( "agsnd2.bin", 0x080000, 0x080000, CRC(d2824ef6) SHA1(32bf329c87a8ea7416cfc217519cd963d4d2430d) )
	ROM_LOAD( "agsnd3.bin", 0x100000, 0x080000, CRC(316549d6) SHA1(72fc19cbee363ba7c71801c480cb87ebf9e64e86) )
ROM_END

ROM_START( m4apachgc )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "ag1_1x.bin", 0x0000, 0x020000, CRC(d478cd5f) SHA1(8e0adee7cc88ff072154a0db8ceee94d40046c01) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "agsnd1.bin", 0x000000, 0x080000, CRC(4fcc8cf1) SHA1(339684ad1bf9f58782bb1ec0d1767fc98bb86b0f) )
	ROM_LOAD( "agsnd2.bin", 0x080000, 0x080000, CRC(d2824ef6) SHA1(32bf329c87a8ea7416cfc217519cd963d4d2430d) )
	ROM_LOAD( "agsnd3.bin", 0x100000, 0x080000, CRC(316549d6) SHA1(72fc19cbee363ba7c71801c480cb87ebf9e64e86) )
ROM_END

ROM_START( m4apachgd )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "ag1_2.bin", 0x0000, 0x020000, CRC(aa857c28) SHA1(5fe95e59f97b2b6a9fa1996d6501c3230f955081) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "agsnd1.bin", 0x000000, 0x080000, CRC(4fcc8cf1) SHA1(339684ad1bf9f58782bb1ec0d1767fc98bb86b0f) )
	ROM_LOAD( "agsnd2.bin", 0x080000, 0x080000, CRC(d2824ef6) SHA1(32bf329c87a8ea7416cfc217519cd963d4d2430d) )
	ROM_LOAD( "agsnd3.bin", 0x100000, 0x080000, CRC(316549d6) SHA1(72fc19cbee363ba7c71801c480cb87ebf9e64e86) )
ROM_END

ROM_START( m4apachge )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "ag1_2i.bin", 0x0000, 0x020000, CRC(71c3fc7f) SHA1(48000f6068d967504d2bde4d5f9974dd102f7368) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "agsnd1.bin", 0x000000, 0x080000, CRC(4fcc8cf1) SHA1(339684ad1bf9f58782bb1ec0d1767fc98bb86b0f) )
	ROM_LOAD( "agsnd2.bin", 0x080000, 0x080000, CRC(d2824ef6) SHA1(32bf329c87a8ea7416cfc217519cd963d4d2430d) )
	ROM_LOAD( "agsnd3.bin", 0x100000, 0x080000, CRC(316549d6) SHA1(72fc19cbee363ba7c71801c480cb87ebf9e64e86) )
ROM_END

ROM_START( m4apachgf )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "ag2_0x.bin", 0x0000, 0x020000, CRC(083f9f62) SHA1(67beac70ec79c240bd231279abdc97b6eb1872a5) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "agsnd1.bin", 0x000000, 0x080000, CRC(4fcc8cf1) SHA1(339684ad1bf9f58782bb1ec0d1767fc98bb86b0f) )
	ROM_LOAD( "agsnd2.bin", 0x080000, 0x080000, CRC(d2824ef6) SHA1(32bf329c87a8ea7416cfc217519cd963d4d2430d) )
	ROM_LOAD( "agsnd3.bin", 0x100000, 0x080000, CRC(316549d6) SHA1(72fc19cbee363ba7c71801c480cb87ebf9e64e86) )
ROM_END



ROM_START( m4bandgd )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "bog.bin", 0x0000, 0x020000, CRC(21186fb9) SHA1(3d536098c7541cbdf02d68a18a38cae71155d7ff) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "bandsofgoldsnd.bin", 0x0000, 0x080000, CRC(95c6235f) SHA1(a13afa048b73fabfad229b5c2f8ef5ee9948d9fb) )
ROM_END

ROM_START( m4bangrs )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bnc3_0.bin", 0x0000, 0x010000, CRC(c30f947a) SHA1(c734bd966142023e2b7b498ba939972ed32c9fd6) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bncsnd1.bin", 0x000000, 0x080000, CRC(593f29c7) SHA1(b743fa6a029b19570ccc31e5108dccec3a752849) )
	ROM_LOAD( "bncsnd2.bin", 0x080000, 0x080000, CRC(589170a9) SHA1(62ec8bfc4d834c07308d5105979b86452340e98b) )
ROM_END

ROM_START( m4bangrsa )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bnc3_0i.bin", 0x0000, 0x010000, CRC(44afb119) SHA1(5145530fee853c7f63a65566bd1b58b62921dcac) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bncsnd1.bin", 0x000000, 0x080000, CRC(593f29c7) SHA1(b743fa6a029b19570ccc31e5108dccec3a752849) )
	ROM_LOAD( "bncsnd2.bin", 0x080000, 0x080000, CRC(589170a9) SHA1(62ec8bfc4d834c07308d5105979b86452340e98b) )
ROM_END

ROM_START( m4bangrsb )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bnc3_0x.bin", 0x0000, 0x010000, CRC(ff267a9b) SHA1(0b07ef99233df32fdc9621b3f1dbca0549ad99a7) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bncsnd1.bin", 0x000000, 0x080000, CRC(593f29c7) SHA1(b743fa6a029b19570ccc31e5108dccec3a752849) )
	ROM_LOAD( "bncsnd2.bin", 0x080000, 0x080000, CRC(589170a9) SHA1(62ec8bfc4d834c07308d5105979b86452340e98b) )
ROM_END


ROM_START( m4bangin )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "bang.hex", 0x0000, 0x020000, CRC(e40f21d3) SHA1(62319967882f01bbd4d10bca52daffd2fe3ec03a) )
	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "bang.chr", 0x0000, 0x000048, CRC(aacbab22) SHA1(1f394b8947486f319743c0703884ecd35214c433) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "bangs1.hex", 0x000000, 0x080000, CRC(aa460a90) SHA1(e5e80d8b14bd976ed104e376c2e6f995870b0d77) )
	ROM_LOAD( "bangs2.hex", 0x080000, 0x080000, CRC(518ebd38) SHA1(6eaaf0cb34dd430f16b88f9d1ed97d6fb59d00ea) )
	ROM_LOAD( "bangs3.hex", 0x100000, 0x080000, CRC(2da78a75) SHA1(95975bc76fc32d05bd998bb75dcafc6eef7661b3) )
ROM_END

ROM_START( m4bangina )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "bang1-5n.p1", 0x0000, 0x020000, CRC(dabb462a) SHA1(c02fa204bfab07d5edbc784ccaca50f119ce8d5a) )
	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "bang.chr", 0x0000, 0x000048, CRC(aacbab22) SHA1(1f394b8947486f319743c0703884ecd35214c433) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "bangs1.hex", 0x000000, 0x080000, CRC(aa460a90) SHA1(e5e80d8b14bd976ed104e376c2e6f995870b0d77) )
	ROM_LOAD( "bangs2.hex", 0x080000, 0x080000, CRC(518ebd38) SHA1(6eaaf0cb34dd430f16b88f9d1ed97d6fb59d00ea) )
	ROM_LOAD( "bangs3.hex", 0x100000, 0x080000, CRC(2da78a75) SHA1(95975bc76fc32d05bd998bb75dcafc6eef7661b3) )
ROM_END

ROM_START( m4banginb )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "bang1-5p.p1", 0x0000, 0x020000, CRC(84bb8da8) SHA1(ae601957a3cd0b7e4b176987a5592d0b7c9be19d) )
	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "bang.chr", 0x0000, 0x000048, CRC(aacbab22) SHA1(1f394b8947486f319743c0703884ecd35214c433) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "bangs1.hex", 0x000000, 0x080000, CRC(aa460a90) SHA1(e5e80d8b14bd976ed104e376c2e6f995870b0d77) )
	ROM_LOAD( "bangs2.hex", 0x080000, 0x080000, CRC(518ebd38) SHA1(6eaaf0cb34dd430f16b88f9d1ed97d6fb59d00ea) )
	ROM_LOAD( "bangs3.hex", 0x100000, 0x080000, CRC(2da78a75) SHA1(95975bc76fc32d05bd998bb75dcafc6eef7661b3) )
ROM_END


ROM_START( m4wwc )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "wack1-9n.p1", 0x0000, 0x020000, CRC(7ba6fd92) SHA1(3a5c7f9b3ebd8593c76132b46163c9d1299e210e) )

	ROM_REGION( 0x020000, "altrevs", 0 )
	ROM_LOAD( "wack1-9p.p1", 0x0000, 0x020000, CRC(4046b5eb) SHA1(e1ec9158810387b41b574202e9f27e7b741ac81c) )
	ROM_LOAD( "wacky.hex", 0x0000, 0x020000, CRC(a94a06fd) SHA1(a5856b6903fdd35f9dca19b114ca56c106a308f2) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	/* 2 sets of sound roms, one contains an extra sample */
	ROM_LOAD( "wacky1.hex", 0x000000, 0x080000, CRC(379d7af6) SHA1(3b1988c1ab570c075572d0e9bf03fcb331ea4a2c) )
	/* rom 2? should it match? */
	ROM_LOAD( "wacky3.hex", 0x100000, 0x080000, CRC(c7def11a) SHA1(6aab2b7f7e4c852891ee09e91a8a085e9b28803f) )

	ROM_LOAD( "wacky1snd.bin", 0x000000, 0x080000, CRC(45d6869a) SHA1(c1294522d190d22852b5c6006c92911f9e89cfac) )
	ROM_LOAD( "wacky2snd.bin", 0x080000, 0x080000, CRC(18b5f8c8) SHA1(e4dc312eea777c2375ba8c2be2f3c2be71bea5c4) )
	ROM_LOAD( "wacky3snd.bin", 0x100000, 0x080000, CRC(0516acad) SHA1(cfecd089c7250cb19c9e4ca251591f820acefd88) )
ROM_END


ROM_START( m4screw )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "scre0-8n.p1", 0x0000, 0x020000, CRC(5e07b33a) SHA1(6e8835edb61bd0777751bfdfe66d729554a9d6eb) )

	ROM_REGION( 0x020000, "altrevs", 0 )
	ROM_LOAD( "scre0-8p.p1", 0x0000, 0x020000, CRC(34a70a77) SHA1(f76de47f6919d380eb0d0eeffc0e5dda72345038) )
	ROM_LOAD( "screwinaroundv0-7.bin", 0x0000, 0x020000, CRC(78a1e3ca) SHA1(a3d6e76a474a3a5cd74e4b527aa575f21825a7aa) )
	//ROM_LOAD( "screwing.bin", 0x0000, 0x020000, CRC(5e07b33a) SHA1(6e8835edb61bd0777751bfdfe66d729554a9d6eb) )


	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "screwsnd.p1", 0x000000, 0x080000, CRC(6fe4888c) SHA1(b02b7f322d22080123e8b18326910031aa9d39b4) )
	ROM_LOAD( "screwsnd.p2", 0x080000, 0x080000, CRC(29e842ee) SHA1(3325b137361c69244fffaa0d0e39e60106eaa5f9) )
	ROM_LOAD( "screwsnd.p3", 0x100000, 0x080000, CRC(91ef193f) SHA1(a356642ae1093cf69486c434673531042ae27be7) )
ROM_END

ROM_START( m4screwp )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "scre0-8p.p1", 0x0000, 0x020000, CRC(34a70a77) SHA1(f76de47f6919d380eb0d0eeffc0e5dda72345038) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "screwsnd.p1", 0x000000, 0x080000, CRC(6fe4888c) SHA1(b02b7f322d22080123e8b18326910031aa9d39b4) )
	ROM_LOAD( "screwsnd.p2", 0x080000, 0x080000, CRC(29e842ee) SHA1(3325b137361c69244fffaa0d0e39e60106eaa5f9) )
	ROM_LOAD( "screwsnd.p3", 0x100000, 0x080000, CRC(91ef193f) SHA1(a356642ae1093cf69486c434673531042ae27be7) )
ROM_END

ROM_START( m4screwa )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "screwinaroundv0-7.bin", 0x0000, 0x020000, CRC(78a1e3ca) SHA1(a3d6e76a474a3a5cd74e4b527aa575f21825a7aa) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "screwsnd.p1", 0x000000, 0x080000, CRC(6fe4888c) SHA1(b02b7f322d22080123e8b18326910031aa9d39b4) )
	ROM_LOAD( "screwsnd.p2", 0x080000, 0x080000, CRC(29e842ee) SHA1(3325b137361c69244fffaa0d0e39e60106eaa5f9) )
	ROM_LOAD( "screwsnd.p3", 0x100000, 0x080000, CRC(91ef193f) SHA1(a356642ae1093cf69486c434673531042ae27be7) )
ROM_END

ROM_START( m4screwb )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "screw0_5.p1", 0x000000, 0x020000, CRC(4c0e8300) SHA1(1fea75f3cb1a96c14bd0e56a95bafd22996d002d) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "screwsnd.p1", 0x000000, 0x080000, CRC(6fe4888c) SHA1(b02b7f322d22080123e8b18326910031aa9d39b4) )
	ROM_LOAD( "screwsnd.p2", 0x080000, 0x080000, CRC(29e842ee) SHA1(3325b137361c69244fffaa0d0e39e60106eaa5f9) )
	ROM_LOAD( "screwsnd.p3", 0x100000, 0x080000, CRC(91ef193f) SHA1(a356642ae1093cf69486c434673531042ae27be7) )
ROM_END

ROM_START( m4bankrd )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "br3_1x.bin", 0x0000, 0x010000, CRC(a7bc60b3) SHA1(73fc3c0f775b88ce4f8ccf7d60399371656c2144) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "raidsnd1.bin", 0x000000, 0x080000, CRC(97427f72) SHA1(ca68bbf9b701a78d69690cddb10bcdcc4214c161) )
	ROM_LOAD( "raidsnd2.bin", 0x080000, 0x080000, CRC(6bc06e6f) SHA1(8f821feeece6fa9b253d3b35c0bc05f0491c359c) )
ROM_END

ROM_START( m4bankrda )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "br0_2.bin", 0x0000, 0x010000, CRC(3368f610) SHA1(6af4f91675228d0bebca0b7fcfd4661c561d1e0b) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "raidsnd1.bin", 0x000000, 0x080000, CRC(97427f72) SHA1(ca68bbf9b701a78d69690cddb10bcdcc4214c161) )
	ROM_LOAD( "raidsnd2.bin", 0x080000, 0x080000, CRC(6bc06e6f) SHA1(8f821feeece6fa9b253d3b35c0bc05f0491c359c) )
ROM_END

ROM_START( m4bankrdb )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "br1_0i.bin", 0x0000, 0x010000, CRC(545aea13) SHA1(ed8b334ccde1581e4e0b3de15c5d42126cb5a752) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "raidsnd1.bin", 0x000000, 0x080000, CRC(97427f72) SHA1(ca68bbf9b701a78d69690cddb10bcdcc4214c161) )
	ROM_LOAD( "raidsnd2.bin", 0x080000, 0x080000, CRC(6bc06e6f) SHA1(8f821feeece6fa9b253d3b35c0bc05f0491c359c) )
ROM_END

ROM_START( m4bankrdc )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "br3_0.bin", 0x0000, 0x010000, CRC(3e6e2ede) SHA1(eb2b00e3eb62acef89d55dff0fa814c75b3df701) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "raidsnd1.bin", 0x000000, 0x080000, CRC(97427f72) SHA1(ca68bbf9b701a78d69690cddb10bcdcc4214c161) )
	ROM_LOAD( "raidsnd2.bin", 0x080000, 0x080000, CRC(6bc06e6f) SHA1(8f821feeece6fa9b253d3b35c0bc05f0491c359c) )
ROM_END

ROM_START( m4bankrdd )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "br3_0i.bin", 0x0000, 0x010000, CRC(b9ce0bbd) SHA1(92144925e0e389db4e0b1dcf88e6fb8d21ada8db) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "raidsnd1.bin", 0x000000, 0x080000, CRC(97427f72) SHA1(ca68bbf9b701a78d69690cddb10bcdcc4214c161) )
	ROM_LOAD( "raidsnd2.bin", 0x080000, 0x080000, CRC(6bc06e6f) SHA1(8f821feeece6fa9b253d3b35c0bc05f0491c359c) )
ROM_END




ROM_START( m4bigapl )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "app1_5.bin", 0x0000, 0x010000, CRC(ebe6e65f) SHA1(aae70efc4b7e0ad9125424acef634361439e0594) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bigapplesnd.p1", 0x000000, 0x080000, CRC(4afba9b0) SHA1(0a60bb5897ed4403c30902ba12290fd61284d0e7) )
	ROM_LOAD( "bigapplesnd.p2", 0x080000, 0x080000, CRC(7de7b74f) SHA1(419998b585e856925d838fe25ecd9f6f41dfc6a8) )
ROM_END

ROM_START( m4bigapla )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "app1_5c", 0x0000, 0x010000, CRC(458e77ce) SHA1(c01f2dd52c67381b4c09051e6a696841ef0777eb) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bigapplesnd.p1", 0x000000, 0x080000, CRC(4afba9b0) SHA1(0a60bb5897ed4403c30902ba12290fd61284d0e7) )
	ROM_LOAD( "bigapplesnd.p2", 0x080000, 0x080000, CRC(7de7b74f) SHA1(419998b585e856925d838fe25ecd9f6f41dfc6a8) )
ROM_END

ROM_START( m4bigaplb )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "ba2_0.bin", 0x0000, 0x010000, CRC(2d3db68d) SHA1(9bed97820a527b83f515dca06f032c332cbc11d2) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bigapplesnd.p1", 0x000000, 0x080000, CRC(4afba9b0) SHA1(0a60bb5897ed4403c30902ba12290fd61284d0e7) )
	ROM_LOAD( "bigapplesnd.p2", 0x080000, 0x080000, CRC(7de7b74f) SHA1(419998b585e856925d838fe25ecd9f6f41dfc6a8) )
ROM_END

ROM_START( m4bigaplc )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "ba2_0d.bin", 0x0000, 0x010000, CRC(aa9d93ee) SHA1(08c58bcffdf943873a713a006ecb104423b9bb93) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bigapplesnd.p1", 0x000000, 0x080000, CRC(4afba9b0) SHA1(0a60bb5897ed4403c30902ba12290fd61284d0e7) )
	ROM_LOAD( "bigapplesnd.p2", 0x080000, 0x080000, CRC(7de7b74f) SHA1(419998b585e856925d838fe25ecd9f6f41dfc6a8) )
ROM_END

ROM_START( m4bigapld )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "ba6_0x.bin", 0x0000, 0x010000, CRC(7c67032f) SHA1(38f44ec527995a850a0b8fe1d9eab4ee9cae06fa) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bigapplesnd.p1", 0x000000, 0x080000, CRC(4afba9b0) SHA1(0a60bb5897ed4403c30902ba12290fd61284d0e7) )
	ROM_LOAD( "bigapplesnd.p2", 0x080000, 0x080000, CRC(7de7b74f) SHA1(419998b585e856925d838fe25ecd9f6f41dfc6a8) )
ROM_END

ROM_START( m4bigaple )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "ba7_0x.bin", 0x0000, 0x010000, CRC(f393490a) SHA1(48d23e9deb60fe99f9cbe5601053ee6df2b9bf8b) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bigapplesnd.p1", 0x000000, 0x080000, CRC(4afba9b0) SHA1(0a60bb5897ed4403c30902ba12290fd61284d0e7) )
	ROM_LOAD( "bigapplesnd.p2", 0x080000, 0x080000, CRC(7de7b74f) SHA1(419998b585e856925d838fe25ecd9f6f41dfc6a8) )
ROM_END


ROM_START( m4bigben )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "b_bv2_7.bin", 0x0000, 0x010000, CRC(9f3a7638) SHA1(b7169dc26a6e136d6daaf8d012f4c3d017e99e4a) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "big-bensnd1.bin", 0x000000, 0x080000, CRC(e41c3ec1) SHA1(a0c09f51229afcd14f09bb9080d4f3bb198b2050) )
	ROM_LOAD( "big-bensnd2.bin", 0x080000, 0x080000, CRC(ed71dbe1) SHA1(e67ca3c178caacb99118bacfcd7612e699f40455) )
ROM_END

ROM_START( m4bigbena )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "b_bv2_9.bin", 0x0000, 0x010000, CRC(86a745ee) SHA1(2347e8e38c743ea4d00faee6a56bb77e05c9c94d) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "big-bensnd1.bin", 0x000000, 0x080000, CRC(e41c3ec1) SHA1(a0c09f51229afcd14f09bb9080d4f3bb198b2050) )
	ROM_LOAD( "big-bensnd2.bin", 0x080000, 0x080000, CRC(ed71dbe1) SHA1(e67ca3c178caacb99118bacfcd7612e699f40455) )
ROM_END

ROM_START( m4bigbenb )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bb1_9p.bin", 0x0000, 0x010000, CRC(c76c5a09) SHA1(b0e3b38998428f535841ab5373d57cb0d5b21ed3) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "big-bensnd1.bin", 0x000000, 0x080000, CRC(e41c3ec1) SHA1(a0c09f51229afcd14f09bb9080d4f3bb198b2050) )
	ROM_LOAD( "big-bensnd2.bin", 0x080000, 0x080000, CRC(ed71dbe1) SHA1(e67ca3c178caacb99118bacfcd7612e699f40455) )
ROM_END

ROM_START( m4bigbenc )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bb2_9.bin", 0x0000, 0x010000, CRC(86a745ee) SHA1(2347e8e38c743ea4d00faee6a56bb77e05c9c94d) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "big-bensnd1.bin", 0x000000, 0x080000, CRC(e41c3ec1) SHA1(a0c09f51229afcd14f09bb9080d4f3bb198b2050) )
	ROM_LOAD( "big-bensnd2.bin", 0x080000, 0x080000, CRC(ed71dbe1) SHA1(e67ca3c178caacb99118bacfcd7612e699f40455) )
ROM_END

ROM_START( m4bigbend )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bb_2_1.bin", 0x0000, 0x010000, CRC(d3511805) SHA1(c86756998d36e729874c71a5d6442785069c57e9) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "big-bensnd1.bin", 0x000000, 0x080000, CRC(e41c3ec1) SHA1(a0c09f51229afcd14f09bb9080d4f3bb198b2050) )
	ROM_LOAD( "big-bensnd2.bin", 0x080000, 0x080000, CRC(ed71dbe1) SHA1(e67ca3c178caacb99118bacfcd7612e699f40455) )
ROM_END

ROM_START( m4bigbene )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bbs_2_9p.bin", 0x0000, 0x010000, CRC(0107608d) SHA1(9e5def90e77f65c366aea2a9ac24d5f17c4d0ae8) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "big-bensnd1.bin", 0x000000, 0x080000, CRC(e41c3ec1) SHA1(a0c09f51229afcd14f09bb9080d4f3bb198b2050) )
	ROM_LOAD( "big-bensnd2.bin", 0x080000, 0x080000, CRC(ed71dbe1) SHA1(e67ca3c178caacb99118bacfcd7612e699f40455) )
ROM_END


ROM_START( m4bigchs )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "bc1_2.bin", 0x0000, 0x020000, CRC(9d68a5f7) SHA1(3b7d7af95b9aaca2cbc249402cf1e3b074dc0817) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "bcsnd1.bin", 0x000000, 0x080000, CRC(7a0a5144) SHA1(84b8a5c58566cf769826023cc221741dd4d6dd0e) )
	ROM_LOAD( "bcsnd2.bin", 0x080000, 0x080000, CRC(9faf37ab) SHA1(03eb4918d7de6e472351a563f2beb652094b98f4) )
	ROM_LOAD( "bcsnd3.bin", 0x100000, 0x080000, CRC(cd6e26de) SHA1(d84274b3b4bc7126e19bf6c6e1aac561a7aaab77) )
ROM_END

ROM_START( m4bigchsa )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "bc1_2i.bin", 0x0000, 0x020000, CRC(462e25a0) SHA1(52e0b6f89a8c933eca0600e776419234c73e4bdc) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "bcsnd1.bin", 0x000000, 0x080000, CRC(7a0a5144) SHA1(84b8a5c58566cf769826023cc221741dd4d6dd0e) )
	ROM_LOAD( "bcsnd2.bin", 0x080000, 0x080000, CRC(9faf37ab) SHA1(03eb4918d7de6e472351a563f2beb652094b98f4) )
	ROM_LOAD( "bcsnd3.bin", 0x100000, 0x080000, CRC(cd6e26de) SHA1(d84274b3b4bc7126e19bf6c6e1aac561a7aaab77) )
ROM_END

ROM_START( m4bigchsb )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "bc1_2x.bin", 0x0000, 0x020000, CRC(2e418b81) SHA1(489c5a70d289176ad0f66fa630621e24b2c18ce1) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "bcsnd1.bin", 0x000000, 0x080000, CRC(7a0a5144) SHA1(84b8a5c58566cf769826023cc221741dd4d6dd0e) )
	ROM_LOAD( "bcsnd2.bin", 0x080000, 0x080000, CRC(9faf37ab) SHA1(03eb4918d7de6e472351a563f2beb652094b98f4) )
	ROM_LOAD( "bcsnd3.bin", 0x100000, 0x080000, CRC(cd6e26de) SHA1(d84274b3b4bc7126e19bf6c6e1aac561a7aaab77) )
ROM_END



ROM_START( m4blztrl )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bt1_2.bin", 0x0000, 0x010000, CRC(184f5277) SHA1(52342577a09f787f77f8e026e8f7a11998681fb5) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "blazingtrails.p1", 0x000000, 0x080000, CRC(2b82701b) SHA1(377fed5d378d6d3abdce59ba9e7f0d7edd756337) )
	ROM_LOAD( "blazingtrails.p2", 0x080000, 0x080000, CRC(9338b7fc) SHA1(9252e94feac0d65f40152bb9d049cea85ecd16fa) )
	ROM_LOAD( "blazingtrails.p3", 0x100000, 0x080000, CRC(ef37f3fa) SHA1(4e71296cd8eb61ff6b4cf9136dbecdcd6d167472) )
ROM_END

ROM_START( m4blztrla )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bt1_3.bin", 0x0000, 0x010000, CRC(6ae5901d) SHA1(0cd2293ca445549e6c0e0a6f9f6adcf5cdc935b0) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "blazingtrails.p1", 0x000000, 0x080000, CRC(2b82701b) SHA1(377fed5d378d6d3abdce59ba9e7f0d7edd756337) )
	ROM_LOAD( "blazingtrails.p2", 0x080000, 0x080000, CRC(9338b7fc) SHA1(9252e94feac0d65f40152bb9d049cea85ecd16fa) )
	ROM_LOAD( "blazingtrails.p3", 0x100000, 0x080000, CRC(ef37f3fa) SHA1(4e71296cd8eb61ff6b4cf9136dbecdcd6d167472) )
ROM_END



ROM_START( m4bodymt )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "bm0-1.bin", 0x0000, 0x020000, CRC(15c379d9) SHA1(04d3c869870a4eda4dfd075b1c1a6efb1cf3bf57) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bmsnd1.bin", 0x000000, 0x080000, CRC(7f645f66) SHA1(db16e92b6b0c9ac12f7305a4f874a304b93404e6) )
	ROM_LOAD( "bmsnd2.bin", 0x080000, 0x080000, CRC(b8062605) SHA1(570deb41ab8523c5d9b6281a86b915852f6a2305) )
ROM_END


ROM_START( m4boltbl )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bfb.bin", 0x8000, 0x008000, CRC(63058a6b) SHA1(ebccc647a937c36ffc6c7cfc01389f04f829999c) )
ROM_END

ROM_START( m4boltbla )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bfb1.1.bin", 0x8000, 0x008000, CRC(7a91122d) SHA1(28229e86feb4411978e556f7f7bd85bfd996b8aa) )
ROM_END

ROM_START( m4boltblb )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bfb9 5p cash.bin", 0x8000, 0x008000, CRC(792bff34) SHA1(6996e87f22df6bac7bbe9908534b7e0480f03ede) )
ROM_END

ROM_START( m4boltblc )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "bolt-gilwern.bin", 0x8000, 0x008000, CRC(74e2c821) SHA1(1dcdc58585d1dcfc93e2aeb3df0cd41705cde196) )
ROM_END



ROM_START( m4cwalk )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "cw1_1.bin", 0x0000, 0x010000, CRC(a1108d79) SHA1(fa2a5510f2bb2d3811550547bad7c3ef0eb0ddc0) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "cakesnd1.bin", 0x000000, 0x080000, CRC(abd16cc4) SHA1(744dd1da6c2126bc8673f14aac556af29878f2c4) )
	ROM_LOAD( "cakesnd2.bin", 0x080000, 0x080000, CRC(34da47c0) SHA1(b55f352a7a62172d1dfe990bef39bcbd50e48597) )
ROM_END



ROM_START( m4cstrik )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cs3_1.bin", 0x0000, 0x020000, CRC(10b68449) SHA1(8e5688b8d240f4ed4429ddfd97366ca4c998b6ab) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "cssnd1.bin", 0x000000, 0x080000, CRC(a6fedbe0) SHA1(a687151734d378d5c9605be82a22ba50f256885f) )
	ROM_LOAD( "cssnd2.bin", 0x080000, 0x080000, CRC(6160f67c) SHA1(c781d47fe3c6f230442e19ca26523b34808b44a1) )
	ROM_LOAD( "cssnd3.bin", 0x100000, 0x080000, CRC(3911d57a) SHA1(2f0a3a15237876d04b5c9cb72648b27966cd7fb6) )
ROM_END

ROM_START( m4cstrika )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cs3_1i.bin", 0x0000, 0x020000, CRC(cbf0041e) SHA1(9baf9f209f1c4bf59f31437d07051a6cb71e877c) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "cssnd1.bin", 0x000000, 0x080000, CRC(a6fedbe0) SHA1(a687151734d378d5c9605be82a22ba50f256885f) )
	ROM_LOAD( "cssnd2.bin", 0x080000, 0x080000, CRC(6160f67c) SHA1(c781d47fe3c6f230442e19ca26523b34808b44a1) )
	ROM_LOAD( "cssnd3.bin", 0x100000, 0x080000, CRC(3911d57a) SHA1(2f0a3a15237876d04b5c9cb72648b27966cd7fb6) )
ROM_END

ROM_START( m4cstrikb )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cs3_1x.bin", 0x0000, 0x020000, CRC(d01bda0b) SHA1(528df320593656040b7491a0f3f24cc489b45722) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "cssnd1.bin", 0x000000, 0x080000, CRC(a6fedbe0) SHA1(a687151734d378d5c9605be82a22ba50f256885f) )
	ROM_LOAD( "cssnd2.bin", 0x080000, 0x080000, CRC(6160f67c) SHA1(c781d47fe3c6f230442e19ca26523b34808b44a1) )
	ROM_LOAD( "cssnd3.bin", 0x100000, 0x080000, CRC(3911d57a) SHA1(2f0a3a15237876d04b5c9cb72648b27966cd7fb6) )
ROM_END

ROM_START( m4cstrikc )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "c_strike", 0x0000, 0x020000, CRC(d1eac7c6) SHA1(e04a0865e2c55c9351d6bc44616c179a8f5ca059) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "cssnd1.bin", 0x000000, 0x080000, CRC(a6fedbe0) SHA1(a687151734d378d5c9605be82a22ba50f256885f) )
	ROM_LOAD( "cssnd2.bin", 0x080000, 0x080000, CRC(6160f67c) SHA1(c781d47fe3c6f230442e19ca26523b34808b44a1) )
	ROM_LOAD( "cssnd3.bin", 0x100000, 0x080000, CRC(3911d57a) SHA1(2f0a3a15237876d04b5c9cb72648b27966cd7fb6) )
ROM_END




ROM_START( m4chacec )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "ca1_0.bin", 0x0000, 0x020000, CRC(0c9a73b7) SHA1(2ee089ce89f29e804371fcfca82bf22a2ac3197b) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "casnd1.bin", 0x000000, 0x080000, CRC(9bbe8fe4) SHA1(2e5406d72ca731a5960be3a621bbd72064745677) )
	ROM_LOAD( "casnd2.bin", 0x080000, 0x080000, CRC(aa9a45d3) SHA1(47289537451aac1049f7a524b079f2912d97b7cf) )
	ROM_LOAD( "casnd3.bin", 0x100000, 0x080000, CRC(5764e36d) SHA1(6601946bda40886e3a606accd7c11b31efcdab28) )
ROM_END

ROM_START( m4chaceca )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "ca1_0x.bin", 0x0000, 0x020000, CRC(a2476d24) SHA1(12f86733c8fa34d84ff6a1840a24eb96bf547c3d) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "casnd1.bin", 0x000000, 0x080000, CRC(9bbe8fe4) SHA1(2e5406d72ca731a5960be3a621bbd72064745677) )
	ROM_LOAD( "casnd2.bin", 0x080000, 0x080000, CRC(aa9a45d3) SHA1(47289537451aac1049f7a524b079f2912d97b7cf) )
	ROM_LOAD( "casnd3.bin", 0x100000, 0x080000, CRC(5764e36d) SHA1(6601946bda40886e3a606accd7c11b31efcdab28) )
ROM_END



ROM_START( m4chacef )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "ca2-1_0.bin", 0x0000, 0x020000, CRC(c45e650d) SHA1(121e5d178c05d9d38dad167083cb0612f70cbd61) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "casnd1.bin", 0x000000, 0x080000, CRC(9bbe8fe4) SHA1(2e5406d72ca731a5960be3a621bbd72064745677) )
	ROM_LOAD( "casnd2.bin", 0x080000, 0x080000, CRC(aa9a45d3) SHA1(47289537451aac1049f7a524b079f2912d97b7cf) )
	ROM_LOAD( "casnd3.bin", 0x100000, 0x080000, CRC(5764e36d) SHA1(6601946bda40886e3a606accd7c11b31efcdab28) )
ROM_END

ROM_START( m4chacefa )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "ca2_1.bin", 0x0000, 0x020000, CRC(be069065) SHA1(63c108df781345fdb64dc5177bc28b121b097d3a) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "casnd1.bin", 0x000000, 0x080000, CRC(9bbe8fe4) SHA1(2e5406d72ca731a5960be3a621bbd72064745677) )
	ROM_LOAD( "casnd2.bin", 0x080000, 0x080000, CRC(aa9a45d3) SHA1(47289537451aac1049f7a524b079f2912d97b7cf) )
	ROM_LOAD( "casnd3.bin", 0x100000, 0x080000, CRC(5764e36d) SHA1(6601946bda40886e3a606accd7c11b31efcdab28) )
ROM_END

ROM_START( m4chacefb )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "ca2_1i.bin", 0x0000, 0x020000, CRC(fceba4d3) SHA1(9e3ba760dc28122f60e610470ce1f7708eefcbfd) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "casnd1.bin", 0x000000, 0x080000, CRC(9bbe8fe4) SHA1(2e5406d72ca731a5960be3a621bbd72064745677) )
	ROM_LOAD( "casnd2.bin", 0x080000, 0x080000, CRC(aa9a45d3) SHA1(47289537451aac1049f7a524b079f2912d97b7cf) )
	ROM_LOAD( "casnd3.bin", 0x100000, 0x080000, CRC(5764e36d) SHA1(6601946bda40886e3a606accd7c11b31efcdab28) )
ROM_END

ROM_START( m4chacefc )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "ca2_1x.bin", 0x0000, 0x020000, CRC(e1d69187) SHA1(623202d8aaa701709bbcf1fabf0ce6db4fcc18ef) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "casnd1.bin", 0x000000, 0x080000, CRC(9bbe8fe4) SHA1(2e5406d72ca731a5960be3a621bbd72064745677) )
	ROM_LOAD( "casnd2.bin", 0x080000, 0x080000, CRC(aa9a45d3) SHA1(47289537451aac1049f7a524b079f2912d97b7cf) )
	ROM_LOAD( "casnd3.bin", 0x100000, 0x080000, CRC(5764e36d) SHA1(6601946bda40886e3a606accd7c11b31efcdab28) )
ROM_END




ROM_START( m4coloss )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "col0_6k.bin", 0x0000, 0x010000, CRC(53d2431a) SHA1(44da207ce0ba24d110a1aaf6c0705f9c2245d212) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "colsnd1.bin", 0x000000, 0x080000, CRC(a6fa2c68) SHA1(e5572b37086a28bee9ce4dfe549ed60ddbffe444) )
	ROM_LOAD( "colsnd2.bin", 0x080000, 0x080000, CRC(8b01f0cb) SHA1(990fb0b51ddf4eb3f436e11d01d0e5e3b2465ac5) )
ROM_END

ROM_START( m4colossa )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "col0_6s.bin", 0x0000, 0x010000, CRC(e4db7ff7) SHA1(0f8a15b40923ac1cf8780b3b99b0ce070ed1d13d) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "colsnd1.bin", 0x000000, 0x080000, CRC(a6fa2c68) SHA1(e5572b37086a28bee9ce4dfe549ed60ddbffe444) )
	ROM_LOAD( "colsnd2.bin", 0x080000, 0x080000, CRC(8b01f0cb) SHA1(990fb0b51ddf4eb3f436e11d01d0e5e3b2465ac5) )
ROM_END

ROM_START( m4colossb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "col2_0x.bin", 0x0000, 0x010000, CRC(fae24132) SHA1(91bdafe8bfba2b6b350c783fd46963846ca481c8) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "colsnd1.bin", 0x000000, 0x080000, CRC(a6fa2c68) SHA1(e5572b37086a28bee9ce4dfe549ed60ddbffe444) )
	ROM_LOAD( "colsnd2.bin", 0x080000, 0x080000, CRC(8b01f0cb) SHA1(990fb0b51ddf4eb3f436e11d01d0e5e3b2465ac5) )
ROM_END

ROM_START( m4colossc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "col3_0x.bin", 0x0000, 0x010000, CRC(40e0ae7f) SHA1(e6a40c07efbde324091f8a52e615e367ccbb4eaf) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "colsnd1.bin", 0x000000, 0x080000, CRC(a6fa2c68) SHA1(e5572b37086a28bee9ce4dfe549ed60ddbffe444) )
	ROM_LOAD( "colsnd2.bin", 0x080000, 0x080000, CRC(8b01f0cb) SHA1(990fb0b51ddf4eb3f436e11d01d0e5e3b2465ac5) )
ROM_END

ROM_START( m4colossd )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "col3_1.bin", 0x0000, 0x010000, CRC(79d7f4fb) SHA1(8d4a19fbde135d95b5f0e6978a8b89baa4dfe139) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "colsnd1.bin", 0x000000, 0x080000, CRC(a6fa2c68) SHA1(e5572b37086a28bee9ce4dfe549ed60ddbffe444) )
	ROM_LOAD( "colsnd2.bin", 0x080000, 0x080000, CRC(8b01f0cb) SHA1(990fb0b51ddf4eb3f436e11d01d0e5e3b2465ac5) )
ROM_END

ROM_START( m4colosse )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "coll10.bin", 0x0000, 0x010000, CRC(a2468607) SHA1(e926025548e4c0ad1e97b35215b2d28c058126dd) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "colsnd1.bin", 0x000000, 0x080000, CRC(a6fa2c68) SHA1(e5572b37086a28bee9ce4dfe549ed60ddbffe444) )
	ROM_LOAD( "colsnd2.bin", 0x080000, 0x080000, CRC(8b01f0cb) SHA1(990fb0b51ddf4eb3f436e11d01d0e5e3b2465ac5) )
ROM_END

ROM_START( m4colossf )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "colossus-v1.0.bin", 0x0000, 0x010000, CRC(4fc41c62) SHA1(1c088dd278e414081e98689a49b8305c3d3d4db3) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "colsnd1.bin", 0x000000, 0x080000, CRC(a6fa2c68) SHA1(e5572b37086a28bee9ce4dfe549ed60ddbffe444) )
	ROM_LOAD( "colsnd2.bin", 0x080000, 0x080000, CRC(8b01f0cb) SHA1(990fb0b51ddf4eb3f436e11d01d0e5e3b2465ac5) )
ROM_END

ROM_START( m4colossg )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "colossus_8.bin", 0x0000, 0x010000, CRC(4ab3ee66) SHA1(2b61b6f9b43592826f7cb755898fcbc4a381f9b3) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "colsnd1.bin", 0x000000, 0x080000, CRC(a6fa2c68) SHA1(e5572b37086a28bee9ce4dfe549ed60ddbffe444) )
	ROM_LOAD( "colsnd2.bin", 0x080000, 0x080000, CRC(8b01f0cb) SHA1(990fb0b51ddf4eb3f436e11d01d0e5e3b2465ac5) )
ROM_END

ROM_START( m4crzcap )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cc1_0.bin", 0x0000, 0x020000, CRC(e227690c) SHA1(df236e03d2a22a712cd740ed90b55d48d29aaf65) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "ccsnd1.bin", 0x000000, 0x080000, CRC(0961a254) SHA1(a6392f00ff6199a1a31395a12695255b9bd67136) )
	ROM_LOAD( "ccsnd2.bin", 0x080000, 0x080000, CRC(555a4a7a) SHA1(552275fcf0bb5476f97ecb37aa2d4431eb3256fa) )
ROM_END

ROM_START( m4crzcapa )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cc1_0i.bin", 0x0000, 0x020000, CRC(3961e95b) SHA1(09deee5c3d016da1f1f1b81ed9e4edd7e1633a64) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "ccsnd1.bin", 0x000000, 0x080000, CRC(0961a254) SHA1(a6392f00ff6199a1a31395a12695255b9bd67136) )
	ROM_LOAD( "ccsnd2.bin", 0x080000, 0x080000, CRC(555a4a7a) SHA1(552275fcf0bb5476f97ecb37aa2d4431eb3256fa) )
ROM_END

ROM_START( m4crzcapb )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cc1_0x.bin", 0x0000, 0x020000, CRC(88a55a48) SHA1(9567a7d08322de21911bad9b7267c7e5041aa1d1) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "ccsnd1.bin", 0x000000, 0x080000, CRC(0961a254) SHA1(a6392f00ff6199a1a31395a12695255b9bd67136) )
	ROM_LOAD( "ccsnd2.bin", 0x080000, 0x080000, CRC(555a4a7a) SHA1(552275fcf0bb5476f97ecb37aa2d4431eb3256fa) )
ROM_END

ROM_START( m4crzcapc )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cc2_0x.bin", 0x0000, 0x020000, CRC(ac987339) SHA1(b56f77120a544893a92689060eb46b6faf9c91dc) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "ccsnd1.bin", 0x000000, 0x080000, CRC(0961a254) SHA1(a6392f00ff6199a1a31395a12695255b9bd67136) )
	ROM_LOAD( "ccsnd2.bin", 0x080000, 0x080000, CRC(555a4a7a) SHA1(552275fcf0bb5476f97ecb37aa2d4431eb3256fa) )
ROM_END

ROM_START( m4crfire )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cf1_1x.bin", 0x0000, 0x020000, CRC(4267b0f8) SHA1(f0160952af1bfcc08970bb31ba872c8c7e6da996) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "cfsnd1.bin", 0x000000, 0x080000, CRC(cf7d98b3) SHA1(56448d620ab0c9af5ea0f56c29457b80407aa715) )
	ROM_LOAD( "cfsnd2.bin", 0x080000, 0x080000, CRC(e413643d) SHA1(b3b1862a79efd8c777c472c9b07668343deb51b6) )
	ROM_LOAD( "cfsnd3.bin", 0x100000, 0x080000, CRC(21b51239) SHA1(f8fb9cfc23467d2789474d160038324d366c58f4) )
ROM_END



ROM_START( m4crfirea )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cf2_1x.bin", 0x0000, 0x020000, CRC(b872d707) SHA1(1565fd8e15d823fc943da7c35347f5c24cde0858) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "cfsnd1.bin", 0x000000, 0x080000, CRC(cf7d98b3) SHA1(56448d620ab0c9af5ea0f56c29457b80407aa715) )
	ROM_LOAD( "cfsnd2.bin", 0x080000, 0x080000, CRC(e413643d) SHA1(b3b1862a79efd8c777c472c9b07668343deb51b6) )
	ROM_LOAD( "cfsnd3.bin", 0x100000, 0x080000, CRC(21b51239) SHA1(f8fb9cfc23467d2789474d160038324d366c58f4) )
ROM_END



ROM_START( m4dblchn )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "doublechance.bin", 0x0000, 0x010000, CRC(6feeeb7d) SHA1(40fe67d854fbf48959e08fdb5743e14d340c16e7) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "doublechancesnd.bin", 0x0000, 0x080000, CRC(3e80f8bd) SHA1(2e3a195b49448da11cc0c089a8a9b462894c766b) )
ROM_END


ROM_START( m4eezee )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "ez2511.bin", 0x0000, 0x010000, CRC(86e93c3f) SHA1(a999830685da5d183058769a0598c338c20accdf) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "eezeefruitssnd.bin", 0x0000, 0x080000, CRC(e6e5aa12) SHA1(0f35eaf0a29050365f53d039e4a7880240c28dc4) )
ROM_END


ROM_START( m4eureka )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "eu1_4i.bin", 0x0000, 0x020000, CRC(2280f25a) SHA1(1898aa5eb73f27b33a902c1696679f6dce115640) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "eusnd1.bin", 0x000000, 0x080000, CRC(fa35fcf4) SHA1(9869fb16383b7aa93d044cfc6fd864a442f225e7) )
	ROM_LOAD( "eusnd2.bin", 0x080000, 0x080000, CRC(f0218dc1) SHA1(28c149c0f94fe724734b6095b34e54a1e7449f28) )
	ROM_LOAD( "eusnd3.bin", 0x100000, 0x080000, CRC(31e63a47) SHA1(985cedec8945c4bb5dec0ed7d888fb4e291bda8b) )
ROM_END

ROM_START( m4eurekaa )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "eu1_4.bin", 0x0000, 0x020000, CRC(f9c6720d) SHA1(cd477099821a36c9731fdaaea900fe3614614798) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "eusnd1.bin", 0x000000, 0x080000, CRC(fa35fcf4) SHA1(9869fb16383b7aa93d044cfc6fd864a442f225e7) )
	ROM_LOAD( "eusnd2.bin", 0x080000, 0x080000, CRC(f0218dc1) SHA1(28c149c0f94fe724734b6095b34e54a1e7449f28) )
	ROM_LOAD( "eusnd3.bin", 0x100000, 0x080000, CRC(31e63a47) SHA1(985cedec8945c4bb5dec0ed7d888fb4e291bda8b) )
ROM_END

ROM_START( m4eurekab )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "eu1_4x.bin", 0x0000, 0x020000, CRC(eb2262e4) SHA1(e807f2a5dccb3ceda4edd2a295fcfd7f154bf54d) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "eusnd1.bin", 0x000000, 0x080000, CRC(fa35fcf4) SHA1(9869fb16383b7aa93d044cfc6fd864a442f225e7) )
	ROM_LOAD( "eusnd2.bin", 0x080000, 0x080000, CRC(f0218dc1) SHA1(28c149c0f94fe724734b6095b34e54a1e7449f28) )
	ROM_LOAD( "eusnd3.bin", 0x100000, 0x080000, CRC(31e63a47) SHA1(985cedec8945c4bb5dec0ed7d888fb4e291bda8b) )
ROM_END



ROM_START( m4firebl )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "fb2_0x.bin", 0x0000, 0x010000, CRC(78b6d22f) SHA1(cb23ac9985a39a6052156732a9be1588207834ce) )
ROM_END

ROM_START( m4firebla )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "fb3_1.bin", 0x0000, 0x010000, CRC(baa497e1) SHA1(5732fbb9b590fc389cad21a40ac264d01461f6e5) )
ROM_END

ROM_START( m4fireblb )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "fb3_3x.bin", 0x0000, 0x010000, CRC(bc4f65cb) SHA1(8397437f6098b246568666e06e8bf40a3f2ea51c) )
ROM_END

ROM_START( m4fireblc )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "fba2_6c", 0x0000, 0x010000, CRC(f2cdbb1f) SHA1(fa12e8992d8af6ded01ab54eedff56ab050e8ee1) )
ROM_END

ROM_START( m4firebld ) // looks weird, bad dump?
	ROM_REGION( 0x080000, "temp", 0 )
	ROM_LOAD( "fba2_6", 0x0000, 0x080000, CRC(cf16b2d0) SHA1(32249fb15708dfa408c4642be7a41fba7aeda657) )

	ROM_REGION( 0x010000, "maincpu", 0 )
	// reports src/mame/drivers/mpu4.c: m4firebld has ROM fba2_6 extending past the defined memory region (bug)
	//ROM_LOAD( "fba2_6", 0x0000, 0x010000, CRC(cf16b2d0) SHA1(32249fb15708dfa408c4642be7a41fba7aeda657) )
	//ROM_IGNORE(0x070000)
	ROM_COPY( "temp", 0x0000, 0x0000, 0x010000 )
ROM_END




ROM_START( m4fright )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "fn4_1x.bin", 0x0000, 0x020000, CRC(f7bb8da6) SHA1(753edb1123d3ea364ded86b566a2c62a039c3d65) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "fnsnd1.bin", 0x000000, 0x080000, CRC(0f7a6d97) SHA1(f812631af8eb46e188d457d567f42aecceb9e5d2) )
	ROM_LOAD( "fnsnd2.bin", 0x080000, 0x080000, CRC(f2d0c27c) SHA1(4d18049a926898f7fbca54dd30519199fe39f8ea) )
	ROM_LOAD( "fnsnd3.bin", 0x100000, 0x080000, CRC(7ad8aecc) SHA1(8d10a27efbde41af8e04ebe7e8b4b921443bd560) )
ROM_END


ROM_START( m4frighta )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "fn4_1.bin", 0x0000, 0x020000, CRC(801c3db2) SHA1(6c3e9b5ac47807196fb7c9e59112fcbd71edb65d) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "fnsnd1.bin", 0x000000, 0x080000, CRC(0f7a6d97) SHA1(f812631af8eb46e188d457d567f42aecceb9e5d2) )
	ROM_LOAD( "fnsnd2.bin", 0x080000, 0x080000, CRC(f2d0c27c) SHA1(4d18049a926898f7fbca54dd30519199fe39f8ea) )
	ROM_LOAD( "fnsnd3.bin", 0x100000, 0x080000, CRC(7ad8aecc) SHA1(8d10a27efbde41af8e04ebe7e8b4b921443bd560) )
ROM_END

ROM_START( m4frightb )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "fn4_1i.bin", 0x0000, 0x020000, CRC(5b5abde5) SHA1(0583c65755954cc228d32672ea55b7f2afc052c4) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "fnsnd1.bin", 0x000000, 0x080000, CRC(0f7a6d97) SHA1(f812631af8eb46e188d457d567f42aecceb9e5d2) )
	ROM_LOAD( "fnsnd2.bin", 0x080000, 0x080000, CRC(f2d0c27c) SHA1(4d18049a926898f7fbca54dd30519199fe39f8ea) )
	ROM_LOAD( "fnsnd3.bin", 0x100000, 0x080000, CRC(7ad8aecc) SHA1(8d10a27efbde41af8e04ebe7e8b4b921443bd560) )
ROM_END

ROM_START( m4frightc )
	ROM_REGION( 0x020000, "maincpu", 0 ) // fixed bits, probably the same as one of the others sets anyway, remove?
	ROM_LOAD( "frnt8ac", 0x0000, 0x020000, BAD_DUMP CRC(db081875) SHA1(1e994dd411c81eb9d152b9fa2c3e53258d680dfa) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "fnsnd1.bin", 0x000000, 0x080000, CRC(0f7a6d97) SHA1(f812631af8eb46e188d457d567f42aecceb9e5d2) )
	ROM_LOAD( "fnsnd2.bin", 0x080000, 0x080000, CRC(f2d0c27c) SHA1(4d18049a926898f7fbca54dd30519199fe39f8ea) )
	ROM_LOAD( "fnsnd3.bin", 0x100000, 0x080000, CRC(7ad8aecc) SHA1(8d10a27efbde41af8e04ebe7e8b4b921443bd560) )
ROM_END

ROM_START( m4frightd )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "frite.nig",  0x000000, 0x020000, CRC(57febdc0) SHA1(001b134be59367c332df4df14930045d9437111a) )
	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "fnsnd1.bin", 0x000000, 0x080000, CRC(0f7a6d97) SHA1(f812631af8eb46e188d457d567f42aecceb9e5d2) )
	ROM_LOAD( "fnsnd2.bin", 0x080000, 0x080000, CRC(f2d0c27c) SHA1(4d18049a926898f7fbca54dd30519199fe39f8ea) )
	ROM_LOAD( "fnsnd3.bin", 0x100000, 0x080000, CRC(7ad8aecc) SHA1(8d10a27efbde41af8e04ebe7e8b4b921443bd560) )
ROM_END

ROM_START( m4frighte )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "fr_ngt._pound5", 0x0000, 0x020000, CRC(3f2570c2) SHA1(98cc2438eb3fbf07481e2f68cac744e9e0a63e0b) )
	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "fnsnd1.bin", 0x000000, 0x080000, CRC(0f7a6d97) SHA1(f812631af8eb46e188d457d567f42aecceb9e5d2) )
	ROM_LOAD( "fnsnd2.bin", 0x080000, 0x080000, CRC(f2d0c27c) SHA1(4d18049a926898f7fbca54dd30519199fe39f8ea) )
	ROM_LOAD( "fnsnd3.bin", 0x100000, 0x080000, CRC(7ad8aecc) SHA1(8d10a27efbde41af8e04ebe7e8b4b921443bd560) )
ROM_END

ROM_START( m4frdrop )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "fruitdrop.bin", 0x0000, 0x010000, CRC(7235d3f0) SHA1(e327e28e341ec859f503b71065c40b5d47f448fe) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "fruitdropsnd.bin", 0x0000, 0x080000, CRC(27880a95) SHA1(6286ab0c342db7de174c3582f56cf9dd60c46883) )
ROM_END


ROM_START( m4gamblr )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "tg4_0k.bin", 0x0000, 0x010000, CRC(d579bd7e) SHA1(b3db3c8a7f30d773a63aab0efe753deacd3db96c) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "gambsnd1.bin", 0x000000, 0x080000, CRC(a3114336) SHA1(539c896ae512a01340471e2e0df542e582b11258) )
	ROM_LOAD( "gambsnd2.bin", 0x080000, 0x080000, CRC(bc8b78bc) SHA1(6a27804483eaed7912fb6a6e673d1ce9f36371cd) )

	ROM_REGION( 0x100000, "altrevs", 0 )
	// different SFX, does this belong to a specific revision?
	ROM_LOAD( "gambsnd1f.bin", 0x000000, 0x080000, CRC(249ae0fd) SHA1(024ae694f6d09b7f2bf5b94e3a07e9267707f794) )
	ROM_LOAD( "gambsnd2f.bin", 0x080000, 0x080000, CRC(bc8b78bc) SHA1(6a27804483eaed7912fb6a6e673d1ce9f36371cd) )
ROM_END

ROM_START( m4gamblra )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "tg4_0ki.bin", 0x0000, 0x010000, CRC(52d9981d) SHA1(3e30120491d0546b3e19b4b84079cecadd6cdb94) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "gambsnd1.bin", 0x000000, 0x080000, CRC(a3114336) SHA1(539c896ae512a01340471e2e0df542e582b11258) )
	ROM_LOAD( "gambsnd2.bin", 0x080000, 0x080000, CRC(bc8b78bc) SHA1(6a27804483eaed7912fb6a6e673d1ce9f36371cd) )
ROM_END

ROM_START( m4gamblrb )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "tg4_1x.bin", 0x0000, 0x010000, CRC(e238c6c2) SHA1(6b148221d8c9468efca8eddc0520f4abf5a38200) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "gambsnd1.bin", 0x000000, 0x080000, CRC(a3114336) SHA1(539c896ae512a01340471e2e0df542e582b11258) )
	ROM_LOAD( "gambsnd2.bin", 0x080000, 0x080000, CRC(bc8b78bc) SHA1(6a27804483eaed7912fb6a6e673d1ce9f36371cd) )
ROM_END


ROM_START( m4gtrain )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "ghosttrainvers3-0.bin", 0x0000, 0x010000, CRC(17f3dd0f) SHA1(0364b4fe3fc273a658feeaecee1ebc0b55a12d98) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "gtsnd1.bin", 0x000000, 0x080000, CRC(7fd83279) SHA1(65b52330e8d6ccf5c0575924a1791e7d2001c3d8) )
	ROM_LOAD( "gtsnd2.bin", 0x080000, 0x080000, CRC(5bfd0ea2) SHA1(af9adcf517801c775eb316c36538b1bf2262ebb2) )
ROM_END

ROM_START( m4gtraina )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "gt3_0kx.bin", 0x0000, 0x010000, CRC(a6a5461f) SHA1(89652a760a6419064f5c9a52c1bfd066e79e345e) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "gtsnd1.bin", 0x000000, 0x080000, CRC(7fd83279) SHA1(65b52330e8d6ccf5c0575924a1791e7d2001c3d8) )
	ROM_LOAD( "gtsnd2.bin", 0x080000, 0x080000, CRC(5bfd0ea2) SHA1(af9adcf517801c775eb316c36538b1bf2262ebb2) )
ROM_END

ROM_START( m4gtrainb )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "gt3_1k.bin", 0x0000, 0x010000, CRC(13934116) SHA1(a7e7420e62df0e34a77800e61a9df4ba1e3772c6) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "gtsnd1.bin", 0x000000, 0x080000, CRC(7fd83279) SHA1(65b52330e8d6ccf5c0575924a1791e7d2001c3d8) )
	ROM_LOAD( "gtsnd2.bin", 0x080000, 0x080000, CRC(5bfd0ea2) SHA1(af9adcf517801c775eb316c36538b1bf2262ebb2) )
ROM_END

ROM_START( m4gtrainc )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "gt3_1ki.bin", 0x0000, 0x010000, CRC(94336475) SHA1(4655631bc65faa82270da606bba8ffcb2d335f26) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "gtsnd1.bin", 0x000000, 0x080000, CRC(7fd83279) SHA1(65b52330e8d6ccf5c0575924a1791e7d2001c3d8) )
	ROM_LOAD( "gtsnd2.bin", 0x080000, 0x080000, CRC(5bfd0ea2) SHA1(af9adcf517801c775eb316c36538b1bf2262ebb2) )
ROM_END



ROM_START( m4gobana )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "go_b_11_dr_c468.bin", 0x0000, 0x010000, CRC(036776d1) SHA1(73d63731978495cedc5e4c095a83925424ac79c3) )
ROM_END


ROM_START( m4gobanaa )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "go_b_11p_9877.bin", 0x0000, 0x010000, CRC(d985a6eb) SHA1(59576ad9e7589651a6479adb57e8c8bf32a8ef97) )
ROM_END

ROM_START( m4gobanab )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "go_b_11p_dr_c46b.bin", 0x0000, 0x010000, CRC(7343588f) SHA1(e40416a8fdea5a71677493047e569d452ba193df) )
ROM_END

ROM_START( m4gobanac )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "go_b_v11_9873.bin", 0x0000, 0x010000, CRC(3c23581c) SHA1(81daa4903ace4419c1329acbba8907c46b6e6233) )
ROM_END

ROM_START( m4gobanad )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "go_bananas_mecca_v2_31a5.bin", 0x0000, 0x010000, CRC(0c7ae1da) SHA1(9b144a2f1b91def5d9f054fba3d9f1adfe957646) )
ROM_END


ROM_START( m4goldfv )
	ROM_REGION( 0x020000, "maincpu", 0 )
	ROM_LOAD( "gf1_4.bin", 0x0000, 0x020000, CRC(9eb00e69) SHA1(3d04b8c6776bead54d21c0a40d51ed044716897e) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "gfsnd.bin", 0x0000, 0x080000, CRC(1bb14a13) SHA1(44e888e625cce27bc550a93fce3747885802f5c2) )
ROM_END

ROM_START( m4gvibes )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "gv0_1.bin", 0x0000, 0x010000, CRC(ba9a507b) SHA1(ba3b2038b50248ec1f3319e2b39d02313ce3ad08) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "gvsnd.p1", 0x0000, 0x080000, CRC(ac56b475) SHA1(8017784e5dd8e6d85857ff989c553d04c2ea217a) )

	ROM_REGION( 0x080000, "altrevs", 0 )
	// different SFX, does this belong to a specific revision?
	ROM_LOAD( "gv2snd.bin", 0x0000, 0x080000, CRC(e11ebc9c) SHA1(3f4e8148bc3687af77838b770bbc219a3f50f1c6) )
ROM_END

ROM_START( m4gvibesa )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "gv0_3.bin", 0x0000, 0x010000, CRC(c20e896e) SHA1(9105ba56b9f4b55a158e4dc2b4268a4b93765c88) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "gvsnd.p1", 0x0000, 0x080000, CRC(ac56b475) SHA1(8017784e5dd8e6d85857ff989c553d04c2ea217a) )
ROM_END


ROM_START( m4haunt )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hh0.3", 0x0000, 0x010000, CRC(95370728) SHA1(63c7f2fa890c385556a570f3e8941f083a3917bc) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "hhsnd1.bin", 0x000000, 0x080000, CRC(a2eff4c6) SHA1(86441371b8efbffb93c6c7d02d45cd5dae73ca45) )
	ROM_LOAD( "hhsnd2.bin", 0x080000, 0x080000, CRC(6eb3f52c) SHA1(7b6f7a5bdc5e9937e0b74ce317c951d9ad82425c) )
ROM_END

ROM_START( m4haunta )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hh0_2x.bin", 0x0000, 0x010000, CRC(3a32332f) SHA1(837b75eb37367ea204c758918ec8eb6370196aa8) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "hhsnd1.bin", 0x000000, 0x080000, CRC(a2eff4c6) SHA1(86441371b8efbffb93c6c7d02d45cd5dae73ca45) )
	ROM_LOAD( "hhsnd2.bin", 0x080000, 0x080000, CRC(6eb3f52c) SHA1(7b6f7a5bdc5e9937e0b74ce317c951d9ad82425c) )
ROM_END

ROM_START( m4hauntb )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hh3_0.bin", 0x0000, 0x010000, CRC(f0fc3475) SHA1(24f3ab5990d40b742416f600ffa50e6fc02990ca) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "hhsnd1.bin", 0x000000, 0x080000, CRC(a2eff4c6) SHA1(86441371b8efbffb93c6c7d02d45cd5dae73ca45) )
	ROM_LOAD( "hhsnd2.bin", 0x080000, 0x080000, CRC(6eb3f52c) SHA1(7b6f7a5bdc5e9937e0b74ce317c951d9ad82425c) )
ROM_END

ROM_START( m4hauntc )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hh3_0i.bin", 0x0000, 0x010000, CRC(775c1116) SHA1(9fcc9d99b0fc97d98c1a74de7f60e3307ee06448) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "hhsnd1.bin", 0x000000, 0x080000, CRC(a2eff4c6) SHA1(86441371b8efbffb93c6c7d02d45cd5dae73ca45) )
	ROM_LOAD( "hhsnd2.bin", 0x080000, 0x080000, CRC(6eb3f52c) SHA1(7b6f7a5bdc5e9937e0b74ce317c951d9ad82425c) )
ROM_END

ROM_START( m4hauntd )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hh3_0x.bin", 0x0000, 0x010000, CRC(42b064db) SHA1(158ec14a34423bea0f9bfb0255ad7b1b2618c9ca) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "hhsnd1.bin", 0x000000, 0x080000, CRC(a2eff4c6) SHA1(86441371b8efbffb93c6c7d02d45cd5dae73ca45) )
	ROM_LOAD( "hhsnd2.bin", 0x080000, 0x080000, CRC(6eb3f52c) SHA1(7b6f7a5bdc5e9937e0b74ce317c951d9ad82425c) )
ROM_END

ROM_START( m4haunte )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hs3_0i.bin", 0x0000, 0x010000, CRC(c4d06c05) SHA1(e9256e656c698723158f835a32cdf668ed6120c8) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "hhsnd1.bin", 0x000000, 0x080000, CRC(a2eff4c6) SHA1(86441371b8efbffb93c6c7d02d45cd5dae73ca45) )
	ROM_LOAD( "hhsnd2.bin", 0x080000, 0x080000, CRC(6eb3f52c) SHA1(7b6f7a5bdc5e9937e0b74ce317c951d9ad82425c) )
ROM_END







ROM_START( m4hisprt )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hisp1.4", 0x0000, 0x010000, CRC(f80ceefb) SHA1(f8925329f8a1f0f0b61d3de9ebc2d76a7b64be45) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "highsnd1.bin", 0x000000, 0x080000, CRC(b5084d9c) SHA1(0b59ec1735ccc641f3883746027aab6660fac471) )
	ROM_LOAD( "highsnd2.bin", 0x080000, 0x080000, CRC(0d3b50e9) SHA1(fdca97ec314e2efdd9fcd471ee509fd83f980df6) )

	//different hw
	//ROM_LOAD( "spir0_1e_demo.p1", 0x0000, 0x020000, CRC(016e68db) SHA1(efb9da76b16352588ba9a831210f135b13c0fec9) )
	//ROM_LOAD( "spir0_1e_demo.p2", 0x0000, 0x020000, CRC(49b62046) SHA1(e07db0ce27896af4f508993d935135264cfe0ba1) )
	//ROM_LOAD( "spirsnd_demo.bin", 0x0000, 0x080000, CRC(15c6771f) SHA1(a99f142f53637af361699a73e229dcce224b117f) )
ROM_END

ROM_START( m4hisprta )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hs1_0x.bin", 0x0000, 0x010000, CRC(0cd54416) SHA1(54c1959ecd0e40b4fd2bce7cbf435f66ddc34626) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "highsnd1.bin", 0x000000, 0x080000, CRC(b5084d9c) SHA1(0b59ec1735ccc641f3883746027aab6660fac471) )
	ROM_LOAD( "highsnd2.bin", 0x080000, 0x080000, CRC(0d3b50e9) SHA1(fdca97ec314e2efdd9fcd471ee509fd83f980df6) )
ROM_END

ROM_START( m4hisprtb )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hs1_3.bin", 0x0000, 0x010000, CRC(816101e8) SHA1(654812e4a6cf76787d944abdd914aa5727e06437) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "highsnd1.bin", 0x000000, 0x080000, CRC(b5084d9c) SHA1(0b59ec1735ccc641f3883746027aab6660fac471) )
	ROM_LOAD( "highsnd2.bin", 0x080000, 0x080000, CRC(0d3b50e9) SHA1(fdca97ec314e2efdd9fcd471ee509fd83f980df6) )
ROM_END


ROM_START( m4hisprtc )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hs3_0.bin", 0x0000, 0x010000, CRC(43704966) SHA1(78989fa9743efc348f1e81ce040ef9eaf00a47fe) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "highsnd1.bin", 0x000000, 0x080000, CRC(b5084d9c) SHA1(0b59ec1735ccc641f3883746027aab6660fac471) )
	ROM_LOAD( "highsnd2.bin", 0x080000, 0x080000, CRC(0d3b50e9) SHA1(fdca97ec314e2efdd9fcd471ee509fd83f980df6) )
ROM_END

ROM_START( m4hisprtd )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hs3_0i.bin", 0x0000, 0x010000, CRC(c4d06c05) SHA1(e9256e656c698723158f835a32cdf668ed6120c8) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "highsnd1.bin", 0x000000, 0x080000, CRC(b5084d9c) SHA1(0b59ec1735ccc641f3883746027aab6660fac471) )
	ROM_LOAD( "highsnd2.bin", 0x080000, 0x080000, CRC(0d3b50e9) SHA1(fdca97ec314e2efdd9fcd471ee509fd83f980df6) )
ROM_END

ROM_START( m4hisprte )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hs3_0x.bin", 0x0000, 0x010000, CRC(9a7276f1) SHA1(a683dcf0272d868dbc8be83ad2debcd174453559) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "highsnd1.bin", 0x000000, 0x080000, CRC(b5084d9c) SHA1(0b59ec1735ccc641f3883746027aab6660fac471) )
	ROM_LOAD( "highsnd2.bin", 0x080000, 0x080000, CRC(0d3b50e9) SHA1(fdca97ec314e2efdd9fcd471ee509fd83f980df6) )
ROM_END


ROM_START( m4hotcsh )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hc3_0k.bin", 0x0000, 0x010000, CRC(e3cfa94a) SHA1(d21d2dac4edbf3fde9adab399bdd530e034af122) )
	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "db.chr", 0x00, 0x48, CRC(0fc2bb52) SHA1(0d0e47938f6e00166e7352732ddfb7c610f44db2) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "hotsnd1.bin", 0x000000, 0x080000, CRC(eef55915) SHA1(b673a05a0313271cc16645f277d37a4a03deced1) )
	ROM_LOAD( "hotsnd2.bin", 0x080000, 0x080000, CRC(92e921ab) SHA1(11e1f3c61a2eddfdcb40f606672d8845000c4ce7) )
ROM_END

ROM_START( m4hotcsha )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hc3_0ki.bin", 0x0000, 0x010000, CRC(646f8c29) SHA1(19d60faf77a7a83efc3ea4b614a4bc1dee53b8d8) )
	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "db.chr", 0x00, 0x48, CRC(0fc2bb52) SHA1(0d0e47938f6e00166e7352732ddfb7c610f44db2) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "hotsnd1.bin", 0x000000, 0x080000, CRC(eef55915) SHA1(b673a05a0313271cc16645f277d37a4a03deced1) )
	ROM_LOAD( "hotsnd2.bin", 0x080000, 0x080000, CRC(92e921ab) SHA1(11e1f3c61a2eddfdcb40f606672d8845000c4ce7) )
ROM_END

ROM_START( m4hotcshb )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "hc3_0kx.bin", 0x0000, 0x010000, CRC(3abebe72) SHA1(fec09ca41e8e43628140456bb44ce6e7c66f5270) )

	ROM_REGION( 0x48, "fakechr", 0 )
	ROM_LOAD( "db.chr", 0x00, 0x48, CRC(0fc2bb52) SHA1(0d0e47938f6e00166e7352732ddfb7c610f44db2) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "hotsnd1.bin", 0x000000, 0x080000, CRC(eef55915) SHA1(b673a05a0313271cc16645f277d37a4a03deced1) )
	ROM_LOAD( "hotsnd2.bin", 0x080000, 0x080000, CRC(92e921ab) SHA1(11e1f3c61a2eddfdcb40f606672d8845000c4ce7) )
ROM_END


ROM_START( m4jne )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "jne2_5.bin", 0x0000, 0x020000, CRC(541794df) SHA1(08ce8fa1f9ab715bf8bd55a71fc25deead204026) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "jnesnd.bin", 0x0000, 0x080000, CRC(47301e22) SHA1(b7ec2ff3b78ceecc0e50142dbbc40929f2526f3f) )
ROM_END


ROM_START( m4kqclub )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "kingsque.p1", 0x8000, 0x008000, CRC(6501e501) SHA1(e289a9418c640415967fafda43f20877b38e3671) )
ROM_END


ROM_START( m4lotty )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "lottytime.bin", 0x0000, 0x010000, CRC(c032c422) SHA1(37c6e10c1fa1cab3de7b4d27ed027604ecea4394) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "lottytimesnd.bin", 0x0000, 0x080000, CRC(e1966300) SHA1(2a69e39310b49c685bc4307e0396a3b9a0849472) )
ROM_END


ROM_START( m4maxmze )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "maximize_dr_v13_5146.bin", 0x0000, 0x010000, CRC(86507f09) SHA1(e2dbf9b77a5155faeeba05a939ae217289949bce) )
ROM_END

ROM_START( m4maxmzea )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "maximize_dr_v13p_514c.bin", 0x0000, 0x010000, CRC(3d483ae3) SHA1(0687912795a597254815b8f359965bdf2e887be1) )
ROM_END

ROM_START( m4maxmzeb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "maximize_mecca_v2_50cf.bin", 0x0000, 0x010000, CRC(88426c07) SHA1(2b026d75cbda9375cbfb62f19c25f31c658c56bc) )
ROM_END

ROM_START( m4maxmzec )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "maximize_v13_7107.bin", 0x0000, 0x010000, CRC(ed52f047) SHA1(d6b62934a92dcd6d4df3ac9ae1d9d143000e7c92) )
ROM_END

ROM_START( m4maxmzed )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "maximize_v13p_710b.bin", 0x0000, 0x010000, CRC(1e71a801) SHA1(4cd213e422c91fa10dceea0a93fd81576b1c3f7f) )
ROM_END



ROM_START( m4mayhem )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "may4_0.bin", 0x0000, 0x010000, CRC(fbbe89eb) SHA1(e5e2e4adabfa41d130dbb7a77c147105ef20ac79) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "mayhemsnd.p1", 0x000000, 0x080000, CRC(f61650ec) SHA1(946fd801ea5f4dd09a911460a709f3942fa412af) )
	ROM_LOAD( "mayhemsnd.p2", 0x080000, 0x080000, CRC(637a6b41) SHA1(d342309b78af21a35f50fb23dd2c7ed737abfdb9) )
ROM_END

ROM_START( m4mayhema )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "mayhemv2.bin", 0x0000, 0x010000, CRC(e7c79dd0) SHA1(b7af1fd4853a6ff33d2f3960737caece91714681) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "mayhemsnd.p1", 0x000000, 0x080000, CRC(f61650ec) SHA1(946fd801ea5f4dd09a911460a709f3942fa412af) )
	ROM_LOAD( "mayhemsnd.p2", 0x080000, 0x080000, CRC(637a6b41) SHA1(d342309b78af21a35f50fb23dd2c7ed737abfdb9) )
ROM_END


ROM_START( m4mecca )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "mecca_money_dr_v4_ef11.bin", 0x0000, 0x010000, CRC(2429ee8b) SHA1(785304c687a658706b26080f6b8f4bc65830dcea) )
ROM_END

ROM_START( m4themob )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "mob1_5.bin", 0x0000, 0x010000, CRC(adbdbf43) SHA1(650d9af466a258d55d9f6703968501a6eebddfef) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "mobsnd1.bin", 0x000000, 0x080000, CRC(b15c0eb4) SHA1(5d06853a1218f10741ea43bd72a48a727b62ddb1) )
	ROM_LOAD( "mobsnd2.bin", 0x080000, 0x080000, CRC(6ef3b72c) SHA1(47bf1edacd9da7249e19342234985c746ebf1c4b) )
	ROM_LOAD( "mobsnd3.bin", 0x100000, 0x080000, CRC(62a2bf65) SHA1(155536dc29eecc34351c5726d26f02ee8cb0d014) )
ROM_END

ROM_START( m4themoba )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "mob1_5d.bin", 0x0000, 0x010000, CRC(2a1d9a20) SHA1(1cd07edcf75f17a98b6377fbfdf88fd7c0abd864) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "mobsnd1.bin", 0x000000, 0x080000, CRC(b15c0eb4) SHA1(5d06853a1218f10741ea43bd72a48a727b62ddb1) )
	ROM_LOAD( "mobsnd2.bin", 0x080000, 0x080000, CRC(6ef3b72c) SHA1(47bf1edacd9da7249e19342234985c746ebf1c4b) )
	ROM_LOAD( "mobsnd3.bin", 0x100000, 0x080000, CRC(62a2bf65) SHA1(155536dc29eecc34351c5726d26f02ee8cb0d014) )
ROM_END


ROM_START( m4themobb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "mob1_6.bin", 0x0000, 0x010000, CRC(9c718009) SHA1(99b259d5a93f4657ad2b4ae6cd0b2e1324178022) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "mobsnd1.bin", 0x000000, 0x080000, CRC(b15c0eb4) SHA1(5d06853a1218f10741ea43bd72a48a727b62ddb1) )
	ROM_LOAD( "mobsnd2.bin", 0x080000, 0x080000, CRC(6ef3b72c) SHA1(47bf1edacd9da7249e19342234985c746ebf1c4b) )
	ROM_LOAD( "mobsnd3.bin", 0x100000, 0x080000, CRC(62a2bf65) SHA1(155536dc29eecc34351c5726d26f02ee8cb0d014) )
ROM_END



ROM_START( m4monspn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ms2_0.bin", 0x0000, 0x010000, CRC(c20172a8) SHA1(0fb97258dd33fa7ff83b8082149069aaf3577480) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "mssnd1.bin", 0x000000, 0x080000, CRC(e8f0e818) SHA1(a874981fc980a0ed49352f5bf89caf80176b3865) )
	ROM_LOAD( "mssnd2.bin", 0x080000, 0x080000, CRC(636c329d) SHA1(27503035ea57c7e03a9a07dfc58da997c47dda34) )
ROM_END

ROM_START( m4monspna )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ms2_0i.bin", 0x0000, 0x010000, CRC(45a157cb) SHA1(619e05bc0ee01bdb3254269619f761b513d77ee8) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "mssnd1.bin", 0x000000, 0x080000, CRC(e8f0e818) SHA1(a874981fc980a0ed49352f5bf89caf80176b3865) )
	ROM_LOAD( "mssnd2.bin", 0x080000, 0x080000, CRC(636c329d) SHA1(27503035ea57c7e03a9a07dfc58da997c47dda34) )
ROM_END

ROM_START( m4monspnb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ms2_0x.bin", 0x0000, 0x010000, CRC(e9e40e01) SHA1(487c11c03bfa582424b680d204417eb5e85abfb4) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "mssnd1.bin", 0x000000, 0x080000, CRC(e8f0e818) SHA1(a874981fc980a0ed49352f5bf89caf80176b3865) )
	ROM_LOAD( "mssnd2.bin", 0x080000, 0x080000, CRC(636c329d) SHA1(27503035ea57c7e03a9a07dfc58da997c47dda34) )
ROM_END

ROM_START( m4nudbon )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "nb4_0.bin", 0x0000, 0x010000, CRC(c7802b0f) SHA1(6ebff8ccbd2baf6de55d764d2e7cb34f2ff2384f) )

// something else? or sound related?
//  ROM_LOAD( "nubnzdl1.bin", 0x0000, 0x002000, CRC(bae682d2) SHA1(650082af9210b0af8b08870a4cdf4196035ea8a5) )
//  ROM_LOAD( "nubnzdl2.bin", 0x0000, 0x002000, CRC(ff150af7) SHA1(02e25200560e8435ebbf19c3ae9c3e9cf00342c1) )
//  ROM_LOAD( "nubnzdl3.bin", 0x0000, 0x004000, CRC(450d7fc9) SHA1(f82acb017e765f7188a874dade6fd1a5d6b2033e) )
ROM_END

ROM_START( m4nudbona )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "nubonza.bin", 0x0000, 0x010000, CRC(ce8b1c9b) SHA1(3f4e019256ddbd668c8cadecf86015b78b5eaf8c) )
ROM_END



ROM_START( m4nudgem )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "gem2_0.bin", 0x0000, 0x010000, CRC(bbab66b8) SHA1(a3a7d40d0ca41e57cd0d6965c0306edca372da1d) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "nudgejems.p1", 0x0000, 0x080000, CRC(e875d82e) SHA1(50fb941ad801397ef3dee651be126c01c9423386) )
ROM_END



ROM_START( m4pbnudg )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pinball nudger v0-1", 0x0000, 0x010000, CRC(8d2e5ded) SHA1(51d4ea44d4e8a7bd53f321fd677b12f6bafbc721) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "pinsnd.bin", 0x0000, 0x080000, CRC(30f61dcb) SHA1(c844272ffc264d6dabe1958ef57d10d1ba0c2b1e) )
ROM_END

ROM_START( m4pbnudga )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pn0_2.bin", 0x0000, 0x010000, CRC(d2aab1e0) SHA1(203257c50f79df46561ece5116277a8d56552b04) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "pinsnd.bin", 0x0000, 0x080000, CRC(30f61dcb) SHA1(c844272ffc264d6dabe1958ef57d10d1ba0c2b1e) )
ROM_END

ROM_START( m4pbnudgb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pn1_0.bin", 0x0000, 0x010000, CRC(95dabff1) SHA1(846577f76c6c99cb05f3aab88de80c3373e570c0) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "pinsnd.bin", 0x0000, 0x080000, CRC(30f61dcb) SHA1(c844272ffc264d6dabe1958ef57d10d1ba0c2b1e) )
ROM_END


ROM_START( m4pitfal )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "pf1_3.bin", 0x0000, 0x020000, CRC(5bdacadf) SHA1(3f48faf92ef25ecbb20f6af90adf5bffdaf8bad8) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "pfsnd1.bin", 0x000000, 0x080000, CRC(50ad158e) SHA1(0efcb9f5683cbe5bdec1e13791d8a01cdfcb5f1a) )
	ROM_LOAD( "pfsnd2.bin", 0x080000, 0x080000, CRC(80ad0d94) SHA1(2c1c60b681cc80624f8bd120034639f1cce06cc5) )
	ROM_LOAD( "pfsnd3.bin", 0x100000, 0x080000, CRC(abc0a0da) SHA1(3048edf44a31d58794a3ee1dad1399559bf14211) )
ROM_END

ROM_START( m4pitfala )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "pf1_4x.bin", 0x0000, 0x020000, CRC(42cd08bc) SHA1(69a5a5158d78c51e188520dbb16a0a89566ea8b3) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "pfsnd1.bin", 0x000000, 0x080000, CRC(50ad158e) SHA1(0efcb9f5683cbe5bdec1e13791d8a01cdfcb5f1a) )
	ROM_LOAD( "pfsnd2.bin", 0x080000, 0x080000, CRC(80ad0d94) SHA1(2c1c60b681cc80624f8bd120034639f1cce06cc5) )
	ROM_LOAD( "pfsnd3.bin", 0x100000, 0x080000, CRC(abc0a0da) SHA1(3048edf44a31d58794a3ee1dad1399559bf14211) )
ROM_END

ROM_START( m4pitfalb )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "pf2_0.bin", 0x0000, 0x020000, CRC(159f1ac6) SHA1(5af44ac650b9408afedc7533c2b3e558a84eb727) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "pfsnd1.bin", 0x000000, 0x080000, CRC(50ad158e) SHA1(0efcb9f5683cbe5bdec1e13791d8a01cdfcb5f1a) )
	ROM_LOAD( "pfsnd2.bin", 0x080000, 0x080000, CRC(80ad0d94) SHA1(2c1c60b681cc80624f8bd120034639f1cce06cc5) )
	ROM_LOAD( "pfsnd3.bin", 0x100000, 0x080000, CRC(abc0a0da) SHA1(3048edf44a31d58794a3ee1dad1399559bf14211) )
ROM_END

ROM_START( m4pitfalc )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "pf2_0x.bin", 0x0000, 0x020000, CRC(ba59a3f9) SHA1(1e2f21c67e8ca41a1cb8e9c412cf911b62511e05) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "pfsnd1.bin", 0x000000, 0x080000, CRC(50ad158e) SHA1(0efcb9f5683cbe5bdec1e13791d8a01cdfcb5f1a) )
	ROM_LOAD( "pfsnd2.bin", 0x080000, 0x080000, CRC(80ad0d94) SHA1(2c1c60b681cc80624f8bd120034639f1cce06cc5) )
	ROM_LOAD( "pfsnd3.bin", 0x100000, 0x080000, CRC(abc0a0da) SHA1(3048edf44a31d58794a3ee1dad1399559bf14211) )
ROM_END



ROM_START( m4purmad )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pmad.p1", 0x0000, 0x010000, CRC(e430510b) SHA1(bb8d443429aa7d39a99de5cd1387154398b74d9c) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "pmadsnd.bin", 0x0000, 0x080000, CRC(88312507) SHA1(64e386c3c9b3f82a390777b61c7207567cc962e7) )
ROM_END


ROM_START( m4revolv )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "revolva.bin", 0x0000, 0x010000, CRC(e195feac) SHA1(dc8d736784819fd15f0e7e29e9f91cf1c601ebb9) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "revolvasnd.p1", 0x000000, 0x080000, CRC(a9138bdf) SHA1(aaa5b3e82a8d89776b636e46ada9aae8a4febaab) )
	ROM_LOAD( "revolvasnd.p2", 0x080000, 0x080000, CRC(5ae7b26a) SHA1(a17cef58e37bccb5954b9acab01f92ff21375ab1) )
ROM_END


ROM_START( m4rckrol )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rock0_1.hex", 0x0000, 0x010000, CRC(2c786a34) SHA1(3800af04550f12e8f58b1929b49e21572f19d589) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "rocksnd.bin", 0x0000, 0x080000, CRC(c3e96650) SHA1(71952267d3149786cfef1dd49cc070664bb007a4) )
ROM_END

ROM_START( m4rckrola )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rock1.hex", 0x0000, 0x010000, CRC(b7dfd181) SHA1(566531bcb9d0b7d3caebed3f7c96f5fb23a7cef2) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "rocksnd.bin", 0x0000, 0x080000, CRC(c3e96650) SHA1(71952267d3149786cfef1dd49cc070664bb007a4) )
ROM_END

ROM_START( m4rckrolb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rock3.bin", 0x0000, 0x010000, CRC(c5134d03) SHA1(c4dfa43ffd077690d0b9a20cb82bbf92a1dc1ac9) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "rocksnd.bin", 0x0000, 0x080000, CRC(c3e96650) SHA1(71952267d3149786cfef1dd49cc070664bb007a4) )
ROM_END







ROM_START( m4rotex )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rotex1_2.bin", 0x0000, 0x010000, CRC(202e794d) SHA1(15b7459db7f3a5317a92138d997e9817d4367750) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "rotsnd.bin", 0x0000, 0x080000, CRC(02f8c2e2) SHA1(33a051e7af6d8c33708f7b4e0654d66312eeede5) )
ROM_END




ROM_START( m4select )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "selectawp.bin", 0x0000, 0x010000, CRC(e248539c) SHA1(b1db8fdc221c70d2a699cb86cea5681527d7d06a) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "selectsnd.bin", 0x0000, 0x080000, CRC(93fd4253) SHA1(69feda7ffc56defd515c9cd1ce204af3d9731a3f) )
ROM_END



ROM_START( m4smshgb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sm2_0x.bin", 0x0000, 0x010000, CRC(52042750) SHA1(2fb5ece50aef457bdbdc1fb880b1a87638551545) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sngsnd.p1", 0x000000, 0x080000, CRC(0bff940e) SHA1(e53df95cd33d759f89f0278312e6e5f9b8abe341) )
	ROM_LOAD( "sngsnd.p2", 0x080000, 0x080000, CRC(414aa5a5) SHA1(c6e6bba8c4655dc761dd040c4d615f9d4a739663) )
ROM_END

ROM_START( m4smshgba )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sm3_0x.bin", 0x0000, 0x010000, CRC(9a4bb2bd) SHA1(8642b15f658e855c0d682fe84b024ddd85eb527e) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sngsnd.p1", 0x000000, 0x080000, CRC(0bff940e) SHA1(e53df95cd33d759f89f0278312e6e5f9b8abe341) )
	ROM_LOAD( "sngsnd.p2", 0x080000, 0x080000, CRC(414aa5a5) SHA1(c6e6bba8c4655dc761dd040c4d615f9d4a739663) )
ROM_END

ROM_START( m4smshgbb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sm6_0.bin", 0x0000, 0x010000, CRC(7a58d60f) SHA1(989c816dadf01500be01e2c333befef0c3e12054) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sngsnd.p1", 0x000000, 0x080000, CRC(0bff940e) SHA1(e53df95cd33d759f89f0278312e6e5f9b8abe341) )
	ROM_LOAD( "sngsnd.p2", 0x080000, 0x080000, CRC(414aa5a5) SHA1(c6e6bba8c4655dc761dd040c4d615f9d4a739663) )
ROM_END

ROM_START( m4smshgbc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sma5_8c", 0x0000, 0x010000, CRC(79e12ac0) SHA1(9e8d4ea8f97d1f73ccab079cbf57aa89a12d2e7d) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "sngsnd.p1", 0x000000, 0x080000, CRC(0bff940e) SHA1(e53df95cd33d759f89f0278312e6e5f9b8abe341) )
	ROM_LOAD( "sngsnd.p2", 0x080000, 0x080000, CRC(414aa5a5) SHA1(c6e6bba8c4655dc761dd040c4d615f9d4a739663) )
ROM_END



ROM_START( m4snklad )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "snakesnladders.bin", 0x0000, 0x010000, CRC(52bdc684) SHA1(00d052629b214b48a9a1d68622ba188206276166) )
ROM_END





ROM_START( m4snookr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "snooker.ts2", 0x8000, 0x004000, CRC(a6906eb3) SHA1(43b91e88f909b758f880d83df4f889f15aa17eb3) )
	ROM_LOAD( "snooker.ts1", 0xc000, 0x004000, CRC(3e3072dd) SHA1(9ea8b270044b48767a2e6c19e8ed257d5491c1d0) )
ROM_END


ROM_START( m4spnwin )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "saw.bin", 0x0000, 0x010000, CRC(f8aac65f) SHA1(2cf8402bffe1638bddc0c2dd145d7be3cc7bd02b) )
ROM_END



ROM_START( m4stakex )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "stakex.bin", 0x0000, 0x010000, CRC(098c7117) SHA1(27f04cfb88ef870fc30afd055cf32ffe448275ea) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "stakexsnd.bin", 0x0000, 0x080000, CRC(baf17991) SHA1(282e0ac9d18299e9f7a0fecaf9edf0cb4205ef0e) )
ROM_END

ROM_START( m4stakexa )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "stakex2.bin", 0x0000, 0x010000, CRC(77ae3f63) SHA1(c5f1cfd5bffcf3156f584757de57ef6530214511) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "stakexsnd.bin", 0x0000, 0x080000, CRC(baf17991) SHA1(282e0ac9d18299e9f7a0fecaf9edf0cb4205ef0e) )
ROM_END


ROM_START( m4stand2 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "stand 2 del 8.bin", 0x08000, 0x08000, CRC(a9a5edc7) SHA1(035d3f3b3373cec475753f1b0de2f4db48d6d288) )
ROM_END



ROM_START( m4supfru )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "supa11l.bin", 0x0000, 0x010000, CRC(6cdbf08a) SHA1(4c0faf7144b9ac19c8d55b81ce51b519570b0d1f) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "supafruitssnd.bin", 0x0000, 0x080000, CRC(b70184ca) SHA1(dba2204cb606f0c6dad8a4c46fbbb1beb5b5e31c) )
ROM_END


ROM_START( m4supfrua )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "supafruits.bin", 0x0000, 0x010000, CRC(c6242e33) SHA1(810be1fb99ddb810c5506a974c9214ce23426a87) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "supafruitssnd.bin", 0x0000, 0x080000, CRC(b70184ca) SHA1(dba2204cb606f0c6dad8a4c46fbbb1beb5b5e31c) )
ROM_END

ROM_START( m4sstrek )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rhr2pprgpatched.bin", 0x0000, 0x010000, CRC(a0b3439d) SHA1(0976537a5170bf4c4f595f7fa04243a68f14b2ae) )
ROM_END

ROM_START( m4ttrail )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tt3_0.bin", 0x0000, 0x010000, CRC(62f31f70) SHA1(e1b50b98cc90513c9fa06d0ea8f70aa45bddc0e6) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "ttsnd1.bin", 0x000000, 0x080000, CRC(6af0c76f) SHA1(8587b499b88b609e48553e610a0ee539f98b70ce) )
	ROM_LOAD( "ttsnd2.bin", 0x080000, 0x080000, CRC(9f243ed1) SHA1(c4b83a9b788e4fa2065ff7a270f0dcdecb125e66) )
ROM_END

ROM_START( m4ttraila )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tt3_0i.bin", 0x0000, 0x010000, CRC(e5533a13) SHA1(0d23503d32c8156112676aaddece1a44614230eb) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "ttsnd1.bin", 0x000000, 0x080000, CRC(6af0c76f) SHA1(8587b499b88b609e48553e610a0ee539f98b70ce) )
	ROM_LOAD( "ttsnd2.bin", 0x080000, 0x080000, CRC(9f243ed1) SHA1(c4b83a9b788e4fa2065ff7a270f0dcdecb125e66) )
ROM_END

ROM_START( m4ttrailb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tt3_0x.bin", 0x0000, 0x010000, CRC(7d00e4ae) SHA1(6bb30af001fc73e354c17a99633b6fa4c50b374d) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "ttsnd1.bin", 0x000000, 0x080000, CRC(6af0c76f) SHA1(8587b499b88b609e48553e610a0ee539f98b70ce) )
	ROM_LOAD( "ttsnd2.bin", 0x080000, 0x080000, CRC(9f243ed1) SHA1(c4b83a9b788e4fa2065ff7a270f0dcdecb125e66) )
ROM_END

ROM_START( m4trimad )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tmad1_1p", 0x000000, 0x010000, CRC(7a49546b) SHA1(cfce6fda74682e6c60a5731fee44c41c5f5bbbeb) )

	ROM_REGION( 0x180000, "msm6376", 0 )
	ROM_LOAD( "triple.s1", 0x000000, 0x080000, CRC(e808f0ae) SHA1(4d88c1d8ed9396629509a0c7614e7510401f1325) )
	ROM_LOAD( "triple.s2", 0x080000, 0x080000, CRC(40161063) SHA1(a24edb311fea466c0c15aebce40044f8db448e50) )
	ROM_LOAD( "triple.s3", 0x100000, 0x080000, CRC(f9109afb) SHA1(4e4a863b60915ddb2865c12af19cc38bcad6d062) )
ROM_END


ROM_START( m4crzbn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "crazybingo.bin", 0x0000, 0x010000, CRC(51b3856c) SHA1(ed04771c69de090f4895920c213ba30754fcc1f9) )

	ROM_REGION( 0x80000, "msm6376", 0 )
	ROM_LOAD( "crazybingosnd.bin", 0x0000, 0x080000, CRC(6a60c412) SHA1(4f038675646510372022145033884ca704d31033) )
ROM_END


ROM_START( m4unibox )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "unibox.bin", 0x0000, 0x010000, CRC(9e99eed4) SHA1(c59f9ab2cae487991b0202bd18c6ab514ba3b29d) )

	ROM_REGION( 0x80000, "msm6376", 0 )
	ROM_LOAD( "uniboxsnd.bin", 0x0000, 0x080000, CRC(cce8fda1) SHA1(e9d6514dc2badb046201ea44802690cf104e6075) )
ROM_END

ROM_START( m4uniboxa )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "unibox1.bin", 0x0000, 0x010000, CRC(dd8fa3bd) SHA1(34719608e429e6ef0cc0ea9a75df7b00439e53ed) )

	ROM_REGION( 0x80000, "msm6376", 0 )
	ROM_LOAD( "uniboxsnd.bin", 0x0000, 0x080000, CRC(cce8fda1) SHA1(e9d6514dc2badb046201ea44802690cf104e6075) )
ROM_END

ROM_START( m4unique )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "unique.bin", 0x000000, 0x010000, CRC(ac8c83da) SHA1(11912c1b9a028e17db2395ca85792c4d10fae3ad) )

	ROM_REGION( 0x80000, "msm6376", 0 )
	ROM_LOAD( "uniquesnd.bin", 0x000000, 0x080000, CRC(177a4c07) SHA1(180f51ba982ccf7d19dfa50e94c295395799c360) )
ROM_END

ROM_START( m4uniquep )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "uniquepatched64k.bin", 0x000000, 0x010000, CRC(889575a2) SHA1(dcd0830d75cb4ddc901f22d71a28836e55d969cc) )

	ROM_REGION( 0x80000, "msm6376", 0 )
	ROM_LOAD( "uniquesnd.bin", 0x000000, 0x080000, CRC(177a4c07) SHA1(180f51ba982ccf7d19dfa50e94c295395799c360) )
ROM_END




ROM_START( m4cshino )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cashino.bin", 0x0000, 0x010000, CRC(0b269457) SHA1(835d28bf361fe9f1410a85f30aa919894c8d9a8e) )
ROM_END


ROM_START( m4exlin )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "extra lines 10p 4.80-2.40 16.3.90.bin", 0x8000, 0x008000, CRC(5e9b81e9) SHA1(84d446542533495b28692ba5f12cfd4fa5deabdd) )
	ROM_LOAD( "extra lines 2p 2.40 21.3.90.bin", 0x8000, 0x008000, CRC(b9736cd1) SHA1(c275e5c4ea550a6814b2b2143e4b84198739524c) )
ROM_END

ROM_START( m4jjc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "jjc.bin", 0x0000, 0x010000, CRC(781095ce) SHA1(f6beee15b45abf4a4f550b527e512bc6a1b603d8) )
	ROM_LOAD( "jjcash4pndv6.bin", 0x0000, 0x010000, CRC(3f28e657) SHA1(2382b96eb6f2b0580464c53a5122a48e25b4c002) )
ROM_END

ROM_START( m4spton )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "spot_on_v8 2_5_10_20p.bin", 0x0000, 0x010000, CRC(d0e27431) SHA1(cbcf637f2c8cf349e6386a54343d35da8aa24186) )
ROM_END

ROM_START( m4supjst )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "super jester 16.2.90 2p 1.bin", 0x8000, 0x008000, CRC(a55dae4f) SHA1(c426a202c312e0aefc5718c844f70b0af9b1c724) )
	ROM_LOAD( "super jester 19.2.90 5p 2.40.bin", 0x8000, 0x008000, CRC(754d821a) SHA1(9b00d0e028e46f8dfed60f3b49ef7c5cc40ac70c) )
	ROM_LOAD( "super jester 2.1.90 10p 4.80 p1.bin", 0x8000, 0x008000, CRC(14f2dc51) SHA1(7dcf54a25bcaa2081f906d0ad44bcae183a32371) )
	ROM_LOAD( "super jester 20p.bin", 0x8000, 0x008000, CRC(be725101) SHA1(b2352e56bec5c8929075697f29526d25626cbc38) )
	ROM_LOAD( "super jester 4.10.89 10p 4 cash.bin", 0x8000, 0x008000, CRC(ca1797b2) SHA1(afd6b365e7bef721084dc33442bb685fef6f7b76) )
	ROM_LOAD( "super jester 4.10.89 10p 4 token.bin", 0x8000, 0x008000, CRC(3b4e4444) SHA1(a3cbbd2657be087c346a5ada7c301584125f8fbc) )
	ROM_LOAD( "superjester", 0x8000, 0x008000, CRC(754d821a) SHA1(9b00d0e028e46f8dfed60f3b49ef7c5cc40ac70c) )
	ROM_LOAD( "31.1.90cj10p4.80v6.bin", 0x8000, 0x008000, CRC(14f2dc51) SHA1(7dcf54a25bcaa2081f906d0ad44bcae183a32371) ) // marked crown jester?
ROM_END



ROM_START( m4bigban )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "big04.p1", 0x0000, 0x020000, CRC(f7ead9c6) SHA1(46c10abb892cb6d427ad508aae96752c14b4cb83) )
	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* Missing? */
ROM_END

ROM_START( m4crzcsn )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "crz03.bin", 0x0000, 0x020000, CRC(48610c4f) SHA1(a62ac8b3ee704ee4e98f9d56bfc723d4cbb25b54) )
	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* Missing? */
ROM_END

ROM_START( m4crzcav )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "gcv05.p1", 0x0000, 0x020000, CRC(b9ba46f6) SHA1(78b745d85b36444c39747982987088a772b20a7e) )
	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* Missing? */
ROM_END

ROM_START( m4dragon )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "dgl01.p1", 0x0000, 0x020000, CRC(d7d39c9b) SHA1(5350c9db549edee30815516b1ce74a018390ff3d) )
	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* Missing? */
ROM_END

ROM_START( m4hilonv )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "hnc02.p1", 0x0000, 0x020000, CRC(33a8022b) SHA1(5168b8f32630aa2cb56f30c941695f1728e4fb7a) )
	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* Missing? */
ROM_END

ROM_START( m4octo )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "oct03.p1", 0x0000, 0x020000, CRC(8df66e94) SHA1(e1ab93982846d83becae36b5814ebbd515b9078e) )
	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* Missing? */
ROM_END

ROM_START( m4sctagt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "gse3_0.p1", 0x0000, 0x010000, CRC(eff705ff) SHA1(6bf96872ef4bcc8f8041c5384d892f072c72be2b) )
	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* Missing? */
ROM_END










ROM_START( m4cld02 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cru0_2.bin", 0x0000, 0x010000, CRC(e3c01944) SHA1(33a2b2c05686f53811349b2980e590fdc4b72756) )
ROM_END

ROM_START( m4barcrz )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "barcrazy.bin", 0x0000, 0x010000, CRC(917ad749) SHA1(cb0a3f6737b8f183d2efb0a3f8adbf86d40a38ff) )

	ROM_REGION( 0x080000, "msm6376", 0 )
	ROM_LOAD( "barcrazysnd.bin", 0x0000, 0x080000, CRC(0e155193) SHA1(7583e9f3e3624f82f2329565bdcbdaa5a5b03ee0) )
ROM_END

ROM_START( m4bonzbn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bingo-bonanza_v1.bin", 0x0000, 0x010000, CRC(3d137ddf) SHA1(1ce23db111448e44a166554dd8853dc379e787da) )

	ROM_REGION( 0x100000, "msm6376", 0 )
	ROM_LOAD( "bingo-bonanzasnd1.bin", 0x000000, 0x080000, CRC(e0eb2a92) SHA1(cbc0b3bba7857d87535d1c2a7459aed60709734a) )
	ROM_LOAD( "bingo-bonanzasnd2.bin", 0x080000, 0x080000, CRC(7db27b28) SHA1(98c5fa4bf8c7f67fae90a1ca98b74057f5ed9b6b) )
ROM_END

ROM_START( m4dnj )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "d.n.j 1-02", 0x0000, 0x010000, CRC(5750843d) SHA1(b87923e84071ea4a1af7566a7f413f8e30e208e9) )
	ROM_LOAD( "d.n.j 1-03", 0x0000, 0x010000, CRC(7b805255) SHA1(f62765bfa66e2422ac0a71ebaff27f1ccd470fe2) )
	ROM_LOAD( "d.n.j 1-06", 0x0000, 0x010000, CRC(aab770c7) SHA1(f24fff8346915017bc43fef9fac356a067676d86) )
ROM_END


ROM_START( m4matdr )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "matador.bin", 0x0000, 0x020000, CRC(367788a4) SHA1(3c9b077a64f993cb60107558efdfcbee0fe5c958) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing */
ROM_END


ROM_START( m4ttak )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tictacp1.bin", 0x8000, 0x008000, CRC(7ae5d93c) SHA1(8066341a267be357b67b1c7e315989e9bda99856) )
	ROM_LOAD( "tictacp2.bin", 0x4000, 0x004000, CRC(33f3d2d1) SHA1(fea9c5766bfb3fb49d88a76458450c15bf38b2b1) )
ROM_END

ROM_START( m4hslo )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "hot30", 0x0000, 0x010000, CRC(62f2c420) SHA1(5ae89a1b585738255e8d9ae153c3c63b4a2893e4) )
ROM_END

ROM_START( m4sbx )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sbx-2.1-cash.bin", 0x8000, 0x008000, CRC(2dca703e) SHA1(aef398f4ed38ba34f28009058c9486a570f64e0f) )

	ROM_REGION( 0x10000, "altrevs", 0 )
	ROM_LOAD( "b_sbx23.bin", 0x8000, 0x008000, CRC(8188e94f) SHA1(dfbfc549d12c8f7c7db6c12ba766c28f1cf0873f) )
	ROM_LOAD( "s bears v1-4 20p po.bin", 0x8000, 0x008000, CRC(03486714) SHA1(91c237956bbec58cc08a3e92543488d8e2daa673) )
	ROM_LOAD( "s bears v2-4 10p 8.bin", 0x8000, 0x008000, CRC(9b94f8d0) SHA1(9808386def14c8a058730e90135a4d6506e6ed3d) )
	ROM_LOAD( "s bears v2-4 20p po.bin", 0x8000, 0x008000, CRC(ad8f8d9d) SHA1(abd808f95b587a84e8b3aad1af9fe1cb613c9821) )
	ROM_LOAD( "superbea.10p", 0x8000, 0x008000, CRC(70020466) SHA1(473c9feb9ce0024b870612af19ec8a47a7798506) )

	ROM_REGION( 0x40000, "upd", 0 ) // not oki at least...
	ROM_LOAD( "sbsnd", 0x0000, 0x040000, CRC(27fd9fe6) SHA1(856fdc95a833affde0ada7041c68a4b6b729b715) )
ROM_END

ROM_START( m4bclimb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bc8pv4.bin", 0x8000, 0x008000, CRC(229a7607) SHA1(b20b2c9f9d19ccd6146affdf519fa4bc0322c971) )

	ROM_REGION( 0x40000, "upd", 0 ) // not oki at least...
	ROM_LOAD( "sbsnd", 0x0000, 0x040000, CRC(27fd9fe6) SHA1(856fdc95a833affde0ada7041c68a4b6b729b715) )
ROM_END

ROM_START( m4captb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "c_bear21.rom", 0x8000, 0x008000, CRC(2e9a42e9) SHA1(0c3f33311f1543daf2ff5c0443dc8c000d49c26d) )

	ROM_REGION( 0x40000, "upd", ROMREGION_ERASE00 ) // not oki at least...
//  ROM_LOAD( "sbsnd", 0x0000, 0x040000, CRC(27fd9fe6) SHA1(856fdc95a833affde0ada7041c68a4b6b729b715) )
ROM_END

ROM_START( m4jungj )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "jj2410p.bin", 0x8000, 0x008000, CRC(490838c6) SHA1(a1e9963df9a429ae594592312e977f22f96c6073) )

	ROM_REGION( 0x10000, "altrevs", 0 )
	ROM_LOAD( "jj2420p.bin", 0x8000, 0x008000, CRC(39329ccf) SHA1(6b79e4fc553bad935ec9989ad5ef3e186e720633) )
	ROM_LOAD( "jjv2_4p.bin", 0x8000, 0x008000, CRC(125a8138) SHA1(18c62df5b331bd09d6dcda6280351e94b7b816fd) )
	ROM_LOAD( "jjv4.bin", 0x8000, 0x008000, CRC(bf583156) SHA1(084c5ed3d96c92f265ad08cc7aed7fe6092217a5) )

	ROM_REGION( 0x40000, "upd", ROMREGION_ERASE00 ) // not oki at least...
ROM_END


ROM_START( m4fsx )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "funspotx.10p", 0x8000, 0x008000, CRC(55199f36) SHA1(7af376781e381582b06972725a2022cc28ba60b3) )

	ROM_REGION( 0x10000, "altrevs", 0 )
	ROM_LOAD( "funspotx.20p", 0x8000, 0x008000, CRC(08d1eb6e) SHA1(7c7c02d9c34696d75490df8596ffe64fba93dcc4) )
	ROM_LOAD( "b_fsv1.bin", 0x8000, 0x008000, CRC(b077f944) SHA1(97d96594b8d2d7232bad087cc55912dec02d7484) )

	ROM_REGION( 0x40000, "upd", ROMREGION_ERASE00 ) // not oki at least...
ROM_END


ROM_START( m4ccop )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cashcop9.bin", 0x0000, 0x010000, CRC(5f993207) SHA1(ab0614e6a1355d275158b1a32f65086e40c2f890) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "cash-cops_v4-de.bin", 0x0000, 0x010000, CRC(df3da824) SHA1(c275a33e4a89f1b9ecbae80cb7b62007b29b9fd2) )
	ROM_LOAD( "cashcop8.bin", 0x0000, 0x010000, CRC(165603df) SHA1(d301696a340ed136a43c5753c8bf73283a925fd7) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	// 3 different sets of samples(!)
	ROM_LOAD( "cash-copssnd1-de.bin", 0x000000, 0x080000, CRC(cd03f7f7) SHA1(4c09a86bcdf9a9eb224b19b932b75c9db3784fad) )
	ROM_LOAD( "cash-copssnd2-de.bin", 0x080000, 0x080000, CRC(107816a2) SHA1(f5d4a0390b85a665a3536da4689ec91b1a2da3ae) )

	ROM_LOAD( "cash-copssnd1.bin", 0x000000, 0x080000, CRC(776a303d) SHA1(a5a282674674f25bc6ca169eeebee7309239871f) )
	ROM_LOAD( "cash-copssnd2.bin", 0x080000, 0x080000, CRC(107816a2) SHA1(f5d4a0390b85a665a3536da4689ec91b1a2da3ae) )

	ROM_LOAD( "cashcops.p1", 0x000000, 0x080000, CRC(9a59a3a1) SHA1(72cfc99b22ec5fb89714c6d2d66760d86dc19f2f) )
	ROM_LOAD( "cashcops.p2", 0x080000, 0x080000, CRC(deb3e755) SHA1(01f92881c451919be549a1c58afa1fa4630bf171) )
ROM_END

ROM_START( m4ccc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ccc12.bin", 0x8000, 0x008000, CRC(570cc766) SHA1(036c95ff6428ab38cceb0537dcc990be78fb331a) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "criss cross crazy sound (27c2001)", 0x0000, 0x040000, CRC(1994c509) SHA1(2bbe91a43aa9953b7776faf67e81e30a4f7b7cb2) )
ROM_END


ROM_START( m4treel )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "trgv1.1s", 0x0000, 0x010000, CRC(a9c76b08) SHA1(a5b3bc980eb58e346cb02d8ca43401f304e5b6de) )

	ROM_REGION( 0x20000, "altrevs", 0 )
	ROM_LOAD( "trgv1.1b", 0x0000, 0x020000, CRC(7eaebef6) SHA1(5ab86329041e7df09cc2e3ce8d5afd44d88c246c) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
ROM_END



ROM_START( m4unkjok )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "joker 10p 3.bin", 0x0000, 0x010000, CRC(009823ac) SHA1(5ab25da5876c87a8d8701f84446bb3d377e4c1ca) )
	ROM_LOAD( "joker 10p 6.bin", 0x0000, 0x010000, CRC(f25f0704) SHA1(35298b49f79c5029277f4777fe88d5e4344c115f) )
	ROM_LOAD( "joker 20p 3 or 6.bin", 0x0000, 0x010000, CRC(cae4397e) SHA1(53b61fd41c97a6ed29ce6a7b555e061ecf2b0ae2) )
	ROM_LOAD( "joker new 20p 6 or 3.bin", 0x0000, 0x010000, CRC(b8d77b97) SHA1(54f69823bb3fd9c2cca014dc7c51913b2d6c8058) )
ROM_END

ROM_START( m4remag )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "remagv2", 0x0000, 0x010000, CRC(80d9c1c2) SHA1(c77d443d92084c324ef75575acca66ffbd9beef3) )
ROM_END

ROM_START( m4rmg )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rmgicdd", 0x0000, 0x010000, CRC(bd64be0d) SHA1(772b80619c7d514a7a253f35137896d6a73bf4c6) )
ROM_END

ROM_START( m4wnud )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "wnudge.bin", 0x8000, 0x008000, CRC(1d935575) SHA1(c4177c41473c0fb511e0ee035961f55ad43be14d) )
ROM_END

ROM_START( m4t266 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "t2 66.bin", 0x0000, 0x010000, CRC(5c99c6bb) SHA1(7b74e0e5207c00b31cb1859e0cc458c0412a1a07) )
ROM_END

ROM_START( m4brnze )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bv25", 0x0000, 0x010000, CRC(5c66f460) SHA1(c7587a6e992549ad8814f77c65b33a17a3641431) )
	ROM_LOAD( "bv25v2", 0x0000, 0x010000, CRC(a675edb3) SHA1(a3c6ee6a0bfb301fed72b45ee8e363d77b8b8dbb) )
	ROM_LOAD( "bv55", 0x0000, 0x010000, CRC(93905bc9) SHA1(e8d3cd125dced43fc2cf23cbccc59110561d2a40) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing */
ROM_END

ROM_START( m4riotrp )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "drt10.bin", 0x0000, 0x010000, CRC(a1badb8a) SHA1(871786ea4e65ecbf61c9a776100321253922d11e) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "dblcsnd.bin", 0x0000, 0x080000, CRC(c90fa8ad) SHA1(a98f03d4b6f5892333279bff7537d4d6d887da62) )
ROM_END

ROM_START( m4aladn )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00  )
	ROM_LOAD( "ei15ng.bin", 0x0000, 0x010000, CRC(c6142523) SHA1(b4101b14bbfa4e2b0b94c40b1ca17d484e3cf21d) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "ei15ngp.bin", 0x0000, 0x010000, CRC(d8c9fcf2) SHA1(11d3a8fc515348c72b827675991b8e990f04f8f0) )
	ROM_LOAD( "ei15og.bin", 0x0000, 0x010000, CRC(5bd857d5) SHA1(f673dd7676aa07cb99af9af3721067455a6af889) )
	ROM_LOAD( "ei15ogp.bin", 0x0000, 0x010000, CRC(45058e04) SHA1(1210502e23d6958860a4d5acd80ba5874d5979c2) )
	ROM_LOAD( "ei26cng.bin", 0x0000, 0x010000, CRC(5c65c8e4) SHA1(ec0cb3f306fdfc43b15fc15fa6e75dbe05d6851f) )
	ROM_LOAD( "ei26cngp.bin", 0x0000, 0x010000, CRC(42b81135) SHA1(1d1500153a1ef2a2f1282f91ee0f371039852768) )
	ROM_LOAD( "eiv35ng.bin", 0x0000, 0x010000, CRC(fc2fbedc) SHA1(af6840359e7721d035ada99839d5f80ed579f7df) )
	ROM_LOAD( "eiv35ogp.bin", 0x0000, 0x010000, CRC(e982cdef) SHA1(1b8d7f6bb7bcbbf80a90a8d54a27b12d9d8fa24e) )
	ROM_LOAD( "eiv36ngp.bin", 0x0000, 0x010000, CRC(e2f2670d) SHA1(c4b5a0c29ff5ca33a9db198c440f8e69b1e0175e) )
	ROM_LOAD( "eiv36og.bin", 0x0000, 0x010000, CRC(f75f143e) SHA1(416708ce11c10134fa63f36a53e211480939f453) )

	ROM_REGION( 0x80000, "upd", 0 )
	ROM_LOAD( "alladinscavesnd.bin", 0x0000, 0x080000, CRC(e3831190) SHA1(3dd0e8beafb628f5138a6943518b477095ac2e56) )
ROM_END

#define M4FRKSTN_SOUND \
	ROM_REGION( 0x40000, "upd", 0 ) \
	ROM_LOAD("fr1snd.bin",	0x00000, 0x40000, CRC(2d77bbde) SHA1(0397ede538e913dc2972e260589022564fcd8fe4) ) \


ROM_START( m4frkstn )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00  )
	ROM_LOAD( "fr1.536",  0x8000, 0x8000,  CRC(422b7209) SHA1(3c3f942d375a83d2470467651bca20f0feabdd3b))

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "db16.bin", 0x0000, 0x008000, CRC(9a36efae) SHA1(57d0df28198443502ff6d168d937a56f56dc00b2) )
	ROM_LOAD( "db35.bin", 0x0000, 0x008000, CRC(c576a072) SHA1(2a2f7c477ab5013b540777d9525554ddd7d3da06) )
	ROM_LOAD( "db35p.bin", 0x0000, 0x008000, CRC(eb929a5e) SHA1(892121a47af12a95d08cc8e4f01076c0169e367f) )
	ROM_LOAD( "db51.bin", 0x0000, 0x008000, CRC(d89c80cb) SHA1(88f5e39cef0448628b21a4f283008e8df2d66814) )
	ROM_LOAD( "db51p.bin", 0x0000, 0x008000, CRC(f678bae7) SHA1(f61778517f5a7a4dd3b0af4b76a3000b9874a116) )
	ROM_LOAD( "frav3", 0x0000, 0x008000, CRC(06b1f6e2) SHA1(4f8013f1e07c2c41e4bac7883294d65e473f8ace) )

	M4FRKSTN_SOUND
ROM_END

ROM_START( m4frkstna )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "frav1.6", 0x08000, 0x008000, CRC(c115aaff) SHA1(e87b41f21f13ca9e3c0b5345111607bee3995f61) )

	M4FRKSTN_SOUND
ROM_END

ROM_START( m4frkstnb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "frav1", 0x0000, 0x10000, CRC(ea6f1b01) SHA1(74bcd36846e352e6d7f97e3c6b2479fd2abd76db) )

	M4FRKSTN_SOUND
ROM_END



ROM_START( m4bagcsh )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cg27.bin", 0x8000, 0x008000, CRC(2e1ce880) SHA1(fcfbbba832ae7d9e79066b27305b3406207caefc) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "cg27p.bin", 0x8000, 0x008000, CRC(775384ac) SHA1(3cff28d2e5c01e84e3d01334cb0ac6e451664726) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4bucclb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "er16.bin", 0x0000, 0x010000, CRC(c57937fe) SHA1(f54a247c16fb143340fff5ef31bdc8bb9ff93be8) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "er16p.bin", 0x0000, 0x010000, CRC(d2d79dc1) SHA1(2141ce61f53a608270171c94659af1d932988707) )
	ROM_LOAD( "er14.bin", 0x0000, 0x010000, CRC(d54de33e) SHA1(975d2b5b6d97f6e943d49003da19e17e8d689b80) )
	ROM_LOAD( "er14p.bin", 0x0000, 0x010000, CRC(c2e34901) SHA1(5811e9b3d49e9a5fa32c764c00d10202aa89455a) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4bullio )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ef15.bin", 0x0000, 0x010000, CRC(8ed6bc1f) SHA1(1deff7a819db267897f14dc76be387c6f4bd55f0) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "ef15p.bin", 0x0000, 0x010000, CRC(900b65ce) SHA1(3747651e98ecd06fb55033cb83a6513d5d4ea032) )
	ROM_LOAD( "clbu-v16.bin", 0x0000, 0x010000, CRC(58d2fed8) SHA1(f9680b61505dea88ee62cc40dc6c375845d768dd) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4carou )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fa110.bin", 0x0000, 0x010000, CRC(ade2a7f8) SHA1(18b9287ab9747ee623e58afa2a4e6f517ff7a8ca) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "fa110p.bin", 0x0000, 0x010000, CRC(8606427d) SHA1(1a3a1ccfeb8229eaff0d9f627ed50a963c9765ed) )
	ROM_LOAD( "fa210.bin", 0x0000, 0x010000, CRC(a3bde0fb) SHA1(54963af8b5e6836be4c0fda4b3248cf1ea68acf2) )
	ROM_LOAD( "fa210p.bin", 0x0000, 0x010000, CRC(8859057e) SHA1(3082a8612a97d868b232c2fb3cd55ef9df166b3a) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "fa_sound.bin", 0x0000, 0x080000, CRC(39837e76) SHA1(74b66f77d9af47a5caab5b6441563b196fdadb37) )
ROM_END








ROM_START( m4cclimb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fu204.bin", 0x0000, 0x010000, CRC(64746a30) SHA1(cdb841270851655d16b42668c4f875d7dfbfd0f1) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "fu204p.bin", 0x0000, 0x010000, CRC(af0dd470) SHA1(d62bdd347130fa34c1f7f46ac9ddaaf6d035dd1c) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4crzcl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fu105.bin", 0x0000, 0x010000, CRC(d7ae1644) SHA1(d04c6f96c0f59c782a170bfabfbf670be28c9d3a) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "fu105f.bin", 0x0000, 0x010000, CRC(d33ea841) SHA1(8672fc875b82d7ca248bd5938fa80f540f5a3842) )
	ROM_LOAD( "fu105fp.bin", 0x0000, 0x010000, CRC(d31ed3f7) SHA1(c8e7916cc7faae3cbb2779b3856b65d03d702666) )
	ROM_LOAD( "fu105p.bin", 0x0000, 0x010000, CRC(dfff65c9) SHA1(c962e5c159301daae952bb7a5b3393aea206a4b1) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4crzclc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cc106.bin", 0x0000, 0x010000, CRC(ff9a5f7f) SHA1(672e8b0fc0d6e985d0862e5f474bc49e1bb0c56c) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4elitc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ed16.bin", 0x0000, 0x010000, CRC(b4216f45) SHA1(b9d8fa4471979f074f327ac6a261000fa929d349) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "ed16p.bin", 0x0000, 0x010000, CRC(3251c0b4) SHA1(30483a67a4c363fa154e3912b4f834b97d955cc2) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4fairg )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fairgroundcrysv2-16cash.bin", 0x0000, 0x010000, CRC(4a6c6470) SHA1(3211fb0245343d0fcf4581352faf606b7785f00c) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "fairgroundcrysv2-1snd.bin", 0x0000, 0x040000, CRC(9b09f98a) SHA1(e980bb0039f087ee563165a3aeb66e627fc3afe9) )
ROM_END








ROM_START( m4frmani )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fo23a.bin", 0x0000, 0x010000, CRC(9c7e5208) SHA1(eae08a464d0dd9c8b9ebf12eeb0c396ddd6c8779) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "fo23ap.bin", 0x0000, 0x010000, CRC(7965c24a) SHA1(8f5ddbf2902fefa23ed0a8b3d634d4d4f81cb1b6) )
	ROM_LOAD( "fo23s.bin", 0x0000, 0x010000, CRC(3cb65a7c) SHA1(6d242729cad68658a135364ddd967664760112c2) )
	ROM_LOAD( "fo23sp.bin", 0x0000, 0x010000, CRC(f3f24d6d) SHA1(251c5bac250fc5e854004f16858884f3905ab26e) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4goldxc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "gx105.b8", 0x0000, 0x010000, CRC(3a94ea0d) SHA1(e81f5edec6bca1d098d2c72e063b2c7456c99eda) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "gx105p.b8", 0x0000, 0x010000, CRC(0e63e76c) SHA1(3af9d635043b54950f56bc5e1da514e5a3683ca0) )
	ROM_LOAD( "gx107.b8", 0x0000, 0x010000, CRC(68d6c894) SHA1(8f69243ddbe7f49191843f1b3e03e41ecbe06d59) )
	ROM_LOAD( "gx107b.b8", 0x0000, 0x010000, CRC(a93a50f9) SHA1(957513761093351ba5f776e925568c2951eea6c0) )
	ROM_LOAD( "gx207.b8", 0x0000, 0x010000, CRC(f32e6821) SHA1(cdcafd7a18237df060144ffc1969b502bdf48e9a) )
	ROM_LOAD( "gx207b.b8", 0x0000, 0x010000, CRC(b91efda9) SHA1(cf8aa37cee983ebc73b6aa1606e7404d0f323d17) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4goldfc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "eo12a.bin", 0x0000, 0x010000, CRC(df519368) SHA1(10d0846ddc23eb49bcc9ba2eb9d779107e3b33f4) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "eo12ap.bin", 0x0000, 0x010000, CRC(c18c4ab9) SHA1(d2752fd09f46c2ad6b5ab0ee96aef9810483b5a4) )
	ROM_LOAD( "goldfevergamev1.1.bin", 0x0000, 0x010000, CRC(7e5f90e5) SHA1(7e4fc6d07e9b9ff0b4c6ef59d494438c2ee0c27b) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "goldfeversamplesound.bin", 0x0000, 0x080000, CRC(eb7d3c7b) SHA1(53b7c048e78506f0188b4dd2750c8dc31a625523) )
ROM_END







ROM_START( m4hirol )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fv105.bin", 0x0000, 0x010000, CRC(d12963a0) SHA1(c5b6f9475ae62f15c4ba9fa391bfb17ece658091) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "fv105p.bin", 0x0000, 0x010000, CRC(facd8625) SHA1(2ace151546a9f8f191713c798cd1f26f4c69ded2) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4kingqn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fi17.bin", 0x0000, 0x010000, CRC(c3ef9e0c) SHA1(8f3def3d5bb3e38df1fc5bec64ed47fc307856d9) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "fi17p.bin", 0x0000, 0x010000, CRC(f726850d) SHA1(feed467b209920c943ff55fffc5fbdcc175dfa49) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "kingsandqueenscrystalsnd.bin", 0x0000, 0x080000, CRC(93e4b644) SHA1(920579db52c5bb820437023e35707780ed503acc) )
ROM_END




ROM_START( m4lotclb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ds14.bin", 0x0000, 0x010000, CRC(a7e9969b) SHA1(daa1b38002c75cf4802078789955ae58d0cf163e) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "ds14p.bin", 0x0000, 0x010000, CRC(2199396a) SHA1(070bca2951261f83baf1a21bf740fd12d1d8daa5) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END







ROM_START( m4montrl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dt17.bin", 0x0000, 0x010000, CRC(18d0c6a0) SHA1(34b055a19f3c0ab975c68a273d8dfd4c326e2089) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "dt17p.bin", 0x0000, 0x010000, CRC(a45f8f82) SHA1(a1f070d34fa260d591a20f3786218c64995cafff) )
	ROM_LOAD( "dt36.bin", 0x0000, 0x010000, CRC(e294cb1b) SHA1(59267859d8db90ca2d7b5413f196f9ac6b9c0f23) )
	ROM_LOAD( "dt36p.bin", 0x0000, 0x010000, CRC(fc4912ca) SHA1(a99fec8b94adca1c372ed0f836f071ccfa1c2415) )
	ROM_LOAD( "moneytrail.bin", 0x0000, 0x010000, CRC(e5533728) SHA1(f1abd59d57a5eca42640992bc543990a3ee8058d) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "moneytrailsnd.bin", 0x0000, 0x040000, CRC(0f0d52dc) SHA1(79e1a89858f95006a1d2a0dd18d677c84a3087c6) )
ROM_END




ROM_START( m4mystiq )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fw103.p1", 0x0000, 0x010000, CRC(85234d9b) SHA1(06457892c6a0fafb826d3d8bc99f23c8b6c4374d) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "fw103-fp.p1", 0x0000, 0x010000, CRC(a5530d17) SHA1(d8585e0ca3ed66db272291561aa8baaab1dff66b) )
	ROM_LOAD( "fw103d.p1", 0x0000, 0x010000, CRC(b1ea569a) SHA1(fa82a25043162930d2424f5df29f71c2c330f666) )
	ROM_LOAD( "fw103fpd.p1", 0x0000, 0x010000, CRC(919a1616) SHA1(15e5db55ecdf1151090401c46a0379d339d9c615) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4nudwin )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dn56.bin", 0x8000, 0x008000, CRC(ca90f7a8) SHA1(1ae92162f02feb5f391617d4180ef1c154e10d1a) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "dn56p.bin", 0x8000, 0x008000, CRC(1640a1ef) SHA1(70d573746244166b358457710e6e2c9437ea2745) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4paracl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dl12.bin", 0x0000, 0x010000, CRC(6398aecf) SHA1(290a21a5b3a15643f657939bccf3d677f22a3ef4) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "dl12p.bin", 0x0000, 0x010000, CRC(e5e8013e) SHA1(8b6cb5aed1b82c8a298c9e798f89ab07ff330caf) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END




ROM_START( m4rlpick )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fh15a.bin", 0x0000, 0x010000, CRC(ac448a6e) SHA1(4a99f7b293476e3e477f37cbd28f1e2a99b0f2d2) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "fh15pa.bin", 0x0000, 0x010000, CRC(29f857b9) SHA1(0f8858bac5865c6c789be052cc037fbdbe87f135) )
	ROM_LOAD( "fh15ps.bin", 0x0000, 0x010000, CRC(b6ec1edf) SHA1(759f18332ac50452c9725ce1a3e53b56334bd365) )
	ROM_LOAD( "fh15s.bin", 0x0000, 0x010000, CRC(55b0a7e7) SHA1(d4568f82040de0fc6eb08fe90754d2b33e427ab0) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4twstr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fda103.bin", 0x0000, 0x010000, CRC(5520b91c) SHA1(0b219c9232e89c5a7da8f857aa58f828cc5730f4) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "fda103p.bin", 0x0000, 0x010000, CRC(61e9a21d) SHA1(940abedbbfaa40534c8be7136369cbf404e4c9a8) )
	ROM_LOAD( "fds103.bin", 0x0000, 0x010000, CRC(4282cba9) SHA1(a664d342482e50604f9a585a1887f342b95caf00) )
	ROM_LOAD( "fds103p.bin", 0x0000, 0x010000, CRC(764bd0a8) SHA1(cea826c94ce28ab3e47071846756a7e1effa5d1b) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END



ROM_START( m4twstcl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fl106.bin", 0x0000, 0x010000, CRC(d43f06a6) SHA1(d3762853dee2779a06a02ad3c1dfd804053d7f7d) )

	ROM_REGION( 0x10000, "altrevs", ROMREGION_ERASE00  )
	ROM_LOAD( "fl106fp.bin", 0x0000, 0x010000, CRC(6716baf7) SHA1(f4f60e1c798682aee17a74ddf376aca113b6b701) )
	ROM_LOAD( "fl106p.bin", 0x0000, 0x010000, CRC(93dd36e7) SHA1(eded1b5748cb24af80ab392a5fd4d2cc312a66a2) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	/* missing? */
ROM_END


ROM_START( m4dz )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dangerzone1_2.bin", 0x8000, 0x08000, CRC(eb4582f8) SHA1(df4cbbbb927b512b1ace34986ce29b17d7815e49) )

	ROM_REGION( 0x100000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "dangerzonesnd.bin", 0x0000, 0x080000, CRC(bdfcffa2) SHA1(9e3be8fd1c42fd19afcde682662bef82f7e0f7e9) )
ROM_END








ROM_START( m4surf )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "s_surfin._pound5", 0x0000, 0x020000, CRC(5f800636) SHA1(5b1789890eea44e5275e13f360876374d862935f) )

	ROM_REGION( 0x40000, "altrevs", 0 )
	ROM_LOAD( "s_surfin.upd", 0x0000, 0x020000, CRC(d0bef9cd) SHA1(9d53bfe8d928b190202bf747c0d7bb4cc0ae0efd) )
	ROM_LOAD( "s_surfin._pound15", 0x0000, 0x020000, CRC(eabce7fd) SHA1(4bb2bbcc7d2917eca72385a21ab85d2d94a882ec) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "s_surf.sn1", 0x000000, 0x080000, CRC(f20a7d69) SHA1(7887230613b497dc71a60125dd1e265ebbc8eb23) )
	ROM_LOAD( "s_surf.sn2", 0x080000, 0x080000, CRC(6c4a9074) SHA1(3b993120156677de893e5dc1e0c5d6e0285c5570) )
ROM_END

ROM_START( m4blkgd )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "blackgoldprg.bin", 0x0000, 0x080000, CRC(a04736b2) SHA1(9e060cc79e7922b38115f1412ed76f8c76deb917) )

	ROM_REGION( 0x40000, "altrevs", 0 )
	ROM_LOAD( "blackgoldversion2.4.bin", 0x0000, 0x040000, CRC(fad4e360) SHA1(23c6a13e8d1ca307b0ef22edffed536675985aca) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "blackgoldsnd1.bin", 0x000000, 0x080000, CRC(d251b59e) SHA1(960b81b87f0fb5000028c863892a273362cb897f) )
	ROM_LOAD( "blackgoldsnd2.bin", 0x080000, 0x080000, CRC(87cbcd1e) SHA1(a6cd186af7c5682e216f549b77735b9bf1b985ae) )
	ROM_LOAD( "blackgoldsnd3.bin", 0x100000, 0x080000, CRC(258f7b83) SHA1(a6df577d98ade8c5c5ff68ef891667e65e83ac17) )
//  ROM_LOAD( "blackgoldsound 1.bin", 0x0000, 0x080000, CRC(d251b59e) SHA1(960b81b87f0fb5000028c863892a273362cb897f) )
//  ROM_LOAD( "blackgoldsound 2.bin", 0x0000, 0x080000, CRC(87cbcd1e) SHA1(a6cd186af7c5682e216f549b77735b9bf1b985ae) )
//  ROM_LOAD( "blackgoldsound 3.bin", 0x0000, 0x080000, CRC(258f7b83) SHA1(a6df577d98ade8c5c5ff68ef891667e65e83ac17) )
ROM_END


ROM_START( m4excam )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ex1_4.bin", 0x0000, 0x010000, CRC(34c4aee2) SHA1(c5487c5b0144ca188bc2e3926a0343fd4c9c565a) )

	ROM_REGION( 0x10000, "altrevs", 0 )
	ROM_LOAD( "ex1_0d.bin", 0x0000, 0x010000, CRC(490c510e) SHA1(21a03d8e2dd4d2c7760acbff5705f925fe9f31be) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "mdmexcalibsnd.p1", 0x000000, 0x080000, CRC(8ea73366) SHA1(3ee45ad98e03177eeef97521df7b3d1945242076) )
	ROM_LOAD( "mdmexcalibsnd.p2", 0x080000, 0x080000, CRC(0fca6ca2) SHA1(2029d15e3b51069f5847ab3846bf6c064f0a3381) )
	ROM_LOAD( "mdmexcalibsnd.p3", 0x100000, 0x080000, CRC(43be816a) SHA1(a95f702ec1bb20f3e0f18984948963b56769f5ba) )
	ROM_LOAD( "mdmexcalibsnd.p4", 0x180000, 0x080000, CRC(ef8a718c) SHA1(093a5fff5bab61fc9276a7f9f3c5b728a50603b3) )
ROM_END


ROM_START( m4front )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ff2_1.bin", 0x0000, 0x010000, CRC(3519cba1) SHA1(d83a5370ee82e258024d20ffacec7050950b1326) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "ffsnd2_1.bin", 0x000000, 0x080000, CRC(9b3cdf12) SHA1(1a209985493f686dd37e91693361ecbf32096f66) )
	ROM_LOAD( "ffsnd2_2.bin", 0x080000, 0x080000, CRC(0fc33bdf) SHA1(6de715e33411050ee1d2a0f08bf1c9a8001ffb4f) )
ROM_END



ROM_START( m4pick )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "pickafr2.nl7", 0x0c000, 0x02000, CRC(7a174563) SHA1(83203392171ba09bd7201cdca5c70c52ec2e65bc) )
	ROM_LOAD( "pickafr1.nl7", 0x0e000, 0x02000, CRC(6ae6e508) SHA1(a7da4151527d0c35f74e971e79ad1ce380315eac) )
ROM_END




ROM_START( m4safar )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "saf4_1.bin", 0x0000, 0x010000, CRC(ad726457) SHA1(4104be61d179024fae9fb9c631677b1ba56d3f00) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )

ROM_END

ROM_START( m4snowbl )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "sb_3.p2", 0x8000, 0x004000, CRC(c22fe04e) SHA1(233ee0795b0029389247a9550ef39af95f671870) )
	ROM_LOAD( "sb_3.p1", 0xc000, 0x004000, CRC(98fdcaba) SHA1(f4a74d5550dd9fc8bff35a583b3289e1bb0be9d5) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END


ROM_START( m4zill )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "zillprgh.bin", 0x0000, 0x080000, CRC(6f831f6d) SHA1(6ab6d7f1752d27bc216bc11533b90178ce188715) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "zillprog.bin", 0x0000, 0x080000, CRC(0f730bab) SHA1(3ea82c8f7d62c70897a5c132273820c9f192cd72) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "zillsnd.bin", 0x0000, 0x080000, CRC(171ed677) SHA1(25d63f4d9c64f13bec4feffa265c5b0c5f6be4ec) )
ROM_END

ROM_START( m4hstr )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "h_s_v1_2.bin", 0x0000, 0x010000, CRC(ef3d3461) SHA1(aa5b1934ab1c6739f36ac7b55d3fda2c640fe4f4) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "happystreak.p1", 0x0000, 0x080000, CRC(b1f328ff) SHA1(2bc6605965cb5743a2f8b813d68cf1646a4bcac1) )
	ROM_LOAD( "hs2_5.bin", 0x0000, 0x010000, CRC(f669a4c9) SHA1(46813ba7104c97eaa851b50019af9b80046d03b3) )
	ROM_LOAD( "hs2_5p.bin", 0x0000, 0x010000, CRC(71c981aa) SHA1(5effe7487e7216078127d3dc4a0a7ad02ad84390) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "happystreak.p1", 0x0000, 0x080000, CRC(b1f328ff) SHA1(2bc6605965cb5743a2f8b813d68cf1646a4bcac1) ) // alt sound rom

	ROM_LOAD( "happystreaksnd.p1", 0x0000, 0x080000, CRC(76cda195) SHA1(21a985cd6cf1f63f4aa799563099a0527a7c0ea2) )
	ROM_LOAD( "happystreaksnd.p2", 0x080000, 0x080000, CRC(f3b4c763) SHA1(7fd6230c13b66a16daad9d45935c7803a5a4c35c) )
ROM_END

ROM_START( m4hstrcs )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "chs3_6.bin", 0x0000, 0x010000, CRC(d097ae0c) SHA1(bd78c14e7f057f173859bcb1db5e6a142d0c4062) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "chs3_6p.bin", 0x0000, 0x010000, CRC(57378b6f) SHA1(cf1cf528b9790c1013d87ccf63dcbf59f365067f) )
	ROM_LOAD( "chs3_6pk.bin", 0x0000, 0x010000, CRC(f95f1afe) SHA1(fffa409e8c7148a840d5dedf490fd9f6975e9476) )
	ROM_LOAD( "chs3_6k.bin", 0x0000, 0x010000, CRC(7eff3f9d) SHA1(31dedb0d9476633e8eb947a687c7b8a94b0e182c) )
	ROM_LOAD( "chs_4_2.bin", 0x0000, 0x010000, CRC(ec148b65) SHA1(2d6252ce68719281f5597955227a1f662743f006) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	/* might use the same roms as the normal version here */
ROM_END


ROM_START( m4ddb )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "ddb3_1.bin", 0x0000, 0x010000, CRC(3b2da727) SHA1(8a677be3b82464d1bf1e97d22adad3b27374079f) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "ddb3_1p.bin", 0x0000, 0x010000, CRC(bc8d8244) SHA1(9b8e0706b3add42e5e4a8b6c6a2f80a333a2f49e) )

	ROM_REGION( 0x200000, "msm6376", 0 )
	ROM_LOAD( "ddbsound1", 0x000000, 0x080000, CRC(47c87bd5) SHA1(c1578ae553c38e93235cea2142cb139170de2a7e) )
	ROM_LOAD( "ddbsound2", 0x080000, 0x080000, CRC(9c733ab1) SHA1(a83c3ebe99703bb016370a8caf76bdeaff5f2f40) )
ROM_END

ROM_START( m4hapfrt )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "hf1_1.bin", 0x0000, 0x010000, CRC(6c16cb05) SHA1(421b164c8410629956177355e505859757c97a6b) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "hf1_1p.bin", 0x0000, 0x010000, CRC(ebb6ee66) SHA1(1f9b67260e5becd013d95358cc89acb1099d655d) )
	ROM_LOAD( "hf1_4pk.bin", 0x0000, 0x010000, CRC(0944b3c6) SHA1(00cdb75dda4f8984f77806047ad79fe9a1a8760a) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4frcrak )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00  )
	ROM_LOAD( "fc_p1.bin", 0xe000, 0x002000, CRC(5716c8cf) SHA1(ddf2c6a70d67932310346bc042239fbe10069f52) )
	ROM_LOAD( "fc_p2.bin", 0xc000, 0x002000, CRC(dc4669f4) SHA1(1112c50792e6976649e4be9314f103acec0c73b3) )
	ROM_LOAD( "fc_p3.bin", 0xa000, 0x002000, CRC(067e3da7) SHA1(6dd0992e57bc68e39a9220a3513705f510f8e9b8) )
ROM_END



ROM_START( m4ewshft )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "each_way_shifter(mdm)_v1-0.bin", 0x0000, 0x010000, CRC(506b6cf0) SHA1(870e356b9785e51c5be5d6bc6af9ea7640b51ee8) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "each_way_shifter-snd1.bin", 0x000000, 0x080000, CRC(b21f9b09) SHA1(69ac3ca2874fc3aebd34dd225a195ad1c0305d00) )
	ROM_LOAD( "each_way_shifter-snd2.bin", 0x080000, 0x080000, CRC(e3ce5ec5) SHA1(9c7eefa4042b1b1aca3d0fbefcad10db34992c43) )
ROM_END

ROM_START( m4jiggin )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "jig2-1n.p1", 0x0000, 0x010000, CRC(9ea16d00) SHA1(4b4f1519eb6565ce76665595154c58cd0d0ab6fd) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "jig2-1p.p1", 0x0000, 0x010000, CRC(09e6e111) SHA1(800a1dbc64c6a631cf3e53bd5f17b5d56955c92e) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "jigsnd1.oki", 0x000000, 0x080000, CRC(581fa143) SHA1(e35186597fc7932d306080ecc82c55af4b769367) )
	ROM_LOAD( "jigsnd2.oki", 0x080000, 0x080000, CRC(34c6fc3a) SHA1(6bfe52a94d8bed5b30d9ed741db7816ddc712aa3) )
ROM_END


ROM_START( m4sunday )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "sunday_sport_v11", 0x0000, 0x010000, CRC(14147d59) SHA1(03b14f4f83a545b3252702267ac012b3be76013d) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4jp777 )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "jpot71", 0x0000, 0x010000, CRC(f4564a05) SHA1(97d21e2268e5d99e6e51cb12c45e09445cff1f50) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4booze )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "boozecruise10_v10.bin", 0x0000, 0x010000, CRC(b37f752b) SHA1(166f7d17694689bd9d51d859c13ddafa1c6e5e7f) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4cbing )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "cherrybingoprg.bin", 0x0000, 0x010000, CRC(00c1d4f3) SHA1(626df7f2f597ed13c32ce0fa8846f2e27ca68eae) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 ) // not oki!
	ROM_LOAD( "cherrybingosnd.p1", 0x000000, 0x100000, CRC(11bed9f9) SHA1(63ed45122dda8e412bb1eaeb967d8a0f925d4bde) )
	ROM_LOAD( "cherrybingosnd.p2", 0x100000, 0x100000, CRC(b2a7ec28) SHA1(307f19ffb46f4a2e8e93923ddb666e50de43a00e) )
ROM_END


ROM_START( m4supsl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "s2v1.0.lp2.64k.bin", 0x6000, 0x002000, CRC(8c622799) SHA1(ffe8a59d37c21c32fd16c812baff2c83b241a43d) )
	ROM_LOAD( "s2v1.0.lp1.256k.bin", 0x8000, 0x008000, CRC(4d963ad0) SHA1(8ec45a33243868afb66d1ea1863124c005bad221) )
ROM_END

ROM_START( m4nod )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "nod.bin", 0x0000, 0x010000, CRC(bc738af5) SHA1(8df436139554ccfb48c4db0a32e3333dbf3c4f46) )
	ROM_REGION( 0x200000, "upd", ROMREGION_ERASE00 )
	ROM_LOAD( "nodsnd.bin", 0x0000, 0x080000, CRC(2134494a) SHA1(3b665bf79567a71195b20e76c50b02707d15b78d) )
ROM_END


ROM_START( m4dcrls )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "70000116.bin", 0x0000, 0x040000, CRC(27e5ad77) SHA1(83cabd8b52efc6c0d5530b55683295208f64abb6) )
	//ROM_LOAD( "dcr_std_340.bin", 0x0000, 0x040000, CRC(27e5ad77) SHA1(83cabd8b52efc6c0d5530b55683295208f64abb6) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "70000117.bin", 0x0000, 0x080000, CRC(4106758c) SHA1(3d2b12f1820a65f00fd70856b7765b6f35a8688e) )
	ROM_LOAD( "70000118.bin", 0x0000, 0x080000, CRC(3603f93c) SHA1(cb969568e0244b465f8b120faba3adb65fe001e6) )
	ROM_LOAD( "70000134.bin", 0x0000, 0x080000, CRC(a81e80e7) SHA1(852c0b3afe8c22b6e6afe585efb8fec7aeb2aecb) )
	ROM_LOAD( "70001115.bin", 0x0000, 0x040000, CRC(26432b07) SHA1(ef7303793252210f3fd07b12f5684b5d2cc828ab) )
//  ROM_LOAD( "dcr_data_340_lv.bin", 0x0000, 0x040000, CRC(26432b07) SHA1(ef7303793252210f3fd07b12f5684b5d2cc828ab) )
	ROM_LOAD( "70001116.bin", 0x0000, 0x040000, CRC(522191e8) SHA1(f80656290295b556d4b67c4458d8f856f8b937fb) )
	ROM_LOAD( "70001117.bin", 0x0000, 0x080000, CRC(7dcf4e36) SHA1(593089aec7efe8b14b953c5d7b0f552c0906730a) )
	ROM_LOAD( "70001118.bin", 0x0000, 0x080000, CRC(6732182c) SHA1(aa6620458d381fc37c226f996eab12840573cf80) )
	ROM_LOAD( "70001134.bin", 0x0000, 0x080000, CRC(a382156e) SHA1(db884dac04f556ecf49f7ecaba0bc3e51a4822f8) )
	ROM_LOAD( "70001172.bin", 0x0000, 0x080000, CRC(32040f0f) SHA1(9f0452bc33e292ce61650f60f2943a3cef0da050) )
	ROM_LOAD( "70001173.bin", 0x0000, 0x080000, CRC(0d5f138a) SHA1(e65832d01b11010a7c71230596e3fbc2c750d175) )
	ROM_LOAD( "dcr_data_340.bin", 0x0000, 0x010000, CRC(fc12e68f) SHA1(f07a42323651ef9aefac24c3b9296a98068c2dc2) )
	ROM_LOAD( "dcr_gala_hopper_340.bin", 0x0000, 0x040000, CRC(e8a19eda) SHA1(14b49d7c9b8ad7c3f8605b2a57740aab2b98d030) )
	ROM_LOAD( "dcr_gala_hopper_340_lv.bin", 0x0000, 0x040000, CRC(e0d08c0e) SHA1(7c6a4e30bacfcbd895e418d4ce66425ec4f118f9) )
	ROM_LOAD( "dcr_mecca_340.bin", 0x0000, 0x040000, CRC(f18ac60f) SHA1(ffdd8d096ebc062a36a8d22cf881d0fa95adc2db) )
	ROM_LOAD( "dcr_mecca_340_lv.bin", 0x0000, 0x040000, CRC(0eb204c1) SHA1(648f5b90776f99155fd54257aabecb8c9f90abec) )
	ROM_LOAD( "dcr_std_340_lv.bin", 0x0000, 0x040000, CRC(d9632301) SHA1(19ac680f00e085d94fc45f765c975f3da1ca1eb3) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "dcr_sounds.bin", 0x0000, 0x09664e, CRC(431cecbc) SHA1(b564ae8d083fef84328526192626a220e979d5ad) ) // intelhex
	ROM_LOAD( "71000110.bin", 0x0000, 0x080000, CRC(0373a197) SHA1(b32bf521e36b5a53170d3a6ec545ce8db3a5094d) )
ROM_END



ROM_START( m4aliz )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "70000000.bin", 0x0000, 0x040000, CRC(56f64dd9) SHA1(11f990c9a6864a969dc9a4146e1ac2c963e3eb9b) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "alizsnd.hi", 0x0000, 0x080000, CRC(c7bd937a) SHA1(cc4d85a3d4cdf57fa96c812a4cd78b599c7052ff) )
	ROM_LOAD( "alizsnd.lo", 0x080000, 0x04e15e, CRC(111cc111) SHA1(413efedbc9e85240df833c10d680b0e907da10b3) )

	ROM_REGION( 0x200000, "misc", ROMREGION_ERASE00 ) // i think this is just the sound roms as intelhex
	ROM_LOAD( "71000000.hi", 0x0000, 0x0bbe9c, CRC(867058c1) SHA1(bd980cb0bb3075854cc2e9b829c31f3742f4f1c2) )
	ROM_LOAD( "71000000.lo", 0x0000, 0x134084, CRC(53046751) SHA1(b8f9eca933315b497732c895f4311f62103344fc) )
ROM_END


ROM_START( m4bluesn )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "bluesboys.bin", 0x0000, 0x020000, CRC(c1395649) SHA1(3cd0eed1f966f5391fe5de496dc747385ebfb556) )
	ROM_RELOAD(0x20000,0x20000)

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "bluesboyz.bi2", 0x000000, 0x080000, CRC(7f19a61b) SHA1(dd8742d84df24e118bdbffb1efffad1c71eb2283) )
	ROM_LOAD( "bluesboyz.bi3", 0x080000, 0x080000, CRC(32363184) SHA1(8f3f53ce4d9f9b54c441263def9d8e23880507a1) )
	ROM_LOAD( "bluesboyz.bi4", 0x100000, 0x080000, CRC(aa94281d) SHA1(e15b7bf97b8e307ed465d9b8cb6e5de0044f6fb5) )
	ROM_LOAD( "bluesboyz.bi5", 0x180000, 0x080000, CRC(d8d7aa2e) SHA1(2d8b86fa63e6649d628c7e343d8f5c329c8f8ced) )
ROM_END


ROM_START( m4c2 )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "ci2-0601.bin", 0x0000, 0x010000, CRC(84cc8aca) SHA1(1471e3ad9c9ba957b6cc99c204fe588cc55fbc50) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END


ROM_START( m4coney )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "70000060.bin", 0x0000, 0x010000, CRC(fda208e4) SHA1(b1a243b2681faa03add4ab6e4df98814f9c52fc5) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4cfinln )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "cfd_7_1.bin", 0x0000, 0x020000, CRC(e42ec2aa) SHA1(6495448c1d11ce0ab9ad794bc3a0981432e22945) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )

	ROM_REGION( 0x20000, "altrevs", 0 )
	// 'Jackpot Version'
	ROM_LOAD( "cfd_d0.bin", 0x0000, 0x020000, CRC(179fcf13) SHA1(abd18ed28118ba0a62ab321a9d963105946d5eef) )
ROM_END


ROM_START( m4dcrazy )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "70000130.bin", 0x0000, 0x080000, CRC(9d700a27) SHA1(c73c7fc4233dace32fac90a6b46dba5c12979160) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "70000131.bin", 0x0000, 0x080000, CRC(a7fcfcc8) SHA1(cc7b164e79d86d68112ac86e1ab9e81885cfcabf) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4ftladn )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "fidse___.5_0", 0x00000, 0x20000, CRC(62347bbf) SHA1(2b1cd5adda831a8c74c9484ee1b616259d3e3981) )
	ROM_RELOAD(0x20000,0x20000)

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4goldnn )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "goldenyears10.bin", 0x0000, 0x020000, CRC(1074bac6) SHA1(967ee64f267a80017fc95bbc6c5a38354e9cab65) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "tgyosnd.p1", 0x000000, 0x080000, CRC(bda49b46) SHA1(fac143003641824bf0db4ac6841292e509fa00da) )
	ROM_LOAD( "tgyosnd.p2", 0x080000, 0x080000, CRC(43d28a0a) SHA1(5863e493e84641e4fabcd69e6402e3bcca87dde2) )
	ROM_LOAD( "tgyosnd.p3", 0x100000, 0x080000, CRC(b5b9eb68) SHA1(8d5a0a687dd7096da8dfd2a59c6fe96f4b1949f9) )
ROM_END



ROM_START( m4jungjk )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "jjsoft_v550_1346_e7a3_lv.bin", 0x0000, 0x040000, CRC(c5315a0c) SHA1(5fd2115e033e0310ded3cfb39f31dc31b4d6bb5a) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "70000102.bin", 0x0000, 0x040000, CRC(e5f03540) SHA1(9a14cb4eade9f6b1c6d6cf78306259dbc108f1a5) )
	ROM_LOAD( "jj.bin", 0x0000, 0x040000, CRC(9e15c1b6) SHA1(9d4f3707f2cc2f0e8eb9051181bf8b368be3cbcf) )
	ROM_LOAD( "jjlump_v400_19a3.bin", 0x0000, 0x040000, CRC(bc86c415) SHA1(6cd828578835dafe5d8d46810dc70d47abd4e8b2) )
	ROM_LOAD( "70000092.bin", 0x0000, 0x040000, CRC(6530bc6c) SHA1(27819e760c84fbb40f354e87910fb15b3058e2a8) )
	ROM_LOAD( "jungle.p1", 0x0000, 0x080000, CRC(ed0eb72c) SHA1(e32590cb3eb7d07fb210bee1be3c0ee01554cb47) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "71000080.lo.hex", 0x0000, 0x134084, CRC(f3866082) SHA1(f33f6d7e078d7072cc7c67672b3afa3e90e1f805) )
	ROM_LOAD( "71000080.hi.hex", 0x0000, 0x12680f, CRC(2a9db1df) SHA1(73823c3db5c68068dadf6d9b4c93b47c0cf13bd3) )
	ROM_LOAD( "71000080.p1", 0x000000, 0x080000, CRC(b39d5e03) SHA1(94c9208601ea230463b460f5b6ea668363d239f4) )
	ROM_LOAD( "71000080.p2", 0x080000, 0x080000, CRC(ad6da9af) SHA1(9ec8c8fd7b9bcd1d4c6ed93726fafe9a50a15894) )
ROM_END

ROM_START( m4clab )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "70000019.bin", 0x0000, 0x040000, CRC(23a12863) SHA1(4047cb8cbc03f96f2b8681b6276e100e8e9194a5) )

	ROM_REGION( 0x40000, "altrevs", 0 )
	ROM_LOAD( "70000020.bin", 0x0000, 0x040000, CRC(88af7368) SHA1(14dea4267a4365286eea1e02b9b44d4053618cbe) )
	ROM_LOAD( "70000052.bin", 0x0000, 0x040000, CRC(99e60d45) SHA1(ec28bdd4ffb9674c2e9f8ab72aac3cb6011e7d6f) )
	ROM_LOAD( "70000053.bin", 0x0000, 0x040000, CRC(3ddf8b29) SHA1(0087ccd3429c081a3121e97d649914ab2cf8caa6) )
	ROM_LOAD( "clab_v300_1028_c23a_lv_10p.bin", 0x0000, 0x040000, CRC(c2ca098a) SHA1(e2bf80366af925a4c880a3377b79d494760f286f) )
	ROM_LOAD( "clab_v300_1031_3fb7_nlv_10p.bin", 0x0000, 0x040000, CRC(1502527f) SHA1(8a3dd5600ad9a073ea6fb8412178daa575002e56) )
	ROM_LOAD( "clab_v300_1033_1175_nlv_20p.bin", 0x0000, 0x040000, CRC(ba26efd7) SHA1(bf9c0dbf6882ddc42680c7d23c9b858eb12e5646) )
	ROM_LOAD( "clab_v400_1249_0162_nlv.bin", 0x0000, 0x040000, CRC(df256c84) SHA1(f4d0fc5acd7d0ac770cae548744ce18dfb9ec67c) )
	ROM_LOAD( "clab_v410_1254_1ca2_lv.bin", 0x0000, 0x040000, CRC(6e2ee8a9) SHA1(e64a01c93b879c9ade441bd4f1ce381f6e65a655) )
	ROM_LOAD( "clab_v410_1254_6e11_nlv.bin", 0x0000, 0x040000, CRC(efe4fcb9) SHA1(8a2f02593c7fbea060c78a98abd82fd970661e05) )
	ROM_LOAD( "clabrom", 0x0000, 0x040000, CRC(d80ecff5) SHA1(2608e95b718ecd49d880fd9911cb97e6644a307d) )
	// clearly don't belong here
	// 95407596.lo = 95407596.lo           sc_cexpl   Cash Explosion (Mazooma) (Scorpion ?)
	// 95407597.hi = 95407597.hi           sc_cexpl   Cash Explosion (Mazooma) (Scorpion ?)


	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "71000010.lo", 0x0000, 0x134084, CRC(c39bbae4) SHA1(eee333376612a96a4c344729a96cc60c217bfde3) )
	ROM_LOAD( "71000010.hi", 0x0000, 0x091c0b, CRC(d0d3cb4f) SHA1(eaacf9ed3a6b6dcda8e1a3edbc3a9a2a51ffcbd8) )
	ROM_LOAD( "clab_snd1_c8a6.bin", 0x0000, 0x080000, CRC(cf9de981) SHA1(e5c73e9b9db9ac512602c2dd586ca5cf65f98bc1) )
	ROM_LOAD( "clab_snd2_517a.bin", 0x080000, 0x080000, CRC(d4eb949e) SHA1(0ebbd1b5e3c86da94f35c69d9d60e36844cc4d7e) )
ROM_END



ROM_START( m4looplt )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "70000500.bin", 0x0000, 0x080000, CRC(040699a5) SHA1(e1ebc23684c5bc1faaac7409d2179488c3022872) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "70000500a.bin", 0x0000, 0x080000, CRC(0668f52d) SHA1(6560309facf0022e3c14421b848f212b18be7550) )
	ROM_LOAD( "70000501.bin", 0x0000, 0x080000, CRC(e2fbbfcf) SHA1(fc060468bf5e732626af8c3d0d6fc119a529c330) )
	ROM_LOAD( "70000501a.bin", 0x0000, 0x080000, CRC(42bef934) SHA1(c332eb6566ef5f9ac56d1c3944635296c21b3193) )
	ROM_LOAD( "70000504.bin", 0x0000, 0x080000, CRC(15e2c1c3) SHA1(69257749f1909b7ecc9c94cc2a27a5d4e6608251) )
	ROM_LOAD( "70000505.bin", 0x0000, 0x080000, CRC(f28f59bd) SHA1(d5cdb0c020693c7922c5243f9d18054d47ed039d) )
	ROM_LOAD( "70000506.bin", 0x0000, 0x080000, CRC(a3d40e9a) SHA1(97ac40e814824450e6705bc3240fffd4d0015b46) )
	ROM_LOAD( "70000507.bin", 0x0000, 0x080000, CRC(756eefe4) SHA1(b253fbd94fdab5df32375a02d16d9ba333e8d71c) )
	ROM_LOAD( "70001500.bin", 0x0000, 0x080000, CRC(0b9761a4) SHA1(e7a5e4b90d2e60808a7797d124308973130c440d) )
	ROM_LOAD( "70001500a.bin", 0x0000, 0x080000, CRC(09f90d2c) SHA1(addfd0d20ef9cafba042aa05ee84db85f060b67a) )
	ROM_LOAD( "70001501.bin", 0x0000, 0x080000, CRC(6ce9f76c) SHA1(467701786f8de136c9780a4ef93be6bb932d235d) )
	ROM_LOAD( "70001501a.bin", 0x0000, 0x080000, CRC(ccacb197) SHA1(c7573f309e9c79b2999229c46f78fd0283c4a064) )
	ROM_LOAD( "70001504.bin", 0x0000, 0x080000, CRC(1a7339c2) SHA1(575477d8abe3765d9cd4345336d0f7fa3a69202a) )
	ROM_LOAD( "70001505.bin", 0x0000, 0x080000, CRC(7c9d111e) SHA1(8f98feb70cdcd77b5e7bb6a015c935403a53f428) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "71000500.bin", 0x0000, 0x080000, CRC(94fe58f4) SHA1(e07d8e6d4b1e660abc4fa08d703fc0e586f3570d) )
ROM_END


ROM_START( m4mgpn )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "mgp15.p1", 0x0000, 0x010000, CRC(ec76233f) SHA1(aa8595c639c83026d7fe5c3a161f8b08ff9a8b46) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "mgpsnd.p1", 0x000000, 0x080000, CRC(d5f0b845) SHA1(6d97d0d4d07407bb0a51e1d62da95c664418a9e9) )
	ROM_LOAD( "mgpsnd.p2", 0x080000, 0x080000, CRC(cefeea06) SHA1(45142ca1bab898dc6f3c32e382ee9157132810a6) )
	ROM_LOAD( "mgpsnd.p3", 0x100000, 0x080000, CRC(be4b3bd0) SHA1(f14c08dc770a24db8bbd00a65d3edf6ee9895ca3) )
	ROM_LOAD( "mgpsnd.p4", 0x180000, 0x080000, CRC(d74b4b03) SHA1(a35c99040a72485a6c2d4a4fdfc203634f6a9ad0) )
ROM_END


ROM_START( m4olygn )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "ogdsx___.8_0", 0x0000, 0x040000, CRC(b51a2538) SHA1(d54f37dc14c44ab66e6d6ba6e2df8bc9ed003054) )
	ROM_REGION( 0x40000, "altrevs", 0 )
	ROM_LOAD( "ogdsxe__.8_0", 0x0000, 0x040000, CRC(13aa70aa) SHA1(3878c181ec07e24060935bec96e5128e6e4baf31) )
	//ROM_LOAD( "state.rst", 0x0000, 0x0000d0, CRC(e43a9cc2) SHA1(478edb0211224ad2732bf9e99160cf72c991edb9) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END


ROM_START( m4rhnote )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "70000120.bin", 0x0000, 0x040000, CRC(d1ce1e1c) SHA1(2fc2b041b4e9fcade4b2ce6a0bc709f4174e2d88) )
	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "70000121.bin", 0x0000, 0x040000, CRC(1e1a26c0) SHA1(8a80a94d280c82887a0f7da607988597df23e1fb) )
	ROM_LOAD( "70000125.bin", 0x0000, 0x080000, CRC(67a617a2) SHA1(3900c0cc3f8e4d52105096c1e21903cb83b8c1b7) )
	ROM_LOAD( "70000126.bin", 0x0000, 0x080000, CRC(68deffbe) SHA1(9b94776aa0416309204987ac9109a65ad3234f1b) )
	ROM_LOAD( "70000132.bin", 0x0000, 0x080000, CRC(50c06d0d) SHA1(8d629d77390b92c5e30104237245f92dc8f52a6c) )
	ROM_LOAD( "70000133.bin", 0x0000, 0x080000, CRC(fb198e1b) SHA1(6fb03680ad29ca750fe2e75f48a05f538ddac9b7) )
	ROM_LOAD( "70000135.bin", 0x0000, 0x080000, CRC(02531c21) SHA1(de9da10bc81ab02ba131da1a1733eda1948dc3cc) )
	ROM_LOAD( "70001122.bin", 0x0000, 0x040000, CRC(13171ffc) SHA1(e49a2080afd27c0de183da64baa2060020910155) )
	ROM_LOAD( "70001124.bin", 0x0000, 0x040000, CRC(8acb2d7d) SHA1(ffd4f0e1f80b41b6f54af31e5dcd41fe12e4ea0b) )
	ROM_LOAD( "70001125.bin", 0x0000, 0x080000, CRC(6b202a88) SHA1(63f7325c8dc373f771f02e5bf9ac0c0d33a906bd) )
	ROM_LOAD( "70001126.bin", 0x0000, 0x080000, CRC(0db90e12) SHA1(0b010ca878ecabb47c0a0eec0badd595b2bafbfb) )
	ROM_LOAD( "70001135.bin", 0x0000, 0x080000, CRC(a9ed9178) SHA1(446919e869a9cc20f469954504adf448474d702b) )
	ROM_LOAD( "70001150.bin", 0x0000, 0x040000, CRC(3c3f4e45) SHA1(114c18e0fa8de224992138b72bf789ace39dffa0) )
	ROM_LOAD( "70001151.bin", 0x0000, 0x040000, CRC(0cb1f440) SHA1(7ebdac6ea495d96c7713a284fdad4da0874de3f2) )
	ROM_LOAD( "70001153.bin", 0x0000, 0x040000, CRC(e8ba9b3a) SHA1(71af6dd77da419868391e01f565c24a70d55b396) )
	ROM_LOAD( "70001160.bin", 0x0000, 0x040000, CRC(2d532681) SHA1(fb4321b6922cf35780adbdc5f030ef0df8d6cc9a) )
	ROM_LOAD( "70001161.bin", 0x0000, 0x040000, CRC(e9a49319) SHA1(001163ece7a405a27fd71fdeb97489db143749a7) )
	ROM_LOAD( "70001502.bin", 0x0000, 0x040000, CRC(d1b332f1) SHA1(07db228705b0bce47107cf5458986e830b988cee) )
	ROM_LOAD( "70001503.bin", 0x0000, 0x040000, CRC(2a44069a) SHA1(0a1581ba552e0e93d6bc3b7298014ea4b6793da1) )
	ROM_LOAD( "70001510.bin", 0x0000, 0x080000, CRC(87cb4cae) SHA1(49c97e0e79a8cd1417e9e07a13afe736d00ef3df) )
	ROM_LOAD( "rhn_data_110_lv.bin", 0x0000, 0x040000, CRC(1f74c472) SHA1(86a170ddb001f817e960e7c166399280ad620bf0) )
	ROM_LOAD( "rhn_gala_hopper_120.bin", 0x0000, 0x040000, CRC(e8ba9b3a) SHA1(71af6dd77da419868391e01f565c24a70d55b396) )
	ROM_LOAD( "rhn_gala_hopper_120_lv.bin", 0x0000, 0x040000, CRC(521b6402) SHA1(7d260c45fa339f5ca34f8e335875ad47bb093a04) )
	ROM_LOAD( "rhn_mecca_120.bin", 0x0000, 0x040000, CRC(f131e386) SHA1(73672e6e66400b953dda7f2254082eff73dbf058) )
	ROM_LOAD( "rhn_mecca_120_lv.bin", 0x0000, 0x040000, CRC(471e5263) SHA1(79c205e0d8e748aa72f9f3fadad248edf71f5ae0) )
	ROM_LOAD( "rhn_std_110.bin", 0x0000, 0x040000, CRC(439f27d2) SHA1(4ad01c4dc9bbab7520fb281198777aea56f600b0) )
	ROM_LOAD( "rhn_std_110_lv.bin", 0x0000, 0x040000, CRC(922b8196) SHA1(6fdbf301aaadacaeabf29ad11c67b22122954051) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "71000120.hex", 0x0000, 0x112961, CRC(5eb5245e) SHA1(449b02baf56e5798f656d9aee497b88d34f562cc) )
	ROM_LOAD( "rhnsnd.bin", 0x0000, 0x080000, CRC(e03eaa43) SHA1(69117021adc1a8968d50703336147a7344c62100) )
ROM_END



ROM_START( m4rhrock )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "rhr_v200_1625_da8c_nlv.bin", 0x0000, 0x040000, CRC(dd67f5b3) SHA1(19b7b57ef20a2ad7997cf748396b246fda87db70) )
	ROM_LOAD( "rhr_v300_1216_ce52_nlv.bin", 0x0000, 0x040000, CRC(86b0d683) SHA1(c6553bf65c055c4f911c215ba112eaa672357290) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 ) // intelhex, needs convertig
	ROM_LOAD( "71000200.hi.hex", 0x0000, 0x0ff0f8, CRC(998e28ea) SHA1(f54a69af16e05119df2697bc01e548ac51ed3e11) )
	ROM_LOAD( "71000200.lo.hex", 0x0000, 0x134084, CRC(ccd0b35f) SHA1(6d3ef65577a46c68f8628675d146f829c9a99659) )
ROM_END


ROM_START( m4rhwhl )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "70001184.bin", 0x0000, 0x080000, CRC(8792d95b) SHA1(24b4f78728db7ee95d1fcd3ba38b49a20baaae6b) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "rhw_v100_1333_6d40_lv.bin", 0x0000, 0x080000, CRC(9ef7b655) SHA1(605822eaee44bebf554218ef7346192a6a84077e) )
//  ROM_LOAD( "rhw_v100_1333_6d40_lv_bad.bin", 0x0000, 0x05420d, CRC(a4ef87c5) SHA1(6768fddb8046fb35a2bba3d26bd4650b6e7a4fa7) ) // I think this is just a bad dump of the above?
	ROM_LOAD( "rhw_v310_0925_0773_lv_p.bin", 0x0000, 0x080000, CRC(11880908) SHA1(0165bacf73dd54959975b3f186e256fd8d690d34) )
	ROM_LOAD( "rhw_v310_0931_fa02_lv.bin", 0x0000, 0x080000, CRC(5642892e) SHA1(7a80edf9aefac9731751afa8250de07004c55e77) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "71000180.hi.hex", 0x0000, 0x04da98, CRC(0ffa11a5) SHA1(a3f8eb00b6771cb49965a717e27d0b544c6b2f4f) ) // as intelhex?
	ROM_LOAD( "71000180.lo.hex", 0x0000, 0x134084, CRC(6dfc7474) SHA1(806b4b8ca5fa868581b4bf33080b9c486ce71bb6) )// as intelhex?
	ROM_LOAD( "redhotwheelssnd.p1", 0x0000, 0x080000, CRC(7b274a71) SHA1(38ba69084819133253b41f2eb1d784104e5f10f7) )
	ROM_LOAD( "redhotwheelssnd.p2", 0x0000, 0x080000, CRC(e36e19e2) SHA1(204554622c9020479b095acd4fbab1f21f829137) )
ROM_END

ROM_START( m4rdeal )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "70000703.bin", 0x0000, 0x080000, CRC(11e51311) SHA1(71a4327fa01cd7e899d423adc34c732ed56118d8) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "70000704.bin", 0x0000, 0x080000, CRC(b161c08b) SHA1(bb914eb900aff0f6eeec33ff8a595a288306e073) )
	ROM_LOAD( "70000723.bin", 0x0000, 0x080000, CRC(bb166401) SHA1(1adf244e97d52cc5a5116a01d804caadd1034507) )
	ROM_LOAD( "70000724.bin", 0x0000, 0x080000, CRC(37df89e5) SHA1(6e7da02053be91f27257e3c5f952bbea9df3bc09) )
	ROM_LOAD( "70001622.bin", 0x0000, 0x080000, CRC(1a8fff86) SHA1(e0bdb4fa233acdf18d535821fb9fc2cf69ac4f5e) )
	ROM_LOAD( "70001623.bin", 0x0000, 0x080000, CRC(97a223f8) SHA1(d1a87391a8178cb47664968031a7d17d9082847b) )
	ROM_LOAD( "70001703.bin", 0x0000, 0x080000, CRC(1b89777e) SHA1(09b87d352d27c2847dec0a147dea7cc75f07ffa5) )
	ROM_LOAD( "70001704.bin", 0x0000, 0x080000, CRC(11f3834d) SHA1(8c56cf60b064e8d755c5e760fcf6f0284ef710f9) )
	ROM_LOAD( "70001744.bin", 0x0000, 0x080000, CRC(6de0d366) SHA1(ec42608f3b9c7cf8e6b81541767a8664478e7ab4) )
	ROM_LOAD( "70001745.bin", 0x0000, 0x080000, CRC(4c0e9cab) SHA1(6003d4553f053a253965ba786553b00b7e197069) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4shoknr )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "snr_v300_1218_3019_lv.bin", 0x0000, 0x040000, CRC(bec80497) SHA1(08de5e29a063b01fb904a156170a3063633115ab) )
	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "snr_v300_1221_c8ff_nlv.bin", 0x0000, 0x040000, CRC(d191b361) SHA1(4146e509e77878a51e32de877768504b3c85e6f8) )
	ROM_LOAD( "snr_v200_1145_047f_lv.bin", 0x0000, 0x040000, CRC(73ef1e1a) SHA1(6ccaf64daa5acacfba4df576281bb5478f2fbd29) )
	ROM_LOAD( "snr_v200_1655_5a69_nlv.bin", 0x0000, 0x040000, CRC(50ba0c6b) SHA1(767fd59858fc55ae95f096f00c54bd619369a56c) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "snrsnd.p1", 0x000000, 0x080000, CRC(985c7c8c) SHA1(d2740ff6192c21af3a8a8a9a92b6fd604b40e9d1) )
	ROM_LOAD( "snrsnd.p2", 0x080000, 0x080000, CRC(6a3a57ce) SHA1(3aaa0a761e17a2a14196cb023b10a49b44ba1046) )
ROM_END


ROM_START( m4shkwav )
	ROM_REGION( 0x80000, "maincpu", 0 )
	ROM_LOAD( "swave_v210_1135_08dd_lv.bin", 0x0000, 0x040000, CRC(ca9d40a3) SHA1(65c9e4aa022eb6fe70d619f67638c37ad578ddbf) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "swave_v210_11376_0bb3_nlv.bin", 0x0000, 0x040000, CRC(3fcaf973) SHA1(28258c8c60e6b542e1789cd8a4cfd530d1ed6084) )
	ROM_LOAD( "swsplv.bin", 0x0000, 0x040000, CRC(1e33e93f) SHA1(3e87f8ed35da776e1968c9574c140cc3984ea8de) )
	ROM_LOAD( "sho1_0lv.bin", 0x0000, 0x080000, CRC(a76d8544) SHA1(8277a2ce311840b8405a087d3dc0bbf97054ad87) )
	ROM_LOAD( "swave_v300_1552_13ed_nlv.bin", 0x0000, 0x040000, CRC(b0e03f04) SHA1(fdd113af30fd9e87b171ecdf3be7e720366476b3) )
	ROM_LOAD( "swave_v300_1555_119d_lv.bin", 0x0000, 0x040000, CRC(45b786d4) SHA1(24fd4fdea684103334385ca329f384796b496e2c) )
	ROM_LOAD( "swsp_v300_1602_e1b2_nlv.bin", 0x0000, 0x040000, CRC(4ed74015) SHA1(0ab2167ba0ce6f1a1317c2087091187b9fa94c27) )
	ROM_LOAD( "swsp_v300_1606_ded8_lv.bin", 0x0000, 0x040000, CRC(bb80f9c5) SHA1(95f577c427204b83bec0128acfd89dda90938d1f) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "71000250.hi.hex", 0x0000, 0x0c852b, CRC(e3a857c7) SHA1(66619b7926ae7df970045fffd7e20763abfe14a4) )
	ROM_LOAD( "71000250.lo.hex", 0x0000, 0x134084, CRC(46758bc5) SHA1(18d02960580646b276e7a6aabdeb4ca449ec5ea0) )
	ROM_LOAD( "shocksnd.p1", 0x000000, 0x080000, CRC(54bf0ddb) SHA1(693b855367972b5a45e9d2d6152849ab2cde38a7) )
	ROM_LOAD( "shocksnd.p2", 0x080000, 0x080000, CRC(facebc55) SHA1(75367473646cfc735f4d1267e13a9c92ea19c4e3) )
ROM_END

ROM_START( m4sinbdn )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "sbds3___.a_1", 0x0000, 0x020000, CRC(9bff0e40) SHA1(f8a1263a58f828554e9df77ed0db78e627666fb5) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END


ROM_START( m4sinbd2 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "sxdsx___.2_0", 0x0000, 0x040000, CRC(4e1f98b5) SHA1(3e16e7a0cdccc9eb1a1bb6f9a0332c4582483eee) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4sinbd3 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "sdd__.3_0", 0x0000, 0x040000, CRC(100098c1) SHA1(b125855c49325972f620463e32fdf124222e27d2) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4sinbdd )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "sdds3__l.1w0", 0x0000, 0x040000, CRC(feedb8cf) SHA1(620b5379164d4da1200d4807199c2dc78d7d89ee) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4sinbdj )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "sbds3__l.9_0", 0x0000, 0x040000, CRC(e425375a) SHA1(d2bdd8e768fc7764054eff574360f3cfb5f4f66d) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4sinbdl )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "sbds3__l.aw0", 0x0000, 0x040000, CRC(c484ef9d) SHA1(62f6644b83dd6abaf80809217edf6a8230a89268) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4sinbdw )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "sbds3___.7w1", 0x0000, 0x020000, CRC(23bc9ce0) SHA1(f750de2b781bc902c65de7109e10a5fc2d4e1c61) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4spotln )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "gsp01.p1", 0x0000, 0x020000, CRC(54c56a07) SHA1(27f21872a7ffe0c497983fa5bbb59e967bf48974) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END


ROM_START( m4sdquid )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "70000352.bin", 0x0000, 0x040000, CRC(303d6177) SHA1(aadff8a81244bfd62d1cc088caf01496e1ff61db) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "70000353.bin", 0x0000, 0x040000, CRC(6e3a9dfc) SHA1(1d5d04140811e17267102c0618ffdaf70f71f717) )
	ROM_LOAD( "70000354.bin", 0x0000, 0x080000, CRC(eb938886) SHA1(e6e882f28230b51091b2543df17c65e491a94f94) )
	ROM_LOAD( "70000355.bin", 0x0000, 0x080000, CRC(c4b857b3) SHA1(6b58ee5d780551a665e737ea291b04ab641e8029) )
	ROM_LOAD( "70001352.bin", 0x0000, 0x040000, CRC(0356e899) SHA1(51666a0ec4d29165f64391efa2339ea82710fa4c) )
	ROM_LOAD( "70001353.bin", 0x0000, 0x040000, CRC(324b4a86) SHA1(fd4cd59e3519e23ab824c84546ab50b3803766bf) )
	ROM_LOAD( "70001354.bin", 0x0000, 0x080000, CRC(e9aee455) SHA1(f9babcb90fe88fd522e4be7d8b794c9c1cd4f780) )
	ROM_LOAD( "70001355.bin", 0x0000, 0x080000, CRC(ebc428c3) SHA1(c75738fc547c5b1783a6c5a6ebb06eea5730683d) )
	ROM_LOAD( "70001401.bin", 0x0000, 0x080000, CRC(f83b1551) SHA1(d32f84938edfc4c3d9763fb3eecf56ed9102d979) )
	ROM_LOAD( "70001411.bin", 0x0000, 0x080000, CRC(fa0c95ec) SHA1(1a792e7ed8fa092fdb34a7b31df316d6afca3e90) )
	ROM_LOAD( "70001451.bin", 0x0000, 0x080000, CRC(2d288982) SHA1(bf3e1e1e20eb2d1d9ca6d0a8b48ef9c57aeb30bd) )
	ROM_LOAD( "70001461.bin", 0x0000, 0x080000, CRC(73238425) SHA1(a744adabd8854c6b45820899f15ebb2c2a74dd4d) )
	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
ROM_END



ROM_START( m4tornad )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "torn_v110_1146_979d_lv.bin", 0x0000, 0x040000, CRC(3160bddd) SHA1(4f36b081c8f6859a3fe55e1f177a0406c2480987) )

	ROM_REGION( 0x80000, "altrevs", 0 )
	ROM_LOAD( "torn_v110_1153_955f_nlv.bin", 0x0000, 0x040000, CRC(c437040d) SHA1(50c5ba655989b7f6a2ee61af0ad007ce825f4364) )
	ROM_LOAD( "tornsp_v110_1148_95bd_nlv.bin", 0x0000, 0x040000, CRC(f0933eb6) SHA1(a726b02ae6298ecfb6a01f7ecb09bac50ca13114) )
	ROM_LOAD( "tornsp_v110_1151_9693_lv.bin", 0x0000, 0x040000, CRC(05c48766) SHA1(e20916cd67904601cd25b3c6f70030c5302d12a6) )
	ROM_LOAD( "torn_v200_1613_efe5_nlv.bin", 0x0000, 0x040000, CRC(32936ae5) SHA1(f7ec8b9c0e74c0e51ea4b9c8f450f64907ce3300) )
	ROM_LOAD( "torn_v200_1617_ece9_lv.bin", 0x0000, 0x040000, CRC(c7c4d335) SHA1(955303d446e78bfa1ceeb07ca62cb6e11e478592) )
	ROM_LOAD( "tornado.bin", 0x0000, 0x040000, CRC(c7c4d335) SHA1(955303d446e78bfa1ceeb07ca62cb6e11e478592) )
	ROM_LOAD( "tornsp_v200_1623_eee3_nlv.bin", 0x0000, 0x040000, CRC(6b4f8baf) SHA1(fea21f43b3bbc1c969a7426ca956898e3680823f) )
	ROM_LOAD( "tornsp_v200_1626_ec93_lv.bin", 0x0000, 0x040000, CRC(9e18327f) SHA1(7682cd172903cd5c26873306e70394c154e66c30) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "71000300.hi.hex", 0x0000, 0x0be342, CRC(f9021a32) SHA1(4bd7d7306385ef37dd9cbb5085dbc104657abc0e) )
	ROM_LOAD( "71000300.lo.hex", 0x0000, 0x134084, CRC(af34658d) SHA1(63a6db1f5ed00fa6208c63e0a2211ba2afe0e9a1) )
	ROM_LOAD( "tornadosnd.p1", 0x0000, 0x080000, CRC(cac88f25) SHA1(6ccbf372d983a47a49caedb8a526fc7703b31ed4) )
	ROM_LOAD( "tornadosnd.p2", 0x080000, 0x080000, CRC(ef4f563d) SHA1(1268061edd93474296e3454e0a2e706b90c0621c) )
ROM_END

ROM_START( m4vivan )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "vlv.bin", 0x0000, 0x010000, CRC(f20c4858) SHA1(94bf19cfa79a1f5347ab61a80cbbce06942187a2) )

	ROM_REGION( 0x200000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "vlvsound1.bin", 0x0000, 0x080000, CRC(ce4da47a) SHA1(7407f8053ee482db4d8d0732fdd7229aa531b405) )
	ROM_LOAD( "vlvsound2.bin", 0x0000, 0x080000, CRC(571c00d1) SHA1(5e7be40d3caae88dc3a580415f8ab796f6efd67f) )
ROM_END


ROM_START( m4tylb )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "tylb-10n.bin", 0x0000, 0x010000, CRC(8339329b) SHA1(3b20be519cc94f03a899372a3cb4f1a584457879) ) /* only 0x8000-0xffff used */

	ROM_REGION( 0x010000, "altrevs", 0 )
	ROM_LOAD( "tylb1_3.bin", 0x0000, 0x010000, CRC(84b1a12d) SHA1(b78270c84cd4b2fc6cd1b0177f37ced83ff7054a) ) /* only 0x8000-0xffff used */

	ROM_REGION( 0x080000, "msm6376", ROMREGION_ERASE00 )
	ROM_LOAD( "tylbsnd.bin", 0x0000, 0x080000, CRC(781175c7) SHA1(43cf6fe91c756cdd4acc735411ac166647bf29e7) )
ROM_END

ROM_START( m4magi7 )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "m716.bin", 0x0000, 0x010000, CRC(a26d52a8) SHA1(34228654a922f6c2b01c3fbf1a58755ec6968cbc) ) /* only 0x8000-0xffff used */

	ROM_REGION( 0x010000, "altrevs", 0 )
	ROM_LOAD( "m799.bin", 0x0000, 0x010000, CRC(2084e4a1) SHA1(5e365fb19b51b2f8c8a42d18c8b7db5b17ac77ff) ) /* only 0x8000-0xffff used */

	ROM_REGION( 0x080000, "msm6376", ROMREGION_ERASE00 )
ROM_END


ROM_START( m4rags )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "ct15.bin", 0x8000, 0x008000, CRC(27eab355) SHA1(0ad6a09015b2ddfe87563f3d88f84d2d5b3c74a0) )
	ROM_REGION( 0x010000, "altrevs", 0 )
	ROM_LOAD( "ct15p.bin", 0x8000, 0x008000, CRC(7ea5df79) SHA1(d1c355db97010dc9075217efe40d6620258ae3e4) )
	ROM_LOAD( "ct21.bin", 0x8000, 0x008000, CRC(78a37f00) SHA1(83c1f17317eecacb1f1172a49540b6e48ba19f7e) )
	ROM_LOAD( "ct21p.bin", 0x8000, 0x008000, CRC(21ec132c) SHA1(91279edc0d680219dfe03160b6abd32cafa74bdf) )

	ROM_REGION( 0x080000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4riocr )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "rg13n.bin", 0x8000, 0x008000, CRC(ebb2c0da) SHA1(14a7efb5747a14eb6aed90b79ceb0622aae88370) )
	ROM_REGION( 0x010000, "altrevs", 0 )
	ROM_LOAD( "rg13p.bin", 0x8000, 0x008000, CRC(013fc8db) SHA1(ef3541c6a57dbe4701e08488527da32d398593d4) )
	ROM_REGION( 0x080000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4ndup )
	ROM_REGION( 0x010000, "maincpu", 0 )
	ROM_LOAD( "ndu26n.bin", 0x8000, 0x008000, CRC(562668f6) SHA1(aaebdb649e0399551f32520d28b27d7654271fee) )

	ROM_REGION( 0x010000, "altrevs", 0 )
	ROM_LOAD( "ndu26p.bin", 0x8000, 0x008000, CRC(f766875e) SHA1(13682878289a248a1540a24bf5c56b200c59da42) )
	ROM_LOAD( "ndub81n.bin", 0x8000, 0x008000, CRC(39d52323) SHA1(37f510560b71705860044f5d4259cf1a34210403) )
	ROM_LOAD( "nduj81n.bin", 0x8000, 0x008000, CRC(769fce55) SHA1(ab53507991f4af96fca3fd82d66ea29baabb69d1) )

	ROM_REGION( 0x080000, "msm6376", ROMREGION_ERASE00 )
ROM_END


ROM_START( m4abra )
	ROM_REGION( 0x040000, "maincpu", 0 )
	ROM_LOAD( "nn_sj___.4_0", 0x0000, 0x040000, CRC(48437d29) SHA1(72a2e9337fc0a004c382931f3af856253c44ed61) )

	ROM_REGION( 0x040000, "altrevs", 0 )
	ROM_LOAD( "nn_sja__.4_0", 0x0000, 0x040000, CRC(766cd4ae) SHA1(4d630b967ede615d325f524c2e4c92c7e7a60886) )
	ROM_LOAD( "nn_sjb__.4_0", 0x0000, 0x040000, CRC(ca77a68a) SHA1(e753c065d299038bae4c451e647b9bcda36421d9) )
	ROM_LOAD( "nn_sjk__.4_0", 0x0000, 0x040000, CRC(19018556) SHA1(6df993939e70a24621d4e732d0670d64fac1cf56) )
	ROM_REGION( 0x080000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4wcnov )
	ROM_REGION( 0x080000, "maincpu", 0 )
	ROM_LOAD( "wcdsxh__.5_0", 0x0000, 0x080000, CRC(a82d11de) SHA1(ece14fd5f56da8cc788c53d5c1404275e9000b65) )
	ROM_REGION( 0x080000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4excaln )
	ROM_REGION( 0x080000, "maincpu", 0 )
	ROM_LOAD( "exdsx___.6_0", 0x0000, 0x080000, CRC(fcdc703c) SHA1(927870723106aebbb2b492ce9bfebe4aa25d0325) )

	ROM_REGION( 0x080000, "altrevs", 0 )
	ROM_LOAD( "exdsx_e_.6_0", 0x0000, 0x080000, CRC(f6421feb) SHA1(5b3cf7fa4bf9711097ed1c9d2d5689329d73193d) )
	ROM_REGION( 0x080000, "msm6376", ROMREGION_ERASE00 )
ROM_END

ROM_START( m4vfm )
	ROM_REGION( 0x080000, "maincpu", 0 )
	ROM_LOAD( "v_f_mon", 0x0000, 0x020000, CRC(e4add02c) SHA1(5ef1bdd532ef0801b96ceae941f3da789039811c) )
	ROM_REGION( 0x080000, "msm6376", ROMREGION_ERASE00 )
ROM_END





// thanks to Project Amber for descramble information
void descramble_crystal( UINT8* region, int start, int end, UINT8 extra_xor)
{
	for (int i=start;i<end;i++)
	{
		UINT8 x = region[i];
		switch (i & 0x58)
		{
			case 0x00: // same as 0x08
			case 0x08: x = BITSWAP8( x^0xca , 3,2,1,0,7,4,6,5 ); break;
			case 0x10: x = BITSWAP8( x^0x30 , 3,0,4,6,1,5,7,2 ); break;
			case 0x18: x = BITSWAP8( x^0x89 , 4,1,2,5,7,0,6,3 ); break;
			case 0x40: x = BITSWAP8( x^0x14 , 6,1,4,3,2,5,0,7 ); break;
			case 0x48: x = BITSWAP8( x^0x40 , 1,0,3,2,5,4,7,6 ); break;
			case 0x50: x = BITSWAP8( x^0xcb , 3,2,1,0,7,6,5,4 ); break;
			case 0x58: x = BITSWAP8( x^0xc0 , 2,3,6,0,5,1,7,4 ); break;
		}
		region[i] = x ^ extra_xor;
	}
}


DRIVER_INIT( crystal )
{
	DRIVER_INIT_CALL(m_frkstn);
	descramble_crystal(machine.region( "maincpu" )->base(), 0x0000, 0x10000, 0x00);
}

DRIVER_INIT( crystali )
{
	DRIVER_INIT_CALL(m_frkstn);
	descramble_crystal(machine.region( "maincpu" )->base(), 0x0000, 0x10000, 0xff); // invert after decrypt?!
}


/* Barcrest */
GAME( 198?, m4tst,        0, mod2    ,   mpu4,       m4tst,   ROT0, "Barcrest","MPU4 Unit Test (Program 4)",GAME_MECHANICAL )
GAME( 198?, m4tst2,       0, mod2    ,   mpu4,       m4tst2,  ROT0, "Barcrest","MPU4 Unit Test (Program 2)",GAME_MECHANICAL )
GAME( 198?, m4clr,        0, mod2    ,   mpu4,       0,       ROT0, "Barcrest","MPU4 Meter Clear ROM",GAME_MECHANICAL )

/* I don't actually think all of these are Barcrest, some are mislabeled */
GAME(199?, m4tenten	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","10 X 10 (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )  // gives WRONG SOUND, NEEDS V1 (can be skipped with 1)
GAME(199?, m421club	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","21 Club (Barcrest) [DTW, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // not english
GAME(199?, m4actbnk	,0			,mod4oki	,mpu4jackpot8tkn	,m4default			,ROT0,   "Barcrest","Action Bank (Barcrest) [ACT] (MPU4)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK ) // set jackpot key to 8GBP TOKEN
GAME(199?, m4actclb	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Action Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK )
GAME(199?, m4actnot	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Action Note (Barcrest) [AN 1.2] (MPU4)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK )
GAME(199?, m4actpak	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Action Pack (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK )
GAME(199?, m4addrd	,m4addr		,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Adders & Ladders (Barcrest) [DAL, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4addr	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Adders & Ladders (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4addrc	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Adders & Ladders Classic (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4addrcc	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Adders & Ladders Classic Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4addrcb	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Adders & Ladders Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4alladv	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","All Cash Advance (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK )
GAME(199?, m4alpha	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Alphabet (Barcrest) [A4B 1.0] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4ambass	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Ambassador (Barcrest) [DAM, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4amhiwy	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","American Highway (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // crash mame
GAME(199?, m4andycpd,m4andycp	,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Andy Capp (Barcrest) [DAC, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4andycp	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Andy Capp (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4andyfl	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Andy Loves Flo (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4andybt	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Andy's Big Time Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4andyfh	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Andy's Full House (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4andyge	,0			,mod4oki	,grtecp				,m_grtecp			,ROT0,   "Barcrest","Andy's Great Escape (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4apach	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Apache (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4atlan	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Atlantis (Barcrest) [DAT, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bagtel	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Bagatelle (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bnknot	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Bank A Note (Barcrest) [BN 1.0] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bnkrol	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Bank Roller Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4btclok	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Beat The Clock (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4berser	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Berserk (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // crashes mame
GAME(199?, m4bigbn	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Big Ben (Barcrest) [DBB, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bigchfd,m4bigchf	,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Big Chief (Barcrest) [BCH, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // why code BCH on a dutch?
GAME(199?, m4bigchf	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Big Chief (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4blkwhd	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Black & White (Barcrest) [Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4blkbuld,m4blkbul	,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Black Bull (Barcrest) [Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4blkbul	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Black Bull (Barcrest) [XSP] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // XSP??
GAME(199?, m4blkcat	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Black Cat (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bj		,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Black Jack (Barcrest) [Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bjc	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Black Jack Club (Barcrest) [Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bja	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Blackjack (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bjac	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Blackjack Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bjack	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Black Jack (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bjsm	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Blackjack Super Multi (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4blstbk	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Blast A Bank (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bluedm	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Blue Diamond (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bluemn	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Blue Moon (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bdash	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Boulder Dash (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // gives WRONG SOUND, NEEDS V1 (can be skipped with 1)
GAME(199?, m4brktak	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Break & Take (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4brdway	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Broadway (Barcrest) [DBR, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4brook	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Brooklyn (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4buc	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Buccaneer (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bucks	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Bucks Fizz Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4calamab,m4calama	,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Calamari Club (Barcrest - Bwb) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4calama	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Calamari Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4calicl	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","California Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4cardcs	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Card Cash (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4cojok	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","Carry On Joker (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4cashat	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Cash Attack (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4cashcn	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Cash Connect (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4cashco	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Cash Counter (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4cashln	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Cash Lines (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // gives WRONG SOUND, NEEDS V1 (can be skipped with 1)
GAME(199?, m4cashmn	,0			,mod4oki	,mpu4jackpot8tkn	,m4default_bigbank	,ROT0,   "Barcrest","Cash Machine (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4cashmx	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Cash Matrix (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4cashzn	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Cash Zone (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4casmul	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Casino Multiplay (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // crashes mame
GAME(199?, m4celclb	,0			,mod2   	,mpu4				,m_ccelbr			,ROT0,   "Barcrest","Celebration Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4centpt	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Centrepoint (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4ceptr	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Ceptor (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4chasei	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Chase Invaders (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4cheryo	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Cherryo (Barcrest) [DCH, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4click	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Clickety Click (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4c999	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Cloud 999 (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4c9		,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Cloud Nine (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4c9c	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Cloud Nine Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4clbcls	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Club Classic (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4clbclm	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Club Climber (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4clbcnt	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Club Connect (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4clbdbl	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Club Double (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4clbshf	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","Club Shuffle (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4clbtro	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Club Tropicana (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4clbveg	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Club Vegas (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4clbx	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Club X (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4copcsh	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Coppa Cash (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4coscas	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Cosmic Casino (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4crkpot	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Crackpot Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4crzjk	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Crazy Jokers (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4crzjwl	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Crown Jewels (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4crjwl	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Crown Jewels Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4crjwl2	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Crown Jewels Mk II Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4crdome	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Crystal Dome (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4crmaze	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Crystal Maze (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4denmen	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Dennis The Menace (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4dbl9	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Double 9's (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4dbldmn	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Double Diamond Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4dblup	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Double Up (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4drac	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Dracula (Barcrest - Nova) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4dtyfre	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Duty Free (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4eighth	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Eighth Wonder (Barcrest) [WON 2.2] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4elite	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Elite (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4eaw	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Everyone's A Winner (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4exprs	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Express (Barcrest) [DXP, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4exgam	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Extra Game (Fairplay - Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4fastfw	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Fast Forward (Barcrest - Bwb) [FFD 1.0] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4class	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","First Class (Barcrest) [DFC, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4flash	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Flash Cash (Barcrest) [FC 1.0] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4fortcb	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Fortune Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4frtlt	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Fruit & Loot (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4frtfl	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Fruit Full (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4frtflc	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Fruit Full Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4frtgm	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Fruit Game (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4frtlnk	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Fruit Link Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4frtprs	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Fruit Preserve (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAMEL(198?,m4gambal	,0			,mod4yam	,gamball			,m_gmball			,ROT0,   "Barcrest","Gamball (Barcrest) (MPU4)",							GAME_REQUIRES_ARTWORK|GAME_MECHANICAL,layout_gamball )//Mechanical ball launcher
GAME(199?, m4gb006	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Games Bond 006 (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4gbust	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Ghost Buster (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4giant	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Giant (Barcrest) [DGI, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4gclue	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Give Us A Clue (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4gldstr	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Gold Strike (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4gldgat	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Golden Gate (Barcrest) [DGG, Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4gldjok	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Golden Joker (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // dutch?
GAME(199?, m4gldnud	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Golden Nudger (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4grbbnk	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Grab The Bank (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4graff	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","Graffiti (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4graffd	,m4graff	,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","Grafitti (Barcrest) [Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4grands	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Grandstand Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4gnsmk	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Gun Smoke (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4hpyjok	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Happy Joker (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4hijinx	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Hi Jinx (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4hirise	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","High Rise (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4hiroll	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","High Roller (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4hittop	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Hit The Top (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4thehit	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","The Hit (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4holdon	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Hold On (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4holdtm	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Hold Timer (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4hotrod	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Hot Rod (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4hypvip	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Hyper Viper (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4hypclb	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Hyper Viper Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4intcep	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Interceptor (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4jpgem	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Jackpot Gems (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4jpgemc	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Jackpot Gems Classic (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4jpjmp	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Jackpot Jump (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4jwlcwn	,0			,mod4oki	,mpu4jackpot8tkn	,m4default_bigbank	,ROT0,   "Barcrest","Jewel In the Crown (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4jok300	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Jokers 300 (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4jokmil	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Jokers Millennium (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4jolgem	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Jolly Gems (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )  // gives WRONG SOUND, NEEDS V1 (can be skipped with 1) (hangs)
GAME(199?, m4joljok	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Jolly Joker (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4joljokd,m4joljok	,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Jolly Joker (Barcrest) [Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4joljokh,m4joljok	,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Jolly Joker (Barcrest) [Hungarian] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4joltav	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Jolly Taverner (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4lastrp	,0			,mod4oki	,mpu4jackpot8tkn	,m4default_bigbank	,ROT0,   "Barcrest","Las Vegas Strip (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4goodtm	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Let The Good Times Roll (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4libty	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Liberty (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4lineup	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Line Up (Bwb - Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // no sound with any system?
GAME(199?, m4loadmn	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Loads A Money (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4luck7	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Lucky 7 (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4luckdv	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Lucky Devil (Barcrest) [Czech] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4luckdvd,m4luckdv	,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Lucky Devil (Barcrest) [Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4lucklv	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Lucky Las Vegas (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4luckst	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Lucky Strike (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4lucksc	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Lucky Strike Club (Barcrest) [MPU 4] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4luckwb	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Lucky Wild Boar (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4luxor	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Luxor (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4madhse	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Mad House (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4magdrg	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Magic Dragon (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4maglin	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Magic Liner (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4magrep	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Magic Replay DeLuxe (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4magtbo	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Magic Turbo  (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4mag7s	,0			,mod4oki	,mpu4jackpot8per	,m4default_bigbank	,ROT0,   "Barcrest","Magnificent 7's (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4makmnt	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Make A Mint (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4megbks	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Mega Bucks (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4meglnk	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Megalink (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4milclb	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Millionaire's Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4mirage	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Mirage (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4moneym	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Money Maker (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4monte	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","Monte Carlo (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4multcl	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Multiplay Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4multwy	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Multiway (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4nhtt	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","New Hit the Top (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4nick	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Nickelodeon (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4nifty	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Nifty Fifty (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4nspot	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Night Spot Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4nile	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Nile Jewels (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4nudgew	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","Nudge A Win (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // no sound with any system?
GAME(199?, m4nudbnk	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","Nudge Banker (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4nnww	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Nudge Nudge Wink Wink (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4nnwwc	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Nudge Nudge Wink Wink Classic (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4nudqst	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Nudge Quest (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4nudshf	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Nudge Shuffle (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4nudup	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Nudge Up (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4num1	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Number One (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4oldtmr	,0			,mod4oki	,mpu4				,m_oldtmr			,ROT0,   "Barcrest","Old Timer (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4casot	,m4oldtmr	,mod4oki	,mpu4				,m_oldtmr			,ROT0,   "Barcrest","Casino Old Timer (Old Timer Sound hack?) (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // uses the same program???
GAME(199?, m4blkwht	,m4oldtmr	,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Black & White (Old Timer Sound hack?) (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // uses the same program???
GAME(199?, m4jpmcla	,m4oldtmr	,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","JPM Classic (Old Timer Sound hack?) (Barcrest) (MPU4)",							GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // uses the same program???
GAME(199?, m4omega	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Omega (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4ordmnd	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Oriental Diamonds (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4overmn	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Over The Moon (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4placbt	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Place Your Bets (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4pont	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Pontoon Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4potblk	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Pot Black (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4ptblkc	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Pot Black Casino (Bwb - Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // main cpu crashes?
GAME(199?, m4potlck	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Pot Luck Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4prem	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Premier (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przdty	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Prize Duty Free (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przfrt	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Prize Fruit & Loot (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przhr	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Prize High Roller (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przlux	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Prize Luxor (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przmon	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Prize Money (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przmns	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Prize Money ShowCase (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przmc	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","Prize Monte Carlo (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przrf	,0			,mod2   	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Prize Rich And Famous (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przrfm	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Prize Run For Your Money (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przsss	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","Prize Spend Spend Spend (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przve	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Prize Viva Esapana (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przwo	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Prize What's On (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4przwta	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Prize Winner Takes All (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4randr	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Random Roulette (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rsg	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Ready Steady Go (Barcrest) (type 1) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rgsa	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Ready Steady Go (Barcrest) (type 2) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4ra		,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","Red Alert (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rdht	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Red Heat (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rhr	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Red Hot Roll (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rhrc	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Red Hot Roll Classic (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rhrcl	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Red Hot Roll Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rwb	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Red White & Blue (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4r2r	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Reel 2 Reel (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rmtp	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Reel Magic Turbo Play (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rmtpd	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Reel Magic Turbo Play Deluxe (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4reelpk	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Reel Poker (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4reeltm	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Reel Timer (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4richfm	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Rich & Famous (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4ringfr	,0			,mod4oki    ,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Ring Of Fire (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rhog	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Road Hog (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rhog2	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Road Hog 2 - I'm Back (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rhogc	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Road Hog Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4roadrn	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Road Runner (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rockmn	,0			,mod4oki	,mpu4jackpot8tkn	,m4default_bigbank	,ROT0,   "Barcrest","Rocket Money (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4royjwl	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Royal Jewels (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4rfym	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Run For Your Money (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4runawy	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Runaway Trail (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4salsa	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Salsa (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4samu	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Samurai (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4sayno	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Say No More (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4showtm	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Show Timer (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4shocm	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Showcase Crystal Maze (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4shodf	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Showcase Duty Free (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4silshd	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Silver Shadow (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4sgrab	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Smash 'n' Grab (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4solsil	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Solid Silver Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4sss	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","Spend Spend Spend (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(1999, m4squid	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Squids In (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4stakeu	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Stake Up Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4str300	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Star Play 300 (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4stards	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Stardust (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4starbr	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Stars And Bars (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4steptm	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Step Timer (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4stopcl	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Stop the Clock (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4sunset	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Sunset Boulevard (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4supslt	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Supa Slot (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4suptrn	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Supatron (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4supbj	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Super Blackjack (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4supbjc	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Super Blackjack Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4supbf	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Super Bucks Fizz Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4suphv	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Super Hyper Viper (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4supst	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Super Streak (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4suptub	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Super Tubes (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4suptwo	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Super Two (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4taj	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Taj Mahal (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4take5	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Take 5 (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4take2	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Take Two (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4takepk	,0			,mod4oki	,mpu4jackpot8per	,m4default_bigbank	,ROT0,   "Barcrest","Take Your Pick (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4tpcl	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Take Your Pick Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4techno	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Techno Reel (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4toot	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Ten Out Of Ten (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4ttdia	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Ten Ten Do It Again (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4tiktak	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Tic Tak Cash (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4toma	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Tomahawk (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4topact	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Top Action (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4topdk	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Top Deck (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4topgr	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Top Gear (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4toprn	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Top Run (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4topst	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Top Stop (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4toptak	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Top Take (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4topten	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Top Tenner (Barcrest) (type 1) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4toptena,m4topten	,mod2		,mpu4				,m4default			,ROT0,   "Barcrest","Top Tenner (Barcrest) (type 2) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4toplot	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Top The Lot (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4toptim	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Top Timer (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4tricol	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Tricolor (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4tribnk	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Triple Bank (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4tridic	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Triple Dice (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4tropcl	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Tropicana Club (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4tupen	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Tuppenny Cracker (Barcrest - Bootleg) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4tbplay	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Turbo Play (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4tbreel	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Turbo Reel (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4tbrldx	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Turbo Reel Deluxe (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4tutfrt	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Tutti Fruity (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4tutcl	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Tutti Fruity Classic (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m421		,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Twenty One (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4twilgt	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Twilight (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4twintm	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Twin Timer (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4twist	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Twist Again (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4univ	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Universe (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4uuaw	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Up Up & Away (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4vegast	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Vegas Strip (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4vegastg,m4vegast	,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Barcrest","Vegas Strip (Barcrest) [German] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4vivaes	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Viva Espana (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4vivess	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Viva Espana Showcase (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4vivalvd,m4vivalv	,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Viva Las Vegas (Barcrest) [Dutch] (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4vivalv	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Viva Las Vegas (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4vivasx	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Barcrest","Viva Las Vegas Six (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4viz	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Viz (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4voodoo	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Voodoo 1000 (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4wayin	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Way In (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4whaton	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","What's On (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4wildms	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Wild Mystery (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4wildtm	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Wild Timer (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4wta	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Winner Takes All (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4ch30	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Barcrest","Unknown MPU4 'CH3 0.1' (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4sb5	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Unknown MPU4 'BSB 0.3' (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4stc	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Barcrest","Unknown MPU4 'STC 0.1' (Barcrest) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )

/* Bwb */

/* are all these really MPU4 hw? , check things like Daytona, doesn't boot at all. */
GAME(199?, m4acechs	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Bwb","Ace Chase (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigmt	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","The Big Match (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bingbl	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","Bingo Belle (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bingbs	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","Bingo Belle Showcase (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bingcl	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","Bingo Club (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4blflsh	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","Blue Flash (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4blsbys	,0			,bwboki		,mpu4				,m_blsbys			,ROT0,   "Bwb","Blues Boys (Bwb) (MPU4)",			GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK )
GAME(199?, m4bluesn	,0			,bwboki		,mpu4				,m_blsbys			,ROT0,	 "Nova","Blues Boys (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL) // German version, still has BWB strings, crashes during boot, but boots by chance the first time?
GAME(199?, m4cshenc	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Cash Encounters (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4czne	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","Cash Zone (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4csoc	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Championship Soccer (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4cpycat	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Copy Cat (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4cpfinl	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Cup Final (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4danced	,0			,mod4oki    ,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Dancing Diamonds (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4daytn	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Daytona (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4excal	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Excalibur (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4exotic	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Exotic Fruits (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4firice	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","Fire & Ice (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4flshlt	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","Flashlite (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4fourmr	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Bwb","Four More (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // no sound with either system?
GAME(199?, m4harle	,0			,mod4oki	,mpu4				,m4default	        ,ROT0,   "Bwb","Harlequin (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hvhel	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Heaven & Hell (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4holywd	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","Hollywood (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4indycr	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Indy Cars (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4jakjok	,0			,mod4oki    ,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Jackpot Jokers (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4jflash	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Jumping Jack Flash (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4kingq	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Kings & Queens (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4kingqc	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Kings & Queens Classic (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4lazy	,0			,mod4oki    ,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Lazy Bones (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4lvlcl	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Lucky Las Vegas Classic (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4ln7	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Lucky No7 (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4madmon	,0			,mod4oki    ,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Mad Money (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4madmnc	,0			,mod4oki    ,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Mad Money Classic (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4mmm	,0			,mod4oki    ,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Money Mummy Money (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4oadrac	,0			,mod4oki    ,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Ooh Aah Dracula (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4orland	,0			,mod4oki    ,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Orlando Magic (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4pzbing	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","Prize Bingo (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4quidin	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Quids In (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4quidis	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Quids In Showcase (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4rackem	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Rack Em Up (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4rbgold	,0			,mod2   	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Rainbow Gold (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4rhfev	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Red Hot Fever (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4rhs	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Rocky Horror Show (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4sinbd	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Sinbad (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4sky	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Sky Sports (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4souls	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Soul Sister (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4specu	,0			,mod4yam	,mpu4				,m4default			,ROT0,   "Bwb","Speculator Club (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // no sound with either system
GAME(199?, m4spinbt	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Spin The Bottle (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4starst	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Stars & Stripes (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4thestr	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","The Streak (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4sunclb	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","Sun Club (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4sunscl	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","Sunset Club (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4supleg	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Super League (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4supscr	,0			,mod4oki    ,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Super Soccer (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4ssclas	,0			,mod4oki    ,mpu4				,m4default			,ROT0,   "Bwb","Super Streak Classic (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4sure	,0			,mod2   	,mpu4				,m4default			,ROT0,   "Bwb","Sure Thing (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4tic	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Tic Tac Toe (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4ticcla	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Tic Tac Toe Classic (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4ticgld	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Tic Tac Toe Gold (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4ticglc	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Tic Tac Toe Gold Classic (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4topdog	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Top Dog (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4trex	,0			,mod4oki	,mpu4				,m4default			,ROT0,   "Bwb","Trex (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4volcan	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Volcano (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4vdexpr	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","Voodoo Express (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4xch	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","X-change (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4xs		,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","X-s (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4xtrm	,0			,mod4oki	,mpu4				,m4default_bigbank	,ROT0,   "Bwb","X-treme (Bwb) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )


/* Empire
   most of these boot (after a single reset to initialize)
   but have broken text, I guess due to the characterizer (protection) */

GAME(199?, m4apachg,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Apache Gold (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4apachga, m4apachg,	mod4oki, mpu4, m4default, ROT0,   "Empire","Apache Gold (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4apachgb, m4apachg,	mod4oki, mpu4, m4default, ROT0,   "Empire","Apache Gold (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4apachgc, m4apachg,	mod4oki, mpu4, m4default, ROT0,   "Empire","Apache Gold (Empire) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4apachgd, m4apachg,	mod4oki, mpu4, m4default, ROT0,   "Empire","Apache Gold (Empire) (MPU4, set 5)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4apachge, m4apachg,	mod4oki, mpu4, m4default, ROT0,   "Empire","Apache Gold (Empire) (MPU4, set 6)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4apachgf, m4apachg,	mod4oki, mpu4, m4default, ROT0,   "Empire","Apache Gold (Empire) (MPU4, set 7)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bangrs,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Bangers 'n' Cash (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bangrsa, m4bangrs,	mod4oki, mpu4, m4default, ROT0,   "Empire","Bangers 'n' Cash (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bangrsb, m4bangrs,	mod4oki, mpu4, m4default, ROT0,   "Empire","Bangers 'n' Cash (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bankrd,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Bank Raid (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bankrda, m4bankrd,	mod4oki, mpu4, m4default, ROT0,   "Empire","Bank Raid (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bankrdb, m4bankrd,	mod4oki, mpu4, m4default, ROT0,   "Empire","Bank Raid (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bankrdc, m4bankrd,	mod4oki, mpu4, m4default, ROT0,   "Empire","Bank Raid (Empire) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bankrdd, m4bankrd,	mod4oki, mpu4, m4default, ROT0,   "Empire","Bank Raid (Empire) (MPU4, set 5)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigchs,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Big Cheese (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigchsa, m4bigchs,	mod4oki, mpu4, m4default, ROT0,   "Empire","Big Cheese (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigchsb, m4bigchs,	mod4oki, mpu4, m4default, ROT0,   "Empire","Big Cheese (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4cstrik,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Cash Strike (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4cstrika, m4cstrik,	mod4oki, mpu4, m4default, ROT0,   "Empire","Cash Strike (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4cstrikb, m4cstrik,	mod4oki, mpu4, m4default, ROT0,   "Empire","Cash Strike (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4cstrikc, m4cstrik,	mod4oki, mpu4, m4default, ROT0,   "Empire","Cash Strike (Empire) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4chacec,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Chase The Ace [Cards] (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4chaceca, m4chacec,	mod4oki, mpu4, m4default, ROT0,   "Empire","Chase The Ace [Cards] (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4chacef,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Chase The Ace [Fruits] (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4chacefa, m4chacef,	mod4oki, mpu4, m4default, ROT0,   "Empire","Chase The Ace [Fruits] (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4chacefb, m4chacef,	mod4oki, mpu4, m4default, ROT0,   "Empire","Chase The Ace [Fruits] (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4chacefc, m4chacef,	mod4oki, mpu4, m4default, ROT0,   "Empire","Chase The Ace [Fruits] (Empire) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4crzcap,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Crazy Capers (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4crzcapa, m4crzcap,	mod4oki, mpu4, m4default, ROT0,   "Empire","Crazy Capers (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4crzcapb, m4crzcap,	mod4oki, mpu4, m4default, ROT0,   "Empire","Crazy Capers (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4crzcapc, m4crzcap,	mod4oki, mpu4, m4default, ROT0,   "Empire","Crazy Capers (Empire) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4crfire,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Crossfire (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // doesn't boot, alarm
GAME(199?, m4crfirea, m4crfire,	mod4oki, mpu4, m4default, ROT0,   "Empire","Crossfire (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // doesn't boot, alarm
GAME(199?, m4eureka,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Eureka (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4eurekaa, m4eureka,	mod4oki, mpu4, m4default, ROT0,   "Empire","Eureka (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4eurekab, m4eureka,	mod4oki, mpu4, m4default, ROT0,   "Empire","Eureka (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4fright,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Fright Night (Empire) (MPU4, v4.1X)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4frighta, m4fright,	mod4oki, mpu4, m4default, ROT0,   "Empire","Fright Night (Empire) (MPU4, v4.1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4frightb, m4fright,	mod4oki, mpu4, m4default, ROT0,   "Empire","Fright Night (Empire) (MPU4, v4.1i)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4frightc, m4fright,	mod4oki, mpu4, m4default, ROT0,   "Empire","Fright Night (Empire) (MPU4, v?.?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // bad dump
GAME(199?, m4frightd, m4fright,	mod4oki, mpu4, m4default, ROT0,   "Empire","Fright Night (Empire) (MPU4, v3.3)",GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4frighte, m4fright,	mod4oki, mpu4, m4default, ROT0,   "Empire","Fright Night (Empire) (MPU4, v3.0)",GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gamblr,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","The Gambler (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gamblra, m4gamblr, mod4oki, mpu4, m4default, ROT0,   "Empire","The Gambler (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gamblrb, m4gamblr, mod4oki, mpu4, m4default, ROT0,   "Empire","The Gambler (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gtrain,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Ghost Train (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gtraina, m4gtrain,	mod4oki, mpu4, m4default, ROT0,   "Empire","Ghost Train (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gtrainb, m4gtrain,	mod4oki, mpu4, m4default, ROT0,   "Empire","Ghost Train (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gtrainc, m4gtrain,	mod4oki, mpu4, m4default, ROT0,   "Empire","Ghost Train (Empire) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4goldfv,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Gold Fever (Empire) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4haunt,   0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Haunted House (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4haunta,  m4haunt,	mod4oki, mpu4, m4default, ROT0,   "Empire","Haunted House (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hauntb,  m4haunt,	mod4oki, mpu4, m4default, ROT0,   "Empire","Haunted House (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hauntc,  m4haunt,	mod4oki, mpu4, m4default, ROT0,   "Empire","Haunted House (Empire) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hauntd,  m4haunt,	mod4oki, mpu4, m4default, ROT0,   "Empire","Haunted House (Empire) (MPU4, set 5)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4haunte,  m4haunt,	mod4oki, mpu4, m4default, ROT0,   "Empire","Haunted House (Empire) (MPU4, set 6)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hisprt,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","High Spirits (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hisprta, m4hisprt,	mod4oki, mpu4, m4default, ROT0,   "Empire","High Spirits (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hisprtb, m4hisprt,	mod4oki, mpu4, m4default, ROT0,   "Empire","High Spirits (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hisprtc, m4hisprt,	mod4oki, mpu4, m4default, ROT0,   "Empire","High Spirits (Empire) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hisprtd, m4hisprt,	mod4oki, mpu4, m4default, ROT0,   "Empire","High Spirits (Empire) (MPU4, set 5)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hisprte, m4hisprt,	mod4oki, mpu4, m4default, ROT0,   "Empire","High Spirits (Empire) (MPU4, set 6)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hotcsh,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Hot Cash (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hotcsha, m4hotcsh, mod4oki, mpu4, m4default, ROT0,   "Empire","Hot Cash (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hotcshb, m4hotcsh,	mod4oki, mpu4, m4default, ROT0,   "Empire","Hot Cash (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4monspn,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Money Spinner (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4monspna, m4monspn,	mod4oki, mpu4, m4default, ROT0,   "Empire","Money Spinner (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4monspnb, m4monspn,	mod4oki, mpu4, m4default, ROT0,   "Empire","Money Spinner (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4pbnudg,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Pinball Nudger (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4pbnudga, m4pbnudg,	mod4oki, mpu4, m4default, ROT0,   "Empire","Pinball Nudger (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4pbnudgb, m4pbnudg,	mod4oki, mpu4, m4default, ROT0,   "Empire","Pinball Nudger (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4pitfal,  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","Pitfall (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // doesn't boot, alarm
GAME(199?, m4pitfala, m4pitfal,	mod4oki, mpu4, m4default, ROT0,   "Empire","Pitfall (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // doesn't boot, alarm
GAME(199?, m4pitfalb, m4pitfal,	mod4oki, mpu4, m4default, ROT0,   "Empire","Pitfall (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // doesn't boot, alarm
GAME(199?, m4pitfalc, m4pitfal,	mod4oki, mpu4, m4default, ROT0,   "Empire","Pitfall (Empire) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // doesn't boot, alarm
GAME(199?, m4ttrail, 0,			mod4oki, mpu4, m4default, ROT0,   "Empire","Treasure Trail (Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4ttraila,m4ttrail,	mod4oki, mpu4, m4default, ROT0,   "Empire","Treasure Trail (Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4ttrailb,m4ttrail,	mod4oki, mpu4, m4default, ROT0,   "Empire","Treasure Trail (Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
// doesn't seem like the other Empire games (starts with RESETTING JNE, licensed, mislabeled?)
GAME(199?, m4jne,	  0,		mod4oki, mpu4, m4default, ROT0,   "Empire","The Jackpot's Not Enough (Empire) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )

/* MDM
   most of these boot and act similar to the Empire games (ie bad text, but run OK) */
GAME(199?, m42punlm,     0,		mod4oki, mpu4, m4default, ROT0,   "Mdm","2p Unlimited (Mdm) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4silnud,     0,		mod4oki, mpu4, m4default, ROT0,   "Mdm?","Silver Nudger (Mdm?) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // code is close to 2p Unlimited, same sound rom
GAME(199?, m4nud2p,     0,		mod4oki, mpu4, m4default, ROT0,   "Mdm?","2p Nudger (Mdm?) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // code is close to 2p Unlimited, same sound rom
GAME(199?, m4ctn,     0,		mod4oki, mpu4, m4default, ROT0,   "Mdm?","Tuppenny Nudger Classic (Mdm?) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // code is close to 2p Unlimited, same sound rom
GAME(199?, m4bigapl,  0,		mod4oki, mpu4, m4default, ROT0,   "Mdm","Big Apple (Mdm) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigapla, m4bigapl,	mod4oki, mpu4, m4default, ROT0,   "Mdm","Big Apple (Mdm) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigaplb, m4bigapl,	mod4oki, mpu4, m4default, ROT0,   "Mdm","Big Apple (Mdm) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigaplc, m4bigapl,	mod4oki, mpu4, m4default, ROT0,   "Mdm","Big Apple (Mdm) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigapld, m4bigapl,	mod4oki, mpu4, m4default, ROT0,   "Mdm","Big Apple (Mdm) (MPU4, set 5)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigaple, m4bigapl,	mod4oki, mpu4, m4default, ROT0,   "Mdm","Big Apple (Mdm) (MPU4, set 6)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4blztrl,  0,		mod4oki, mpu4, m4default, ROT0,   "Mdm","Blazing Trails (Mdm) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4blztrla, m4blztrl,	mod4oki, mpu4, m4default, ROT0,   "Mdm","Blazing Trails (Mdm) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bodymt,  0,		mod4oki, mpu4, m4default, ROT0,   "Mdm","Body Match (Mdm) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // doesn't boot, various alarms
GAME(199?, m4coloss,  0,		mod4oki, mpu4, m4default, ROT0,   "Mdm","Colossus (Mdm) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4colossa, m4coloss, mod4oki, mpu4, m4default, ROT0,   "Mdm","Colossus (Mdm) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4colossb, m4coloss, mod4oki, mpu4, m4default, ROT0,   "Mdm","Colossus (Mdm) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4colossc, m4coloss, mod4oki, mpu4, m4default, ROT0,   "Mdm","Colossus (Mdm) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4colossd, m4coloss, mod4oki, mpu4, m4default, ROT0,   "Mdm","Colossus (Mdm) (MPU4, set 5)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4colosse, m4coloss, mod4oki, mpu4, m4default, ROT0,   "Mdm","Colossus (Mdm) (MPU4, set 6)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4colossf, m4coloss, mod4oki, mpu4, m4default, ROT0,   "Mdm","Colossus (Mdm) (MPU4, set 7)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4colossg, m4coloss, mod4oki, mpu4, m4default, ROT0,   "Mdm","Colossus (Mdm) (MPU4, set 8)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4firebl,  0,		mod2    ,mpu4, m4default, ROT0,   "Mdm","Fireball (Mdm) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // hangs after spin (sound status?)
GAME(199?, m4firebla, m4firebl,	mod2    ,mpu4, m4default, ROT0,   "Mdm","Fireball (Mdm) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // hangs after spin (sound status?)
GAME(199?, m4fireblb, m4firebl,	mod2    ,mpu4, m4default, ROT0,   "Mdm","Fireball (Mdm) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // hangs after spin (sound status?)
GAME(199?, m4fireblc, m4firebl,	mod2    ,mpu4, m4default, ROT0,   "Mdm","Fireball (Mdm) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // hangs after spin (sound status?)
GAME(199?, m4firebld, m4firebl,	mod2    ,mpu4, m4default, ROT0,   "Mdm","Fireball (Mdm) (MPU4, set 5)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // hangs after spin (sound status?)
GAME(199?, m4mayhem,  0,		mod4oki, mpu4, m4default, ROT0,   "Mdm","Mayhem (Mdm) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4mayhema, m4mayhem,	mod4oki, mpu4, m4default, ROT0,   "Mdm","Mayhem (Mdm) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4themob,  0,		mod4oki, mpu4, m4default, ROT0,   "Mdm","The Mob (Mdm) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4themoba, m4themob, mod4oki, mpu4, m4default, ROT0,   "Mdm","The Mob (Mdm) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4themobb, m4themob, mod4oki, mpu4, m4default, ROT0,   "Mdm","The Mob (Mdm) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4nudbon,  0,		mod2    ,mpu4, m4default, ROT0,   "Mdm","Nudge Bonanza (Mdm) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4nudbona, m4nudbon,	mod2    ,mpu4, m4default, ROT0,   "Mdm","Nudge Bonanza (Mdm) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4nudgem,  0,		mod4oki, mpu4, m4default, ROT0,   "Mdm","Nudge Gems (Mdm) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4smshgb, 0,			mod4oki, mpu4, m4default, ROT0,   "Mdm","Smash 'n' Grab (Mdm) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4smshgba,m4smshgb,	mod4oki, mpu4, m4default, ROT0,   "Mdm","Smash 'n' Grab (Mdm) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4smshgbb,m4smshgb,	mod4oki, mpu4, m4default, ROT0,   "Mdm","Smash 'n' Grab (Mdm) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4smshgbc,m4smshgb,	mod4oki, mpu4, m4default, ROT0,   "Mdm","Smash 'n' Grab (Mdm) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4snklad, 0,			mod2    ,mpu4, m4default, ROT0,   "Mdm","Snakes & Ladders (Mdm) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )

/* Pcp */

GAME(199?, m4cshino	,0,			mod2    ,mpu4, m4default, ROT0,   "Pcp","Cashino Deluxe (Pcp) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4jjc	,0,			mod2    ,mpu4, m4default, ROT0,   "Pcp","Jumping Jack Cash (Pcp) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4spton	,0,			mod2    ,mpu4, m4default, ROT0,   "Pcp","Spot On (Pcp) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4clbrpl	,0,			mod2	,mpu4, m4default, ROT0,   "Pcp","Club Replay (PCP) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK ) // was also marked as a barcrest set?
GAME(199?, m4exlin	,0,			mod2    ,mpu4, m4default, ROT0,   "Pcp","Extra Lines (Pcp) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4supjst	,0,			mod2    ,mpu4, m4default, ROT0,   "Pcp","Super Jester (Pcp) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )

/* Nova - German licensed Barcrest / Bwb sets? */

GAME(199?, m4bigban	,0			,mod4oki    	,mpu4				,m4default			,ROT0,   "Nova","Big Bandit (Nova) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4crzcsn	,0			,mod4oki    	,mpu4				,m4default			,ROT0,   "Nova","Crazy Casino (Nova) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4crzcav	,0			,mod4oki    	,mpu4				,m4default			,ROT0,   "Nova","Crazy Cavern (Nova) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4dragon	,0			,mod4oki    	,mpu4				,m4default			,ROT0,   "Nova","Dragon (Nova) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4hilonv	,0			,mod4oki    	,mpu4				,m4default			,ROT0,   "Nova","Hi Lo Casino (Nova) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4octo	,0			,mod4oki    	,mpu4				,m4default			,ROT0,   "Nova","Octopus (Nova) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )
GAME(199?, m4sctagt	,0			,mod4oki    	,mpu4				,m4default			,ROT0,   "Nova","Secret Agent (Nova) (MPU4)",						GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK )

/* Union
  these don't boot, at best you get a 'CLEAR' message */
GAME(199?, m4cwalk,   0,		mod4oki, mpu4, m4default, ROT0,   "Union","Cake Walk (Union) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4eezee,   0,		mod4oki, mpu4, m4default, ROT0,   "Union","Eezee Fruits (Union) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4frdrop,  0,		mod4oki, mpu4, m4default, ROT0,   "Union","Fruit Drop (Union) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gobana,  0,		mod2    ,mpu4, m4default, ROT0,   "Union","Go Bananas (Union) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gobanaa, m4gobana,	mod2    ,mpu4, m4default, ROT0,   "Union","Go Bananas (Union) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gobanab, m4gobana,	mod2    ,mpu4, m4default, ROT0,   "Union","Go Bananas (Union) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gobanac, m4gobana,	mod2    ,mpu4, m4default, ROT0,   "Union","Go Bananas (Union) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gobanad, m4gobana,	mod2    ,mpu4, m4default, ROT0,   "Union","Go Bananas (Union) (MPU4, set 5)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4lotty,	  0,		mod2    ,mpu4, m4default, ROT0,   "Union","Lotty Time (Union) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4maxmze,  0,		mod2    ,mpu4, m4default, ROT0,   "Union","Maximize (Union) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4maxmzea, m4maxmze,	mod2    ,mpu4, m4default, ROT0,   "Union","Maximize (Union) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4maxmzeb, m4maxmze,	mod2    ,mpu4, m4default, ROT0,   "Union","Maximize (Union) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4maxmzec, m4maxmze,	mod2    ,mpu4, m4default, ROT0,   "Union","Maximize (Union) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4maxmzed, m4maxmze,	mod2    ,mpu4, m4default, ROT0,   "Union","Maximize (Union) (MPU4, set 5)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4mecca,   0,		mod2    ,mpu4, m4default, ROT0,   "Union","Mecca Money (Union) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4purmad,  0,		mod4oki, mpu4, m4default, ROT0,   "Union","Pure Madness (Union)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4revolv,  0,		mod4oki, mpu4, m4default, ROT0,   "Union","Revolva (Union) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4rotex,  0,			mod4oki, mpu4, m4default, ROT0,   "Union","Rotex (Union) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4select, 0,			mod4oki, mpu4, m4default, ROT0,   "Union","Select (Union) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4supfru, 0,			mod4oki, mpu4, m4default, ROT0,   "Union","Supafruits (Union) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4supfrua,m4supfru,	mod4oki, mpu4, m4default, ROT0,   "Union","Supafruits (Union) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4trimad, 0,			mod4oki, mpu4, m4default, ROT0,   "Union","Triple Madness (Union) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4unibox, 0,			mod4oki, mpu4, m4default, ROT0,   "Union","Unibox (Union) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4uniboxa,m4unibox,	mod4oki, mpu4, m4default, ROT0,   "Union","Unibox (Union) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4unique, 0,			mod4oki, mpu4, m4default, ROT0,   "Union","Unique (Union) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4uniquep,m4unique,	mod4oki, mpu4, m4default, ROT0,   "Union","Unique (Union) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4crzbn,  0,			mod4oki, mpu4, m4default, ROT0,   "Union","Crazy Bingo (Union) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )



/* Union + Empire
   same as Union above */
GAME(199?, m4gvibes,  0,		mod4oki, mpu4, m4default, ROT0,   "Union / Empire","Good Vibrations (Union - Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4gvibesa, m4gvibes,	mod4oki, mpu4, m4default, ROT0,   "Union / Empire","Good Vibrations (Union - Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4rckrol,  0,		mod4oki, mpu4, m4default, ROT0,   "Union / Empire","Rock 'n' Roll (Union - Empire) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4rckrola, m4rckrol,	mod4oki, mpu4, m4default, ROT0,   "Union / Empire","Rock 'n' Roll (Union - Empire) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4rckrolb, m4rckrol,	mod4oki, mpu4, m4default, ROT0,   "Union / Empire","Rock 'n' Roll (Union - Empire) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )

/* Others */

GAME(199?, m4aao,     0,		mod4oki, mpu4, m4default, ROT0,   "Eurotek","Against All Odds (Eurotek) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bandgd,  0,		mod4oki, mpu4, m4default, ROT0,   "Eurogames","Bands Of Gold (Eurogames) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bangin,  0,		mod4oki, mpu4, m4default, ROT0,   "Global","Bangin' Away (Global) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bangina, m4bangin,	mod4oki, mpu4, m4default, ROT0,   "Global","Bangin' Away (Global) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4banginb, m4bangin,	mod4oki, mpu4, m4default, ROT0,   "Global","Bangin' Away (Global) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4wwc,	  0,		mod4oki, mpu4, m4default, ROT0,   "Global","Wacky Weekend Club (Global) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4screw,	  0,		mod4oki, mpu4, m4default, ROT0,   "Global","Screwin' Around (Global) (MPU4, v0.8)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4screwp,  m4screw,	mod4oki, mpu4, m4default, ROT0,   "Global","Screwin' Around (Global) (MPU4, v0.8) (Protocol)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4screwa,  m4screw,	mod4oki, mpu4, m4default, ROT0,   "Global","Screwin' Around (Global) (MPU4, v0.7)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4screwb,  m4screw,	mod4oki, mpu4, m4default, ROT0,   "Global","Screwin' Around (Global) (MPU4, v0.5)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4vfm,	  0,		mod4oki, mpu4, m4default, ROT0,   "Global","Value For Money (Global) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigben,  0,		mod4oki, mpu4, m4default, ROT0,   "Coinworld","Big Ben (Coinworld) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigbena, m4bigben,	mod4oki, mpu4, m4default, ROT0,   "Coinworld","Big Ben (Coinworld) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigbenb, m4bigben,	mod4oki, mpu4, m4default, ROT0,   "Coinworld","Big Ben (Coinworld) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigbenc, m4bigben,	mod4oki, mpu4, m4default, ROT0,   "Coinworld","Big Ben (Coinworld) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigbend, m4bigben,	mod4oki, mpu4, m4default, ROT0,   "Coinworld","Big Ben (Coinworld) (MPU4, set 5)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bigbene, m4bigben,	mod4oki, mpu4, m4default, ROT0,   "Coinworld","Big Ben (Coinworld) (MPU4, set 6)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4kqclub,  0,		mod2    ,mpu4, m4default, ROT0,   "Newby","Kings & Queens Club (Newby) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4snookr,  0,		mod2    ,mpu4, m4default, ROT0,   "Eurocoin","Snooker (Eurocoin) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // works?
GAME(199?, m4spnwin,  0,		mod2    ,mpu4, m4default, ROT0,   "Cotswold Microsystems","Spin A Win (Cotswold Microsystems) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // works?
GAME(199?, m4stakex,  0,		mod4oki, mpu4, m4default, ROT0,   "Leisurama","Stake X (Leisurama) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // can't coin, no sound
GAME(199?, m4stakexa, m4stakex,	mod4oki, mpu4, m4default, ROT0,   "Leisurama","Stake X (Leisurama) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // works?
GAME(199?, m4sstrek,  0,		mod2    ,mpu4, m4default, ROT0,   "bootleg","Super Streak (bootleg) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // works?, no sound
GAME(199?, m4boltbl,  0,		mod2    ,mpu4, m4default, ROT0,   "DJE","Bolt From The Blue (DJE) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4boltbla, m4boltbl,	mod2    ,mpu4, m4default, ROT0,   "DJE","Bolt From The Blue (DJE) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4boltblb, m4boltbl,	mod2    ,mpu4, m4default, ROT0,   "DJE","Bolt From The Blue (DJE) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4boltblc, m4boltbl,	mod2    ,mpu4, m4default, ROT0,   "DJE","Bolt From The Blue (DJE) (MPU4, set 4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4dblchn,  0,		mod4oki, mpu4, m4default, ROT0,   "DJE","Double Chance (DJE) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4stand2,  0,		mod2    ,mpu4, m4default, ROT0,   "DJE","Stand To Deliver (DJE) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )

/* Unknown stuff that looks like it might be MPU4, but needs further verification, some could be bad */

GAME(199?, m4barcrz	,  0,		mod4oki	,mpu4, m4default, ROT0,   "unknown","Bar Crazy (unknown) (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bonzbn	,  0,		mod4oki	,mpu4, m4default, ROT0,   "unknown","Bingo Bonanza (unknown) (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4cld02	,  0,		mod4oki	,mpu4, m4default, ROT0,   "unknown","Unknown MPU4 'CLD 0.2C' (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4dnj	,  0,		mod4oki	,mpu4, m4default, ROT0,   "unknown","Double Nudge (unknown) (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4matdr	,  0,		mod4oki	,mpu4, m4default, ROT0,   "unknown","Matador (unknown) (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4ttak	,  0,		mod2	,mpu4, m4default, ROT0,   "unknown","Tic Tac Take (unknown) (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hslo	,  0,		mod2	,mpu4, m4default, ROT0,   "unknown","Unknown MPU4 'HOT 3.0' (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4unkjok	,  0,		mod2	,mpu4, m4default, ROT0,   "unknown","Unknown MPU4 'Joker' (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4remag	,  0,		mod2	,mpu4, m4default, ROT0,   "unknown","Unknown MPU4 'ZTP 0.7' (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4rmg	,  0,		mod2	,mpu4, m4default, ROT0,   "unknown","Unknown MPU4 'CTP 0.4' (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4wnud	,  0,		mod2	,mpu4, m4default, ROT0,   "unknown","Unknown MPU4 'W Nudge' (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4t266	,  0,		mod2	,mpu4, m4default, ROT0,   "unknown","Unknown MPU4 'TTO 1.1' (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4brnze	,  0,		mod4oki, mpu4, m4default, ROT0,   "unknown","Bronze Voyage (unknown) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4riotrp	,  0,		mod4oki, mpu4, m4default, ROT0,   "unknown","Rio Tropico (unknown) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )

/* *if* these are MPU4 they have a different sound system at least - The copyright strings in them are 'AET' tho (Ace?) - Could be related to the Crystal stuff? */
GAME(199?, m4sbx	,  0,		mpu4crys	,mpu4, m_frkstn, ROT0,   "AET/Coinworld","Super Bear X (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bclimb	,  0,		mpu4crys	,mpu4, m_frkstn, ROT0,   "AET/Coinworld","Bear Climber (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4captb	,  0,		mpu4crys	,mpu4, m_frkstn, ROT0,   "AET/Coinworld","Captain Bear (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4jungj	,  0,		mpu4crys	,mpu4, m_frkstn, ROT0,   "AET/Coinworld","Jungle Japes (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4fsx	,  0,		mpu4crys	,mpu4, m_frkstn, ROT0,   "AET/Coinworld","Fun Spot X (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4ccop	,  0,		mod4oki		,mpu4, m4default, ROT0,   "Coinworld","Cash Cops (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4ccc	,  0,		mod4oki		,mpu4, m4default, ROT0,   "Coinworld","Criss Cross Crazy (Coinworld) (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4treel	,  0,		mod2		,mpu4, m4default, ROT0,   "Jpm","Turbo Reels (Jpm) (MPU4?)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )




GAME(199?, m4frkstn	,0			,mpu4crys	,mpu4		,m_frkstn,	ROT0,   "Crystal","Frank 'n' Stein (Crystal) (MPU4, set 1)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4frkstna,m4frkstn	,mpu4crys	,mpu4		,m_frkstn,	ROT0,   "Crystal","Frank 'n' Stein (Crystal) (MPU4, set 2)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4frkstnb,m4frkstn	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Frank 'n' Stein (Crystal) (MPU4, set 3)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // this set is encrypted
GAME(199?, m4aladn	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Aladdin's Cave (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bagcsh	,0      	,mpu4crys	,mpu4		,m_frkstn,	ROT0,   "Crystal","Bags Of Cash Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bucclb	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Buccaneer Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4bullio	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Bullion Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4carou	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Carousel Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4cclimb	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Crazy Climber (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4crzcl	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Crazy Climber Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4crzclc	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Crazy Club Climber (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4elitc	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Elite Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4fairg	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Fairground (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4frmani	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Fruit Mania (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4goldxc	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Gold Exchange Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4goldfc	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Gold Fever (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hirol	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Hi Roller Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4kingqn	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Kings & Queens Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4lotclb	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Lottery Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4montrl	,0      	,mpu4crys	,mpu4		,crystali,	ROT0,   "Crystal","Money Trail (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // encryption is inverted!
GAME(199?, m4mystiq	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Mystique Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4nudwin	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Nudge & Win (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4paracl	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Paradise Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4rlpick	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Reel Picks (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4twstr	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Twister (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4twstcl	,0      	,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Twister Club (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4dz		,0			,mpu4crys	,mpu4		,crystal,	ROT0,   "Crystal","Danger Zone (Crystal) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4tylb	,0			,mod4oki	,mpu4		,m4default,	ROT0,   "Crystal","Thank Your Lucky Bars (Crystal) (MPU4)",GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL )
GAME(199?, m4magi7	,0			,mod4oki	,mpu4		,m4default,	ROT0,   "Crystal","Magic 7's (Crystal) (MPU4)",GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL )
GAME(199?, m4rags	,0			,mod4oki	,mpu4		,m4default,	ROT0,   "Crystal","Rags To Riches Club (Crystal) (MPU4)",GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL )
GAME(199?, m4riocr	,0			,mod4oki	,mpu4		,m4default,	ROT0,   "Crystal","Rio Grande (Crystal) (MPU4)",GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL )
GAME(199?, m4ndup	,0			,mod4oki	,mpu4		,m4default,	ROT0,   "Crystal","Nudge Double Up Deluxe (Crystal) (MPU4)",GAME_NOT_WORKING|GAME_NO_SOUND|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL )

//SWP
GAMEL(1989?,  m4conn4,        0, mod2    ,   connect4,   connect4,   ROT0, "Dolbeck Systems","Connect 4",GAME_IMPERFECT_GRAPHICS|GAME_REQUIRES_ARTWORK,layout_connect4 )

GAME(199?, m4surf, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Gemini","Super Surfin' (Gemini) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4blkgd, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Gemini","Black Gold (Gemini) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4excam, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Mdm","Excalibur (Mdm) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4front, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Mdm","Final Frontier (Mdm) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4pick, 0,			mod2    ,mpu4, m4default, ROT0,   "Jpm","Pick A Fruit (Jpm) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4safar, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Mdm","Safari Club (Mdm) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4zill, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Pure Leisure","Zillionare's Challenge (Pure Leisure) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )

GAME(199?, m4snowbl, 0,			mod2    ,mpu4, m4default, ROT0,   "Mdm","Snowball Bingo (Mdm) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hstr, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Coinworld","Happy Streak (Coinworld) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hstrcs, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Coinworld","Casino Happy Streak (Coinworld) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4ddb,  0,			mod4oki    ,mpu4, m4default, ROT0,   "Coinworld","Ding Dong Bells (Coinworld) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4hapfrt,  0,			mod4oki    ,mpu4, m4default, ROT0,   "Coinworld","Happy Fruits (Coinworld) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )

GAME(199?, m4frcrak, 0,			mod2    ,mpu4, m4default, ROT0,   "Pcp","Fruit Cracker (Pcp) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )

GAME(199?, m4ewshft, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Mdm","Each Way Shifter (Mdm) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4jiggin, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Global","Jiggin' In The Riggin' (Global) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4sunday, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Pcp","Sunday Sport (Pcp) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4jp777, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Cotswold Microsystems","Jackpot 777 (Cotswold Microsystems) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4booze, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Extreme","Booze Cruise (Extreme) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND )
GAME(199?, m4cbing, 0,			mod4oki    ,mpu4, m4default, ROT0,   "Redpoint Systems","Cherry Bingo (Redpoint Systems) (MPU4)",   GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_NO_SOUND ) // custom sound system


GAME( 1986, m4supsl	, 0			, mod2, mpu4,		m4default		,	  ROT0,       "Unknown",   "Supa Silva (Bellfruit?) (MPU4)", GAME_NOT_WORKING|GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK )
GAME( 199?, m4nod		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Eurotech",   "Nod And A Wink (Eurotech) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL) // this has valid strings in it BEFORE the bfm decode, but decodes to valid code, does it use some funky mapping, or did they just fill unused space with valid looking data?

GAME( 199?, m4dcrls		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Mazooma",   "Double Crazy Reels (Mazooma) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)

// not sure about several of the nova ones
GAME( 199?, m4aliz		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "AlizBaz (Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4c2		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Circus Circus 2 (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4coney		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "Coney Island (Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4cfinln		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Cup Final (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4dcrazy		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "D' Crazy Reels (Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4goldnn		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Golden Years (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4jungjk		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "Jungle Jackpots (Mazooma - Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4clab		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "Cash Lab (Mazooma - Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4looplt		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "Loop The Loot (Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4mgpn		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Monaco Grand Prix (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4olygn		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Olympic Gold (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4rhnote		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "Red Hot Notes (Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4rhrock		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "Red Hot Rocks (Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4rhwhl		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "Red Hot Wheels (Mazooma - Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4rdeal		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "Reel Deal (Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4shoknr		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "Shock 'n' Roll (Mazooma - Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4shkwav		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "Shockwave (Mazooma - Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4sinbdn		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Sinbad (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4sinbd2		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Sinbad Deluxe 2 (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4sinbd3		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Sinbad Deluxe 3 (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4sinbdd		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Sinbad Deluxe [Wall Mount] (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4sinbdj		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Sinbad [Jackpot Link] (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4sinbdl		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Sinbad [Jackpot Link] [Wall Mount] (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4sinbdw		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Sinbad [Wall Mount] (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4spotln		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Spotlight (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4sdquid		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "Sundance Quid (Qps) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4tornad		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Qps",   "Tornado (Qps - Mazooma) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4vivan		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Viva Las Vegas (Nova) (MPU4)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME( 199?, m4ftladn		, 0			,  mod4oki		, mpu4		, m4default		, 0,		 "Nova",   "Find the Lady (Nova)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)

// these don't contain a valid vector in the first bank?
GAME(199?, m4abra	,0			,bwboki		,mpu4				,m_blsbys			,ROT0,	 "Bwb","Abracadabra (Bwb) (MPU4?)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME(199?, m4wcnov	,0			,bwboki		,mpu4				,m_blsbys			,ROT0,	 "Nova","World Cup (Nova) (MPU4?)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)
GAME(199?, m4excaln	,0			,bwboki		,mpu4				,m_blsbys			,ROT0,	 "Nova","Excalibur (Nova) (MPU4?)", GAME_SUPPORTS_SAVE|GAME_REQUIRES_ARTWORK|GAME_NOT_WORKING|GAME_MECHANICAL)


