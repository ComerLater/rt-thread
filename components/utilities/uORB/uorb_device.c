
/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-09-12     latercomer   the first version
 */


#define LOG_TAG "uorb.device"
#define LOG_LVL LOG_LVL_INFO

#include "uorb_device.h"
#include <rtdbg.h>


static rt_err_t rt_uorb_init(struct rt_device *dev)
{
    return RT_EOK;
}


static rt_err_t rt_uorb_open(struct rt_device *dev, rt_uint16_t oflag)
{
    return RT_EOK;
}


static rt_err_t rt_uorb_close(struct rt_device *dev)
{
    return RT_EOK;
}

static rt_ssize_t rt_uorb_read(rt_device_t *dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    RT_ASSERT(dev != RT_NULL);

    if (size == 0)
    {
        return 0;
    }

    struct rt_uorb_device *uorb = (struct rt_uorb_device *)dev;

    return 0;
}


static rt_ssize_t rt_uorb_write(struct rt_device *dev,
                                rt_off_t          pos,
                                const void       *buffer,
                                rt_size_t         size)
{
    return 0;
}

static rt_err_t rt_uorb_control(struct rt_device *dev,
                                int               cmd,
                                void             *args)
{
    return RT_EOK;
}


const static struct rt_device_ops _uorb_ops =
    {
        .init    = rt_uorb_init,
        .open    = rt_uorb_open,
        .close   = rt_uorb_close,
        .read    = rt_uorb_read,
        .write   = rt_uorb_write,
        .control = rt_uorb_control,
};

rt_err_t rt_uorb_register(struct rt_uorb_device *node, const char *name, uint32_t flag, void *data)
{
    RT_ASSERT(node != RT_NULL);
    RT_ASSERT(meta != RT_NULL);

    struct rt_device *device = &(node->parent);
    device->type             = RT_Device_Class_Char;
    device->rx_indicate      = RT_NULL;
    device->tx_complete      = RT_NULL;

#ifdef RT_USING_DEVICE_OPS
    device->ops = &_uorb_ops;
#else
    device->init    = rt_uorb_init;
    device->open    = rt_uorb_open;
    device->close   = rt_uorb_close;
    device->read    = rt_uorb_read;
    device->write   = rt_uorb_write;
    device->control = rt_uorb_control;
#endif
    device->user_data = data;

    rt_err_t ret = rt_device_register(device, name, flag);

#ifdef RT_USING_POSIX_STDIO
    /* set fops */
    device->fops = &_uorb_fops;
#endif
    return ret;
}
