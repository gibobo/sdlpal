#include "util.h"
#include <stdio.h>

void *UTIL_fopen(const char *_FileName, const char *_Mode) {
   if (_FileName == NULL || _Mode == NULL)
      TerminateOnError("UTIL_fopen() called with invalid parameters\n");

   FILE *fp = fopen(_FileName, _Mode);
   if (fp == NULL)
      TerminateOnError("UTIL_fopen() returns a null pointer\n");

   return fp;
}

void *UTIL_fopen_without_checking(const char *_FileName, const char *_Mode) {
   if (_FileName == NULL || _Mode == NULL)
      TerminateOnError("UTIL_fopen() called with invalid parameters\n");

   return fopen(_FileName, _Mode);
}

int UTIL_fseek(void *_Stream, long _Offset, int _Origin) {
   if (_Stream == NULL)
      TerminateOnError("UTIL_fseek() called with invalid parameters\n");

   return fseek(_Stream, _Offset, _Origin);
}

unsigned int UTIL_fread(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream) {
   if (_Buffer == NULL || _Stream == NULL)
      TerminateOnError("UTIL_fread() called with invalid parameters\n");

   return (unsigned int)fread(_Buffer, _ElementSize, _ElementCount, _Stream);
}

unsigned int UTIL_fwrite(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream) {
   if (_Buffer == NULL || _Stream == NULL)
      TerminateOnError("UTIL_fread() called with invalid parameters\n");

   return (unsigned int)fwrite(_Buffer, _ElementSize, _ElementCount, _Stream);
}

void UTIL_fclose(void *fp) {
   if (fp != NULL) {
      fclose(fp);
   }
}

long flength(void *fp) {
   long old_pos = ftell(fp);
   if (old_pos == -1)
      return -1;
   if (UTIL_fseek(fp, 0, SEEK_END) == -1)
      return -1;
   long length = ftell(fp);
   UTIL_fseek(fp, old_pos, SEEK_SET);
   return length;
}
