/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-09-12     latercomer   the first version
 */


#ifndef _UORB_H_
#define _UORB_H_


#include <rtthread.h>


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


/**
 * Object metadata.
 */
struct orb_metadata
{
    const char    *o_name;            /**< unique object name */
    const uint16_t o_size;            /**< object size */
    const uint16_t o_size_no_padding; /**< object size w/o padding at the end (for logger) */
    const char    *o_fields;          /**< semicolon separated list of fields (with type) */
    uint8_t        o_id;              /**< ORB_ID enum */
};

typedef const struct orb_metadata *orb_id_t;
typedef const struct orb_metadata  orb_metadata_t;


/**
 * Generates a pointer to the uORB metadata structure for
 * a given topic.
 *
 * The topic must have been declared previously in scope
 * with ORB_DECLARE().
 *
 * @param _name		The name of the topic.
 */
#define ORB_ID(_name) &__orb_##_name

/**
 * Declare (prototype) the uORB metadata for a topic (used by code generators).
 *
 * @param _name		The name of the topic.
 */
#if defined(__cplusplus)
#define ORB_DECLARE(_name) extern "C" const struct orb_metadata __orb_##_name
#else
#define ORB_DECLARE(_name) extern const struct orb_metadata __orb_##_name
#endif //__cplusplus


/**
 * Define (instantiate) the uORB metadata for a topic.
 *
 * The uORB metadata is used to help ensure that updates and
 * copies are accessing the right data.
 *
 * Note that there must be no more than one instance of this macro
 * for each topic.
 *
 * @param _name		The name of the topic.
 * @param _struct	The structure the topic provides.
 * @param _size_no_padding	Struct size w/o padding at the end
 * @param _fields	All fields in a semicolon separated list e.g: "float[3] position;bool armed"
 * @param _orb_id_enum	ORB ID enum e.g.: ORB_ID::vehicle_status
 */
#define ORB_DEFINE(_name, _struct, _size_no_padding, _fields, _orb_id_enum) \
    const struct orb_metadata __orb_##_name = {                             \
        #_name,                                                             \
        sizeof(_struct),                                                    \
        _size_no_padding,                                                   \
        _fields,                                                            \
        _orb_id_enum,                                                       \
    };                                                                      \
    struct hack


/**
 * ORB topic advertiser handle.
 *
 * Advertiser handles are global; once obtained they can be shared freely
 * and do not need to be closed or released.
 *
 * This permits publication from interrupt context and other contexts where
 * a file-descriptor-based handle would not otherwise be in scope for the
 * publisher.
 */
typedef void *orb_advertise_t;


typedef struct orb_callback_s
{
    rt_list_t list;
    void (*call)();
} orb_callback_t;


typedef struct orb_node_s
{
    rt_list_t                  list;
    const struct orb_metadata *meta;
    rt_uint8_t                 instance;
    rt_uint8_t                 queue_size;
    rt_uint32_t                generation;
    rt_list_t                  callbacks;
    rt_bool_t                  advertised;
    rt_uint8_t                 subscriber_count;
    rt_bool_t                  data_valid;
    rt_uint8_t                *data;
} orb_node_t;


typedef struct orb_subscribe_s
{
    const struct orb_metadata *meta;
    rt_uint8_t                 instance;
    rt_tick_t                  interval;

    struct rt_uorb_device *node;
    rt_uint32_t            generation;
    rt_tick_t              last_update;
    rt_bool_t              callback_registered;
} orb_subscribe_t;


#ifdef __cplusplus
}
#endif // __cplusplus


#endif // _UORB_H_
