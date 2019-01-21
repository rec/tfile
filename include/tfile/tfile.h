#pragma once

#include <stddef.h>
#include <stdio.h>

#include <fstream>
#include <string>

#ifdef __cpp_exceptions
#include <stdexcept>
#endif

namespace tfile {

/** Read an entire file in and return it as a string. */
std::string read(const char* filename);

/** Write an entire file at once. */
size_t write(const char* filename, const char* data, size_t length);

/** Write an entire file at once from a null-terminated string. */
size_t write(const char* filename, const char* data);

/** Write an entire file at once. */
size_t write(const char* filename, const std::string& s);

/** Return the size in bytes of a file. */
size_t size(const char* filename);

/*
File Openers
===============

File Openers are for applications which need to keep a file open for
processing. File Openers are a thin wrapper over the C file handle type FILE*,
which is the basis of I/O in C and C++.

An File Opener offers these advantages over a raw FILE*:

  * Automatically closes the FILE* in its destructor
  * Prevents impossible reads or writes at compile time
  * Throws an exception if the file won't open and exceptions are enabled
  * Can iterate one line at a time

"Impossible reads or writes" means that there is nothing in C or C++ to
prevent writing code to, say, read from a write-only file handle - it will
simply return an error at runtime - but a File Opener provides read or
write methods only if they actually work, so these errors can be caught at
compile-time.

There are six File Openers, corresponding to the six modes in which files
can be opened:

  * `tfile::Reader`: read-only, position at start of file - mode "r"
  * `tfile::ReaderWriter`: read-write, position at start of file - mode "r+"
  * `tfile::Writer`: write-only, truncate file to empty - mode "w"
  * `tfile::TruncateReaderWriter`: read-write, truncate to empty - mode "w+"
  * `tfile::Appender`: write-only, position at end of file - mode "a"
  * `tfile::ReaderAppender`: read-write, position at end of file - mode "a+"

For more information on file opening modes, see
http://man7.org/linux/man-pages/man3/fopen.3.html

Examples of usage:

  tfile::Reader reader("myfile2.txt");
  std::string s(10);
  auto bytes_read = reader.read(s);   // at most 10
  // reader.write("hello");  // Won't compile

  tfile::Appender("myfile2.txt").write("a new line\n");
  // tfile::Appender("myfile2.txt").read();  // won't compile

  while (true) {
      auto line = reader.readLine();
      if (line.empty())
          break;
      // ...
  }
*/

class Reader;  // mode "r"
class ReaderWriter;  // mode "r+"
class Writer;  // mode "w"
class TruncateReaderWriter; // mode "w+"
class Appender;  // mode "a"
class ReaderAppender;  // mode "a+"

//
// Implementation details follow
//

namespace detail {

class Opener {
  protected:
    Opener(const char* filename, const char* mode)
            : file_(fopen(filename, mode)) {
#ifdef __cpp_exceptions
        if (not file_)
            throw std::runtime_error(filename);
#endif
    }

    Opener(Opener&& other) : file_(other.file_) {
        other.file_ = nullptr;
    }

    Opener& operator=(Opener&& other) {
        close();
        file_ = other.file_;
        other.file_ = nullptr;
        return *this;
    }

    Opener() : file_(nullptr) {}
    Opener(const Opener&) = delete;
    Opener& operator=(const Opener&) = delete;

  public:
    ~Opener() { close(); }

    void close() {
        if (file_) {
            fclose(file_);
            file_ = nullptr;
        }
    }

    FILE* get() { return file_; }

    int seek(off_t offset, int whence) {
        return fseeko(file_, offset, whence);
    }

  protected:
    FILE* file_;
};

class Reader : public Opener {
  public:
    using Opener::Opener;

    size_t read(char* data, size_t length) {
        return fread(data, 1, length, file_);
    }

    size_t read(std::string& s) {
        return read(&s[0], s.size());
    }

    std::string read(size_t size) {
        std::string result(size, '\0');
        result.resize(read(result));
        return result;
    }

    /**
       Reads a single line from a file.
     */
    std::string readLine() {
        std::string line;
        char ch = '\0';
        while (true) {
            auto len = read(&ch, 1);
            if (not len or ch == '\n')
                break;
            if (ch != '\r')
                line += ch;
        }
        return line;
    }
};

class Writer : public Opener {
  public:
    using Opener::Opener;

    size_t write(const char* data, size_t length) {
        return fwrite(data, 1, length, file_);
    }

    size_t write(const char* data) {
        return write(data, strlen(data));
    }

    size_t write(const std::string& s) {
        return write(s.data(), s.size());
    }

};

class ReaderWriter : public Reader, public Writer {
  public:
    using Reader::Reader;
};

}  // namespace detail


class Reader : public detail::Reader {
  public:
    Reader(const char* filename) : detail::Reader(filename, "r") {}
};

class ReaderWriter : public detail::ReaderWriter {
  public:
    ReaderWriter(const char* filename) : detail::ReaderWriter(filename, "r+") {}
};

class Writer : public detail::Writer {
  public:
    Writer(const char* filename) : detail::Writer(filename, "w") {}
};

class TruncateReaderWriter : public detail::ReaderWriter {
  public:
    TruncateReaderWriter(const char* filename)
            : detail::ReaderWriter(filename, "w+") {}
};

class Appender : public detail::Writer {
  public:
    Appender(const char* filename) : detail::Writer(filename, "a") {}
};

class ReaderAppender : public detail::ReaderWriter {
  public:
    ReaderAppender(const char* filename)
            : detail::ReaderWriter(filename, "a+") {}
};

inline
std::string read(const char* filename) {
    std::string result(size(filename), '\0');
    Reader(filename).read(result);
    return result;
}

inline
size_t write(const char* filename, const char* data, size_t length) {
    return Writer(filename).write(data, length);
}

inline
size_t write(const char* filename, const char* data) {
    return write(filename, data, strlen(data));
}

inline
size_t write(const char* filename, const std::string& s) {
    return write(filename, s.data(), s.size());
}


inline
size_t size(const char* filename) {
    // From http://stackoverflow.com/questions/5840148
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

}  // namespace tfile {
