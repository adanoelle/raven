# Sanitizers.cmake — ASan, UBSan, TSan

message(STATUS "Enabling Address Sanitizer + Undefined Behavior Sanitizer")

add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address,undefined)
