#include "util.h"
#include <stdio.h>

void *UTIL_fopen(const char *_FileName, const char *_Mode) {
   FILE *fp = NULL;
   if (_FileName == NULL || _Mode == NULL)
      TerminateOnError("UTIL_fopen() called with invalid parameters\n");
   else
      fp = fopen(_FileName, _Mode);
   if (fp == NULL)
      TerminateOnError("UTIL_fopen() returns a null pointer\n");
   return fp;
}

void *UTIL_fopen_without_checking(const char *_FileName, const char *_Mode) {
   if (_FileName == NULL || _Mode == NULL)
      TerminateOnError("UTIL_fopen() called with invalid parameters\n");
   else
      return fopen(_FileName, _Mode);
   return NULL;
}

int UTIL_fseek(void *_Stream, long _Offset, int _Origin) {
   if (_Stream == NULL)
      TerminateOnError("UTIL_fseek() called with invalid parameters\n");
   else
      return fseek((FILE *)_Stream, _Offset, _Origin);
   return 0;
}

unsigned int UTIL_fread(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream) {
   if (_Buffer == NULL || _Stream == NULL)
      TerminateOnError("UTIL_fread() called with invalid parameters\n");
   else
      return (unsigned int)fread(_Buffer, _ElementSize, _ElementCount, (FILE *)_Stream);
   return 0;
}

unsigned int UTIL_fwrite(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream) {
   if (_Buffer == NULL || _Stream == NULL)
      TerminateOnError("UTIL_fread() called with invalid parameters\n");
   else
      return (unsigned int)fwrite(_Buffer, _ElementSize, _ElementCount, (FILE *)_Stream);
   return 0;
}

void UTIL_fclose(void *fp) {
   if (fp != NULL)
      fclose((FILE *)fp);
}

long flength(void *fp) {
   long old_pos = ftell((FILE *)fp);
   if (old_pos == -1)
      return -1;
   if (UTIL_fseek(fp, 0, SEEK_END) == -1)
      return -1;
   long length = ftell((FILE *)fp);
   UTIL_fseek(fp, old_pos, SEEK_SET);
   return length;
}
