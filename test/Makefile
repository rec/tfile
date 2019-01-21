all: run

tfile_test: tfile_test.cpp ../include/tfile/tfile.h
	g++ -std=c++11 -I../include tfile_test.cpp -o tfile_test

run: tfile_test
	 ./tfile_test ${ARGS}
