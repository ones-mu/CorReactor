# This function will overwrite the standard predefined macro "__FILE__".
# "__FILE__" expands to the name of the current input file, but cmake
# input the absolute path of source file, any code using the macro 
# would expose sensitive information, such as MORDOR_THROW_EXCEPTION(x),
# so we'd better overwirte it with filename.
#这个函数的目的是为了避免绝对路径的暴露（cmake默认使用决定路径），使用这个函数后，__FILE__这个宏就是相对路径地址
#这个函数放在 add_executable 之后、target_link_libraries 之前插入调用，或放到add_library后面调用
function(force_redefine_file_macro_for_sources targetname)
    get_target_property(source_files "${targetname}" SOURCES)
    foreach(sourcefile ${source_files})
        # Get source file's current list of compile definitions.
        get_property(defs SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS)
        # Get the relative path of the source file in project directory
        get_filename_component(filepath "${sourcefile}" ABSOLUTE)
        string(REPLACE ${PROJECT_SOURCE_DIR}/ "" relpath ${filepath})
        list(APPEND defs "__FILE__=\"${relpath}\"")
        # Set the updated compile definitions on the source file.
        set_property(
            SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS ${defs}
            )
    endforeach()
endfunction()

# function(force_redefine_file_macro_for_sources targetname)
#     get_target_property(source_files "${targetname}" SOURCES)
#     foreach(sourcefile ${source_files})
#         get_filename_component(filepath "${sourcefile}" ABSOLUTE)
#         string(REPLACE "${PROJECT_SOURCE_DIR}/" "" relpath "${filepath}")
#         # 改用 target_compile_definitions 添加
#         target_compile_definitions(${targetname} PRIVATE "__FILE__=\"${relpath}\"")
#     endforeach()
# endfunction()

# add_executable(test_loggers test_logger.cpp)
# force_redefine_file_macro_for_sources(test_loggers)
# add_dependencies(test_loggers REACTORWEBSERVER)
# target_include_directories(test_loggers PUBLIC ${ABS_SRC_DIR_INCLUDE})
# target_link_libraries(test_loggers PRIVATE REACTORWEBSERVER)
get_filename_component(ABS_SRC_DIR_INCLUDE ../include ABSOLUTE)
function(add_extra_test_executable targetname src depends libs)
    add_executable(${targetname} ${src})
    add_dependencies(${targetname} ${depends})
    force_redefine_file_macro_for_sources(${targetname})
    target_include_directories(${targetname} PUBLIC ${ABS_SRC_DIR_INCLUDE})
    target_link_libraries(${targetname} PRIVATE ${libs})
endfunction()
