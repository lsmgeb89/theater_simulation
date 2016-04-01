--src
   |
   main.cc: main entrance of program
   |
   theater.cc theater.h: implementation of class Theater
   |
   utils.h: Semaphore and logging utility
   |
   CMakeLists.txt: a CMake file for building project

movies.txt: teacher's test case

summary.pdf: summary document

design.pdf: design document

How to compile (only support on csgrads1.utdallas.edu by using Linux shell command):
1. create a build folder outside the src folder
   (eg: mkdir build_minsizerel)
2. change directory to the build folder
   (eg: cd build_minsizerel)
2. cmake 'path_to_source_root' -DCMAKE_BUILD_TYPE=MINSIZEREL
   (eg: cmake ../src -DCMAKE_BUILD_TYPE=MINSIZEREL)
3. make

How to run:
'path_to_theater_simulation' 'path_to_movies_file'
(eg: ./theater_simulation ../movies.txt)
