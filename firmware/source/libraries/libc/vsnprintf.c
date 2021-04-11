/*
 * vsnprintf.c
 *
 *  Created on: Aug 7, 2018
 *      Author: xps15
 */

#ifndef SRC_LIBRARIES_LIBC_VSNPRINTF_C_
#define SRC_LIBRARIES_LIBC_VSNPRINTF_C_

#include "vsnprintf.h"
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>

//we assume if we printed a binary 64 bit number we would need 64 chars
// note printf does not current allow binary but...
#ifdef MAX_STRING_LEN
#define MAX_CHAR_NUMBER MAX_STRING_LEN
#else
#define MAX_CHAR_NUMBER (66) //maximum number of chars in a number
#endif

typedef enum {
	FLAG_NONE = 0,
	FLAG_WIDTH = (1U << 0),
	FLAG_LEFT = (1U << 1), //left justified
	FLAG_PLUS = (1U << 2), //include plus sign
	FLAG_UNSIGNED = (1UL << 3), // number should be unsigned
	FLAG_HEX_UPPER = (1U << 4), //print hex as upper case
	FLAG_PAD_ZERO =  (1U << 5), //pad with zeros
}Flags_t;



#pragma GCC optimize ("O3")
static size_t u64t0a(char *buf, size_t maxLen, uint64_t value, uint8_t base, Flags_t flags)
{
	size_t i;
	uint64_t n;
	uint64_t x;

	i=0;

	if (base==0)
	{
		buf[0]='e';
		buf[1]='r';
		buf[2]='r';
		buf[3]='\0';
		return 3;
	}

	if (value == 0)
	{
		buf[i]='0';
		return 1;
	}

	if (base>16 || base<=1)
	{
		//unsupported based
		return i;
	}

	n=1;
	x=value;
	while (x>=base)
	{
		x=x/(uint64_t)base;
		n=n*base;
	}

	while (i<maxLen && n>0)
	{
		if (x<=9)
		{
			buf[i++]=(char)('0'+(char)x);
		} else
		{
			char y='a';
			if (flags & FLAG_HEX_UPPER)
			{
				y='A';
			}
			buf[i++]=(char)(y+(char)x-10);
		}

		value=value-(x*n);
		if (n==1)
		{
			return i;
		}else
		{
			n=n/(uint64_t)base;
		}
		x=value/n;
	}

	//we should never get here
	return 0;
}

#pragma GCC optimize ("O3")
size_t l64toa(char *buf, size_t maxLen, uint64_t value, uint8_t base, Flags_t flags)
{
	int64_t x;
	size_t i,n;
	int neg=0;

	memset(buf,' ',maxLen);
	buf[maxLen-1]='\0';

	i=0;
	if ((flags & FLAG_UNSIGNED)==0)
	{
		x=(int64_t)value;
		if (x<0)
		{
			buf[i++]='-';
			value=(uint64_t)(-x);
			neg=1;
		}
	}

	if (neg==0 && (flags & FLAG_PLUS))
	{
		buf[i++]='+';
	}

	//now lets add number to buffer
	n=u64t0a(&buf[i],maxLen-i, value,base,flags)+i;
	return n;
}

#pragma GCC optimize ("O3")
static size_t uftoa(char *buf, size_t maxLen, double value, uint8_t prec)
{
	size_t i;
	double x;
	uint8_t p;
	
	if (isnan(value))
	{
	    buf[0]='n';
	    buf[1]='a';
	    buf[2]='n';
	    buf[3]='\0';
	    return 3;
	}
	if (isinf(value))
	{
	    buf[0]='i';
	    buf[1]='n';
	    buf[2]='f';
	    buf[3]='\0';
	    return 3;
	}
	if (value<0) //this only supports unsigned
	{
		value=-value;
	}

	//do the rounding
	x=0.5;
	for(i=0; i<prec; i++)
	{
		x=x/10.0;
	}
	value=value+x;

	x=1.0;
	while((x*10)<value)
	{
		x=x*10.0;
	}

	i=0;
//	if (value<1.0)
//	{
//		buf[i++]='0';
//	}

	while (x>=1.0 && i<maxLen)
	{
		uint8_t c;
		c=(uint8_t)(value/x);
		buf[i++]=(char)(c+'0');
		value -= (double)c * x;
		x=x/10.0;
	}

	buf[i++]='.';

	p=0;
	while(p<prec && i<maxLen)
	{
		uint8_t c;
		c=(uint8_t)(value*10.0);
		buf[i++]=(char)(c+'0');
		value=value*10.0 - ((double)c);
		p++;
	}
	buf[i]='\0';
	return i;
}


