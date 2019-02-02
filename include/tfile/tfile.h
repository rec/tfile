#pragma once

#include <stddef.h>
#include <stdio.h>

#include <fstream>
#include <string>
#include <vector>

#ifdef __cpp_exceptions
#include <stdexcept>
#endif

namespace tfile {

/** Return the size in bytes of a file. */
size_t size(const char* filename);

/** Read an entire file into a string. */
void read(const char* filename, std::string&);

/** Read an entire file in and return it as a strings. */
std::string read(const char* filename);

using Lines = std::vector<std::string>;

/** Read a file into a vector of strings with the platform's line-endings */
Lines readLines(const char* filename);
void readLines(const char* filename, Lines&);

/** Write an entire file at once */
size_t write(const char* filename, const char* data, size_t length);
size_t write(const char* filename, const char* data);  // null-terminated
size_t write(const char* filename, const std::string& s);

/** Write a container of strings with the platform's line-endings */
template <typename Container = Lines>
size_t writeLines(const char* filename, Container);

/** Write a range of strings with the platform's line-endings */
template <typename ForwardIt>
size_t writeLines(const char* filename, ForwardIt begin, ForwardIt end);

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

#ifndef TFILE_SYSTEM_NEWLINE
  #ifdef _WIN32
    #define TFILE_SYSTEM_NEWLINE windows
  #else
    #define TFILE_SYSTEM_NEWLINE unix
  #endif
#endif

/** See https://en.wikipedia.org/wiki/Newline */
enum class Newline {
    cr_lf,  // Windows
    lf,     // Unix and MacOS

    atari8,
    cr,
    lf_cr,
    nl,
    rs,
    zx8x,
    unix = lf,
    windows = cr_lf,
    ibm = nl,
    system = TFILE_SYSTEM_NEWLINE
};

template <Newline = Newline::system>
const char* newlineString();

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

template <Newline NL, typename Reader> class LineReader;
template <Newline NL, typename Writer> class LineWriter;

/** Base for all Readers */
template <typename Derived>
class ReaderBase {
  public:
    size_t read(char* data, size_t length);
    size_t read(std::string& s);
    std::string read(size_t size);

    /** Return a LineReader */
    template <Newline NL = Newline::system>
    LineReader<NL, ReaderBase> lines();

    /** Return a LineReader - for use in ReaderWriters */
    template <Newline NL = Newline::system>
    LineReader<NL, ReaderBase> readLines();
};

/** Base for all Writerse */
template <typename Derived>
class WriterBase {
  public:
    size_t write(const char* data, size_t length);
    size_t write(const char* data);
    size_t write(const std::string&);

    /** Return a LineWriter */
    template <Newline NL = Newline::system>
    LineWriter<NL, WriterBase> lines();

    /** Return a LineWriter - for use in ReaderWriters */
    template <Newline NL = Newline::system>
    LineWriter<NL, WriterBase> writeLines();
};

/** Read lines from a reader, taking newlines into account */
template <Newline NL, typename Reader>
class LineReader {
  public:
    LineReader(Reader& reader) : reader_(reader) {}

    /** Try to read a single line, return true if successful */
    bool readOne(std::string&);

    /** Apply a function to each line */
    template <typename Function>
    void forEach(Function);

    /** Use each line to fill an insert iterator */
    template <typename InserterIt>
    void fill(InserterIt);

    /** Read all lines into a container */
    template <typename Container>
    void read(Container&);

    /** Read all lines and return a container */
    template <typename Container = Lines>
    Container read();

  private:
    Reader& reader_;
};

/** Write lines to a writer, adding newlines */
template <Newline NL, typename Writer>
class LineWriter {
  public:
    LineWriter(Writer& writer) : writer_(writer) {}

    /** Write a single line, return the number of bytes written */
    size_t writeOne(const std::string&);

    /** Write an iterator of lines with newlines */
    template <typename ForwardIt>
    size_t write(ForwardIt begin, ForwardIt end);

    /** Write a container of lines, adding newlines */
    template <typename Container = Lines>
    size_t write(Container);

