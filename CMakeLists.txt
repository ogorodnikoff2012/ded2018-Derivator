cmake_minimum_required(VERSION 3.8)
project(Derivator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb3 -Wall -Wextra -fsanitize=address,undefined -D_GLIBCXX_DEBUG")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb3 -Wall -Wextra -fsanitize=address,undefined")

include_directories(include)

add_executable(repl src/repl.cpp src/expression.cpp)

add_custom_target(preproc g++ -DPREPROC_ONLY -P -E ${CMAKE_CURRENT_SOURCE_DIR}/include/calculus_grammar.h -o grammar_preproc.h
    COMMAND view grammar_preproc.h)

add_custom_target(run rlwrap ./repl DEPENDS repl)