size_t _ftoa(char *buf, size_t maxLen, double value, uint8_t prec)
{
	if (value<0)
	{
		value=-value;
		buf[0]='-';
		return uftoa(&buf[1],maxLen-1,value,prec)+1;
	}
	return uftoa(buf,maxLen,value,prec);
}

//reads two chars from buf and returns number
// if it is 8,16,32,or64
static uint8_t getSize(const char *buf)
{
	uint8_t c;
	uint8_t ret=0;
	c=(uint8_t)buf[0];
	if (c>='0' && c<='9')
	{
		ret=(uint8_t)(c-'0');
	}
	c=(uint8_t)buf[1];
	if (c>='0' && c<='9')
	{
		ret= (uint8_t)(ret*10+c-(uint8_t)'0');
	}
	if (ret==8 || ret==16 || ret==32 || ret==64)
	{
		return ret;
	}
	return 0;
}

#pragma GCC optimize ("O3")
size_t _vsnprintf(char *buf, size_t maxLen, const char *fmt, va_list ap)
{
	size_t i;


	char num[MAX_CHAR_NUMBER];

	if (maxLen==0)
	{
		return 0;
	}

	i=0;
	while (*fmt && i<(maxLen-1)) //we need one space for next char and one for the '\0'
	{
		if (*fmt == '%')
		{
			bool done=false;
			bool inPrec=false;
			bool firstChar=true;
			bool processed=true; //this indicates the char was processed
			Flags_t flags=FLAG_NONE;
			uint8_t prec=0;
			uint32_t width=0;
			uint32_t bitSize=0;
			uint8_t base=10;
			uint32_t n=0;
			int64_t sx;
			uint64_t ux;

			while(!done && *fmt && processed)
			{
				processed=false;
				fmt++;
				if (*fmt == '%')
				{
					if (i<(maxLen))
					{
						buf[i++]='%';
					}

					done=true;
				}

				if (*fmt == '0' && width==0 && !inPrec)
				{
					flags |= FLAG_PAD_ZERO;
					processed=true;
				}
				if (*fmt==' ')
				{
					done=true;
				}
				if (*fmt=='.')
				{
					inPrec=true;
					processed=true;
				}

				if (*fmt == 'l')
				{
					bitSize += 32;
					processed=true;
				}

				if (*fmt == '+' && firstChar)
				{
					flags |= FLAG_PLUS;
					processed=true;
				}
				if (*fmt == '-' && firstChar)
				{
					flags |= FLAG_LEFT;
					processed=true;
				}

				if (*fmt >='0' && *fmt<='9')
				{
					if (!inPrec)
					{
						width=(uint32_t)width*10+((uint32_t)*fmt-(uint32_t)'0');
					} else
					{
						prec=(uint8_t)((uint32_t)prec*10+((uint32_t)*fmt-(uint32_t)'0'));
					}
					processed=true;
				}

				if (*fmt =='c')
				{
					done=true;
					char c;
#if (UINT_MAX <= 0xFFUL)
					c=va_arg(ap,uint8_t);
#else
					c=(char)va_arg(ap,unsigned int);
#endif
					if (i<(maxLen))
					{
						buf[i++]=c;
					}
				}//'c'


				if (*fmt =='s')
				{
					done=true;
					char * ptr;
					ptr=(char*)va_arg(ap,char*);
					if (ptr== NULL)
					{
						buf[0]='\0';
						return 0; 
					}

					n=(uint8_t)strlen(ptr);

					while (n<width)
					{
						if (i<(maxLen))
						{
							if (flags & FLAG_PAD_ZERO)
							{
								buf[i++]='0';
							}else
							{
								buf[i++]=' ';
							}
						}
						width--;
					}

					while(*ptr && i<(maxLen))
					{
						buf[i++]=*ptr++;
					}
				} // 's'



				if (*fmt == 'x' || *fmt == 'X' || *fmt=='o' || *fmt == 'p')
				{
					if (*fmt == 'X')
					{
						flags |=FLAG_HEX_UPPER;
					}
					base=16;

					if (*fmt=='o')
					{
						base=8;
					}

					uint32_t size;
					done=true;
					size=getSize(fmt+1);
					if (size==8)
					{
						fmt++; //skip char
					}
					if (size>=10 && size<=99)
					{
						fmt+=2;
					}
					if (size>=100 && size<=999) //just in case we add 128bits
					{
						fmt+=3;
					}
					if (size != 0 && bitSize==0)
					{
						bitSize=size;
					}

					switch (bitSize)
					{
						case 64:
							ux=va_arg(ap,uint64_t);
							break;
						case 32:
							ux=va_arg(ap,uint32_t);
							break;
						case 16:
#if (UINT_MAX <= 0xFFFFUL)
							ux=va_arg(ap,uint16_t);
#else
							ux=va_arg(ap,unsigned int);
#endif
							break;
						case 8:
#if (UINT_MAX <= 0xFFUL)
							ux=va_arg(ap,uint8_t);
#else
							ux=va_arg(ap,unsigned int);
#endif
							break;
						default:
							ux=va_arg(ap,uint32_t);
							break;
					}

					n=u64t0a(num,MAX_CHAR_NUMBER, ux,base,flags);

					while (n<width)
					{
						if (i<(maxLen))
						{
							if (flags & FLAG_PAD_ZERO)
							{
								buf[i++]='0';
							}else
							{
								buf[i++]=' ';
							}
						}
						width--;
					}

					if ((i+n)<(maxLen))
					{
						memcpy(&buf[i],num,n);
						i+=n;
					}
				} //'x' || 'X'

				if (*fmt == 'f' || *fmt == 'F' || *fmt=='e') //TODO implement 'e'
				{
					double x;
					done=true; //flag that we are done
					x=va_arg(ap,double); //get value from stack

					//do the '+' if needed
					if ((flags & FLAG_PLUS) && x>0)
					{
						if (i<(maxLen))
						{
							buf[i++]='+';
							if (width>0)
							{
								width--;
							}
						}
					}

					//do '-' minus sign if needed
					if (x<0)
					{
						if (i<(maxLen))
						{
							buf[i++]='-';
							if (width>0)
							{
								width--;
							}
						}
						x=-x;
					}

					//clamp our precision
					if (prec>9)
					{
						prec=9;
					}
					if (prec==0)
					{
						prec=6;
					}
					n=uftoa(num,MAX_CHAR_NUMBER, x, prec);

					while (n<width)
					{
						if (i<(maxLen))
						{
							if (flags & FLAG_PAD_ZERO)
							{
								buf[i++]='0';
							}else
							{
								buf[i++]=' ';
							}
						}
						width--;
					}

					if ((i+n)<(maxLen))
					{
						memcpy(&buf[i],num,n);
						i+=n;
					}
				} //'f' || 'F'

				if (*fmt == 'i' || *fmt == 'd')
				{
					uint32_t size;
					done=true;
					size=getSize(fmt+1);
					if (size==8)
					{
						fmt++; //skip char
					}
					if (size>=10 && size<=99)
					{
						fmt+=2;
					}
					if (size>=100 && size<=999) //just in case we add 128bits
					{
						fmt+=3;
					}
					if (size != 0 && bitSize==0)
					{
						bitSize=size;
					}

					switch (bitSize)
					{
						case 64:
							sx=(int64_t)va_arg(ap,uint64_t);
							break;
						case 32:
							sx=(int32_t)va_arg(ap,uint32_t);
							break;
						case 16:
#if (UINT_MAX <= 0xFFFFUL)
							sx=va_arg(ap,int16_t);
#else
							sx=(int)va_arg(ap, int);
#endif
							break;
						case 8:
#if (UINT_MAX <= 0xFFUL)
							sx=va_arg(ap,int8_t);
#else
							sx=(int)va_arg(ap, int);
#endif
							break;
						default:
							sx=(int32_t)va_arg(ap,uint32_t);
							break;
					}
					if ((flags & FLAG_PLUS) && sx>0)
					{
						if (i<(maxLen))
						{
							buf[i++]='+';
							if (width>0)
							{
								width--;
							}
						}
					}
					if (sx<0)
					{
						if (i<(maxLen))
						{
							buf[i++]='-';
							if (width>0)
							{
								width--;
							}
						}
						sx=-sx;
					}
					n=u64t0a(num,MAX_CHAR_NUMBER, (uint64_t)sx,base,flags);

					while (n<width)
					{
						if (i<(maxLen))
						{
							if (flags & FLAG_PAD_ZERO)
							{
								buf[i++]='0';
							}else
							{
								buf[i++]=' ';
							}
						}
						width--;
					}

					if ((i+n)<(maxLen))
					{
						memcpy(&buf[i],num,n);
						i+=n;
					}
				} //'d' || 'i'
				if (*fmt == 'u')
				{
					uint32_t size;
					done=true;
					size=getSize(fmt+1);
					if (size==8)
					{
						fmt++; //skip char
					}
					if (size>=10 && size<=99)
					{
						fmt+=2;
					}
					if (size>=100 && size<=999) //just in case we add 128bits
					{
						fmt+=3;
					}
					if (size != 0 && bitSize==0)
					{
						bitSize=size;
					}

					switch (bitSize)
					{
						case 64:
							ux=va_arg(ap,uint64_t);
							break;
						case 32:
							ux=va_arg(ap,uint32_t);
							break;
						case 16:
#if (UINT_MAX <= 0xFFFFUL)
							ux=va_arg(ap,uint16_t);
#else
							ux=va_arg(ap,unsigned int);
#endif
							break;
						case 8:
#if (UINT_MAX <= 0xFFUL)
							ux=va_arg(ap,uint8_t);
#else
							ux=va_arg(ap,unsigned int);
#endif
							break;
						default:
							ux=va_arg(ap,uint32_t);
							break;
					}
					if ((flags & FLAG_PLUS))
					{
						if (i<(maxLen))
						{
							buf[i++]='+';
							if (width>0)
							{
								width--;
							}
						}
					}
					n=u64t0a(num,MAX_CHAR_NUMBER, ux,base,flags);

					while (n<width)
					{
						if (i<(maxLen))
						{
							if (flags & FLAG_PAD_ZERO)
							{
								buf[i++]='0';
							}else
							{
								buf[i++]=' ';
							}
						}
						width--;
					}

					if ((i+n)<(maxLen))
					{
						memcpy(&buf[i],num,n);
						i+=n;
					}
				} //'u'
				firstChar=false;
			} //while(!done)

		} else
		{
			if (i<(maxLen-1))
			{
				buf[i++]=*fmt;
			}
		}
		fmt++;
	}

	//always add termination character. 
	if (i<(maxLen))
	{
		buf[i]='\0';
	}else
	{
		buf[maxLen-1]='\0';
	}
	return i;
}

size_t _snprintf(char *ptrStr, size_t maxLen, const char *fmt, ...)
{
	size_t ret;
	va_list ap;
	va_start(ap,fmt);
	ret=_vsnprintf(ptrStr, maxLen, fmt,ap);
	va_end(ap);
	return ret;
}

#endif /* SRC_LIBRARIES_LIBC_VSNPRINTF_C_ */
