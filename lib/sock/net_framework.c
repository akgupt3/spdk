/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "spdk/log.h"
#include "spdk/net.h"
#include "spdk/queue.h"

static STAILQ_HEAD(, spdk_net_framework) g_net_frameworks =
	STAILQ_HEAD_INITIALIZER(g_net_frameworks);

static spdk_net_init_cb g_init_cb_fn = NULL;
static void *g_init_cb_arg = NULL;

static spdk_net_fini_cb g_fini_cb_fn = NULL;
static void *g_fini_cb_arg = NULL;

struct spdk_net_framework *g_next_net_framework = NULL;

static inline struct spdk_net_framework *
get_next_net_framework(struct spdk_net_framework *net)
{
	return net ? STAILQ_NEXT(net, link) : STAILQ_FIRST(&g_net_frameworks);
}

void
spdk_net_framework_init_next(int rc)
{
	if (rc) {
		SPDK_ERRLOG("Net framework %s failed to initalize with error %d\n", g_next_net_framework->name, rc);
		g_init_cb_fn(g_init_cb_arg, rc);
		return;
	}

	g_next_net_framework = get_next_net_framework(g_next_net_framework);
	if (g_next_net_framework == NULL) {
		g_init_cb_fn(g_init_cb_arg, 0);
		return;
	}

	g_next_net_framework->init();
}

void
spdk_net_framework_start(spdk_net_init_cb cb_fn, void *cb_arg)
{
	g_init_cb_fn = cb_fn;
	g_init_cb_arg = cb_arg;

	spdk_net_framework_init_next(0);
}

void
spdk_net_framework_fini_next(void)
{
	g_next_net_framework = get_next_net_framework(g_next_net_framework);
	if (g_next_net_framework == NULL) {
		g_fini_cb_fn(g_fini_cb_arg);
		return;
	}

	g_next_net_framework->fini();
}

void
spdk_net_framework_fini(spdk_net_fini_cb cb_fn, void *cb_arg)
{
	g_fini_cb_fn = cb_fn;
	g_fini_cb_arg = cb_arg;

	spdk_net_framework_fini_next();
}

void
spdk_net_framework_register(struct spdk_net_framework *frame)
{
	STAILQ_INSERT_TAIL(&g_net_frameworks, frame, link);
}
