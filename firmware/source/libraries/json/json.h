/*
 * @file json.h
 * @brief
 *
 *  Created on: Aug 23, 2019
 *  @author tstern
 */

#ifndef SOURCE_LIBRARIES_JSON_JSON_H_
#define SOURCE_LIBRARIES_JSON_JSON_H_

#include <memory.h>
#include <stdint.h>
#include <functional>

size_t json_write(const void *data, size_t numBytes, 
		const char *tag_name,
		std::function<size_t(const uint8_t *,size_t)> ptrWrite);

#endif /* SOURCE_LIBRARIES_JSON_JSON_H_ */
