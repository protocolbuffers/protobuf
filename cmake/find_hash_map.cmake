include(CheckCXXSourceCompiles)

function(find_hash_map)
  set(HAVE_HASH_MAP 1 PARENT_SCOPE)
  set(HAVE_HASH_SET 1 PARENT_SCOPE)
  # Search for hash_map in the following order:
  #  1. <unordered_map> ::std::unordered_map
  #  2. <tr1/unordered_map> ::std::tr1::unordered_map
  #  3. <hash_map> ::hash_map
  #  4. <hash_map>  ::stdext::hash_map
  #  5. <ext/hash_map> ::std::hash_map
  #  6. <ext/hash_map> ::__gnu_cxx::hash_map
  check_cxx_source_compiles("
    #include <unordered_map>
    int main() {  ::std::unordered_map<int, int> v; return v[0]; }
  " HAS_STD_UNORDERED_MAP)
  if (HAS_STD_UNORDERED_MAP)
    set(HASH_NAMESPACE ::std PARENT_SCOPE)
    set(HASH_MAP_H <unordered_map> PARENT_SCOPE)
    set(HASH_MAP_CLASS unordered_map PARENT_SCOPE)
    set(HASH_SET_H <unordered_set> PARENT_SCOPE)
    set(HASH_SET_CLASS unordered_set PARENT_SCOPE)
    return()
  endif (HAS_STD_UNORDERED_MAP)

  check_cxx_source_compiles("
    #include <tr1/unordered_map>
    int main() {  ::std::tr1::unordered_map<int, int> v; return v[0]; }
  " HAS_STD_TR1_UNORDERED_MAP)
  if (HAS_STD_TR1_UNORDERED_MAP)
    set(HASH_NAMESPACE ::std::tr1 PARENT_SCOPE)
    set(HASH_MAP_H <tr1/unordered_map> PARENT_SCOPE)
    set(HASH_MAP_CLASS unordered_map PARENT_SCOPE)
    set(HASH_SET_H <tr1/unordered_set> PARENT_SCOPE)
    set(HASH_SET_CLASS unordered_set PARENT_SCOPE)
    return()
  endif (HAS_STD_TR1_UNORDERED_MAP)

  check_cxx_source_compiles("
    #include <hash_map>
    int main() {  ::hash_map<int, int> v; return v[0]; }
  " HAS_HASH_MAP)
  if (HAS_HASH_MAP)
    set(HASH_NAMESPACE :: PARENT_SCOPE)
    set(HASH_MAP_H <hash_map> PARENT_SCOPE)
    set(HASH_MAP_CLASS hash_map PARENT_SCOPE)
    set(HASH_SET_H <hash_set> PARENT_SCOPE)
    set(HASH_SET_CLASS hash_set PARENT_SCOPE)
    return()
  endif (HAS_HASH_MAP)

  check_cxx_source_compiles("
    #include <hash_map>
    int main() {  ::stdext::hash_map<int, int> v; return v[0]; }
  " HAS_STDEXT_HASH_MAP)
  if (HAS_STDEXT_HASH_MAP)
    set(HASH_NAMESPACE ::stdext PARENT_SCOPE)
    set(HASH_MAP_H <hash_map> PARENT_SCOPE)
    set(HASH_MAP_CLASS hash_map PARENT_SCOPE)
    set(HASH_SET_H <hash_set> PARENT_SCOPE)
    set(HASH_SET_CLASS hash_set PARENT_SCOPE)
    return()
  endif (HAS_STDEXT_HASH_MAP)

  check_cxx_source_compiles("
    #include <ext/hash_map>
    int main() {  ::std::hash_map<int, int> v; return v[0]; }
  " HAS_STD_HASH_MAP)
  if (HAS_STD_HASH_MAP)
    set(HASH_NAMESPACE ::std PARENT_SCOPE)
    set(HASH_MAP_H <ext/hash_map> PARENT_SCOPE)
    set(HASH_MAP_CLASS hash_map PARENT_SCOPE)
    set(HASH_SET_H <ext/hash_set> PARENT_SCOPE)
    set(HASH_SET_CLASS hash_set PARENT_SCOPE)
    return()
  endif (HAS_STD_HASH_MAP)

  check_cxx_source_compiles("
    #include <ext/hash_map>
    int main() {  ::__gnu_cxx::hash_map<int, int> v; return v[0]; }
  " HAS_GNU_CXX_HASH_MAP)
  if (HAS_GNU_CXX_HASH_MAP)
    set(HASH_NAMESPACE ::gnu_cxx PARENT_SCOPE)
    set(HASH_MAP_H <ext/hash_map> PARENT_SCOPE)
    set(HASH_MAP_CLASS hash_map PARENT_SCOPE)
    set(HASH_SET_H <ext/hash_set> PARENT_SCOPE)
    set(HASH_SET_CLASS hash_set PARENT_SCOPE)
    return()
  endif (HAS_GNU_CXX_HASH_MAP)

  set(HAVE_HASH_MAP 0 PARENT_SCOPE)
  set(HAVE_HASH_SET 0 PARENT_SCOPE)
endfunction()

function(find_hash_compare)
  if (MSVC)
    check_cxx_source_compiles("
      #include ${HASH_MAP_H}
      int main() { ::std::hash_compare<int> cp; return cp(0); }
    " HAS_STD_HASH_COMPARE)
    if (HAS_STD_HASH_COMPARE)
      set(HASH_COMPARE ::std::hash_compare PARENT_SCOPE)
      return()
    endif (HAS_STD_HASH_COMPARE)

    check_cxx_source_compiles("
      #include ${HASH_MAP_H}
      int main() { ::stdext::hash_compare<int> cp; return cp(0); }
    " HAS_STDEXT_HASH_COMPARE)
    if (HAS_STDEXT_HASH_COMPARE)
      set(HASH_COMPARE ::stdext::hash_compare PARENT_SCOPE)
      return()
    endif (HAS_STDEXT_HASH_COMPARE)
  endif (MSVC)
  set(HASH_COMPARE PARENT_SCOPE)
endfunction()

find_hash_map()
find_hash_compare()
