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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "pesign.h"

int read_cert(int certfd, CERTCertificate **cert)
{
	struct stat statbuf;
	char *certstr = NULL;
	int rc;

	rc = fstat(certfd, &statbuf);
	if (rc < 0)
		return rc;

	int i = 0, j = statbuf.st_size;
	certstr = calloc(1, j + 1);
	if (!certstr)
		return -1;

	while (i < statbuf.st_size) {
		int x;
		x = read(certfd, certstr + i, j);
		if (x < 0) {
			free(certstr);
			return -1;
		}
		i += x;
		j -= x;
	}

	*cert = CERT_ConvertAndDecodeCertificate(certstr);
	free(certstr);
	if (!*cert)
		return -1;
	return 0;
}