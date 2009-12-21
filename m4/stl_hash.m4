# We check two things: where the include file is for hash_map, and
# what namespace hash_map lives in within that include file.  We
# include AC_TRY_COMPILE for all the combinations we've seen in the
# wild.  We define one of HAVE_HASH_MAP or HAVE_EXT_HASH_MAP depending
# on location, and HASH_NAMESPACE to be the namespace hash_map is
# defined in.
#
# Ideally we'd use AC_CACHE_CHECK, but that only lets us store one value
# at a time, and we need to store two (filename and namespace).
# prints messages itself, so we have to do the message-printing ourselves
# via AC_MSG_CHECKING + AC_MSG_RESULT.  (TODO(csilvers): can we cache?)

AC_DEFUN([AC_CXX_STL_HASH],
  [AC_MSG_CHECKING(the location of hash_map)
  AC_LANG_SAVE
   AC_LANG_CPLUSPLUS
   ac_cv_cxx_hash_map_header=""
   ac_cv_cxx_hash_map_class=""
   for location in [tr1/unordered_map ext/hash_map hash_map]; do
     for namespace in [std::tr1 __gnu_cxx "" std stdext]; do
       for name in [unordered_map hash_map]; do

         if test -z "$ac_cv_cxx_hash_map_header"; then

           AC_TRY_COMPILE([#include <$location>],
                          [${namespace}::$name<int, int> t],
                          [ac_cv_cxx_hash_map_header="<$location>";
                           ac_cv_cxx_hash_namespace="$namespace";
                           ac_cv_cxx_hash_map_class="$name";])
         fi
       done
     done
   done
   ac_cv_cxx_hash_set_header=`echo "$ac_cv_cxx_hash_map_header" | sed s/map/set/`;
   ac_cv_cxx_hash_set_class=`echo "$ac_cv_cxx_hash_map_class" | sed s/map/set/`;
   if test -n "$ac_cv_cxx_hash_map_header"; then
      AC_DEFINE(HAVE_HASH_MAP, 1, [define if the compiler has hash_map])
      AC_DEFINE(HAVE_HASH_SET, 1, [define if the compiler has hash_set])
      AC_DEFINE_UNQUOTED(HASH_MAP_H,$ac_cv_cxx_hash_map_header,
                         [the location of <hash_map>])
      AC_DEFINE_UNQUOTED(HASH_SET_H,$ac_cv_cxx_hash_set_header,
                         [the location of <hash_set>])
      AC_DEFINE_UNQUOTED(HASH_MAP_CLASS,$ac_cv_cxx_hash_map_class,
                         [the name of <hash_set>])
      AC_DEFINE_UNQUOTED(HASH_SET_CLASS,$ac_cv_cxx_hash_set_class,
                         [the name of <hash_set>])
      AC_DEFINE_UNQUOTED(HASH_NAMESPACE,$ac_cv_cxx_hash_namespace,
                         [the namespace of hash_map/hash_set])
      AC_MSG_RESULT([$ac_cv_cxx_hash_map_header])
   else
      AC_MSG_RESULT()
      AC_MSG_WARN([could not find an STL hash_map])
   fi
])
