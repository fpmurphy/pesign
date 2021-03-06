/*
 * Copyright 2011-2012 Red Hat, Inc.
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
#include <string.h>

#include "pesign.h"

#include <nspr4/prerror.h>
#include <nss3/nss.h>

static int
generate_algorithm_id_list(SECAlgorithmID ***algorithm_list_p, cms_context *ctx)
{
	SECAlgorithmID **algorithms = NULL;
	int err = 0;

	algorithms = PORT_ArenaZAlloc(ctx->arena, sizeof (SECAlgorithmID *) *
						  2);
	if (!algorithms)
		return -1;

	algorithms[0] = PORT_ArenaZAlloc(ctx->arena, sizeof(SECAlgorithmID));
	if (!algorithms[0]) {
		err = PORT_GetError();
		goto err_list;
	}

	if (generate_algorithm_id(ctx, algorithms[0],
			ctx->digest_oid_tag) < 0) {
		err = PORT_GetError();
		goto err_item;
	}

	*algorithm_list_p = algorithms;
	return 0;
err_item:
	PORT_ZFree(algorithms[0], sizeof (SECAlgorithmID));
err_list:
	PORT_ZFree(algorithms, sizeof (SECAlgorithmID *) * 2);
	PORT_SetError(err);
	return -1;
}

void
free_algorithm_list(SECAlgorithmID **algorithm_list, cms_context *ctx)
{
	if (!algorithm_list)
		return;

#if 0
	for (int i = 0; algorithm_list[i] != NULL; i++) {
		PORT_ZFree(algorithm_list[i], sizeof (SECAlgorithmID));
	}
	PORT_ZFree(algorithm_list, sizeof (SECAlgorithmID *) * 2);
#endif
}

static int
generate_certificate_list(SECItem ***certificate_list_p, cms_context *ctx)
{
	SECItem **certificates = NULL;

	certificates = PORT_ArenaZAlloc(ctx->arena, sizeof (SECItem *) * 2);
	if (!certificates)
		return -1;
	
	certificates[0] = PORT_ArenaZAlloc(ctx->arena, sizeof (SECItem));
	if (!certificates[0]) {
		int err = PORT_GetError();
		PORT_ZFree(certificates, sizeof (SECItem) * 2);
		PORT_SetError(err);
		return -1;
	}

	SECITEM_CopyItem(ctx->arena, certificates[0], &ctx->cert->derCert);
	*certificate_list_p = certificates;
	return 0;
}

static void
free_certificate_list(SECItem **certificate_list, cms_context *ctx)
{
	if (!certificate_list)
		return;

#if 0
	for (int i = 0; certificate_list[i] != NULL; i++)
		PORT_Free(certificate_list[i]);
	PORT_ZFree(certificate_list, sizeof (SECItem) * 2);
#endif
}

int
generate_signerInfo_list(SpcSignerInfo ***signerInfo_list_p, cms_context *ctx)
{
	SpcSignerInfo **signerInfo_list;
	int err;

	if (!signerInfo_list_p)
		return -1;

	signerInfo_list = PORT_ArenaZAlloc(ctx->arena,
					sizeof (SpcSignerInfo *) * 2);
	if (!signerInfo_list)
		return -1;

	signerInfo_list[0] = PORT_ArenaZAlloc(ctx->arena,
						sizeof (SpcSignerInfo));
	if (!signerInfo_list[0]) {
		err = PORT_GetError();
		goto err_list;
	}
	
	if (generate_spc_signer_info(signerInfo_list[0], ctx) < 0) {
		err = PORT_GetError();
		goto err_item;
	}

	*signerInfo_list_p = signerInfo_list;
	return 0;
err_item:
#if 0
	PORT_ZFree(signerInfo_list[0], sizeof (SpcSignerInfo));
#endif
err_list:
#if 0
	PORT_ZFree(signerInfo_list, sizeof (SpcSignerInfo *) * 2);
#endif
	PORT_SetError(err);
	return -1;
}

void
free_signerInfo_list(SpcSignerInfo **signerInfo_list, cms_context *ctx)
{
	
}

typedef struct {
	SECItem version;
	SECAlgorithmID **algorithms;
	SpcContentInfo cinfo;
	SECItem **certificates;
	SECItem **crls;
	SpcSignerInfo **signerInfos;
} SignedData;

SEC_ASN1Template SignedDataTemplate[] = {
	{
	.kind = SEC_ASN1_SEQUENCE,
	.offset = 0,
	.sub = NULL,
	.size = sizeof (SignedData)
	},
	{
	.kind = SEC_ASN1_INTEGER,
	.offset = offsetof(SignedData, version),
	.sub = &SEC_IntegerTemplate,
	.size = sizeof (SECItem)
	},
	{
	.kind = SEC_ASN1_SET_OF,
	.offset = offsetof(SignedData, algorithms),
	.sub = &AlgorithmIDTemplate,
	.size = sizeof (SECItem),
	},
	{
	.kind = SEC_ASN1_INLINE,
	.offset = offsetof(SignedData, cinfo),
	.sub = &SpcContentInfoTemplate,
	.size = sizeof (SpcContentInfo),
	},
	{
	.kind = SEC_ASN1_CONTEXT_SPECIFIC | 0 |
		SEC_ASN1_CONSTRUCTED |
		SEC_ASN1_OPTIONAL,
	.offset = offsetof(SignedData, certificates),
	.sub = &SEC_SetOfAnyTemplate,
	.size = sizeof(SECItem**),
	},
	{
	.kind = SEC_ASN1_CONTEXT_SPECIFIC | 1 |
		SEC_ASN1_CONSTRUCTED |
		SEC_ASN1_OPTIONAL,
	.offset = offsetof(SignedData, crls),
	.sub = &SEC_SetOfAnyTemplate,
	.size = sizeof (SECItem **),
	},
	{
	.kind = SEC_ASN1_SET_OF,
	.offset = offsetof(SignedData, signerInfos),
	.sub = &SpcSignerInfoTemplate,
	.size = 0,
	},
	{ 0, }
};

typedef struct {
	SECItem contentType;
	SECItem content;
} ContentInfo;

SEC_ASN1Template ContentInfoTemplate[] = {
	{
	.kind = SEC_ASN1_SEQUENCE,
	.offset = 0,
	.sub = NULL,
	.size = sizeof (ContentInfo),
	},
	{
	.kind = SEC_ASN1_OBJECT_ID,
	.offset = offsetof(ContentInfo, contentType),
	.sub = &SEC_ObjectIDTemplate,
	.size = sizeof (SECItem),
	},
	{
	.kind = SEC_ASN1_CONTEXT_SPECIFIC | 0 |
		SEC_ASN1_CONSTRUCTED |
		SEC_ASN1_EXPLICIT,
	.offset = offsetof(ContentInfo, content),
	.sub = &SEC_AnyTemplate,
	.size = sizeof (SECItem),
	},
	{ 0, } 
};

int
generate_spc_signed_data(SECItem *sdp, cms_context *ctx)
{
	SignedData sd;

	if (!sdp)
		return -1;

	memset(&sd, '\0', sizeof (sd));

	if (SEC_ASN1EncodeInteger(ctx->arena, &sd.version, 1) == NULL)
		return -1;

	if (generate_algorithm_id_list(&sd.algorithms, ctx) < 0)
		goto err;
	
	if (generate_spc_content_info(&sd.cinfo, ctx) < 0)
		goto err_algorithms;

	if (generate_certificate_list(&sd.certificates, ctx) < 0)
		goto err_cinfo;

	sd.crls = NULL;

	if (generate_signerInfo_list(&sd.signerInfos, ctx) < 0)
		goto err_certificate_list;

	SECItem encoded = { 0, };
	if (SEC_ASN1EncodeItem(ctx->arena, &encoded, &sd, SignedDataTemplate)
			== NULL) {
		fprintf(stderr, "Could not encode SignedData: %s\n",
			PORT_ErrorToString(PORT_GetError()));
		goto err_signer_infos;
	}

	ContentInfo sdw;
	memset(&sdw, '\0', sizeof (sdw));

	SECOidData *oid = SECOID_FindOIDByTag(SEC_OID_PKCS7_SIGNED_DATA);

	memcpy(&sdw.contentType, &oid->oid, sizeof (sdw.contentType));
	memcpy(&sdw.content, &encoded, sizeof (sdw.content));

	SECItem wrapper = { 0, };
	if (SEC_ASN1EncodeItem(ctx->arena, &wrapper, &sdw,
			ContentInfoTemplate) == NULL) {
		fprintf(stderr, "Could not encode SignedData: %s\n",
			PORT_ErrorToString(PORT_GetError()));
		goto err_signed_data;
	}

	memcpy(sdp, &wrapper, sizeof(*sdp));
	return 0;
err_signed_data:
	SECITEM_FreeItem(&encoded, PR_FALSE);
err_signer_infos:
	free_signerInfo_list(sd.signerInfos, ctx);
err_certificate_list:
	free_certificate_list(sd.certificates, ctx);
err_cinfo:
	free_spc_content_info(&sd.cinfo, ctx);
err_algorithms:
	free_algorithm_list(sd.algorithms, ctx);
err:
#if 0
	SECITEM_FreeItem(&sd.version, PR_TRUE);
#endif
	return -1;
}
