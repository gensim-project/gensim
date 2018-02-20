CMAKE?=cmake
CMAKE_BUILD_TYPE?=DEBUG
TESTING_ENABLED=FALSE

all :
	mkdir -p build
	+cd build && $(CMAKE) -DTESTING_ENABLED=$(TESTING_ENABLED) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) .. && cd .. && make -C build
	
clean:
	rm -rf build

test: all
	make -C build test
