#include <gtest/gtest.h>
#include "tfile.h"

auto const testFilename = "/tmp/tfile.file1.txt";

struct FileDeleter {
    ~FileDeleter() {
        if (remove(name))
            printf("Failed to remove file %s", name);
    }

    char const* name;
};

TEST(File, Read) {
    FileDeleter deleter{testFilename};

    // Write a whole file.
    tfile::write(testFilename, "Hello, world");

    // Read it back and test it.
    EXPECT_EQ(tfile::read(testFilename), "Hello, world");
}

TEST(File, ReaderWriter) {
    FileDeleter deleter{testFilename};

    {
        // Write a file.
        auto writer = tfile::writer(testFilename);
        writer.write("hello");
        writer.write(" ");
    }

    {
        // Append to it.
        auto appender = tfile::appender(testFilename);
        appender.write("world");
    }

    {
        // Read the file in tiny chunks.
        auto reader = tfile::reader(testFilename);
        std::string buffer(3, ' ');

        EXPECT_EQ(reader.read(buffer), 3);
        EXPECT_EQ(buffer, "hel");
        EXPECT_EQ(reader.read(buffer), 3);
        EXPECT_EQ(buffer, "lo ");
        EXPECT_EQ(reader.read(buffer), 3);
        EXPECT_EQ(buffer, "wor");

        // Last chunk is incomplete.
        buffer = std::string(3, ' ');
        EXPECT_EQ(reader.read(buffer), 2);
        EXPECT_EQ(buffer, "ld ");
    }
}
