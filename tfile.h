#pragma once

#include <stdio.h>

#include <fstream>
#include <stdexcept>
#include <string>
#include <type_traits>

/*
  tfile is a small header-only library for C++11 and up which offers a small
  number of extremely useful features:

    * Handle: a zero-cost abstraction around C's FILE* classic file handle
    * useful functions to read a whole file at once, write a whole file at once,
      and get the bytesize of a file

  The type FILE*, called a "file handle", is the basis of nearly all file I/O in
  C and C++.

  tfile::Handle offers some advantages over FILE*:

    1. It automatically closes the FILE* when the variable goes out of scope
    2. It prevents impossible reads or writes at compile time
    3. By default, it throws an exception if the FILE* fails to open
    4. There's a readLine method that reads a single line at a time.

  File handles are usually created in C/C++ using fopen - see
  http://man7.org/linux/man-pages/man3/fopen.3.html

  File handles can be opened in six modes:

    * r: read-only, position at the start of the file
    * r+: read-write, position at the start of the file
    * w: write-only, truncate the file to empty
    * w+: read-write, truncate the file to empty
    * a: write-only, position at the end of the file
    * a+: read-write, position at the end of the file

  There is nothing in C or C++ to prevent writing code to, say, read from a
  write-only file handle - it will simply return an error at runtime.

  But if you use a Handle, only the methods you can actually use are provided.
  Attemping to read from a write-only Handle will result in an  error at
  compilation time.

  Example of usage:

    auto contents = tfile::read("myfile.txt");

    auto handle = tfile::reader("myfile2.txt");

    // handle.write("hello");  // Won't compile

    while (true) {
        auto line = handle.readLine();
        if (line.empty())
            break;
        // Process the line here.
    }
*/


namespace tfile {

/** Read an entire file in and return it as a string. */
std::string read(char const* filename);

/** Write an entire file at once. */
void write(char const* filename, char const* data, size_t length);

/** Write an entire file at once from a null-terminated string. */
void write(char const* filename, char const* data);

/** Write an entire file at once.  The class String needs to have data() and
    size() methods like those of std::string .*/
template <class String>
void write(char const* filename, String const& s);

/** Return the size in bytes of a file. */
size_t size(char const* filename);

/** Position is a relative position in a file for seeks. */
enum class Position {
    start = SEEK_SET,
    current = SEEK_CUR,
    end = SEEK_END
};

/** A Handle encapsulates a FILE* handle, making sure it's closed when the
    Handle goes out of scope.  A Handle which can perhaps read or write,
    depending on the CAN_READ and CAN_WRITE boolean template parameters.

    Methods:

    // Read bytes from the file up to the end of the MutableBytes, or the file.
    // Return the number of bytes read.
    // Only enabled when CAN_READ is true.
    size_t read(char* data, size_t length);

    // Write the Bytes into the file
    // Return the number of bytes written.
    // Only enabled when CAN_WRITE is true.
    size_t write(char* data, size_t length);

    // Seeks to the Position and offset.
    // Returns false on failure, not that that's much use.
    bool seek(Position position, long offset = 0) {
*/

template <bool CAN_READ, bool CAN_WRITE>
class Handle;

Handle<true, false> reader(char const* filename);             // mode "r"
Handle<true,  true> readWriter(char const* filename);         // mode "r+"
Handle<false, true> writer(char const* filename);             // mode "w"
Handle<true,  true> truncateReadWriter(char const* filename); // mode "w+"
Handle<true,  true> appender(char const* filename);           // mode "a"
Handle<true,  true> readAppender(char const* filename);       // mode "a+"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Implementation details follow.

template <bool CAN_READ, bool CAN_WRITE>
class Handle {
  public:
    static const size_t BLOCK_SIZE = 1024;

    Handle(char const* name, char const* mode) {
        file_ = fopen(name, mode);
        if (not file_)
            throw std::runtime_error(name);
    }

    Handle(Handle&&) = default;
    Handle(Handle const&) = delete;

    ~Handle() {
        fclose(file_);
    }

    FILE* get() {
        return file_;
    }

    template <class Size = size_t>
    typename std::enable_if<CAN_READ, Size>::type
    read(char* data, size_t length) {
        return fread(data, 1, length, file_);
    }

    template <class Size = size_t>
    typename std::enable_if<CAN_READ, Size>::type
    read(std::string& s) {
        return read(&s[0], s.size());
    }

    template <class String = std::string>
    typename std::enable_if<CAN_READ, String>::type
    read(size_t size = BLOCK_SIZE) {
        std::string result(size, '\0');
        result.resize(read(result));
        return result;
    }

    /**
       Reads a single line from a file.
     */
    template <class String = std::string>
    typename std::enable_if<CAN_READ, String>::type
    readLine() {
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

    template <class Size = size_t>
    typename std::enable_if<CAN_WRITE, Size>::type
    write(char const* data, size_t length) {
        return fwrite(data, 1, length, file_);
    }

    template <class Size = size_t>
    typename std::enable_if<CAN_WRITE, Size>::type
    write(char const* data) {
        return write(data, strlen(data));
    }

    template <class String, class Size = size_t>
    typename std::enable_if<CAN_WRITE, Size>::type
    write(String const& s) {
        return fwrite(s.data(), 1, s.size(), file_);
    }

    bool seek(Position position, long offset = 0) {
        return not fseek(file_, offset, int(position));
    }

  private:
    FILE* file_;
};


inline
Handle<true, false> reader(char const* filename) {
    return {filename, "r"};
}

inline
Handle<true,  true> readWriter(char const* filename) {
    return {filename, "r+"};
}

inline
Handle<false, true> writer(char const* filename) {
    return {filename, "w"};
}

inline
Handle<true,  true> truncateReadWriter(char const* filename) {
    return {filename, "w+"};
}

inline
Handle<true,  true> appender(char const* filename) {
    return {filename, "a"};
}

inline
Handle<true,  true> readAppender(char const* filename) {
    return {filename, "a+"};
}

inline
std::string read(char const* filename) {
    std::string result(size(filename), '\0');
    reader(filename).read(result);
    return result;
}

template <class String>
void write(char* const filename, char const* data, size_t length) {
    writer(filename).write(data, length);
}

inline
size_t size(const char* filename)
{
    // From http://stackoverflow.com/questions/5840148
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

} // tfile
