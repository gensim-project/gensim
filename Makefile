
all :
	mkdir -p build
	+cd build && cmake .. && cd .. && make -C build
	
clean:
	rm -rf build

test: all
	make -C build test
