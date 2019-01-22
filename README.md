tfile - tiny C++11 file utilities
---------------------------

tfile is a tiny header-only library for C++11 and beyond which offers a small
number of essential features:

  * functions to read a whole file at once, write a whole file at once,
    and get the bytesize of a file;

* and Openers: zero-cost abstractions that wrap C's FILE* classic file
   handle

The functions
=================

* `std::string tfile::read(char const* filename)`
* `size_t write(char const* filename, char const* data, size_t length)`
* `size_t write(char const* filename, char const* data)`
* `size_t write(char const* filename, const std::string& s)`
* `size_t tfile::size()` returns the byte size of a file.

Examples of usage:

    auto data = tfile::read("myfile.txt");
    data += "another line\n";
    tfile::write("myfile.txt", data);
    std::cout << "myfile.txt: filesize=" << tfile::size("myfile.txt");

Openers
==================

Openers are for applications which need to keep a file open for
processing. Openers are a thin wrapper over the C file handle type FILE*,
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

There are six Openers, corresponding to the six modes in which files
can be opened:

  * `tfile::Reader`: read-only, position at start of file - mode `"r"`
  * `tfile::ReaderWriter`: read-write, position at start of file - mode `"r+"`
  * `tfile::Writer`: write-only, truncate file to empty - mode `"w"`
  * `tfile::TruncateReaderWriter`: read-write, truncate to empty - mode `"w+"`
  * `tfile::Appender`: write-only, position at end of file - mode `"a"`
  * `tfile::ReaderAppender`: read-write, position at end of file - mode `"a+"`

For the exact signatures of methods, see the file
[tfile.h](https://github.com/rec/tfile/blob/master/include/tfile/tfile.h).

For more information on file opening modes, see
http://man7.org/linux/man-pages/man3/fopen.3.html

Examples of usage:

    tfile::Reader reader("myfile2.txt");
    std::string s(10);
    auto bytes_read = reader.read(s);   // read at most 10 chars
    // reader.write("hello");  // Won't compile

    tfile::Appender("myfile2.txt").write("a new line\n");
    // tfile::Appender("myfile2.txt").read();  // won't compile

    while (true) {
        auto line = reader.readLine();
        if (line.empty())
            break;
        // ...
    }
