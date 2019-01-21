#include <tfile/tfile.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

auto const testFilename = "/tmp/tfile.file1.txt";

struct FileDeleter {
    ~FileDeleter() {
        if (remove(name))
            printf("Failed to remove file %s", name);
    }

    char const* name;
};

TEST_CASE("read", "[read]") {
    FileDeleter deleter{testFilename};

    // Write a whole file.
    tfile::write(testFilename, "Hello, world");

    // Read it back and test it.
    REQUIRE(tfile::read(testFilename) == "Hello, world");
}

TEST_CASE("write", "[write]") {
    FileDeleter deleter{testFilename};

    {
        // Write a file.
        tfile::Writer writer(testFilename);
        writer.write("hello");
        writer.write(" ");
    }

    // Append five characters
    tfile::Appender(testFilename).write("world");

    // Read the file in tiny chunks.
    tfile::Reader reader(testFilename);
    std::string buffer(3, ' ');

    REQUIRE(reader.read(buffer) == 3);
    REQUIRE(buffer == "hel");
    REQUIRE(reader.read(buffer) == 3);
    REQUIRE(buffer == "lo ");
    REQUIRE(reader.read(buffer) == 3);
    REQUIRE(buffer == "wor");

    // Last chunk is incomplete.
    buffer = std::string(3, ' ');
    REQUIRE(reader.read(buffer) == 2);
    REQUIRE(buffer == "ld ");

    REQUIRE(tfile::size(testFilename) == 11);
}
