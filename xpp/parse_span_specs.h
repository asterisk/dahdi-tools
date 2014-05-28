#ifndef	PARSE_SPAN_SPECS_H
#define	PARSE_SPAN_SPECS_H

/*
 * Written by Oron Peled <oron@actcom.co.il>
 * Copyright (C) 2014, Xorcom
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define	MAX_SPANNO	4	/* E1/T1 spans -- always in first unit. 1-based */

enum tdm_codec {
	TDM_CODEC_UNKNOWN,
	TDM_CODEC_ULAW,
	TDM_CODEC_ALAW,
};

struct span_specs {
	char *buf;
	enum tdm_codec span_is_alaw[MAX_SPANNO];
};

struct span_specs *parse_span_specifications(const char *spec_string, int default_is_alaw);
void free_span_specifications(struct span_specs *span_specs);
void print_span_specifications(struct span_specs *span_specs, FILE *output);

#endif	/* PARSE_SPAN_SPECS_H */
