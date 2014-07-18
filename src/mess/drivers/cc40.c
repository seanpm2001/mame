// license:BSD-3-Clause
// copyright-holders:hap
/***************************************************************************

  ---------------------------------------------
  | ---------------------------------------   |
  | |                                     |   |
  | | LCD 1 line, 31 chars + indicators   |   |
  | |                                     |   ---------------
  | ---------------------------------------                 |
  |                                                         |
  |                                                         |
  |                                                         |
  |                           *HD44100H                     |
  |       *HD44780A00                                       |
  |                                                         |
  |                                                         |
  |                                                         |
  ----|||||||||||-----------------------------|||||||||||----
      |||||||||||                             |||||||||||
  ----|||||||||||-----------------------------|||||||||||----
  |                                                         |
  |            HM6116LP-4    HM6116LP-4                     |
  |                                                         |
  |                                                         |
  |             HM6116LP-4                    TMX70C20N2L   |
  |                                                         |
  |                              AMI 8304BXH                |
  |             HN61256PC09                                 |
  |                                             *Cartridge  |
  |                                           ---------------
  |                                           |
  |       -------------------------------------
  |*HEXBUS|
  ---------

  HM6116LP-4    - Hitachi 2KB SRAM
  HN61256PC09   - Hitachi DIP-28 32KB CMOS Mask PROM
  TMX70C20N2L   - Texas Instruments TMS70C20 CPU (128 bytes RAM, 2KB ROM) @ 2.5MHz - "X" implies prototype
  AMI 8304BXH   - 74-pin QFP AMI Gate Array
  HD44100H      - 60-pin QFP Hitachi HD44100 LCD Driver
  HD44780A00    - 80-pin TFP Hitachi HD44780 LCD Controller
  
  *             - indicates that it's on the other side of the PCB
  

  TODO:
  - HD44100 is not accessed by the CPU, is it connected to the HD44780?
    Probably responsible for the LCD indicators, how?

***************************************************************************/

#include "emu.h"
#include "cpu/tms7000/tms7000.h"
#include "video/hd44780.h"
#include "sound/dac.h"


class cc40_state : public driver_device
{
public:
	cc40_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_dac(*this, "dac")
	{ }

	required_device<tms70c20_device> m_maincpu;
	required_device<dac_device> m_dac;
	
	ioport_port *m_key_matrix[8];

	UINT8 m_power;
	UINT8 m_bus_control;
	UINT8 m_banks;
	UINT8 m_clock_control;
	UINT8 m_key_select;
	
	void update_lcd_indicator(UINT8 y, UINT8 x, int state);

	DECLARE_READ8_MEMBER(bus_control_r);
	DECLARE_WRITE8_MEMBER(bus_control_w);
	DECLARE_WRITE8_MEMBER(power_w);
	DECLARE_WRITE8_MEMBER(sound_w);
	DECLARE_READ8_MEMBER(battery_r);
	DECLARE_READ8_MEMBER(bankswitch_r);
	DECLARE_WRITE8_MEMBER(bankswitch_w);
	DECLARE_READ8_MEMBER(clock_r);
	DECLARE_WRITE8_MEMBER(clock_w);
	DECLARE_READ8_MEMBER(keyboard_r);
	DECLARE_WRITE8_MEMBER(keyboard_w);

	virtual void machine_reset();
	virtual void machine_start();
	DECLARE_PALETTE_INIT(cc40);
};


/***************************************************************************

  Video

***************************************************************************/

PALETTE_INIT_MEMBER(cc40_state, cc40)
{
	palette.set_pen_color(0, rgb_t(138, 146, 148));
	palette.set_pen_color(1, rgb_t(92, 83, 88));
}

void cc40_state::update_lcd_indicator(UINT8 y, UINT8 x, int state)
{
	;
}

