/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "video/resnet.h"



UINT8 *circusc_videoram,*circusc_colorram;
static tilemap *bg_tilemap;

UINT8 *circusc_spritebank;
UINT8 *circusc_scroll;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Circus Charlie has one 32x8 palette PROM and two 256x4 lookup table PROMs
  (one for characters, one for sprites).
  The palette PROM is connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/

PALETTE_INIT( circusc )
{
	static const int resistances_rg[3] = { 1000, 470, 220 };
	static const int resistances_b [2] = { 470, 220 };
	double rweights[3], gweights[3], bweights[2];
	int i;

	/* compute the color output resistor weights */
	compute_resistor_weights(0,	255, -1.0,
			3, &resistances_rg[0], rweights, 0, 0,
			3, &resistances_rg[0], gweights, 0, 0,
			2, &resistances_b[0],  bweights, 0, 0);

	/* allocate the colortable */
	machine->colortable = colortable_alloc(machine, 32);

	/* create a lookup table for the palette */
	for (i = 0; i < 0x20; i++)
	{
		int bit0, bit1, bit2;
		int r, g, b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = combine_3_weights(rweights, bit0, bit1, bit2);

		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = combine_3_weights(gweights, bit0, bit1, bit2);

		/* blue component */
		bit0 = (color_prom[i] >> 6) & 0x01;
		bit1 = (color_prom[i] >> 7) & 0x01;
		b = combine_2_weights(bweights, bit0, bit1);

		colortable_palette_set_color(machine->colortable, i, MAKE_RGB(r, g, b));
	}

	/* color_prom now points to the beginning of the lookup table */
	color_prom += 32;

	/* characters map to the upper 16 palette entries */
	for (i = 0; i < 0x100; i++)
	{
		UINT8 ctabentry = color_prom[i] & 0x0f;
		colortable_entry_set_value(machine->colortable, i, ctabentry + 0x10);
	}

	/* sprites map to the lower 16 palette entries */
	for (i = 0x100; i < 0x200; i++)
	{
		UINT8 ctabentry = color_prom[i] & 0x0f;
		colortable_entry_set_value(machine->colortable, i, ctabentry);
	}
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static TILE_GET_INFO( get_tile_info )
{
	UINT8 attr = circusc_colorram[tile_index];
	tileinfo->category = (attr & 0x10) >> 4;

	SET_TILE_INFO(0,
				  circusc_videoram[tile_index] + ((attr & 0x20) << 3),
				  attr & 0x0f,
				  TILE_FLIPYX((attr & 0xc0) >> 6));
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( circusc )
{
	bg_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_TYPE_PEN,8,8,32,32);

	tilemap_set_scroll_cols(bg_tilemap,32);
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE8_HANDLER( circusc_videoram_w )
{
	circusc_videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}

WRITE8_HANDLER( circusc_colorram_w )
{
	circusc_colorram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}

WRITE8_HANDLER( circusc_flipscreen_w )
{
	flip_screen_set(data & 1);
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(running_machine *machine, mame_bitmap *bitmap, const rectangle *cliprect)
{
	int offs;
	UINT8 *sr;


	if ((*circusc_spritebank & 0x01) != 0)
		sr = spriteram;
	else
		sr = spriteram_2;

	for (offs = 0; offs < spriteram_size;offs += 4)
	{
		int code = sr[offs + 0] + 8 * (sr[offs + 1] & 0x20);
		int color = sr[offs + 1] & 0x0f;
		int sx = sr[offs + 2];
		int sy = sr[offs + 3];
		int flipx = sr[offs + 1] & 0x40;
		int flipy = sr[offs + 1] & 0x80;

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}


		drawgfx(bitmap,machine->gfx[1],
				code, color,
				flipx,flipy,
				sx,sy,
				cliprect,TRANSPARENCY_PENS,
				colortable_get_transpen_mask(machine->colortable, machine->gfx[1], color, 0));
	}
}

VIDEO_UPDATE( circusc )
{
	int i;

	for (i = 0;i < 10;i++)
		tilemap_set_scrolly(bg_tilemap,i,0);
	for (i = 10;i < 32;i++)
		tilemap_set_scrolly(bg_tilemap,i,*circusc_scroll);

	fillbitmap(bitmap, machine->pens[0], cliprect);
	tilemap_draw(bitmap,cliprect,bg_tilemap,1,0);
	draw_sprites(machine,bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	return 0;
}
