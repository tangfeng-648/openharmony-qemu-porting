/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Ethernet Bonding driver Vendor Hooks
 *
 * Copyright (c) 2022, Huawei Tech. Co., Ltd.
 */

#ifndef TRACE_HOOKS_BONDING_H
#define TRACE_HOOKS_BONDING_H

#ifdef CONFIG_VENDOR_BOND_HOOKS

#undef TRACE_SYSTEM
#define TRACE_SYSTEM bonding

#define TRACE_INCLUDE_PATH trace / hooks
#if !defined(_TRACE_HOOK_BONDING_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_BONDING_H
#include <linux/tracepoint.h>
#include <trace/hooks/vendor_hooks.h>

DECLARE_HOOK(vendor_bond_check_dev_link, TP_PROTO(uint16_t intGpioNum, void *data), TP_ARGS(intGpioNum, data));

#endif
/* This part must be outside protection */
#include <trace/define_trace.h>

#endif

#endif /* TRACE_HOOKS_BONDING_H */
