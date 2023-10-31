/*
 * Copyright (c) 2020–2021 Qorvo, Inc
 *
 * All rights reserved.
 *
 * NOTICE: All information contained herein is, and remains the property
 * of Qorvo, Inc. and its suppliers, if any. The intellectual and technical
 * concepts herein are proprietary to Qorvo, Inc. and its suppliers, and
 * may be covered by patents, patent applications, and are protected by
 * trade secret and/or copyright law. Dissemination of this information
 * or reproduction of this material is strictly forbidden unless prior written
 * permission is obtained from Qorvo, Inc.
 *
 */

#ifndef COMPAT_NET_GENETLINK_H
#define COMPAT_NET_GENETLINK_H

#include "net/netlink.h"

struct genl_info {
	struct netlink_ext_ack *extack;
	u32 snd_seq;
	u32 snd_portid;
};

#endif /* COMPAT_NET_GENETLINK_H */
