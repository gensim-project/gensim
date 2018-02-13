
all : 
	mkdir -p build
	cd build && cmake ..
	+make -C build

release :
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=RELEASE .. 
	+make -C build


clean :
	rm -rf build
