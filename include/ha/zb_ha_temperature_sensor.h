/*
 * Copyright (c) Cold Bear Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ZB_HA_TEMPERATURE_SENSOR_H
#define ZB_HA_TEMPERATURE_SENSOR_H 1

#define ZB_HA_DEVICE_VER_FART_SENSOR 0
#define ZB_HA_DEVICE_VER_TEMPERATURE_SENSOR 0

// Two measured_values and battery_remaining_percentage
#define ZB_REPORTING_ATTR_COUNT 20

#define ZB_FART_SENSOR_IN_CLUSTER_NUM 3
#define ZB_FART_SENSOR_OUT_CLUSTER_NUM 0

#define ZB_HA_TEMPERATURE_SENSOR_IN_CLUSTER_NUM 1
#define ZB_HA_TEMPERATURE_SENSOR_OUT_CLUSTER_NUM 0

#define ZB_ZCL_DECLARE_FART_SENSOR_CLUSTER_LIST(                          \
    cluster_list_name,                                                    \
    basic_attr_list,                                                      \
    identify_attr_list,                                                   \
    power_config_attr_list)                                               \
    zb_zcl_cluster_desc_t cluster_list_name[] =                           \
        {                                                                 \
            ZB_ZCL_CLUSTER_DESC(                                          \
                ZB_ZCL_CLUSTER_ID_POWER_CONFIG,                           \
                ZB_ZCL_ARRAY_SIZE(power_config_attr_list, zb_zcl_attr_t), \
                (power_config_attr_list),                                 \
                ZB_ZCL_CLUSTER_SERVER_ROLE,                               \
                ZB_ZCL_MANUF_CODE_INVALID),                               \
            ZB_ZCL_CLUSTER_DESC(                                          \
                ZB_ZCL_CLUSTER_ID_IDENTIFY,                               \
                ZB_ZCL_ARRAY_SIZE(identify_attr_list, zb_zcl_attr_t),     \
                (identify_attr_list),                                     \
                ZB_ZCL_CLUSTER_SERVER_ROLE,                               \
                ZB_ZCL_MANUF_CODE_INVALID),                               \
            ZB_ZCL_CLUSTER_DESC(                                          \
                ZB_ZCL_CLUSTER_ID_BASIC,                                  \
                ZB_ZCL_ARRAY_SIZE(basic_attr_list, zb_zcl_attr_t),        \
                (basic_attr_list),                                        \
                ZB_ZCL_CLUSTER_SERVER_ROLE,                               \
                ZB_ZCL_MANUF_CODE_INVALID)}

#define ZB_ZCL_DECLARE_FART_SENSOR_SIMPLE_DESC(ep_name, ep_id, in_clust_num, out_clust_num) \
    ZB_DECLARE_SIMPLE_DESC(in_clust_num, out_clust_num);                                    \
    ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num)                                     \
    simple_desc_##ep_name =                                                                 \
        {                                                                                   \
            ep_id,                                                                          \
            ZB_AF_HA_PROFILE_ID,                                                            \
            ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,                                             \
            ZB_HA_DEVICE_VER_FART_SENSOR,                                                   \
            0,                                                                              \
            in_clust_num,                                                                   \
            out_clust_num,                                                                  \
            {ZB_ZCL_CLUSTER_ID_BASIC,                                                       \
             ZB_ZCL_CLUSTER_ID_IDENTIFY,                                                    \
             ZB_ZCL_CLUSTER_ID_POWER_CONFIG}}

#define ZB_HA_DECLARE_FART_SENSOR_EP(ep_name, ep_id, cluster_list)                      \
    ZB_ZCL_DECLARE_FART_SENSOR_SIMPLE_DESC(ep_name,                                     \
                                           ep_id,                                       \
                                           ZB_FART_SENSOR_IN_CLUSTER_NUM,               \
                                           ZB_FART_SENSOR_OUT_CLUSTER_NUM);             \
                                                                                        \
    ZBOSS_DEVICE_DECLARE_REPORTING_CTX(reporting_info_device_all_ep,                    \
                                       ZB_REPORTING_ATTR_COUNT);                        \
                                                                                        \
    ZB_AF_DECLARE_ENDPOINT_DESC(ep_name,                                                \
                                ep_id,                                                  \
                                ZB_AF_HA_PROFILE_ID,                                    \
                                0,                                                      \
                                NULL,                                                   \
                                ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), \
                                cluster_list,                                           \
                                (zb_af_simple_desc_1_1_t *)&simple_desc_##ep_name,      \
                                ZB_REPORTING_ATTR_COUNT,                                \
                                reporting_info_device_all_ep,                           \
                                0,                                                      \
                                NULL);

#define ZB_HA_DECLARE_TEMPERATURE_SENSOR_CLUSTER_LIST(cluster_list_name, temp_measure_attr_list) \
    zb_zcl_cluster_desc_t cluster_list_name[] =                                                  \
        {                                                                                        \
            ZB_ZCL_CLUSTER_DESC(                                                                 \
                ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,                                              \
                ZB_ZCL_ARRAY_SIZE(temp_measure_attr_list, zb_zcl_attr_t),                        \
                (temp_measure_attr_list),                                                        \
                ZB_ZCL_CLUSTER_SERVER_ROLE,                                                      \
                ZB_ZCL_MANUF_CODE_INVALID)}

#define ZB_ZCL_DECLARE_TEMPERATURE_SENSOR_TEMPERATURE_DESC(ep_name, ep_id, in_clust_num, out_clust_num) \
    ZB_DECLARE_SIMPLE_DESC_VA(in_clust_num, out_clust_num, ep_name);                                    \
    ZB_AF_SIMPLE_DESC_TYPE_VA(in_clust_num, out_clust_num, ep_name)                                     \
    simple_desc_##ep_name =                                                                             \
        {                                                                                               \
            ep_id,                                                                                      \
            ZB_AF_HA_PROFILE_ID,                                                                        \
            ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,                                                         \
            ZB_HA_DEVICE_VER_TEMPERATURE_SENSOR,                                                        \
            0,                                                                                          \
            in_clust_num,                                                                               \
            out_clust_num,                                                                              \
            {                                                                                           \
                ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,                                                     \
            }}

#define ZB_HA_DECLARE_TEMPERATURE_SENSOR_EP_1(ep_name, ep_id, cluster_list)                       \
    ZB_ZCL_DECLARE_TEMPERATURE_SENSOR_TEMPERATURE_DESC(ep_name,                                   \
                                                       ep_id,                                     \
                                                       ZB_HA_TEMPERATURE_SENSOR_IN_CLUSTER_NUM,   \
                                                       ZB_HA_TEMPERATURE_SENSOR_OUT_CLUSTER_NUM); \
                                                                                                  \
    ZB_AF_DECLARE_ENDPOINT_DESC(ep_name,                                                          \
                                ep_id,                                                            \
                                ZB_AF_HA_PROFILE_ID,                                              \
                                0,                                                                \
                                NULL,                                                             \
                                ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t),           \
                                cluster_list,                                                     \
                                (zb_af_simple_desc_1_1_t *)&simple_desc_##ep_name,                \
                                ZB_REPORTING_ATTR_COUNT,                                          \
                                reporting_info_device_all_ep,                                     \
                                0,                                                                \
                                NULL);

#define ZB_HA_DECLARE_TEMPERATURE_SENSOR_EP_2(ep_name, ep_id, cluster_list)                       \
    ZB_ZCL_DECLARE_TEMPERATURE_SENSOR_TEMPERATURE_DESC(ep_name,                                   \
                                                       ep_id,                                     \
                                                       ZB_HA_TEMPERATURE_SENSOR_IN_CLUSTER_NUM,   \
                                                       ZB_HA_TEMPERATURE_SENSOR_OUT_CLUSTER_NUM); \
    ZB_AF_DECLARE_ENDPOINT_DESC(ep_name,                                                          \
                                ep_id,                                                            \
                                ZB_AF_HA_PROFILE_ID,                                              \
                                0,                                                                \
                                NULL,                                                             \
                                ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t),           \
                                cluster_list,                                                     \
                                (zb_af_simple_desc_1_1_t *)&simple_desc_##ep_name,                \
                                ZB_REPORTING_ATTR_COUNT,                                          \
                                reporting_info_device_all_ep,                                     \
                                0,                                                                \
                                NULL);

#endif /* ZB_HA_TEMPERATURE_SENSOR_H */