/*
 * Copyright (C) 2023 Dave Waataja
 *
 */

#ifndef DA_FLASH_H__
#define DA_FLASH_H__

/** @file flash.h
 * @brief Module for interacting with the xiao flash
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initializes QSPI flash module
 *
 *  @retval 0   If the operation was successful.
 *              Otherwise, a (negative) error code is returned.
 */
int da_flash_init(); 

/** @brief Uninitializes the QSPI flash module
 */
void da_flash_uninit();

/** @brief Initializes the command handling module
 * 
 *  @param  command. Command to send to the flash
 *
 *  @retval 0   If the operation was successful.
 *              Otherwise, a (negative) error code is returned.
 */
int da_flash_command(uint8_t command);

#ifdef __cplusplus
}
#endif

#endif /* DA_FLASH_H__ */
