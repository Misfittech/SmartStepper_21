/*
 * string.c
 *
 *  Created on: Jan 8, 2020
 *      Author: trampas
 */

#include "string.h"
#include <stdbool.h>
#include <ctype.h>

int stricmp(char const *a, char const *b)
{
    for (;; a++, b++) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d != 0 || !*a)
            return d;
    }
}

size_t get_string_after(const char *haystack, const char *needle, char *output, size_t max_length)
{
  char *ptr;
  ptr=strstr(haystack,needle);
  memset(output,0,max_length);
  if (ptr != NULL)
  {
    size_t n;
    ptr+=strlen(needle);
    n=strlen(ptr);
    if (n>(max_length-1))
    {
      n=max_length-1;
    }
    memcpy(output,ptr,n);
    output[n]='\0';
    n=strip(output);
    n=trim_left(output,"\"");
    n=trim_right(output,"\"");  
    return n;
  }
  return 0;
}

char* stristr( const char* str1, const char* str2 )
{
    const char* p1 = str1 ;
    const char* p2 = str2 ;
    const char* r = *p2 == 0 ? str1 : 0 ;

    while( *p1 != 0 && *p2 != 0 )
    {
        if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
        {
            if( r == 0 )
            {
                r = p1 ;
            }

            p2++ ;
        }
        else
        {
            p2 = str2 ;
            if( r != 0 )
            {
                p1 = r + 1 ;
            }

            if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
            {
                r = p1 ;
                p2++ ;
            }
            else
            {
                r = 0 ;
            }
        }

        p1++ ;
    }

    return *p2 == 0 ? (char*)r : 0 ;
}
static size_t remove_first_char(char *str)
{
	size_t n,i;
	
	n=strlen(str);
	for (i=1; i<n; i++)
	{
		str[i-1]=str[i];
	}
	str[i-1]='\0';
	return n-1; 
}


size_t trim_left(char *str,const char *tokens)
{
	size_t i,n;
	bool found=false;
	
	n=strlen(tokens);
	while(!found && str[0]!='\0')
	{
		found=true;
		for(i=0; i<n; i++)
		{
			if (str[0]==tokens[i])
			{
				found=false;
				remove_first_char(str);
				break;
			}
		}
	}
	return strlen(str);
}



size_t trim_right(char *str,const char *tokens)
{
	size_t i,n,j;
	bool found=false;
	
	n=strlen(tokens);
	j=strlen(str);
	while(!found && j!=0)
	{
		found=true;
		for(i=0; i<n; i++)
		{
			if (str[j-1]==tokens[i])
			{
				found=false;
				str[j-1]='\0';
				break;
			}
		}
		j=strlen(str);
	}
	return strlen(str);
}

size_t lstrip(char *str)
{
	return trim_left(str," \t\r\n\v\f"); 
}

size_t rstrip(char *str)
{
	return trim_right(str," \t\r\n\v\f"); 
}

size_t strip(char *str)
{
	rstrip(str);
	return lstrip(str); 
}
