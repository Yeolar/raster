#include "File.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

namespace srilm {

const char *wordSeparators = " \t\r\n";

#define START_BUF_LEN 128  // needs to be > 2

File::File(const char *name, const char *mode, int exitOnError)
    : name(name ? strdup(name) : 0),
      lineno(0),
      exitOnError(exitOnError),
      skipComments(true),
      fp(NULL),
      buffer((char *)malloc(START_BUF_LEN)),
      bufLen(START_BUF_LEN),
      reuseBuffer(false),
      strFileLen(0),
      strFilePos(0),
      strFileActive(0) {
  assert(buffer != 0);

  if (name) {
    fp = ::fopen(name, mode);
  }

  if (fp == NULL) {
    if (exitOnError) {
      perror(name);
      exit(exitOnError);
    }
  }
  strFile = "";
}

File::File(FILE *fp, int exitOnError)
    : name(0),
      lineno(0),
      exitOnError(exitOnError),
      skipComments(true),
      fp(fp),
      buffer((char *)malloc(START_BUF_LEN)),
      bufLen(START_BUF_LEN),
      reuseBuffer(false),
      strFileLen(0),
      strFilePos(0),
      strFileActive(0) {
  assert(buffer != 0);
  strFile = "";
}

File::File(const char *fileStr, int exitOnError, int reserved_length)
    : name(0),
      lineno(0),
      exitOnError(exitOnError),
      skipComments(true),
      fp(NULL),
      buffer((char *)malloc(START_BUF_LEN)),
      bufLen(START_BUF_LEN),
      reuseBuffer(false),
      strFileLen(0),
      strFilePos(0),
      strFileActive(0) {
  assert(buffer != 0);

  strFile = fileStr;
  strFileLen = strFile.length();
  strFileActive = 1;
  // only reserve space if bigger than current capacity
  if (reserved_length > strFileLen)
    strFile.reserve(reserved_length);
}

File::File(std::string &fileStr, int exitOnError, int reserved_length)
    : name(0),
      lineno(0),
      exitOnError(exitOnError),
      skipComments(true),
      fp(NULL),
      buffer((char *)malloc(START_BUF_LEN)),
      bufLen(START_BUF_LEN),
      reuseBuffer(false),
      strFileLen(0),
      strFilePos(0),
      strFileActive(0) {
  assert(buffer != 0);

  strFile = fileStr;
  strFileLen = strFile.length();
  strFileActive = 1;
  // only reserve space if bigger than current capacity
  if (reserved_length > strFileLen)
    strFile.reserve(reserved_length);
}

File::~File() {
  /*
   * If we opened the file (name != 0), then we should close it
   * as well.
   */
  if (name != 0) {
    close();
    free(name);
  }

  if (buffer)
    free(buffer);
  buffer = NULL;
}

int File::close() {
  int status = 0;

  if (fp) {
    status = ::fclose(fp);
  }

  fp = NULL;
  if (status != 0) {
    if (exitOnError != 0) {
      perror(name ? name : "");
      exit(exitOnError);
    }
  }
  return status;
}

bool File::reopen(const char *newName, const char *mode) {
  strFile = "";
  strFileLen = 0;
  strFilePos = 0;
  strFileActive = 0;

  /*
   * If we opened the file (name != 0), then we should close it
   * as well.
   */
  if (name != 0) {
    close();
    free(name);
  }

  /*
   * Open new file as in File::File()
   */
  name = newName ? strdup(newName) : 0;

  if (name) {
    fp = ::fopen(name, mode);
  }

  if (fp == 0) {
    if (exitOnError) {
      perror(name);
      exit(exitOnError);
    }

    return false;
  }

  return true;
}

bool File::reopen(const char *mode) {
  strFile = "";
  strFileLen = 0;
  strFilePos = 0;
  strFileActive = 0;

  if (fp == NULL) {
    return false;
  }

  if (fflush(fp) != 0) {
    if (exitOnError != 0) {
      perror(name ? name : "");
      exit(exitOnError);
    }
  }

  FILE *fpNew = fdopen(fileno(fp), mode);

  if (fpNew == 0) {
    return false;
  } else {
    // XXX: we can't fclose(fp), so the old stream object becomes garbage
    fp = fpNew;
    return true;
  }
}

bool File::reopen(const char *fileStr, int reserved_length) {
  if (name != 0) {
    close();
  }

  strFile = fileStr;
  strFileLen = strFile.length();
  strFilePos = 0;
  strFileActive = 1;
  // only reserve space if bigger than current capacity
  if (reserved_length > strFileLen)
    strFile.reserve(reserved_length);

  return true;
}

bool File::reopen(std::string &fileStr, int reserved_length) {
  if (name != 0) {
    close();
  }

  strFile = fileStr;
  strFileLen = strFile.length();
  strFilePos = 0;
  strFileActive = 1;
  // only reserve space if bigger than current capacity
  if (reserved_length > strFileLen)
    strFile.reserve(reserved_length);

  return true;
}

bool File::error() {
  if (strFileActive)
    return 0;  // i/o using strings not file pointer, so no error

  return (fp == 0) || ferror(fp);
};

char *File::getline() {
  if (reuseBuffer) {
    reuseBuffer = false;
    return buffer;
  }

  while (1) {
    unsigned bufOffset = 0;
    bool lineDone = false;

    do {
      if (fgets(buffer + bufOffset, bufLen - bufOffset) == 0) {
        if (bufOffset == 0) {
          return 0;
        } else {
          buffer[bufOffset] = '\0';
          break;
        }
      }

      /*
       * Check if line end has been reached
       */
      unsigned numbytes = strlen(buffer + bufOffset);

      if (numbytes > 0 && buffer[bufOffset + numbytes - 1] != '\n') {
        if (bufOffset + numbytes >= bufLen - START_BUF_LEN) {
          /*
           * enlarge buffer
           */
          // cerr << "!REALLOC!" << endl;
          bufLen *= 2;
          buffer = (char *)realloc(buffer, bufLen);
          assert(buffer != 0);
        }
        bufOffset += numbytes;
      } else {
        lineDone = true;
      }
    } while (!lineDone);

    lineno++;

    /*
     * skip entirely blank lines
     */
    const char *p = buffer;
    while (*p && isspace(*p))
      p++;
    if (*p == '\0') {
      continue;
    }

    /*
     * skip comment lines (started with double '#')
     */
    if (skipComments && buffer[0] == '#' && buffer[1] == '#') {
      continue;
    }

    reuseBuffer = false;
    return buffer;
  }
}

void File::ungetline() { reuseBuffer = true; }

std::ostream &File::position(std::ostream &stream) {
  if (name) {
    stream << name << ": ";
  }
  return stream << "line " << lineno << ": ";
}

std::ostream &File::offset(std::ostream &stream) {
  if (name) {
    stream << name << ": ";
  }
  if (fp) {
    return stream << "offset " << ::ftello(fp) << ": ";
  } else {
    return stream << "offset unknown "
                  << ": ";
  }
}

/*------------------------------------------------------------------------*
 * "stdio" functions:
 *------------------------------------------------------------------------*/

int File::fgetc() {
  if (fp) {
    return ::fgetc(fp);
  }

  if (!strFileActive || strFileLen <= 0 || strFilePos >= strFileLen)
    return EOF;

  return strFile.at(strFilePos++);
}

// override fgets in case object using strFile
char *File::fgets(char *str, int n) {
  if (fp) {
    return ::fgets(str, n, fp);
  }

  if (!str || n <= 0)
    return NULL;

  int i = 0;

  for (i = 0; i < n - 1; i++) {
    int c = fgetc();
    if (c == EOF) {
      break;
    }

    str[i] = c;
    // xxx use \r on MacOS X?
    if (c == '\n') {
      // include \n in result
      i++;
      break;
    }
  }

  // always terminate
  str[i] = '\0';
  if (i == 0)
    return NULL;
  else
    return str;
}

int File::fputc(int c) {
  if (fp) {
    return ::fputc(c, fp);
  }

  // error condition, no string active
  if (!strFileActive)
    return EOF;

  strFile += c;

  return 0;
}

int File::fputs(const char *str) {
  if (fp) {
    return ::fputs(str, fp);
  }

  // error condition, no string active
  if (!strFileActive)
    return -1;

  strFile += str;

  return 0;
}

int File::fprintf(const char *format, ...) {
  if (fp) {
    va_list args;
    va_start(args, format);
    int num_written = vfprintf(fp, format, args);
    va_end(args);
    return num_written;
  }

  // error condition, no string active
  if (!strFileActive)
    return -1;

  // This is the default max size to append at any one time. On sgi we
  // get a buffer overrrun if we exceed this but elsewhere we manually
  // allocate a larger buffer if needed.
  const int maxMessage = 4096;
  char message[maxMessage];
  va_list args;
  va_start(args, format);

  // Return value not consistent...
  // Non-Windows: >= 0 is number of bytes needed in buffer not including
  // NULL terminator.
  // Windows: Returns -1 if output truncated.
  int checkSize = vsnprintf(message, maxMessage, format, args);
  if ((checkSize >= maxMessage) || (checkSize < 0)) {
    int curSize;
    if (checkSize >= maxMessage) {
      // Should know exact size needed
      curSize = checkSize + 1;
    } else {
      // Start with double initial size
      curSize = maxMessage * 2;
    }
    bool success = false;
    // Loop until successful but also impose 1GB cap on buffer size.
    const int maxAlloc = 1000000000;
    while (!success) {
      va_end(args);
      va_start(args, format);
      char *buf = new char[curSize];
      checkSize = vsnprintf(buf, curSize, format, args);
      if ((checkSize >= 0) && (checkSize < curSize)) {
        strFile += buf;
        success = true;
      } else {
        // Try larger size
        if (curSize <= maxAlloc / 2) {
          curSize *= 2;
        } else if (curSize < maxAlloc) {
          // Don't exceed cap
          curSize = maxAlloc;
        } else {
          // Fail
          delete[] buf;
          if (exitOnError) {
            exit(exitOnError);
          }
          strFile += "In class File, failed writing to buffer\n";
          break;
        }
      }
      delete[] buf;
    }
  } else {
    strFile += message;
  }

  va_end(args);

  return 0;
}

size_t File::fread(void *data, size_t size, size_t n) {
  if (fp) {
    return ::fread(data, size, n, fp);
  }

  // not supported for input from string
  return 0;
}

size_t File::fwrite(const void *data, size_t size, size_t n) {
  if (fp) {
    return ::fwrite(data, size, n, fp);
  }

  // not supported for output to string
  return 0;
}

long long File::ftell() {
  if (fp) {
    return ::ftello(fp);
  }

  // error condition, no string active
  if (!strFileActive)
    return -1;

  return (long long)strFilePos;
}

int File::fseek(long long offset, int origin) {
  if (fp) {
    return ::fseeko(fp, offset, origin);
  }

  // error condition, no string active
  if (!strFileActive)
    return -1;

  // xxx doesn't do (much) error checking
  if (origin == SEEK_CUR) {
    strFilePos += offset;
  } else if (origin == SEEK_END) {
    strFilePos = strFileLen + offset;  // use negative offset!
  } else if (origin == SEEK_SET) {
    strFilePos = offset;
  } else {
    // invalid origin
    return -1;
  }

  // xxx we check that position is not negative, but (currently) allow it to be
  // greater than length
  if (strFilePos < 0)
    strFilePos = 0;

  return 0;
}

const char *File::c_str() {
  if (fp)
    return 0;

  // error condition, no string active
  if (!strFileActive)
    return NULL;

  return strFile.c_str();
}

const char *File::data() {
  if (fp)
    return 0;

  // error condition, no string active
  if (!strFileActive)
    return NULL;

  return strFile.data();
}

size_t File::length() {
  if (fp)
    return 0;

  // error condition, no string active
  if (!strFileActive)
    return 0;

  return strFile.length();
}

}

