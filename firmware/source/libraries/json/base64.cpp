/*
 * @file base64.cpp
 * @brief
 *
 *  Created on: Aug 23, 2019
 *  @author tstern
 */


#include <stdlib.h>
#include "base64.h"
#include "libraries/syslog/syslog.h"

static const char base64_table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const uint8_t base64_decode_table[256] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,
0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };


static int mod_table[] = {0, 2, 1};

size_t base64_encode(const uint8_t *data,
                    size_t input_length, 
                    char *encoded_data, size_t encoded_length) 
{
    size_t n; 
    n = BASE64_ENCODE_SIZE(input_length);
    
    //check that output buffer is large enough 
    if (encoded_length < n)
    {
	return 0;
    }

    for (uint32_t i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (uint8_t)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (uint8_t)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (uint8_t)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = base64_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
    {
        encoded_data[n - 1 - i] = '=';
    }

    return n;
}


size_t base64_write(const uint8_t *data,
                    size_t input_length, 
                    std::function<size_t(const uint8_t *,size_t)> ptrWrite)
{
    size_t n;
    size_t x;
    uint8_t encoded_data[64];
    
    uint32_t index;
    
    //n = BASE64_ENCODE_SIZE(input_length);

    index=0;
    for (size_t i = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (uint8_t)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (uint8_t)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (uint8_t)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        n=4;
        if (i>=input_length)
        {
            memset(&encoded_data[index],'=',4);
            n=mod_table[input_length % 3];
        }

        if (sizeof(encoded_data)<(index+4))
        {
            x=ptrWrite(encoded_data,index);

	    if (x!=index)
	    {
		ERROR("Could not write all the data");
		return i;
	    }
	    index=0;
        }
        
        
        if (n>0) {encoded_data[index] = base64_table[(triple >> 3 * 6) & 0x3F];}
        if (n>1) encoded_data[index+1] = base64_table[(triple >> 2 * 6) & 0x3F];
        if (n>2) encoded_data[index+2] = base64_table[(triple >> 1 * 6) & 0x3F];
        if (n>3) encoded_data[index+3] = base64_table[(triple >> 0 * 6) & 0x3F];
        index+=4;
       
    }
    
    x=ptrWrite(encoded_data,index);

    if (x!=index)
    {
	ERROR("Could not write all the data");
	return input_length-index;
    }

    return input_length;
}


size_t base64_decode(const char *encoded_data,
                             size_t input_length,
			     uint8_t *decoded_data,
                             size_t decoded_length) 
{

    uint32_t n;
    
    if (input_length % 4 != 0) return 0; 

    n = input_length / 4 * 3;
  
    
    if (encoded_data[input_length - 1] == '=') n--;
    if (encoded_data[input_length - 2] == '=') n--;
    
    if (decoded_length<n)
    {
	return 0;
    }

    if (decoded_data == NULL) return 0;

    for (uint32_t i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = encoded_data[i] == '=' ? 0 & i++ : base64_decode_table[(uint8_t)encoded_data[i++]];
        uint32_t sextet_b = encoded_data[i] == '=' ? 0 & i++ : base64_decode_table[(uint8_t)encoded_data[i++]];
        uint32_t sextet_c = encoded_data[i] == '=' ? 0 & i++ : base64_decode_table[(uint8_t)encoded_data[i++]];
        uint32_t sextet_d = encoded_data[i] == '=' ? 0 & i++ : base64_decode_table[(uint8_t)encoded_data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < n) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < n) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < n) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return n;
}

