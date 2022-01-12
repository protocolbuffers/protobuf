shopt -s globstar
sed -i -f rename.sed **/*.c **/*.cc **/*.h **/*.hpp **/*.py
clang-format -i **/*.c **/*.cc **/*.h **/*.hpp
