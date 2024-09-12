/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-09-12     latercomer   the first version
 */

#define LOG_TAG "uorb"
#define LOG_LVL LOG_LVL_INFO

#include "uORB.h"
#include <rtdbg.h>

static rt_list_t _node_list;

// round up to nearest power of two
// Such as 0 => 1, 1 => 1, 2 => 2 ,3 => 4, 10 => 16, 60 => 64, 65...255 => 128
// Note: When the input value > 128, the output is always 128
static inline rt_uint8_t
round_pow_of_two_8(rt_uint8_t n)
{
    if (n == 0)
    {
        return 1;
    }

    // Avoid is already a power of 2
    rt_uint8_t value = n - 1;

    // Fill 1
    value |= value >> 1U;
    value |= value >> 2U;
    value |= value >> 4U;

    // Unable to round-up, take the value of round-down
    if (value == RT_UINT8_MAX)
    {
        value >>= 1U;
    }

    return value + 1;
}


struct rt_uorb_node *uorb_node_create(const struct orb_metadata *meta, const rt_uint8_t instance, rt_uint8_t queue_size)
{
    RT_ASSERT(meta != RT_NULL);

    struct rt_uorb_node *node = (struct rt_uorb_node *)rt_calloc(sizeof(struct rt_uorb_node), 1);

    node->meta             = meta;
    node->instance         = instance;
    node->queue_size       = queue_size;
    node->generation       = 0;
    node->advertised       = 0;
    node->subscriber_count = 0;
    node->data_valid       = 0;
    node->data             = RT_NULL;

    char *name = rt_calloc(RT_NAME_MAX, 1);
    rt_snprintf(name, RT_NAME_MAX, "%s%d", meta->o_name, instance);

    rt_list_insert_after(_node_list.prev, &node->list);

    // 注册设备
    rt_uorb_register(node, name, 0, RT_NULL);

    return node;
}

rt_err_t uorb_node_delete(struct rt_uorb_node *node)
{
    return 0;
}

struct rt_uorb_node *uorb_node_find(const struct orb_metadata *meta, int instance)
{
    // 遍历_node_list
}

int uorb_node_read(struct rt_uorb_node *node, void *data, int *generation)
{
    RT_ASSERT(node != RT_NULL);
    RT_ASSERT(data != RT_NULL);

    if (!node->data)
    {
        return 0;
    }

    if (node->queue_size == 1)
    {
        rt_memcpy(data, node->data, node->meta->o_size);
        if (generation)
        {
            generation = node->generation;
        }
    }
    else
    {
        // TODO:
    }

    return node->meta->o_size;
}


int uorb_node_write(struct rt_uorb_node *node, void *data)
{
    RT_ASSERT(node != RT_NULL);
    RT_ASSERT(data != RT_NULL);

    if (!node->data)
    {
        const size_t size = node->meta->o_size * node->queue_size;
        node->data        = rt_calloc(size, 1);
    }

    if (!node->data)
    {
        return -1;
    }

    rt_memcpy(node->data, (node->meta->o_size * node->generation % node->queue_size), data, node->meta->o_size);

    // callbacks
    // for ()
    // {

    // }

    node->data_valid = 1;
    node->generation++;

    return node->meta->o_size;
}

orb_subscribe_t orb_subscribe_multi(const struct orb_metadata *meta, unsigned instance)
{
    struct rt_uorb_subscribe *sub = rt_calloc(sizeof(struct rt_uorb_subscribe), 1);

    sub->meta       = meta;
    sub->instance   = instance;
    sub->generation = 0;
    sub->node       = uorb_node_find(meta, instance);

    return sub;
}

int orb_check(orb_subscribe_t sub, rt_bool_t *updated)
{
}

int orb_copy(const struct orb_metadata *meta, int handle, void *buffer)
{
}

int orb_unsubscribe(int handle)
{
}

orb_advertise_t orb_advertise_multi_queue(const struct orb_metadata *meta, const void *data, int *instance,
                                          unsigned int queue_size)
{
}


int orb_publish(const struct orb_metadata *meta, orb_advertise_t handle, const void *data)
{
}

int orb_unadvertise(orb_advertise_t handle)
{
}
