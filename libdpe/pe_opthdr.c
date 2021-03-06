/*
 * Copyright 2011 Red Hat, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s): Peter Jones <pjones@redhat.com>
 */

#include "libdpe.h"

void *
pe_getopthdr(Pe *pe)
{
	switch (pe_kind(pe)) {
	case PE_K_PE_EXE:
		return pe->state.pe32_exe.opthdr;
	case PE_K_PE64_EXE:
		return pe->state.pe32plus_exe.opthdr;
	default:
		return NULL;
	}
}
