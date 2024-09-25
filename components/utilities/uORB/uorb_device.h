/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-09-12     latercomer   the first version
 */


#ifndef _UORB_DEVICE_H_
#define _UORB_DEVICE_H_

#include "uORB.h"
#include <rtdevice.h>


struct rt_uorb_device
{
    struct rt_device  parent;
    struct orb_node_s node;
};


#endif //_UORB_DEVICE_H_