static HD44780_PIXEL_UPDATE(cc40_pixel_update)
{
	if (line == 1 && pos == 15)
	{
		// the last char is used to control lcd indicators
		cc40_state *driver_state = device.machine().driver_data<cc40_state>();
		driver_state->update_lcd_indicator(y, x, state);
	}
	else if (line < 2 && pos < 16)
	{
		// internal: 2*16, external: 1*31 + indicators
		bitmap.pix16(y, line*16*6 + pos*6 + x) = state;
	}
}



/***************************************************************************

  I/O, Memory Maps

***************************************************************************/

READ8_MEMBER(cc40_state::bus_control_r)
{
	return m_bus_control;
}

WRITE8_MEMBER(cc40_state::bus_control_w)
{
	m_bus_control = data;
}

WRITE8_MEMBER(cc40_state::power_w)
{
	// d0: power-on hold latch
	m_power = data & 1;
	
	// stop running
	if (!m_power)
		m_maincpu->set_input_line(INPUT_LINE_RESET, ASSERT_LINE);
}

WRITE8_MEMBER(cc40_state::sound_w)
{
	// d0: piezo control
	m_dac->write_signed8((data & 1) ? 0x7f : 0);
}

READ8_MEMBER(cc40_state::battery_r)
{
	// d0: low battery sense line (0 = low power)
	return 1;
}

READ8_MEMBER(cc40_state::bankswitch_r)
{
	return m_banks;
}

WRITE8_MEMBER(cc40_state::bankswitch_w)
{
	// d0-d1: system rom bankswitch
	membank("bank1")->set_entry(data & 3);
	
	// d1-d2: cartridge rom bankswitch
	
	m_banks = data & 0x0f;
}

READ8_MEMBER(cc40_state::clock_r)
{
	return m_clock_control;
}

WRITE8_MEMBER(cc40_state::clock_w)
{
	// d3: enable clock divider
	if (data & 8)
	{
		if (m_clock_control != (data & 0x0f))
		{
			// d0-d2: clock divider (2.5MHz /3 to /17 in steps of 2)
			double div = (~data & 7) * 2 + 1;
			m_maincpu->set_clock_scale(1 / div);
		}
	}
	else if (m_clock_control & 8)
	{
		// high to low
		m_maincpu->set_clock_scale(1);
	}
	
	m_clock_control = data & 0x0f;
}

READ8_MEMBER(cc40_state::keyboard_r)
{
	UINT8 ret = 0;

	// read selected keyboard rows
	for (int i = 0; i < 8; i++)
	{
		if (m_key_select >> i & 1)
			ret |= m_key_matrix[i]->read();
	}
	
	return ret;
}

WRITE8_MEMBER(cc40_state::keyboard_w)
{
	// d(0-7): select keyboard column
	m_key_select = data;
}


static ADDRESS_MAP_START( main_map, AS_PROGRAM, 8, cc40_state )
	ADDRESS_MAP_UNMAP_HIGH

	AM_RANGE(0x0110, 0x0110) AM_READWRITE(bus_control_r, bus_control_w)
	AM_RANGE(0x0111, 0x0111) AM_WRITE(power_w)
	AM_RANGE(0x0112, 0x0112) AM_NOP // hexbus data
	AM_RANGE(0x0113, 0x0113) AM_NOP // hexbus available
	AM_RANGE(0x0114, 0x0114) AM_NOP // hexbus handshake
	AM_RANGE(0x0115, 0x0115) AM_WRITE(sound_w)
	AM_RANGE(0x0116, 0x0116) AM_READ(battery_r)
	AM_RANGE(0x0119, 0x0119) AM_READWRITE(bankswitch_r, bankswitch_w)
	AM_RANGE(0x011a, 0x011a) AM_READWRITE(clock_r, clock_w)
	AM_RANGE(0x011e, 0x011f) AM_DEVREADWRITE("hd44780", hd44780_device, read, write)

	AM_RANGE(0x0800, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x17ff) AM_RAM
	AM_RANGE(0x3000, 0x37ff) AM_RAM
	AM_RANGE(0xd000, 0xefff) AM_ROMBANK("bank1")
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_io_map, AS_IO, 8, cc40_state )
	AM_RANGE(TMS7000_PORTA, TMS7000_PORTA) AM_READ(keyboard_r)
	AM_RANGE(TMS7000_PORTB, TMS7000_PORTB) AM_WRITE(keyboard_w)
