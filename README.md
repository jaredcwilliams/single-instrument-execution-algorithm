# single-instrument-execution-algorithm 
Built CSV parsers that replay rows of market data on a wall-time-relative schedule to a server application by TCP, using chrono and POSIX libraries in C++.

(Work In Progress) - Concurrent listening to two ports, maintain rolling VWAP over specified time window, command line interface

Requirements: C++23

Test by running server executable then md_client executable
