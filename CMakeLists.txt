cmake_minimum_required(VERSION 3.8)
project(Derivator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -ggdb3 -Wall -Wextra -fsanitize=address,undefined -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra")

include_directories(include)

set(CALCULUS_SRC src/calculus/constant.cpp src/calculus/expression.cpp src/calculus/negate_op.cpp src/calculus/product.cpp
    src/calculus/differentiate_op.cpp src/calculus/variable.cpp src/calculus/call_op.cpp src/calculus/sum.cpp src/calculus/function.cpp
    src/calculus/power_op.cpp src/calculus/subst_op.cpp)

add_library(calculus STATIC ${CALCULUS_SRC})

add_executable(repl src/repl.cpp)
add_executable(tex src/tex.cpp)

target_link_libraries(repl calculus)
target_link_libraries(tex calculus)
