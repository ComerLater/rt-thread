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
#include <stdbool.h>

static rt_list_t _orb_node_list;

// Determine the data range
static inline bool is_in_range(unsigned left, unsigned value, unsigned right)
{
    if (right > left)
    {
        return (left <= value) && (value <= right);
    }
    else
    { // Maybe the data overflowed and a wraparound occurred
        return (left <= value) || (value <= right);
    }
}

// round up to nearest power of two
// Such as 0 => 1, 1 => 1, 2 => 2 ,3 => 4, 10 => 16, 60 => 64, 65...255 => 128
// Note: When the input value > 128, the output is always 128
static inline rt_uint8_t round_pow_of_two_8(rt_uint8_t n)
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


orb_node_t *orb_node_create(const struct orb_metadata_s *meta, const rt_uint8_t instance, rt_uint8_t queue_size)
{
    RT_ASSERT(meta != RT_NULL);

    orb_node_t *node = (orb_node_t *)rt_calloc(sizeof(orb_node_t), 1);

    node->meta             = meta;
    node->instance         = instance;
    node->queue_size       = round_pow_of_two_8(queue_size);
    node->generation       = 0;
    node->advertised       = 0;
    node->subscriber_count = 0;
    node->data_valid       = 0;
    node->data             = RT_NULL;


    rt_list_insert_after(_orb_node_list.prev, &node->list);

    // 注册设备
    // char name[RT_NAME_MAX];
    // rt_snprintf(name, RT_NAME_MAX, "%s%d", meta->o_name, instance);
    // rt_uorb_register(node, name, 0, RT_NULL);

    return node;
}

rt_err_t orb_node_delete(orb_node_t *node)
{
    RT_ASSERT(node != RT_NULL);

    if (!node)
    {
        return -RT_ERROR;
    }

    node->advertised = false;

    if (node->subscriber_count == 0)
    {
        rt_free(node);
    }

    return RT_EOK;
}


orb_node_t *orb_node_find(const struct orb_metadata_s *meta, int instance)
{
    // 遍历_node_list
    rt_list_t  *pos;
    orb_node_t *node;

    rt_enter_critical();
    rt_list_for_each(pos, &_orb_node_list)
    {
        node = rt_list_entry(pos, orb_node_t, list);
        if (node->meta == meta && node->instance == instance)
        {
            rt_exit_critical();
            return node;
        }
    }
    rt_exit_critical();


    return RT_NULL;
}

bool orb_node_exists(const struct orb_metadata_s *meta, int instance)
{
    if (!meta)
    {
        return false;
    }

    if (instance < 0 || instance > (ORB_MULTI_MAX_INSTANCES - 1))
    {
        return false;
    }

    orb_node_t *node = orb_node_find(meta, instance);

    if (node && node->advertised)
    {
        return true;
    }

    return false;
}


int orb_node_read(orb_node_t *node, void *data, int *generation)
{
    RT_ASSERT(node != RT_NULL);
    RT_ASSERT(data != RT_NULL);

    if (!node->data)
    {
        return 0;
    }


    const unsigned current_generation = node->generation;
    unsigned       updated_generation = generation ? (*generation) : current_generation;


    if (node->queue_size == 1)
    {
        rt_memcpy(data, node->data, node->meta->o_size);
        updated_generation = current_generation;
    }
    else
    {
        if (current_generation == updated_generation)
        {
            updated_generation--;
        }

        if (!is_inrange(current_generation - node->queue_size, updated_generation, current_generation - 1))
        {
            updated_generation = current_generation - node->queue_size;
        }

        rt_memcpy(data, node->data + (node->meta->o_size * (updated_generation % node->queue_size)), node->meta->o_size);

        updated_generation++;
    }

    if (generation)
    {
        generation = updated_generation;
    }

    return node->meta->o_size;
}


int orb_node_write(orb_node_t *node, void *data)
{
    RT_ASSERT(node != RT_NULL);
    RT_ASSERT(data != RT_NULL);

    // create buffer
    if (!node->data)
    {
        const size_t size = node->meta->o_size * node->queue_size;
        node->data        = rt_calloc(size, 1);
    }

    // buffer invalid
    if (!node->data)
    {
        return -RT_ERROR;
    }

    rt_enter_critical();

    // copy data to buffer
    rt_memcpy(node->data + (node->meta->o_size * (node->generation % node->queue_size)), data, node->meta->o_size);

    // invoke callbacks
    rt_list_t      *pos;
    orb_callback_t *item;
    rt_list_for_each(pos, &node->callbacks)
    {
        item = rt_list_entry(pos, orb_callback_t, list);
        if (item->call)
        {
            item->call();
        }
    }

    // mark data valid
    node->data_valid = true;

    // update generation
    node->generation++;

    rt_exit_critical();

    return node->meta->o_size;
}

