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

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <fnmatch.h>
#include <sys/time.h>
#include "parse_span_specs.h"

void free_span_specifications(struct span_specs *span_specs)
{
	if (span_specs) {
		if (span_specs->buf)
			free(span_specs->buf);
		free(span_specs);
	}
}

static enum tdm_codec is_alaw_span_type(const char *span_type)
{
	assert(span_type);
	if (strcmp(span_type, "E1") == 0)
		return TDM_CODEC_ALAW;
	else if (strcmp(span_type, "T1") == 0)
		return TDM_CODEC_ULAW;
	return TDM_CODEC_UNKNOWN;
}

struct span_specs *parse_span_specifications(const char *spec_string, int default_is_alaw)
{
	struct span_specs *span_specs;
	char *p;
	int spanno;
	int i;

	if (!spec_string)
		return NULL;
	/* Allocate  and Initialize */
	span_specs = calloc(sizeof(char *), MAX_SPANNO);
	if (!span_specs)
		goto err;
	for (spanno = 0; spanno < MAX_SPANNO; spanno++)
		span_specs->span_is_alaw[spanno] = TDM_CODEC_UNKNOWN;
	span_specs->buf = strdup(spec_string);
	if (!span_specs->buf)
		goto err;
	for (i = 0;; i++) {
		char *curr_item;
		char *tokenize_key;
		char *key;
		char *value;
		enum tdm_codec is_alaw;
		int matched;

		/* Split to items */
		p = (i == 0) ? span_specs->buf : NULL;
		p = strtok_r(p, " \t,", &curr_item);
		if (!p)
			break;

		/* Split to <span>:<type> */
		key = strtok_r(p, ":", &tokenize_key);
		if (!key) {
			fprintf(stderr,
				"Missing ':' (item #%d inside '%s')\n",
				i+1, spec_string);
			goto err;
		}
		value = strtok_r(NULL, ":", &tokenize_key);
		if (!value) {
			fprintf(stderr,
				"Missing value after ':' (item #%d inside '%s')\n",
				i+1, spec_string);
			goto err;
		}

		/* Match span specification and set alaw/ulaw */
		is_alaw = is_alaw_span_type(value);
		if (is_alaw == TDM_CODEC_UNKNOWN) {
			fprintf(stderr,
				"Illegal span type '%s' (item #%d inside '%s')\n",
				value, i+1, spec_string);
			goto err;
		}
		matched = 0;
		for (spanno = 0; spanno < MAX_SPANNO; spanno++) {
			char tmpbuf[BUFSIZ];

			snprintf(tmpbuf, sizeof(tmpbuf), "%d", spanno + 1);
			if (fnmatch(p, tmpbuf, 0) == 0) {
				matched++;
				span_specs->span_is_alaw[spanno] = is_alaw;
			}
		}
		if (!matched) {
			fprintf(stderr,
				"Span specification '%s' does not match any span (item #%d inside '%s')\n",
				key, i+1, spec_string);
			goto err;
		}
	}

	/* Set defaults */
	for (spanno = 0; spanno < MAX_SPANNO; spanno++) {
		if (span_specs->span_is_alaw[spanno] == TDM_CODEC_UNKNOWN) {
			span_specs->span_is_alaw[spanno] = default_is_alaw;
		}
	}
	return span_specs;
err:
	free_span_specifications(span_specs);
	return NULL;
}

void print_span_specifications(struct span_specs *span_specs, FILE *output)
{
	int spanno;

	if (!span_specs)
		return;
	for (spanno = 0; spanno < MAX_SPANNO; spanno++) {
		enum tdm_codec is_alaw;

		is_alaw = span_specs->span_is_alaw[spanno];
		fprintf(output, "%d %s\n",
			spanno+1, (is_alaw == TDM_CODEC_ALAW) ? "alaw" : "ulaw");
	}
}
