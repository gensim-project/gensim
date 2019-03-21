
ADD_EXECUTABLE(archc-test archc-test.cpp)
find_package(Antlr REQUIRED)

TARGET_LINK_LIBRARIES(archc-test PRIVATE gensim-grammar ${ANTLR_LIB})