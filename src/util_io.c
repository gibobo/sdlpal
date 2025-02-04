#include "util.h"

void *UTIL_fopen(const char *_FileName, const char *_Mode) {
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