bool orb_node_ready(orb_subscribe_t *handle)
{
    if (!handle)
    {
        return false;
    }

    if (handle->node)
    {
        return handle->node->advertised;
    }


    handle->node = orb_node_find(handle->meta, handle->instance);

    if (handle->node)
    {
        handle->node->subscriber_count++;
        handle->generation = handle->node->generation - (handle->node->data_valid ? 1 : 0);

        return handle->node->advertised;
    }

    return false;
}

orb_subscribe_t *orb_subscribe_multi(const struct orb_metadata_s *meta, unsigned instance)
{
    orb_subscribe_t *sub = rt_calloc(sizeof(orb_subscribe_t), 1);

    sub->meta       = meta;
    sub->instance   = instance;
    sub->interval   = 0;
    sub->generation = 0;
    sub->node       = orb_node_find(meta, instance);

    return sub;
}

int orb_unsubscribe(orb_subscribe_t *handle)
{
    if (!handle)
    {
        return -RT_ERROR;
    }

    if (handle->node)
    {
        handle->node->subscriber_count--;
    }

    handle->node       = RT_NULL;
    handle->generation = 0;

    rt_free(handle);
    return RT_EOK;
}

int orb_check(orb_subscribe_t *handle, rt_bool_t *updated)
{
    if (!handle || !updated)
    {
        return -RT_ERROR;
    }

    if (!orb_node_ready(handle))
    {
        return -RT_ERROR;
    }

    if (hrt_elapsed_time(&handle->last_update) >= handle->interval)
    {
        updated = handle->generation != handle->node->generation;
    }
    else
    {
        updated = false;
    }

    return RT_EOK;
}

int orb_copy(const struct orb_metadata_s *meta, orb_subscribe_t *handle, void *buffer)
{
    if (!buffer)
    {
        return -RT_ERROR;
    }

    if (!meta && !handle)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}


orb_advertise_t orb_advertise_multi_queue(const struct orb_metadata_s *meta, const void *data, int *instance,
                                          unsigned int queue_size)
{
    if (!meta)
    {
        return RT_NULL;
    }

    orb_node_t *node = RT_NULL;

    // 允许的最大instance个数

    int max_inst = ORB_MULTI_MAX_INSTANCES;
    int inst     = 0;

    if (!instance)
    {
        node = orb_node_find(meta, 0);
        if (node)
        {
            max_inst = 0;
        }
        else
        {
            max_inst = 1;
        }
    }

    // 搜索实例是否存在，不存在则创建，存在则判断是否公告
    for (inst = 0; inst < max_inst; inst++)
    {
        node = orb_node_find(meta, inst);

        if (node)
        {
            if (!node->advertised)
            {
                break;
            }
        }
        else
        {
            node = orb_node_create(meta, inst, queue_size);
            break;
        }
    }

    // 如果node找到/创建成功，发布首次数据，且返回instance
    if (node)
    {
        // 标记为已经公告，只有公告过的主题才能copy和publish数据
        node->advertised = true;
        if (data)
        {
            orb_node_write(node, data);
        }
        // 返回inst
        if (instance)
        {
            *instance = inst;
        }
    }

    return node;
}

int orb_unadvertise(orb_node_t *node)
{
    if (!node)
    {
        return -RT_ERROR;
    }

    node->advertised = false;
    return RT_EOK;
}


int orb_publish(const struct orb_metadata_s *meta, orb_node_t *node, const void *data)
{
    if (!data)
    {
        return -RT_ERROR;
    }

    if (!meta && !node)
    {
        return -RT_ERROR;
    }
    else if (meta && !node)
    {
        node = orb_node_find(meta, 0);
        if (!node)
        {
            return -RT_ERROR;
        }
    }
    else if (meta && node)
    {
        if (node->meta != meta)
        {
            return -RT_ERROR;
        }
    }
    else // (!meta && node)
    {
    }

    if (orb_node_write(node, data) == node->meta->o_size)
    {
        return RT_EOK;
    }

    return -RT_ERROR;
}

