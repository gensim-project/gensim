
ADD_EXECUTABLE(genc-test genc-test.cpp)
find_package(Antlr REQUIRED)

TARGET_LINK_LIBRARIES(genc-test PRIVATE gensim-grammar ${ANTLR_LIB})