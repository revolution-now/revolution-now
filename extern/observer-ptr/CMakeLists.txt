add_library( observer-ptr INTERFACE )
add_library( observer-ptr::observer-ptr ALIAS observer-ptr )
target_include_directories( observer-ptr INTERFACE include )
