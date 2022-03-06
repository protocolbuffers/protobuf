shopt -s globstar
sed -E -i -f rename.sed **/*.c **/*.cc **/*.h **/*.hpp **/*.py

# Since sed can't handle multi-line patterns:
perl -i -pe 'BEGIN{undef $/;} s/\bupb_decode\(([^,\)]+),([^,]+),([^,]+),([^,]+),([^,\)]+)\)/upb_Decode(\1, \2, \3, \4, NULL, 0, \5)/smg' **/*.c **/*.cc **/*.h **/*.hpp
#perl -i -pe 'BEGIN{undef $/;} s/\bupb_Encode\(([^,\)]+),([^,]+),([^,]+),([^,\)]+)\)/upb_Encode(\1, \2, 0, \3, \4)/smg' **/*.c **/*.cc **/*.h **/*.hpp

clang-format -i **/*.c **/*.cc **/*.h **/*.hpp
