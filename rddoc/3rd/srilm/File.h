/*
 * File.h -- File I/O utilities for LM
 */

#pragma once

#include <iostream>
#include <stdio.h>

namespace srilm {

const unsigned int maxWordsPerLine = 50000;

extern const char *wordSeparators;

/*
 * A File object is a wrapper around a stdio FILE pointer.  If presently
 * provides two kinds of convenience.
 *
 * - constructors and destructors manage opening and closing of the stream.
 *   The stream is checked for errors on closing, and the default behavior
 *   is to exit() with an error message if a problem was found.
 * - the getline() method strips comments and keeps track of input line
 *   numbers for error reporting.
 *
 * File object can be cast to (FILE *) to perform most of the standard
 * stdio operations in a seamless way.
 *
 * The File object can also read/write to a std::string, for file
 * access "to memory".
 *
 * To read from an existing string, allocate the File object using:
 * File(char *, size_t) or File(std::string&) and then call any File()
 * accessor function.  For reading, you can also allocate the File
 * object using File(NULL, exitOnError) and then reopen it using
 * File.reopen(char *, size_t) or File.reopen(std::string&).
 *
 * To write to a string, allocate the File object using:
 * File("", 0, exitOnError, reserved_length).
 * Alternatively, use File(NULL, exitOnError) followed by
 * File.reopen("", 0, reserved_length).
 *
 * NOTE: String I/O does not yet support binary data
 * (unless initialized from std::string?).
 * NOTE: For backwards compatibility, File object preferentially uses
 * FILE * object if it exists.
 */
class File {
public:
  // Note that prior to September, 2014, internal member variable
  // only stored exact pointer to name, now makes copy of name
  // since otherwise user needs to ensure name is not changed
  // or deleted (or stack variable) during lifetime of File object
  // (or prior to reopen with new name).
  File(const char *name, const char *mode, int exitOnError = 1);
  File(FILE *fp = 0, int exitOnError = 1);
  // Initialize strFile with contents of string.  strFile will be
  // resized to "reserved_length" if this value is bigger than the
  // string size.
  File(const char *fileStr, int exitOnError = 1, int reserved_length = 0);
  File(std::string &fileStr, int exitOnError = 1, int reserved_length = 0);
  ~File();

  char *getline();
  void ungetline();
  int close();
  bool reopen(const char *name, const char *mode);
  bool reopen(const char *mode);  // switch to binary I/O
  // [close() and] reopen File and initialize strFile with contents of string
  bool reopen(const char *fileStr, int reserved_length = 0);
  bool reopen(std::string &fileStr, int reserved_length = 0);
  bool error();

  std::ostream &position(std::ostream &stream = std::cerr);
  std::ostream &offset(std::ostream &stream = std::cerr);

  char *name;
  unsigned int lineno;
  bool exitOnError;
  bool skipComments;

  // Provide "stdio" equivalent functions for the case where the
  // File class is wrapping a string instead of a FILE, since
  // casting File to (FILE *) won't work in this case.  The
  // functions should perform the same as their namesakes, but will
  // not set errno.
  char *fgets(char *str, int n);
  int fgetc();
  int fputc(int c);
  int fputs(const char *str);
  // uses internal 4KB buffer
  int fprintf(const char *format, ...);
  size_t fread(void *data, size_t size, size_t n);
  size_t fwrite(const void *data, size_t size, size_t n);
  long long ftell();
  int fseek(long long offset, int origin);

  // get string contents from File() object, provided we are doing string I/O
  const char *c_str();
  const char *data();
  size_t length();

private:
  FILE *fp;

  char *buffer;
  unsigned bufLen;
  bool reuseBuffer;

  // read/write from/to string instead of file
  std::string strFile;
  int strFileLen;
  int strFilePos;
  int strFileActive;
};

}

