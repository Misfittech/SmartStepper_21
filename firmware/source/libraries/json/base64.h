/*
 * @file base64.h
 * @brief
 *
 *  Created on: Aug 23, 2019
 *  @author tstern
 */

#ifndef SOURCE_LIBRARIES_JSON_BASE64_H_
#define SOURCE_LIBRARIES_JSON_BASE64_H_

#include <memory.h>
#include <stdint.h>
#include <functional>

#define  BASE64_ENCODE_SIZE(x) ( 4 * (((x) + 2) / 3))
#define  BASE64_DECODE_SIZE(x) ((x)/4*3 +((x)%4))

size_t base64_encode(const uint8_t *data,
                    size_t input_length, 
                    uint8_t *out_data, size_t output_length);

size_t base64_decode(const char *encoded_data,
                             size_t input_length,
			     uint8_t *decoded_data,
                             size_t decoded_length);

size_t base64_write(const uint8_t *data,
                    size_t input_length, 
                    std::function<size_t(const uint8_t *,size_t)> ptrWrite);

#endif /* SOURCE_LIBRARIES_JSON_BASE64_H_ */
