CMAKE=cmake

all :
	mkdir -p build
	+cd build && $(CMAKE) .. && cd .. && make -C build
	
clean:
	rm -rf build

test: all
	make -C build test
