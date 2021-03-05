/*
 * @file json.cpp
 * @brief
 *
 *  Created on: Aug 23, 2019
 *  @author tstern
 */

#include "json.h"
#include "base64.h"
#include "libraries/syslog/syslog.h"
#include "libraries/libc/vsnprintf.h"
#include "libraries/crc/crc.h"

size_t json_write(const void *data, size_t numBytes, 
		const char *tag_name,
		std::function<size_t(const uint8_t *,size_t)> ptrWrite)
{
    size_t n;
    size_t j=0;
    char str[255]; 
    uint32_t crc;
    
    //print the tag name 
    _snprintf(str,sizeof(str), "{ \"%s\":{\"value\":\"",tag_name); 
    n=ptrWrite((uint8_t *)str,strlen(str));
    j+=n;
    if (n!=strlen(str))
    {
	ERROR("Write failed");
	return j;
    }
    
    //print the data
    n=base64_write((uint8_t *)data, numBytes, ptrWrite);
    j+=n;
    if (n!=numBytes)
    {
	ERROR("Write failed %d %d",n, numBytes);
	return j;
    }
    
    //calculate the crc
    crc32(data,numBytes, &crc);
    _snprintf(str,sizeof(str), "\", \"CRC32\":\""); 
    n=ptrWrite((uint8_t *)str,strlen(str));
    j+=n;
    if (n!=strlen(str))
    {
	ERROR("Write failed");
	return j;
    }
    
    
    n=base64_write((uint8_t *)&crc, sizeof(crc), ptrWrite);
    j+=n;
    if (n!=sizeof(crc))
    {
	ERROR("Write failed");
	return j;
    }    
    
    //print ending
    _snprintf(str,sizeof(str), "\"}}"); 
    n=ptrWrite((uint8_t *)str,strlen(str));
    j+=n;
    if (n!=strlen(str))
    {
	ERROR("Write failed");
	return j;
    }
    
    return j;
    
}


