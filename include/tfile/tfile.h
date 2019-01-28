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

enum class Mode {read, readWrite, write, truncate, append, readAppend};

template <Mode>
class Opener;

using Reader = Opener<Mode::read>;
using ReaderWriter = Opener<Mode::readWrite>;
using Writer = Opener<Mode::write>;
using TruncateReaderWriter = Opener<Mode::truncate>;
using Appender = Opener<Mode::append>;
using ReaderAppender = Opener<Mode::readAppend>;

/** OpenerBase class for all openers. */
class OpenerBase {
  public:
    explicit
    OpenerBase(FILE* file = nullptr) : file_(file) {}

    explicit
    OpenerBase(const char* filename, const char* mode);
    ~OpenerBase() { close(); }

    /** Get the current FILE* */
    FILE* get() { return file_; }

    /** Close the FILE* and empty it. */
    int close();

    /** Close the current FILE* and set to new value */
    void set(FILE* file);

    /** Release the current FILE* so it won't get closed on destruction
        and return it */
    FILE* release();

    /**
       Seek to any offset in the file.
       whence can be SEEK_SET, SEEK_CUR, or SEEK_END
    */
    int seek(off_t offset, int whence = SEEK_SET);

    /** Return true if the file is non-empty and closed.

        See https://stackoverflow.com/questions/5431941 for potential
        traps with this method.
     */
    bool eof() const { return file_ && feof(file_); }

  protected:
    FILE* file_;
};

/** Base class for all Readers. */
class ReaderBase : public OpenerBase {
  public:
    using OpenerBase::OpenerBase;


    size_t read(char* data, size_t length);
    size_t read(std::string& s);
    std::string read(size_t size);

    /** Reads a single line from the file. */
    std::string readLine();
};

/** Base class for all writers, written as a mix-in so we can have
    ReaderWriters */
template <typename Base>
class WriterMixIn : public Base {
  public:
    using Base::Base;

    size_t write(const char* data, size_t length);
    size_t write(const char* data);
    size_t write(const std::string& s);
};

using WriterBase = WriterMixIn<OpenerBase>;
using ReaderWriterBase = WriterMixIn<ReaderBase>;

//
// Implementation details follow
//

inline
OpenerBase::OpenerBase(const char* filename, const char* mode)
        : file_(fopen(filename, mode)) {
#ifdef __cpp_exceptions
    if (not file_)
        throw std::runtime_error(filename);
#endif
}

inline
int OpenerBase::close() {
    int result = 0;
    if (file_) {
        result = fclose(file_);
        file_ = nullptr;
    }
    return result;
}

inline
void OpenerBase::set(FILE* file) {
    close();
    file_ = file;
}

inline
FILE* OpenerBase::release() {
    auto f = file_;
    file_ = nullptr;
    return f;
}

inline
int OpenerBase::seek(off_t offset, int whence) {
    return fseeko(file_, offset, whence);
}

inline
size_t ReaderBase::read(char* data, size_t length) {
    return fread(data, 1, length, file_);
}

inline
size_t ReaderBase::read(std::string& s) {
    return read(&s[0], s.size());
}

inline
std::string ReaderBase::read(size_t size) {
    std::string result(size, '\0');
    result.resize(read(result));
    return result;
}

/**
   Reads a single line from a file.
 */
inline
std::string ReaderBase::readLine() {
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

template <typename Base>
size_t WriterMixIn<Base>::write(const char* data, size_t length) {
    return fwrite(data, 1, length, this->file_);
}

template <typename Base>
size_t WriterMixIn<Base>::write(const char* data) {
    return write(data, strlen(data));
}

template <typename Base>
size_t WriterMixIn<Base>::write(const std::string& s) {
    return write(s.data(), s.size());
}

template <Mode>
struct OpenerTraits {
    struct Opener;
    static const char* const mode();
};

template <>
struct OpenerTraits<Mode::read> {
    using Opener = ReaderBase;
    static const char* const mode() { return "r";}
};

template <>
struct OpenerTraits<Mode::readWrite> {
    using Opener = ReaderWriterBase;
    static const char* const mode() { return "r+";}
};

template <>
struct OpenerTraits<Mode::write> {
    using Opener = WriterBase;
    static const char* const mode() { return "w";}
};

template <>
struct OpenerTraits<Mode::truncate> {
    using Opener = ReaderWriterBase;
    static const char* const mode() { return "w+";}
};

template <>
struct OpenerTraits<Mode::append> {
    using Opener = WriterBase;
    static const char* const mode() { return "a";}
};

template <>
struct OpenerTraits<Mode::readAppend> {
    using Opener = ReaderWriterBase;
    static const char* const mode() { return "a+";}
};

template <Mode MODE>
class Opener : public OpenerTraits<MODE>::Opener {
  public:
    using Traits = OpenerTraits<MODE>;
    using Base = typename Traits::Opener;

    Opener() {}

    explicit Opener(const char* filename) : Base(filename, Traits::mode()) {}
    explicit Opener(Opener&& other) noexcept : Base(other.release()) {}
    Opener(const Opener&) = delete;

    Opener& operator=(Opener&& other) {
        this->set(other.release());
        return *this;
    }

    Opener& operator=(const Opener& other) = delete;
};

template <typename SizeFunction>
std::string testableRead(const char* filename, SizeFunction size) {
    // The file size might change between getting the size and reading it: see
    // https://www.reddit.com/r/cpp/comments/aiprv9/tiny_file_utilities/eerbyza/

    std::string result(size(filename), '\0');
    Reader reader(filename);
    result.resize(reader.read(result));

    while (true) {
        static const auto BUFFER_SIZE = 16;
        char buffer[BUFFER_SIZE];
        if (auto bytes = reader.read(buffer, BUFFER_SIZE))
            result.insert(result.end(), buffer, buffer + bytes);
        else
            break;
    }

    return result;
}

inline
std::string read(const char* filename) {
    return testableRead(filename, size);
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
