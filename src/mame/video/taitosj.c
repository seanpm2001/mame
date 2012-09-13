/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "emu.h"
#include "video/resnet.h"
#include "includes/taitosj.h"


#define GLOBAL_FLIP_X			(*state->m_video_mode & 0x01)
#define GLOBAL_FLIP_Y			(*state->m_video_mode & 0x02)
#define SPRITE_RAM_PAGE_OFFSET	((*state->m_video_mode & 0x04) ? 0x80 : 0x00)
#define SPRITES_ON				(*state->m_video_mode & 0x80)
#define TRANSPARENT_PEN			(0x40)



static const int layer_enable_mask[3] = { 0x10, 0x20, 0x40 };

typedef void (*copy_layer_func_t)(running_machine &, bitmap_ind16 &,
								  const rectangle &, int, int *, rectangle *);


/***************************************************************************

  I call the three layers with the conventional names "front", "middle" and
  "back", because that's their default order, but they can be arranged,
  together with the sprites, in any order. The priority is selected by
  register 0xd300, which works as follow:

  bits 0-3 go to A4-A7 of a 256x4 PROM
  bit 4 selects D0/D1 or D2/D3 of the PROM
  bit 5-7 n.c.
  A0-A3 of the PROM is fed with a mask of the inactive layers
  (i.e. all-zero) in the order sprites-front-middle-back
  the 2-bit code which comes out from the PROM selects the layer
  to display.

  Here is a dump of one of these PROMs; on the right is the resulting order
  (s = sprites f = front m = middle b = back). Note that, in theory, the
  PROM could encode some really funky priority schemes which couldn't be
  reconducted to the simple layer order given here. Luckily, none of the
  games seem to do that. Actually, all of them seem to use the same PROM,
  with the exception of Wild Western.

                                                        d300 pri    d300 pri
  00: 08 09 08 0A 00 05 00 0F 08 09 08 0A 00 05 00 0F |  00  sfmb    10  msfb
  10: 08 09 08 0B 00 0D 00 0F 08 09 08 0A 00 05 00 0F |  01  sfbm    11  msbf
  20: 08 0A 08 0A 04 05 00 0F 08 0A 08 0A 04 05 00 0F |  02  smfb    12  mfsb
  30: 08 0A 08 0A 04 07 0C 0F 08 0A 08 0A 04 05 00 0F |  03  smbf    13  mfbs
  40: 08 0B 08 0B 0C 0F 0C 0F 08 09 08 0A 00 05 00 0F |  04  sbfm    14  mbsf
  50: 08 0B 08 0B 0C 0F 0C 0F 08 0A 08 0A 04 05 00 0F |  05  sbmf    15  mbfs
  60: 0D 0D 0C 0E 0D 0D 0C 0F 01 05 00 0A 01 05 00 0F |  06  fsmb    16  bsfm
  70: 0D 0D 0C 0F 0D 0D 0C 0F 01 09 00 0A 01 05 00 0F |  07  fsbm    17  bsmf
  80: 0D 0D 0E 0E 0D 0D 0C 0F 05 05 02 0A 05 05 00 0F |  08  fmsb    18  bfsm
  90: 0D 0D 0E 0E 0D 0D 0F 0F 05 05 0A 0A 05 05 00 0F |  09  fmbs    19  bfms
  A0: 0D 0D 0F 0F 0D 0D 0F 0F 09 09 08 0A 01 05 00 0F |  0A  fbsm    1A  bmsf
  B0: 0D 0D 0F 0F 0D 0D 0F 0F 09 09 0A 0A 05 05 00 0F |  0B  fbms    1B  bmfs
  C0: 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F |  0C   -      1C   -
  D0: 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F |  0D   -      1D   -
  E0: 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F |  0E   -      1E   -
  F0: 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F |  0F   -      1F   -

***************************************************************************/



