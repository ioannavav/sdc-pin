# sdc-pin
Intel PIN implementation of QED


Explanation of the files:
- parametrized_pintool.cpp --> the main pintool I created
- 1000_long_mul_test  --> the executable of a matrix-matrix multiplication test I created as an example. You can your own test executable.


Command to execute:
For the first core X:
time taskset -c X ../../../pin -t obj-intel64/parametrized_pintool.so -interval 30 -maxcount 20000000 -- ./1000_long_mul_test > /path/to/save/the/huge/output

Then execute the same for the other cores (make sure you save the output to separate files).


Explanation of the parameters:
-interval : every how many (dynamically executed) instructions do we instrument (print values). It's similar to block size, but different since with the assembly approach the block size was # program instructions (static), while now it's dynamic.
-maxcount (optional, but controls execution time significantly) : This puts a limit to how many prints we do. It's different than the parameter limit we had at the assembly approach, since limit was the # of calls to print_register inserted, but each of these calls was executed multiple times. Here, maxcount is the absolute value of times we print. Therefore, set this to a very big number for your program to not finish prematurely. Or don't set it at all and your program will run until it terminates.
