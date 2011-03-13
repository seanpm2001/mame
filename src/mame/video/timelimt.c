#include "emu.h"
#include "includes/timelimt.h"


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Time Limit has two 32 bytes palette PROM, connected to the RGB output this
  way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/

PALETTE_INIT( timelimt ) {
	int i;

	for (i = 0;i < machine->total_colors();i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (*color_prom >> 6) & 0x01;
		bit1 = (*color_prom >> 7) & 0x01;
		b = 0x4f * bit0 + 0xa8 * bit1;

		palette_set_color(machine,i,MAKE_RGB(r,g,b));
		color_prom++;
	}
}

/***************************************************************************

    Start the video hardware emulation.

***************************************************************************/

static TILE_GET_INFO( get_bg_tile_info )
{
	timelimt_state *state = machine->driver_data<timelimt_state>();
	SET_TILE_INFO(1, state->bg_videoram[tile_index], 0, 0);
}

static TILE_GET_INFO( get_fg_tile_info )
{
	timelimt_state *state = machine->driver_data<timelimt_state>();
	UINT8 *videoram = state->videoram;
	SET_TILE_INFO(0, videoram[tile_index], 0, 0);
}

VIDEO_START( timelimt )
{
	timelimt_state *state = machine->driver_data<timelimt_state>();
	state->bg_tilemap = tilemap_create(machine, get_bg_tile_info, tilemap_scan_rows,
		 8, 8, 64, 32);

	state->fg_tilemap = tilemap_create(machine, get_fg_tile_info, tilemap_scan_rows,
		 8, 8, 32, 32);

	tilemap_set_transparent_pen(state->fg_tilemap, 0);
}

/***************************************************************************/

WRITE8_HANDLER( timelimt_videoram_w )
{
	timelimt_state *state = space->machine->driver_data<timelimt_state>();
	UINT8 *videoram = state->videoram;
	videoram[offset] = data;
	tilemap_mark_tile_dirty(state->fg_tilemap, offset);
}

WRITE8_HANDLER( timelimt_bg_videoram_w )
{
	timelimt_state *state = space->machine->driver_data<timelimt_state>();
	state->bg_videoram[offset] = data;
	tilemap_mark_tile_dirty(state->bg_tilemap, offset);
}

WRITE8_HANDLER( timelimt_scroll_x_lsb_w )
{
	timelimt_state *state = space->machine->driver_data<timelimt_state>();
	state->scrollx &= 0x100;
	state->scrollx |= data & 0xff;
}

WRITE8_HANDLER( timelimt_scroll_x_msb_w )
{
	timelimt_state *state = space->machine->driver_data<timelimt_state>();
	state->scrollx &= 0xff;
	state->scrollx |= ( data & 1 ) << 8;
}

WRITE8_HANDLER( timelimt_scroll_y_w )
{
	timelimt_state *state = space->machine->driver_data<timelimt_state>();
	state->scrolly = data;
}


static void draw_sprites(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	timelimt_state *state = machine->driver_data<timelimt_state>();
	UINT8 *spriteram = state->spriteram;
	int offs;

	for( offs = state->spriteram_size; offs >= 0; offs -= 4 )
	{
		int sy = 240 - spriteram[offs];
		int sx = spriteram[offs+3];
		int code = spriteram[offs+1] & 0x3f;
		int attr = spriteram[offs+2];
		int flipy = spriteram[offs+1] & 0x80;
		int flipx = spriteram[offs+1] & 0x40;

		code += ( attr & 0x80 ) ? 0x40 : 0x00;
		code += ( attr & 0x40 ) ? 0x80 : 0x00;

		drawgfx_transpen( bitmap, cliprect,machine->gfx[2],
				code,
				attr & 7,
				flipx,flipy,
				sx,sy,0);
	}
}


SCREEN_UPDATE( timelimt )
{
	timelimt_state *state = screen->machine->driver_data<timelimt_state>();
	tilemap_set_scrollx(state->bg_tilemap, 0, state->scrollx);
	tilemap_set_scrolly(state->bg_tilemap, 0, state->scrolly);
	tilemap_draw(bitmap, cliprect, state->bg_tilemap, 0, 0);

	draw_sprites(screen->machine, bitmap, cliprect);

	tilemap_draw(bitmap, cliprect, state->fg_tilemap, 0, 0);
	return 0;
}