/***************************************************************************

  Convert the color PROMs into a more useable format.

  The Taito games don't have a color PROM. They use RAM to dynamically
  create the palette. The resolution is 9 bit (3 bits per gun).

  The RAM is connected to the RGB output this way:

  bit 0 -- inverter -- 270 ohm resistor  -- RED
  bit 7 -- inverter -- 470 ohm resistor  -- RED
        -- inverter -- 1  kohm resistor  -- RED
        -- inverter -- 270 ohm resistor  -- GREEN
        -- inverter -- 470 ohm resistor  -- GREEN
        -- inverter -- 1  kohm resistor  -- GREEN
        -- inverter -- 270 ohm resistor  -- BLUE
        -- inverter -- 470 ohm resistor  -- BLUE
  bit 0 -- inverter -- 1  kohm resistor  -- BLUE

***************************************************************************/

static void set_pens(running_machine &machine)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	static const int resistances[3] = { 1000, 470, 270 };
	double rweights[3], gweights[3], bweights[3];
	int i;

	/* compute the color output resistor weights */
	compute_resistor_weights(0,	255, -1.0,
			3, resistances, rweights, 0, 0,
			3, resistances, gweights, 0, 0,
			3, resistances, bweights, 0, 0);

	for (i = 0; i < 0x40; i++)
	{
		int bit0, bit1, bit2;
		int r, g, b, val;

		/* red component */
		val = state->m_paletteram[(i << 1) | 0x01];
		bit0 = (~val >> 6) & 0x01;
		bit1 = (~val >> 7) & 0x01;
		val = state->m_paletteram[(i << 1) | 0x00];
		bit2 = (~val >> 0) & 0x01;
		r = combine_3_weights(rweights, bit0, bit1, bit2);

		/* green component */
		val = state->m_paletteram[(i << 1) | 0x01];
		bit0 = (~val >> 3) & 0x01;
		bit1 = (~val >> 4) & 0x01;
		bit2 = (~val >> 5) & 0x01;
		g = combine_3_weights(gweights, bit0, bit1, bit2);

		/* blue component */
		val = state->m_paletteram[(i << 1) | 0x01];
		bit0 = (~val >> 0) & 0x01;
		bit1 = (~val >> 1) & 0x01;
		bit2 = (~val >> 2) & 0x01;
		b = combine_3_weights(bweights, bit0, bit1, bit2);

		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

static void compute_draw_order(running_machine &machine)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	int i;
	UINT8 *color_prom = state->memregion("proms")->base();

	/* do a simple conversion of the PROM into layer priority order. Note that */
	/* this is a simplification, which assumes the PROM encodes a sensible priority */
	/* scheme. */
	for (i = 0; i < 32; i++)
	{
		int j;
		int mask = 0;	/* start with all four layers active, so we'll get the highest */
						/* priority one in the first loop */
		for (j = 3; j >= 0; j--)
		{
			int data = color_prom[0x10 * (i & 0x0f) + mask] & 0x0f;

			if (i & 0x10)
				data = data >> 2;
			else
				data = data & 0x03;

			mask |= (1 << data);	/* in next loop, we'll see which of the remaining */
									/* layers has top priority when this one is transparent */
			state->m_draw_order[i][j] = data;
		}
	}
}


void taitosj_state::video_start()
{
	int i;

	m_sprite_layer_collbitmap1.allocate(16,16);

	for (i = 0; i < 3; i++)
	{
		machine().primary_screen->register_screen_bitmap(m_layer_bitmap[i]);
		machine().primary_screen->register_screen_bitmap(m_sprite_layer_collbitmap2[i]);
	}

	m_sprite_sprite_collbitmap1.allocate(32,32);
	m_sprite_sprite_collbitmap2.allocate(32,32);

	machine().gfx[0]->set_source(m_characterram);
	machine().gfx[1]->set_source(m_characterram);
	machine().gfx[2]->set_source(m_characterram + 0x1800);
	machine().gfx[3]->set_source(m_characterram + 0x1800);

	compute_draw_order(machine());
}



