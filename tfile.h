#pragma once
#include <stdio.h>

#include <fstream>
#include <string>
#include <type_traits>

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

/**
   These next functions open a Handler corresponding to the different file
   opening modes from fopen - see man fopen(3) for more details.
 */
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
    Handle(char const* name, char const* mode) : file_(fopen(name, mode)) {
    }

    ~Handle() {
        fclose(file_);
    }

    bool error() const {
        return not file_;
    }

    explicit operator bool() const {
        return file_;
    }

    Handle(Handle&&) = default;
    Handle(Handle const&) = delete;

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
void write(char const* filename, String const& s) {
    writer(filename).write(s);
}

inline
void write(char const* filename, char const* data, size_t length) {
    writer(filename).write(data, length);
}

inline
void write(char const* filename, char const* data) {
    writer(filename).write(data);
}

inline
size_t size(const char* filename)
{
    // From http://stackoverflow.com/questions/5840148
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

} // tfile
