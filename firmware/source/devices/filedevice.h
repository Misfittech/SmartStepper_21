/*
 * filedevice.h
 *
 *  Created on: Oct 16, 2019
 *      Author: trampas
 */

#ifndef SRC_DEVICES_FILEDEVICE_H_
#define SRC_DEVICES_FILEDEVICE_H_


#include <inttypes.h>
#include <stddef.h>
#include "board.h"

class FileDevice
{
  public:
    virtual size_t write(const char *fname, const uint8_t *ptrData, size_t numberBytes, bool append=true)=0;
    virtual void writeClose(const char *fname) {(void)fname;};
    virtual size_t read(const char *fname, uint8_t *ptrData, size_t numberBytes, size_t offset=0)=0;
    virtual bool isReady(void)=0;
    virtual size_t fileSize(const char *fname)=0;
    virtual bool removeFile(const char *fname)=0;
    
  };

inline static size_t copyfile(FileDevice *ptrFileDevice, const char *srcFname,const char *destFname)
{
	uint8_t buf[512];
	size_t bytes=0,n;
	n=ptrFileDevice->read(srcFname, buf, sizeof(buf), 0);
	while(n>0)
	{
		size_t x;
			bool append=true;
		if (bytes == 0)
		{
			append=false;
		}
		x=ptrFileDevice->write(destFname,buf,n,append);
		bytes+=x;
		if (x!=n) 
		{
			ERROR("could not copy file %s",destFname);
			return bytes;
		}
		n=ptrFileDevice->read(srcFname, buf, sizeof(buf), bytes);
	}
	return bytes;
}

inline static size_t copyfile(FileDevice *ptrFileDeviceSrc, FileDevice *ptrFileDeviceDst,const char *srcFname,const char *destFname)
{
	uint8_t buf[512];
	size_t bytes=0,n;
	n=ptrFileDeviceSrc->read(srcFname, buf, sizeof(buf), 0);
	while(n>0)
	{
		size_t x;
		bool append=true;
		if (bytes == 0)
		{
			append=false;
		}
		x=ptrFileDeviceDst->write(destFname,buf,n,append);
		bytes+=x;
		if (x!=n) 
		{
			ERROR("could not copy file %s",destFname);
			return bytes;
		}
		n=ptrFileDeviceSrc->read(srcFname, buf, sizeof(buf), bytes);
	}
	return bytes;
}
#endif /* SRC_DEVICES_FILEDEVICE_H_ */