ADDRESS_MAP_END



/***************************************************************************

  Inputs

***************************************************************************/

static INPUT_PORTS_START( cc40 )
	// 8x8 keyboard matrix, RESET and ON buttons are not on it
	// The numpad number keys are shared with the ones on the main keyboard.
	// Unused entries are not connected, but some might have a purpose for factory testing(?)
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR('8') PORT_CHAR('(')

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')

	PORT_START("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')

	PORT_START("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')

	PORT_START("IN4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("IN5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0') PORT_CHAR('\'')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL)) PORT_NAME("CLR  UCL")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH_PAD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD)) PORT_NAME("/")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR('9') PORT_CHAR(')')

	PORT_START("IN6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CODE(KEYCODE_DEL_PAD) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_PLUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD)) PORT_NAME("+")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13) PORT_NAME("ENTER")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS_PAD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR(UCHAR_MAMEKEY(ASTERISK)) PORT_NAME("*")

	PORT_START("IN7")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_1) PORT_NAME("SHIFT")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_2) PORT_NAME("CTL")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END)) PORT_NAME("BREAK")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME)) PORT_NAME("RUN")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(PGUP)) PORT_NAME("FN")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_NAME("OFF")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


/***************************************************************************

  Machine Config

***************************************************************************/

void cc40_state::machine_reset()
{
	m_power = 1;
}

void cc40_state::machine_start()
{
	static const char *const tags[] = { "IN0", "IN1", "IN2", "IN3", "IN4", "IN5", "IN6", "IN7" };
	for (int i = 0; i < 8; i++)
		m_key_matrix[i] = ioport(tags[i]);
	
	UINT8 *ROM = memregion("maincpu")->base();
	membank("bank1")->configure_entries(0, 4, &ROM[0x10000], 0x2000);
	membank("bank1")->set_entry(0);
	
	// zerofill
	m_power = 0;
	m_bus_control = 0;
	m_banks = 0;
	m_clock_control = 0;
	m_key_select = 0;
	
	// register for savestates
	save_item(NAME(m_power));
	save_item(NAME(m_bus_control));
	save_item(NAME(m_banks));
	save_item(NAME(m_clock_control));
	save_item(NAME(m_key_select));
}

static MACHINE_CONFIG_START( cc40, cc40_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", TMS70C20, XTAL_2_5MHz)
	MCFG_CPU_PROGRAM_MAP(main_map)
	MCFG_CPU_IO_MAP(main_io_map)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", LCD)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_SIZE(6*31, 9*1)
	MCFG_SCREEN_VISIBLE_AREA(0, 6*31-1, 0, 9*1-1)
	MCFG_DEFAULT_LAYOUT(layout_lcd)
	MCFG_SCREEN_UPDATE_DEVICE("hd44780", hd44780_device, screen_update)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_PALETTE_ADD("palette", 2)
	MCFG_PALETTE_INIT_OWNER(cc40_state, cc40)

	MCFG_HD44780_ADD("hd44780")
	MCFG_HD44780_LCD_SIZE(2, 16) // internal: 2*16, external: 1*31 + indicators
	MCFG_HD44780_PIXEL_UPDATE_CB(cc40_pixel_update)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_DAC_ADD("dac")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END


/***************************************************************************

  Game drivers

***************************************************************************/

ROM_START( cc40 )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "tms70c20.bin", 0xf800, 0x0800, CRC(a21bf6ab) SHA1(3da8435ecbee143e7fa149ee8e1c92949bade1d8) )
	ROM_LOAD( "cc40.bin", 0x10000, 0x8000, CRC(f5322fab) SHA1(1b5c4052a53654363c458f75eac7a27f0752def6) )
ROM_END


COMP( 1983, cc40, 0, 0, cc40, cc40, driver_device, 0, "Texas Instruments", "Compact Computer 40", GAME_NOT_WORKING )