READ8_MEMBER(taitosj_state::taitosj_gfxrom_r)
{
	UINT8 ret;

	offs_t offs = m_gfxpointer[0] | (m_gfxpointer[1] << 8);

	if (offs < 0x8000)
		ret = memregion("gfx1")->base()[offs];
	else
		ret = 0;

	offs = offs + 1;

	m_gfxpointer[0] = offs & 0xff;
	m_gfxpointer[1] = offs >> 8;

	return ret;
}



WRITE8_MEMBER(taitosj_state::taitosj_characterram_w)
{
	if (m_characterram[offset] != data)
	{
		if (offset < 0x1800)
		{
			machine().gfx[0]->mark_dirty((offset / 8) & 0xff);
			machine().gfx[1]->mark_dirty((offset / 32) & 0x3f);
		}
		else
		{
			machine().gfx[2]->mark_dirty((offset / 8) & 0xff);
			machine().gfx[3]->mark_dirty((offset / 32) & 0x3f);
		}

		m_characterram[offset] = data;
	}
}

WRITE8_MEMBER(taitosj_state::junglhbr_characterram_w)
{
	taitosj_characterram_w(space, offset, data ^ 0xfc);
}


WRITE8_MEMBER(taitosj_state::taitosj_collision_reg_clear_w)
{
	m_collision_reg[0] = 0;
	m_collision_reg[1] = 0;
	m_collision_reg[2] = 0;
	m_collision_reg[3] = 0;
}


INLINE int get_sprite_xy(taitosj_state *state, UINT8 which, UINT8* sx, UINT8* sy)
{
	offs_t offs = which * 4;

	*sx =       state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs + 0] - 1;
	*sy = 240 - state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs + 1];

	return (*sy < 240);
}


INLINE gfx_element *get_sprite_gfx_element(running_machine &machine, UINT8 which)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	offs_t offs = which * 4;

	return machine.gfx[(state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs + 3] & 0x40) ? 3 : 1];
}


static int check_sprite_sprite_bitpattern(running_machine &machine,
										  int sx1, int sy1, int which1,
										  int sx2, int sy2, int which2)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	int x, y, minx, miny, maxx = 16, maxy = 16;

	offs_t offs1 = which1 * 4;
	offs_t offs2 = which2 * 4;

	/* normalize coordinates to (0,0) and compute overlap */
	if (sx1 < sx2)
	{
		sx2 -= sx1;
		sx1 = 0;
		minx = sx2;
	}
	else
	{
		sx1 -= sx2;
		sx2 = 0;
		minx = sx1;
	}

	if (sy1 < sy2)
	{
		sy2 -= sy1;
		sy1 = 0;
		miny = sy2;
	}
	else
	{
		sy1 -= sy2;
		sy2 = 0;
		miny = sy1;
	}

	/* draw the sprites into separate bitmaps and check overlapping region */
	state->m_sprite_layer_collbitmap1.fill(TRANSPARENT_PEN);
	drawgfx_transpen(state->m_sprite_sprite_collbitmap1, state->m_sprite_sprite_collbitmap1.cliprect(), get_sprite_gfx_element(machine, which1),
			state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs1 + 3] & 0x3f,
			0,
			state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs1 + 2] & 0x01,
			state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs1 + 2] & 0x02,
			sx1, sy1, 0);

	state->m_sprite_sprite_collbitmap2.fill(TRANSPARENT_PEN);
	drawgfx_transpen(state->m_sprite_sprite_collbitmap2, state->m_sprite_sprite_collbitmap2.cliprect(), get_sprite_gfx_element(machine, which2),
			state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs2 + 3] & 0x3f,
			0,
			state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs2 + 2] & 0x01,
			state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs2 + 2] & 0x02,
			sx2, sy2, 0);

	for (y = miny; y < maxy; y++)
		for (x = minx; x < maxx; x++)
			if ((state->m_sprite_sprite_collbitmap1.pix16(y, x) != TRANSPARENT_PEN) &&
			    (state->m_sprite_sprite_collbitmap2.pix16(y, x) != TRANSPARENT_PEN))
				return 1;  /* collided */

	return 0;
}


