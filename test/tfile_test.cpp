#include <tfile/tfile.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

auto const testFilename = "/tmp/tfile.file1.txt";
auto const testFilename2 = "/tmp/tfile.file2.txt";

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

TEST_CASE("move semantics", "[move]") {
    int i = 0;

    FileDeleter d1{testFilename}, d2{testFilename2};

    tfile::write(testFilename, "");
    tfile::write(testFilename2, "");

    {
        tfile::ReaderWriter rw1(testFilename);
        tfile::ReaderWriter rw2(testFilename2);
        tfile::ReaderWriter rw3(std::move(rw1));
        rw1 = std::move(rw2);
        REQUIRE(rw2.get() == nullptr);

        rw1.write("hello, move");
        rw3.write("hello, three");
        rw1.seek(0);
        std::string s;

        REQUIRE(rw1.readLine(s) == true);
        REQUIRE(s == "hello, move");
    }

    REQUIRE(tfile::read(testFilename) == "hello, three");
    REQUIRE(tfile::read(testFilename2) == "hello, move");
}

TEST_CASE("file size gets smaller in read", "[smaller file]") {
    FileDeleter d1{testFilename};

    tfile::write(testFilename, "short");
    auto s = tfile::testableRead(testFilename, [] (const char*) { return 8; });
    REQUIRE(s == "short");
}

TEST_CASE("file size gets larger in read", "[larger file]") {
    FileDeleter d1{testFilename};

    tfile::write(testFilename, "much too long");
    auto s = tfile::testableRead(testFilename, [] (const char*) { return 8; });
    REQUIRE(s == "much too long");
}

TEST_CASE("file size gets two buffers larger in read", "[much larger file]") {
    FileDeleter d1{testFilename};

    tfile::write(testFilename, "much much much much much much much too long");
    auto s = tfile::testableRead(testFilename, [] (const char*) { return 8; });
    REQUIRE(s == "much much much much much much much too long");
}

TEST_CASE("readLine linux", "[readLine linux]") {
    FileDeleter d1{testFilename};

    tfile::write(testFilename, "line1\nl\rine2\r\nline3");

    tfile::Reader reader(testFilename);
    std::vector<std::string> lines;
    std::string line;
    while (readLineLinux(reader, line))
        lines.push_back(line);

    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == "line1");
    REQUIRE(lines[1] == "l\rine2\r");
    REQUIRE(lines[2] == "line3");
}

TEST_CASE("readLine windows", "[readLine windows]") {
    FileDeleter d1{testFilename};
    tfile::write(testFilename, "line1\nl\rine2\r\nline3");

    tfile::Reader reader(testFilename);
    std::vector<std::string> lines;
    std::string line;
    while (readLineWindows(reader, line))
        lines.push_back(line);

    REQUIRE(lines.size() == 2);
    REQUIRE(lines[0] == "line1\nl\rine2");
    REQUIRE(lines[1] == "line3");
}

TEST_CASE("line iteration", "[line iteration]") {
    FileDeleter d1{testFilename};
    tfile::writeLines(testFilename, {"hello", "world", ""});

 auto lines = tfile::Reader(testFilename).readLines();
    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == "hello");
    REQUIRE(lines[1] == "world");
    REQUIRE(lines[2] == "");
}

TEST_CASE("line iteration2", "[line iteration2]") {
    FileDeleter d1{testFilename};
    tfile::writeLines(testFilename, {"hello", "world", ""});

    std::vector<std::string> lines;
    tfile::Reader(testFilename).fillLines(std::back_inserter(lines));
    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == "hello");
    REQUIRE(lines[1] == "world");
    REQUIRE(lines[2] == "");
}

TEST_CASE("line iteration3", "[line iteration3]") {
    FileDeleter d1{testFilename};
    tfile::writeLines(testFilename, {"hello", "world", ""});

    int i = 0;
    tfile::Reader(testFilename).forEachLine([&](std::string s) {
        if (++i == 1) {
            REQUIRE(s == "hello");
        } else if (i == 2) {
            REQUIRE(s == "world");
        } else if (i == 3) {
            REQUIRE(s == "");
        } else {
            REQUIRE(false);
        }
    });
}

namespace tfile {
namespace test {

struct Base {
    int name = 0;
};

struct D1 : Base {
    D1() { name = 1; }
    int get1() const { return name; }
};

struct D2 : Base {
    D2() { name = 2; }
    int get2() const { return name; }
};

struct D3 : D2, D1 {
    D3() : D2(), D1() {}
};

}  // test
}  // tfile

TEST_CASE("multiple inheritance", "[multiple inheritance]") {
    // Test to show the diamond problem
    // https://www.reddit.com/r/cpp/comments/aiprv9/tiny_file_utilities/effhohr/
    tfile::test::D3 d3;
    REQUIRE(d3.get1() == 1);
    REQUIRE(d3.get2() == 2);
}