  private:
    Writer& writer_;
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
size_t ReaderBase<Derived>::read(char* data, size_t length) {
    auto fp = static_cast<Derived*>(this)->get();
    return fread(data, 1, length, fp);
}

template <typename Derived>
size_t ReaderBase<Derived>::read(std::string& s) {
    return read(&s[0], s.size());
}

template <typename Derived>
std::string ReaderBase<Derived>::read(size_t size) {
    std::string result(size, '\0');
    result.resize(read(result));
    return result;
}

template <Newline NL, typename Reader>
bool LineReader<NL, Reader>::readOne(std::string& line) {
    static const auto newline = newlineString<NL>();
    static const auto len = strlen(newline);

    size_t seen = 0;  // How many characters have we seen in newline?
    bool empty = true;
    line.resize(0);

    // Emit a partial newline if any
    auto emitPartial = [&] () {
        if (seen) {
            for (size_t i = 0; i < seen; ++i)
                line += newline[i];
            seen = 0;
        }
    };

    while (true) {
        char ch;
        if (not reader_.read(&ch, 1)) {
            emitPartial();
            return not empty;
        }

        empty = false;
        if (ch == newline[seen]) {
            if (++seen >= len)
                return true;
        } else {
            emitPartial();
            line += ch;
        }
    }
}

template <Newline NL, typename Reader>
template <typename Function>
void LineReader<NL, Reader>::forEach(Function f) {
    std::string s;
    while (readOne(s))
        f(s);
}

template <Newline NL, typename Reader>
template <typename InserterIt>
void LineReader<NL, Reader>::fill(InserterIt begin) {
    forEach([&] (const std::string& s) {
        *begin = s;
    });
}

template <Newline NL, typename Reader>
template <typename Container>
void LineReader<NL, Reader>::read(Container& c) {
    fill(std::back_inserter(c));
}

template <Newline NL, typename Reader>
template <typename Container>
Container LineReader<NL, Reader>::read() {
    Container c;
    read(c);
    return c;
}

template <typename Derived>
template <Newline NL>
LineReader<NL, ReaderBase<Derived>> ReaderBase<Derived>::lines() {
    return {*this};
}

template <typename Derived>
template <Newline NL>
LineReader<NL, ReaderBase<Derived>> ReaderBase<Derived>::readLines() {
    return {*this};
}

template <Newline NL, typename Writer>
size_t LineWriter<NL, Writer>::writeOne(const std::string& s) {
    return writer_.write(s) + writer_.write(newlineString<NL>());
}

/** Write a container of lines, adding newlines */
template <Newline NL, typename Writer>
template <typename Container>
size_t LineWriter<NL, Writer>::write(Container c) {
    using std::begin;
    using std::end;
    return write(begin(c), end(c));
}

/** Write a container of lines, adding newlines */
template <Newline NL, typename Writer>
template <typename ForwardIt>
size_t LineWriter<NL, Writer>::write(ForwardIt begin, ForwardIt end) {
    size_t size = 0;
    for (; begin != end; ++begin)
        size += writeOne(*begin);
    return size;
}

template <typename Derived>
template <Newline NL>
LineWriter<NL, WriterBase<Derived>> WriterBase<Derived>::lines() {
    return {*this};
}

template <typename Derived>
template <Newline NL>
LineWriter<NL, WriterBase<Derived>> WriterBase<Derived>::writeLines() {
    return {*this};
}

template <typename Derived>
size_t WriterBase<Derived>::write(const char* data, size_t length) {
    auto fp = static_cast<Derived*>(this)->get();
    return fwrite(data, 1, length, fp);
}

template <typename Derived>
size_t WriterBase<Derived>::write(const char* data) {
    return write(data, strlen(data));
}

template <typename Derived>
size_t WriterBase<Derived>::write(const std::string& s) {
    return write(s.data(), s.size());
}

template <Mode>
struct Traits {
    struct Base;
};

using Read = FileHandle<ReaderBase>;
using Write = FileHandle<WriterBase>;
using ReadWrite = FileHandle<ReaderBase, WriterBase>;

template <> struct Traits<Mode::read> { using Base = Read; };
template <> struct Traits<Mode::readWrite> { using Base = ReadWrite; };
template <> struct Traits<Mode::write> { using Base = Write; };
template <> struct Traits<Mode::truncate> { using Base = ReadWrite; };
template <> struct Traits<Mode::append> { using Base = Write; };
template <> struct Traits<Mode::readAppend> { using Base = ReadWrite; };

template <> const char* newlineString<Newline::atari8>() { return "\x9b"; }
template <> const char* newlineString<Newline::cr>() { return "\r"; }
template <> const char* newlineString<Newline::cr_lf>() { return "\r\n"; }
template <> const char* newlineString<Newline::lf>() { return "\n"; }
template <> const char* newlineString<Newline::lf_cr>() { return "\n\r"; }
template <> const char* newlineString<Newline::nl>() { return "\x15"; }
template <> const char* newlineString<Newline::rs>() { return "\x1e"; }
template <> const char* newlineString<Newline::zx8x>() { return "\x76"; }

template <> const char* modeString<Mode::read>() { return "r"; }
template <> const char* modeString<Mode::readWrite>() { return "r+"; }
template <> const char* modeString<Mode::write>() { return "w"; }
template <> const char* modeString<Mode::truncate>() { return "w+"; }
template <> const char* modeString<Mode::append>() { return "a"; }
template <> const char* modeString<Mode::readAppend>() { return "a+"; }

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

inline
void readLines(const char* filename, Lines& lines) {
    Reader(filename).lines().read(lines);
}

inline
Lines readLines(const char* filename) {
    return Reader(filename).lines().read();
}

inline
size_t writeLines(const char* filename, const Lines& lines) {
    return writeLines<>(filename, lines);
}

inline
size_t size(const char* filename) {
    // From http://stackoverflow.com/questions/5840148
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

template <typename Container>
size_t writeLines(const char* filename, Container c) {
    return Writer(filename).writeLines().write(c);
}

template <typename ForwardIt>
size_t writeLines(const char* filename, ForwardIt begin, ForwardIt end) {
    return Writer(filename).writeLines().write(begin, end);
}

}  // namespace tfile