static void check_sprite_sprite_collision(running_machine &machine)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	if (SPRITES_ON)
	{
		int which1;

		/* chech each pair of sprites */
		for (which1 = 0; which1 < 0x20; which1++)
		{
			int which2;
			UINT8 sx1, sy1;

			if ((which1 >= 0x10) && (which1 <= 0x17)) continue;	/* no sprites here */

			if (!get_sprite_xy(state, which1, &sx1, &sy1)) continue;

			for (which2 = which1 + 1; which2 < 0x20; which2++)
			{
				UINT8 sx2, sy2;

				if ((which2 >= 0x10) && (which2 <= 0x17)) continue;	  /* no sprites here */

				if (!get_sprite_xy(state, which2, &sx2, &sy2)) continue;

				/* quickly rule out any pairs that cannot be touching */
				if ((abs((INT8)sx1 - (INT8)sx2) < 16) &&
					(abs((INT8)sy1 - (INT8)sy2) < 16))
				{
					int reg;

					if (!check_sprite_sprite_bitpattern(machine, sx1, sy1, which1, sx2, sy2, which2))  continue;

					/* mark sprite as collided */
					/* note that only the sprite with the higher number is marked */
					/* as collided. This is how the hardware works and required */
					/* by Pirate Pete to be able to finish the last round. */

					/* the last sprite has to be moved at the start of the list */
					if (which2 == 0x1f)
					{
						reg = which1 >> 3;
						if (reg == 3)  reg = 2;

						state->m_collision_reg[reg] |= (1 << (which1 & 0x07));
					}
					else
					{
						reg = which2 >> 3;
						if (reg == 3)  reg = 2;

						state->m_collision_reg[reg] |= (1 << (which2 & 0x07));
					}
				}
			}
		}
	}
}


static void calculate_sprite_areas(running_machine &machine, int *sprites_on, rectangle *sprite_areas)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	int which;
	int width = machine.primary_screen->width();
	int height = machine.primary_screen->height();

	for (which = 0; which < 0x20; which++)
	{
		UINT8 sx, sy;

		if ((which >= 0x10) && (which <= 0x17)) continue;	/* no sprites here */

		if (get_sprite_xy(state, which, &sx, &sy))
		{
			int minx, miny, maxx, maxy;

			if (GLOBAL_FLIP_X)
				sx = 238 - sx;

			if (GLOBAL_FLIP_Y)
				sy = 242 - sy;

			minx = sx;
			miny = sy;

			maxx = minx + 15;
			maxy = miny + 15;

			/* check for bitmap bounds to avoid illegal memory access */
			if (minx < 0) minx = 0;
			if (miny < 0) miny = 0;
			if (maxx >= width - 1)
				maxx = width - 1;
			if (maxy >= height - 1)
				maxy = height - 1;

			sprite_areas[which].min_x = minx;
			sprite_areas[which].max_x = maxx;
			sprite_areas[which].min_y = miny;
			sprite_areas[which].max_y = maxy;

			sprites_on[which] = 1;
		}
		/* sprite is off */
		else
			sprites_on[which] = 0;
	}
}


