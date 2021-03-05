/*
 * crc.h
 *
 *  Created on: Jul 21, 2019
 *      Author: Trampas
 */

#ifndef SOURCE_LIBRARIES_CRC_CRC_H_
#define SOURCE_LIBRARIES_CRC_CRC_H_

#include <stdint.h>
#include <memory.h>

void crc32(const void *data, size_t n_bytes, uint32_t* crc);

#endif /* SOURCE_LIBRARIES_CRC_CRC_H_ */
