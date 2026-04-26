# remove everything from app android directory
- Only the code that should be bootstraped with JNI by SDL belongs to this directory [android app that has main entry point](./android/app/src/main/cpp/android_client.cpp) i guess. Everything else should be withing CMake targets, not under ./android directory.  
