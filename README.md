tfile - a tiny generic C++11 file class.

* Automatically closes a file handle when it leaves scope.

* Attempting to read or write a file that is fopened in the wrong mode is caught
  at compile time.

* Implemented in one small header.
