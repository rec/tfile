#pragma once

#include <stddef.h>
#include <stdio.h>

#include <fstream>
#include <string>

#ifdef __cpp_exceptions
#include <stdexcept>
#endif

#ifndef TFILE_WINDOWS
#define TFILE_WINDOW defined(_WIN32)
#endif

namespace tfile {

// Functions to read and write a file all at once

/** Read an entire file into a string. */
void read(const char* filename, std::string&);

/** Read an entire file in and return it as a string. */
std::string read(const char* filename);

/** Write an entire file at once. */
size_t write(const char* filename, const char* data, size_t length);
size_t write(const char* filename, const char* data);  // null-terminated
size_t write(const char* filename, const std::string& s);

// Functions to read and write lines.
// The line-endings depend on the platform, are removed
// on read, and added back on write

const char* lineEnding();

using Lines = std::vector<std::string>;

/** Read all the lines of a file into a container. */
template <typename Container = Lines>
void readLines(const char* filename, Container& c);

/** Read all the lines of a file and return as a container. */
template <typename Container = Lines>
Container readLines(const char* filename);

/** Call a function for each line in a file */
template <typename Function>
void forEachLine(const char* filename, Function);

/** Write all the lines in a container over a file */
template <typename Container = Lines>
size_t writeLines(const char* filename, Container);

/** Write all the lines in a range over a file */
template <typename ForwardIt>
size_t writeLines(const char* filename, ForwardIt begin, ForwardIt end);

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

    // You can't write a reader, so this won't compile:
    // reader.write("hello");

    // Open a file, appear a line, close it.
    tfile::Appender("myfile2.txt").writeLine("a new line");

    // You can't read an Appender, so this won't compile:
    // tfile::Appender("myfile2.txt").read();

    // Iterate through lines.

    std::string line;
    while (reader.readLine(line)) {
       // Do things to `line` here
    }

    // Another way to do that:

    reader.forEachLine([] (const std::string& line) {
        // Do things to `line` here
    });
*/

enum class Mode {read, readWrite, write, truncate, append, readAppend};

template <Mode> const char* modeString();
const char* modeString(Mode);

template <Mode>
class Opener;

using Reader = Opener<Mode::read>;
using ReaderWriter = Opener<Mode::readWrite>;
using Writer = Opener<Mode::write>;
using TruncateReaderWriter = Opener<Mode::truncate>;
using Appender = Opener<Mode::append>;
using ReaderAppender = Opener<Mode::readAppend>;

/** Wraps a file, with mixins for reading or writing */
template <template <typename> class ... Mixins>
class FileHandle : public Mixins<FileHandle<Mixins...>>... {
  public:
    explicit
    FileHandle(FILE* file = nullptr) : file_(file) {}

    explicit
    FileHandle(const char* filename, const char* mode);
    ~FileHandle() { close(); }

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

  private:
    FILE* file_;
};

/** Reader Mixin */
template <typename Derived>
class ReaderMixin {
  public:
    size_t read(char* data, size_t length);
    size_t read(std::string& s);
    std::string read(size_t size);

    /** Reads a single line from the file, discarding trailing \r\n on Windows
        or \n elsewhere */
    bool readLine(std::string&);

    template <typename InserterIt>
    void fillLines(InserterIt begin) {
        std::string s;
        while (readLine(s))
            *begin = s;
    }

    /** Apply a function to each line */
    template <typename Function>
    void forEachLine(Function f) {
        std::string s;
        while (readLine(s))
            f(s);
    }

    /** Read lines into a container */
    template <typename Container>
    void readLines(Container& c) {
        fillLines(std::back_inserter(c));
    }

    template <typename Container = Lines>
    Container readLines() {
        Container c;
        readLines(c);
        return c;
    }

    FILE* get() { return static_cast<Derived*>(this)->get(); }
};

/** Writer Mixin */
template <typename Derived>
class WriterMixin {
  public:
    size_t write(const char* data, size_t length);
    size_t write(const char* data);
    size_t write(const std::string&);

    /** Write a single line, adding line endings */
    size_t writeLine(const std::string&);

    /** Write an iterator of lines with line endings */
    template <typename ForwardIt>
    size_t writeLines(ForwardIt begin, ForwardIt end) {
        size_t size = 0;
        for (; begin != end; ++begin)
            size += writeLine(*begin);
        return size;
    }

    /** Write a container of lines, adding line endings */
    template <typename Container = Lines>
    size_t writeLines(Container c) {
        using std::begin;
        using std::end;
        return this->writeLines(begin(c), end(c));
    }