static int check_sprite_layer_bitpattern(running_machine &machine, int which, rectangle *sprite_areas)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	int y, x;
	offs_t offs = which * 4;
	int result = 0;  /* no collisions */

	int	check_layer_1 = *state->m_video_mode & layer_enable_mask[0];
	int	check_layer_2 = *state->m_video_mode & layer_enable_mask[1];
	int	check_layer_3 = *state->m_video_mode & layer_enable_mask[2];

	int minx = sprite_areas[which].min_x;
	int miny = sprite_areas[which].min_y;
	int maxx = sprite_areas[which].max_x + 1;
	int maxy = sprite_areas[which].max_y + 1;

	int flip_x = (state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs + 2] & 0x01) ^ GLOBAL_FLIP_X;
	int flip_y = (state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs + 2] & 0x02) ^ GLOBAL_FLIP_Y;

	/* draw sprite into a bitmap and check if layers collide */
	state->m_sprite_layer_collbitmap1.fill(TRANSPARENT_PEN);
	drawgfx_transpen(state->m_sprite_layer_collbitmap1, state->m_sprite_layer_collbitmap1.cliprect(),get_sprite_gfx_element(machine, which),
			state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs + 3] & 0x3f,
			0,
			flip_x, flip_y,
			0,0,0);

	for (y = miny; y < maxy; y++)
		for (x = minx; x < maxx; x++)
			if (state->m_sprite_layer_collbitmap1.pix16(y - miny, x - minx) != TRANSPARENT_PEN) /* is there anything to check for ? */
			{
				if (check_layer_1 && (state->m_sprite_layer_collbitmap2[0].pix16(y, x) != TRANSPARENT_PEN))
					result |= 0x01;  /* collided with layer 1 */

				if (check_layer_2 && (state->m_sprite_layer_collbitmap2[1].pix16(y, x) != TRANSPARENT_PEN))
					result |= 0x02;  /* collided with layer 2 */

				if (check_layer_3 && (state->m_sprite_layer_collbitmap2[2].pix16(y, x) != TRANSPARENT_PEN))
					result |= 0x04;  /* collided with layer 3 */
			}

	return result;
}


static void check_sprite_layer_collision(running_machine &machine, int *sprites_on, rectangle *sprite_areas)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	if (SPRITES_ON)
	{
		int which;

		/* check each sprite */
		for (which = 0; which < 0x20; which++)
		{
			if ((which >= 0x10) && (which <= 0x17)) continue;	/* no sprites here */

			if (sprites_on[which])
				state->m_collision_reg[3] |= check_sprite_layer_bitpattern(machine, which, sprite_areas);
		}
	}
}


static void draw_layers(running_machine &machine)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	offs_t offs;

	state->m_layer_bitmap[0].fill(TRANSPARENT_PEN);
	state->m_layer_bitmap[1].fill(TRANSPARENT_PEN);
	state->m_layer_bitmap[2].fill(TRANSPARENT_PEN);

	for (offs = 0; offs < 0x0400; offs++)
	{
		int sx = offs % 32;
		int sy = offs / 32;

		if (GLOBAL_FLIP_X) sx = 31 - sx;
		if (GLOBAL_FLIP_Y) sy = 31 - sy;

		drawgfx_transpen(state->m_layer_bitmap[0],state->m_layer_bitmap[0].cliprect(),machine.gfx[state->m_colorbank[0] & 0x08 ? 2 : 0],
				state->m_videoram_1[offs],
				state->m_colorbank[0] & 0x07,
				GLOBAL_FLIP_X,GLOBAL_FLIP_Y,
				8*sx,8*sy,0);

		drawgfx_transpen(state->m_layer_bitmap[1],state->m_layer_bitmap[1].cliprect(),machine.gfx[state->m_colorbank[0] & 0x80 ? 2 : 0],
				state->m_videoram_2[offs],
				(state->m_colorbank[0] >> 4) & 0x07,
				GLOBAL_FLIP_X,GLOBAL_FLIP_Y,
				8*sx,8*sy,0);

		drawgfx_transpen(state->m_layer_bitmap[2],state->m_layer_bitmap[2].cliprect(),machine.gfx[state->m_colorbank[1] & 0x08 ? 2 : 0],
				state->m_videoram_3[offs],
				state->m_colorbank[1] & 0x07,
				GLOBAL_FLIP_X,GLOBAL_FLIP_Y,
				8*sx,8*sy,0);
	}
}


