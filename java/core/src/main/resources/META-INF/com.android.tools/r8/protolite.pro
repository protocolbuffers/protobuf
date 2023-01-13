# Skip runtime check for isOnAndroidDevice().
# One line to make it easy to remove with sed.
# Safe to default since proguard rules are only active on Android.
-assumevalues class com.google.protobuf.Android { static boolean ASSUME_ANDROID return true; }

# Enable protobuf-related optimizations.
# https://github.com/protocolbuffers/protobuf/issues/11252
-shrinkunusedprotofields