    FILE* get() { return static_cast<Derived*>(this)->get(); }
};

//
// Implementation details follow
//

template <template <typename> class ... Mixins>
FileHandle<Mixins...>::FileHandle(const char* filename, const char* mode)
        : file_(fopen(filename, mode)) {
#ifdef __cpp_exceptions
    if (not file_)
        throw std::runtime_error(filename);
#endif
}

template <template <typename> class ... Mixins>
int FileHandle<Mixins...>::close() {
    int result = 0;
    if (file_) {
        result = fclose(file_);
        file_ = nullptr;
    }
    return result;
}

template <template <typename> class ... Mixins>
void FileHandle<Mixins...>::set(FILE* file) {
    close();
    file_ = file;
}

template <template <typename> class ... Mixins>
FILE* FileHandle<Mixins...>::release() {
    auto f = file_;
    file_ = nullptr;
    return f;
}

template <template <typename> class ... Mixins>
int FileHandle<Mixins...>::seek(off_t offset, int whence) {
    return fseeko(file_, offset, whence);
}

template <typename Derived>
size_t ReaderMixin<Derived>::read(char* data, size_t length) {
    return fread(data, 1, length, get());
}

template <typename Derived>
size_t ReaderMixin<Derived>::read(std::string& s) {
    return read(&s[0], s.size());
}

template <typename Derived>
std::string ReaderMixin<Derived>::read(size_t size) {
    std::string result(size, '\0');
    result.resize(read(result));
    return result;
}

template <typename Reader>
bool readLineLinux(Reader& reader, std::string& line) {
    bool empty = true;
    line.resize(0);

    while (true) {
        char ch;
        auto len = reader.read(&ch, 1);
        if (not len)
            return not empty;
        if (ch == '\n')
            return true;
        empty = false;
        line += ch;
    }
}

template <typename Reader>
bool readLineWindows(Reader& reader, std::string& line) {
    bool empty = true, wasCarriageReturn = false;
    line.resize(0);

    while (true) {
        char ch;
        auto len = reader.read(&ch, 1);
        if (not len) {
            if (not wasCarriageReturn)
                return not empty;
            line += '\r';
            return true;
        }

        if (ch == '\n') {
            if (wasCarriageReturn)
                return true;
            line += ch;
        } else {
            if (wasCarriageReturn) {
                line += '\r';
                wasCarriageReturn = false;
            }
            if (ch == '\r') {
                wasCarriageReturn = true;
            } else {
                empty = false;
                line += ch;
            }
        }
    }
}

template <typename Derived>
bool ReaderMixin<Derived>::readLine(std::string& line) {
#if TFILE_WINDOWS
    return readLineWindows(*this, line);
#else
    return readLineLinux(*this, line);
#endif
}

template <typename Derived>
size_t WriterMixin<Derived>::write(const char* data, size_t length) {
    return fwrite(data, 1, length, get());
}

template <typename Derived>
size_t WriterMixin<Derived>::write(const char* data) {
    return write(data, strlen(data));
}

template <typename Derived>
size_t WriterMixin<Derived>::write(const std::string& s) {
    return write(s.data(), s.size());
}

template <typename Derived>
size_t WriterMixin<Derived>::writeLine(const std::string& s) {
    return write(s) + write(lineEnding());
}

using ReaderBase = FileHandle<ReaderMixin>;
using WriterBase = FileHandle<WriterMixin>;
using ReaderWriterBase = FileHandle<ReaderMixin, WriterMixin>;

template <> const char* modeString<Mode::read>() { return "r"; }
template <> const char* modeString<Mode::readWrite>() { return "r+"; }
template <> const char* modeString<Mode::write>() { return "w"; }
template <> const char* modeString<Mode::truncate>() { return "w+"; }
template <> const char* modeString<Mode::append>() { return "a"; }
template <> const char* modeString<Mode::readAppend>() { return "a+"; }

template <Mode>
struct Traits {
    struct Base;
};

template <> struct Traits<Mode::read> { using Base = ReaderBase; };
template <> struct Traits<Mode::readWrite> { using Base = ReaderWriterBase; };
template <> struct Traits<Mode::write> { using Base = WriterBase; };
template <> struct Traits<Mode::truncate> { using Base = ReaderWriterBase; };
template <> struct Traits<Mode::append> { using Base = WriterBase; };
template <> struct Traits<Mode::readAppend> { using Base = ReaderWriterBase; };

template <Mode MODE>
class Opener : public Traits<MODE>::Base {
  public:
    using Base = typename Traits<MODE>::Base;

    Opener() {}

    explicit Opener(const char* filename) : Base(filename, modeString<MODE>()) {}
    explicit Opener(Opener&& other) noexcept : Base(other.release()) {}
    Opener(const Opener&) = delete;

    Opener& operator=(Opener&& other) {
        this->set(other.release());
        return *this;
    }

    Opener& operator=(const Opener& other) = delete;
};

template <typename SizeFunction>
void testableRead(const char* filename, std::string& s, SizeFunction size) {
    // The file size might change between getting the size and reading it: see
    // https://www.reddit.com/r/cpp/comments/aiprv9/tiny_file_utilities/eerbyza/

    s.resize(size(filename));

    Reader reader(filename);
    s.resize(reader.read(s));

    while (true) {
        static const auto BUFFER_SIZE = 16;
        char buffer[BUFFER_SIZE];
        if (auto bytes = reader.read(buffer, BUFFER_SIZE))
            s.insert(s.end(), buffer, buffer + bytes);
        else
            break;
    }
}

inline
void read(const char* filename, std::string& s) {
    testableRead(filename, s, size);
}


inline
std::string read(const char* filename) {
    std::string s;
    read(filename, s);
    return s;
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

/** Read all the lines of a file into a container. */
template <typename Container>
void readLines(const char* filename, Container& c) {
    Reader(filename).readLines(c);
}

/** Read all the lines of a file and return as a container. */
template <typename Container>
Container readLines(const char* filename) {
    return Reader(filename).readLines<Container>();
}

/** Call a function for each line in a file */
template <typename Function>
void forEachLine(const char* filename, Function f) {
    return Reader(filename).forEachLine(f);
}

template <typename Container>
size_t writeLines(const char* filename, Container c) {
    return Writer(filename).writeLines(c);
}

template <typename ForwardIt>
size_t writeLines(const char* filename, ForwardIt begin, ForwardIt end) {
    return Writer(filename).writeLines(begin, end);
}

inline
size_t size(const char* filename) {
    // From http://stackoverflow.com/questions/5840148
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

inline
const char* lineEnding() {
#if TFILE_WINDOWS
    return "\r\n";
#else
    return "\n";
#endif
}

}  // namespace tfile
