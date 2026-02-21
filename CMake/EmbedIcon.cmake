function(embed_icon target_name icon_path)
    if(NOT EXISTS ${icon_path})
        message(WARNING "Icon NOT found at: ${icon_path}")
        return()
    endif()

    get_filename_component(icon_name ${icon_path} NAME_WE)
    set(output_cpp "${CMAKE_BINARY_DIR}/IconBinaryData.cpp")
    set(output_h "${CMAKE_BINARY_DIR}/IconBinaryData.h")

    # Read binary file
    file(READ ${icon_path} filedata HEX)
    string(LENGTH ${filedata} filelen)
    math(EXPR filelen "${filelen} / 2")

    # Generate header
    file(WRITE ${output_h} "#pragma once\n")
    file(APPEND ${output_h} "namespace IconData {\n")
    file(APPEND ${output_h} "    extern const unsigned char ${icon_name}_png[];\n")
    file(APPEND ${output_h} "    extern const int ${icon_name}_png_size;\n")
    file(APPEND ${output_h} "}\n")

    # Generate cpp with hex array
    file(WRITE ${output_cpp} "#include \"IconBinaryData.h\"\n")
    file(APPEND ${output_cpp} "namespace IconData {\n")
    file(APPEND ${output_cpp} "    const unsigned char ${icon_name}_png[] = {")

    # Convert hex to C++ format (optimized for speed/memory in CMake)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " hex_data ${filedata})
    file(APPEND ${output_cpp} "\n        ${hex_data}\n")
    file(APPEND ${output_cpp} "    };\n")
    file(APPEND ${output_cpp} "    const int ${icon_name}_png_size = ${filelen};\n")
    file(APPEND ${output_cpp} "}\n")

    target_sources(${target_name} PRIVATE ${output_cpp})
    target_include_directories(${target_name} PRIVATE ${CMAKE_BINARY_DIR})
    message(STATUS "Embedded icon: ${icon_name} (${filelen} bytes)")
endfunction()