static void draw_sprites(running_machine &machine, bitmap_ind16 &bitmap)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	/*
       sprite visibility area is missing 4 pixels from the sides, surely to reduce
       wraparound side effects. This was verified on a real Elevator Action.
       Note that the clipping is asymmetrical. This matches the real thing.
       I'm not sure of what should happen when the screen is flipped, though.
     */
	const rectangle spritevisiblearea(0*8+3, 32*8-1-1, 2*8, 30*8-1);
	const rectangle spritevisibleareaflip(0*8+1, 32*8-3-1, 2*8, 30*8-1);

	if (SPRITES_ON)
	{
		int sprite;

		/* drawing order is a bit strange. The last sprite has to be moved at the start of the list. */
		for (sprite = 0x1f; sprite >= 0; sprite--)
		{
			UINT8 sx, sy;

			int which = (sprite - 1) & 0x1f;	/* move last sprite at the head of the list */
			offs_t offs = which * 4;

			if ((which >= 0x10) && (which <= 0x17)) continue;	/* no sprites here */

			if (get_sprite_xy(state, which, &sx, &sy))
			{
				int code = state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs + 3] & 0x3f;
				int color = 2 * ((state->m_colorbank[1] >> 4) & 0x03) + ((state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs + 2] >> 2) & 0x01);
				int flip_x = state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs + 2] & 0x01;
				int flip_y = state->m_spriteram[SPRITE_RAM_PAGE_OFFSET + offs + 2] & 0x02;

				if (GLOBAL_FLIP_X)
				{
					sx = 238 - sx;
					flip_x = !flip_x;
				}

				if (GLOBAL_FLIP_Y)
				{
					sy = 242 - sy;
					flip_y = !flip_y;
				}

				drawgfx_transpen(bitmap, GLOBAL_FLIP_X ? spritevisibleareaflip : spritevisiblearea,get_sprite_gfx_element(machine, which), code, color,
						flip_x, flip_y, sx, sy,0);

				/* draw with wrap around. The horizontal games (eg. sfposeid) need this */
				drawgfx_transpen(bitmap, GLOBAL_FLIP_X ? spritevisibleareaflip : spritevisiblearea,get_sprite_gfx_element(machine, which), code, color,
						flip_x, flip_y, sx - 0x100, sy,0);
			}
		}
	}
}


static void taitosj_copy_layer(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect,
							   int which, int *sprites_on, rectangle *sprite_areas)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	static const int fudge1[3] = { 3,  1, -1 };
	static const int fudge2[3] = { 8, 10, 12 };

	if (*state->m_video_mode & layer_enable_mask[which])
	{
		int i, scrollx, scrolly[32];

		scrollx = state->m_scroll[2 * which];

		if (GLOBAL_FLIP_X)
			scrollx =  (scrollx & 0xf8) + ((scrollx + fudge1[which]) & 7) + fudge2[which];
		else
			scrollx = -(scrollx & 0xf8) + ((scrollx + fudge1[which]) & 7) + fudge2[which];

		if (GLOBAL_FLIP_Y)
			for (i = 0;i < 32;i++)
				scrolly[31 - i] =  state->m_colscrolly[32 * which + i] + state->m_scroll[2 * which + 1];
		else
			for (i = 0;i < 32;i++)
				scrolly[i]      = -state->m_colscrolly[32 * which + i] - state->m_scroll[2 * which + 1];

		copyscrollbitmap_trans(bitmap, state->m_layer_bitmap[which], 1, &scrollx, 32, scrolly, cliprect, TRANSPARENT_PEN);

		/* store parts covered with sprites for sprites/layers collision detection */
		for (i = 0; i < 0x20; i++)
		{
			if ((i >= 0x10) && (i <= 0x17)) continue; /* no sprites here */

			if (sprites_on[i])
				copyscrollbitmap(state->m_sprite_layer_collbitmap2[which], state->m_layer_bitmap[which], 1, &scrollx, 32, scrolly, sprite_areas[i]);
		}
	}
}


