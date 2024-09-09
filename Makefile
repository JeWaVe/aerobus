### this makefile is used for development -- use cmake

clean:
	rm -f *.exe
	rm -rf documentation
	rm -rf build

doc:
	doxygen Doxyfile
	cd documentation/latex && make

tests:
	echo "RUNNING TESTS WITH GCC"
	cmake -D CMAKE_C_COMPILER=gcc -D CMAKE_CXX_COMPILER=g++ -S . -B build
	cmake --build build
	cd build && ctest
	rm -rf build
	echo "RUNNING TESTS WITH CLANG"
	cmake -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++ -S . -B build
	cmake --build build
	cd build && ctest

all: clean build run