static void kikstart_copy_layer(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect,
								int which, int *sprites_on, rectangle *sprite_areas)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	if (*state->m_video_mode & layer_enable_mask[which])
	{
		int i, scrolly, scrollx[32 * 8];

		for (i = 1; i < 32*8; i++)	/* 1-255 ! */
			if (GLOBAL_FLIP_Y)
				switch (which)
				{
				case 0:	scrollx[32 * 8 - i] = 0 ;break;
				case 1:	scrollx[32 * 8 - i] = state->m_kikstart_scrollram[i] + ((state->m_scroll[2 * which] + 0x0a) & 0xff);break;
				case 2:	scrollx[32 * 8 - i] = state->m_kikstart_scrollram[0x100 + i] + ((state->m_scroll[2 * which] + 0xc) & 0xff);break;
				}
			else
				switch (which)
				{
				case 0:	scrollx[i] = 0 ;break;
				case 1:	scrollx[i] = 0xff - state->m_kikstart_scrollram[i - 1] - ((state->m_scroll[2 * which] - 0x10) & 0xff);break;
				case 2:	scrollx[i] = 0xff - state->m_kikstart_scrollram[0x100 + i - 1] - ((state->m_scroll[2 * which] - 0x12) & 0xff);break;
				}

		scrolly = state->m_scroll[2 * which + 1];	/* always 0 */
		copyscrollbitmap_trans(bitmap, state->m_layer_bitmap[which], 32 * 8, scrollx, 1, &scrolly, cliprect, TRANSPARENT_PEN);

		/* store parts covered with sprites for sprites/layers collision detection */
		for (i = 0; i < 0x20; i++)
		{
			if ((i >= 0x10) && (i <= 0x17)) continue; /* no sprites here */

			if (sprites_on[i])
				copyscrollbitmap(state->m_sprite_layer_collbitmap2[which], state->m_layer_bitmap[which], 32 * 8, scrollx, 1, &scrolly, sprite_areas[i]);
		}
	}
}


static void copy_layer(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect,
					   copy_layer_func_t copy_layer_func, int which, int *sprites_on, rectangle *sprite_areas)
{
	if (which == 0)
		draw_sprites(machine, bitmap);
	else
		copy_layer_func(machine, bitmap, cliprect, which - 1, sprites_on, sprite_areas);
}


static void copy_layers(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect,
						copy_layer_func_t copy_layer_func, int *sprites_on, rectangle *sprite_areas)
{
	taitosj_state *state = machine.driver_data<taitosj_state>();
	int i = 0;

	/* fill the screen with the background color */
	bitmap.fill(8 * (state->m_colorbank[1] & 0x07), cliprect);

	for (i = 0; i < 4; i++)
	{
		int which = state->m_draw_order[*state->m_video_priority & 0x1f][i];

		copy_layer(machine, bitmap, cliprect, copy_layer_func, which, sprites_on, sprite_areas);
	}
}


static void check_collision(running_machine &machine, int *sprites_on, rectangle *sprite_areas)
{
	check_sprite_sprite_collision(machine);

	check_sprite_layer_collision(machine, sprites_on, sprite_areas);

	/*check_layer_layer_collision();*/	/* not implemented !!! */
}


static int video_update_common(running_machine &machine, bitmap_ind16 &bitmap,
							   const rectangle &cliprect, copy_layer_func_t copy_layer_func)
{
	int sprites_on[0x20];			/* 1 if sprite is active */
	rectangle sprite_areas[0x20];	/* areas on bitmap (sprite locations) */

	set_pens(machine);

	draw_layers(machine);

	calculate_sprite_areas(machine, sprites_on, sprite_areas);

	copy_layers(machine, bitmap, cliprect, copy_layer_func, sprites_on, sprite_areas);

	/*check_sprite_layer_collision() uses drawn bitmaps, so it must me called _AFTER_ draw_layers() */
	check_collision(machine, sprites_on, sprite_areas);

	return 0;
}


SCREEN_UPDATE_IND16( taitosj )
{
	return video_update_common(screen.machine(), bitmap, cliprect, taitosj_copy_layer);
}


SCREEN_UPDATE_IND16( kikstart )
{
	return video_update_common(screen.machine(), bitmap, cliprect, kikstart_copy_layer);
}
