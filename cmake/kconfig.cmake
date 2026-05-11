# kconfig.cmake — KConfig-style configuration system for CMake
#
# Parses Kconfig files and creates corresponding CMake cache variables +
# generates config.hpp with compile-time defines.
#
# Usage:
#   include(kconfig)
#   kconfig_parse(Kconfig)                    — parse and create cache variables
#   kconfig_generate(<output_path>)           — generate config.hpp
#   kconfig_dump()                            — print final configuration
#   kconfig_validate()                        — verify constraints

include_guard(GLOBAL)

# ── Internal state ───────────────────────────────────────────────────────────

set(_KCONFIG_OPTIONS "" CACHE INTERNAL "All parsed Kconfig options")
set(_KCONFIG_MENUS "" CACHE INTERNAL "Menu structure")

# ── Parse helpers ────────────────────────────────────────────────────────────

function(_kconfig_parse_file file_path)

    if(NOT EXISTS "${file_path}")
        message(FATAL_ERROR "kconfig: file not found: ${file_path}")
    endif()

    file(STRINGS "${file_path}" _lines)

    set(_current_menu "")
    set(_current_config "")
    set(_in_help OFF)
    set(_help_lines "")

    foreach(_line IN LISTS _lines)
        string(STRIP "${_line}" _line)

        # Skip empty lines and comments
        if(_line STREQUAL "" OR _line MATCHES "^#")
            continue()
        endif()

        # Handle help text accumulation
        if(_in_help)
            if(_line MATCHES "^endmenu$" OR _line MATCHES "^menu " OR _line MATCHES "^config ")
                # End of help block
                set(_in_help OFF)
                set_property(GLOBAL PROPERTY "KCONFIG_HELP_${_current_config}" "${_help_lines}")
                set(_help_lines "")
                # Fall through to process the current non-help line
            else()
                string(APPEND _help_lines "${_line}\n")
                continue()
            endif()
        endif()

        if(_line MATCHES "^help$")
            set(_in_help ON)
            continue()

        elseif(_line MATCHES "^menu \"(.+)\"")
            set(_current_menu "${CMAKE_MATCH_1}")
            list(APPEND _KCONFIG_MENUS "${_current_menu}")
            continue()

        elseif(_line MATCHES "^endmenu$")
            set(_current_menu "")
            continue()

        elseif(_line MATCHES "^config ([A-Za-z_][A-Za-z0-9_]*)")
            set(_current_config "${CMAKE_MATCH_1}")
            list(APPEND _KCONFIG_OPTIONS "${_current_config}")

            # Initialize defaults
            set_property(GLOBAL PROPERTY "KCONFIG_TYPE_${_current_config}" "")
            set_property(GLOBAL PROPERTY "KCONFIG_PROMPT_${_current_config}" "")
            set_property(GLOBAL PROPERTY "KCONFIG_DEFAULT_${_current_config}" "")
            set_property(GLOBAL PROPERTY "KCONFIG_DEPENDS_${_current_config}" "")
            set_property(GLOBAL PROPERTY "KCONFIG_SELECT_${_current_config}" "")
            set_property(GLOBAL PROPERTY "KCONFIG_RANGE_${_current_config}" "")
            set_property(GLOBAL PROPERTY "KCONFIG_MENU_${_current_config}" "${_current_menu}")
            continue()

        elseif(_line MATCHES "^([a-z_]+) (.+)$" AND NOT _current_config STREQUAL "")
            set(_key "${CMAKE_MATCH_1}")
            set(_value "${CMAKE_MATCH_2}")

            if(_key STREQUAL "type")
                set_property(GLOBAL PROPERTY "KCONFIG_TYPE_${_current_config}" "${_value}")

            elseif(_key STREQUAL "prompt")
                string(REGEX REPLACE "^\"(.*)\"$" "\\1" _prompt_val "${_value}")
                set_property(GLOBAL PROPERTY "KCONFIG_PROMPT_${_current_config}" "${_prompt_val}")

            elseif(_key STREQUAL "default")
                set_property(GLOBAL PROPERTY "KCONFIG_DEFAULT_${_current_config}" "${_value}")

            elseif(_key STREQUAL "depends")
                # Value is "on <condition>"
                string(REGEX REPLACE "^on " "" _dep "${_value}")
                set_property(GLOBAL PROPERTY "KCONFIG_DEPENDS_${_current_config}" "${_dep}")

            elseif(_key STREQUAL "select")
                set_property(GLOBAL PROPERTY "KCONFIG_SELECT_${_current_config}" "${_value}")

            elseif(_key STREQUAL "range")
                set_property(GLOBAL PROPERTY "KCONFIG_RANGE_${_current_config}" "${_value}")
            endif()

            continue()
        endif()
    endforeach()

    # Flush last help block if open
    if(_in_help AND NOT _current_config STREQUAL "")
        set_property(GLOBAL PROPERTY "KCONFIG_HELP_${_current_config}" "${_help_lines}")
    endif()

    set(_KCONFIG_OPTIONS "${_KCONFIG_OPTIONS}" CACHE INTERNAL "All parsed Kconfig options")
    set(_KCONFIG_MENUS "${_KCONFIG_MENUS}" CACHE INTERNAL "Menu structure")
endfunction()

# ── Cache variable creation ──────────────────────────────────────────────────

function(_kconfig_create_cache)
    foreach(_opt IN LISTS _KCONFIG_OPTIONS)
        get_property(_type GLOBAL PROPERTY "KCONFIG_TYPE_${_opt}")
        get_property(_prompt GLOBAL PROPERTY "KCONFIG_PROMPT_${_opt}")
        get_property(_default GLOBAL PROPERTY "KCONFIG_DEFAULT_${_opt}")

        if(_type STREQUAL "bool")
            option(${_opt} "${_prompt}" ${_default})
        elseif(_type STREQUAL "int")
            set(${_opt} "${_default}" CACHE STRING "${_prompt}")
        elseif(_type STREQUAL "string")
            set(${_opt} "${_default}" CACHE STRING "${_prompt}")
        endif()
    endforeach()
endfunction()

# ── Dependency resolution ────────────────────────────────────────────────────

function(_kconfig_resolve_deps)
    set(_changed ON)

    # Iterate until stable (handle transitive dependencies)
    while(_changed)
        set(_changed OFF)

        foreach(_opt IN LISTS _KCONFIG_OPTIONS)
            get_property(_dep_raw GLOBAL PROPERTY "KCONFIG_DEPENDS_${_opt}")
            get_property(_select GLOBAL PROPERTY "KCONFIG_SELECT_${_opt}")

            if(_dep_raw STREQUAL "" AND _select STREQUAL "")
                continue()
            endif()

            # Evaluate dependency condition using CMake variables
            if(NOT _dep_raw STREQUAL "")
                _kconfig_eval_condition("${_dep_raw}" _satisfied)
                if(NOT _satisfied AND ${_opt})
                    message(STATUS "kconfig: disabling ${_opt} (dependency '${_dep_raw}' not met)")
                    set(${_opt} OFF CACHE BOOL "" FORCE)
                    set(_changed ON)
                endif()
            endif()

            # Handle select (auto-enable)
            if(NOT _select STREQUAL "" AND ${_opt} AND NOT ${_select})
                message(STATUS "kconfig: auto-enabling ${_select} (selected by ${_opt})")
                set(${_select} ON CACHE BOOL "" FORCE)
                set(_changed ON)
            endif()
        endforeach()
    endwhile()
endfunction()

# ── Condition evaluator ─────────────────────────────────────────────────────

function(_kconfig_eval_condition condition result)
    # Replace NOT with CMake NOT
    string(REGEX REPLACE "NOT ([A-Za-z_][A-Za-z0-9_]*)" "NOT \\1_NEG" _cond "${condition}")

    # Create negated variables for NOT expressions
    string(REGEX MATCHALL "NOT ([A-Za-z_][A-Za-z0-9_]*)_NEG" _negs "${_cond}")
    foreach(_neg IN LISTS _negs)
        string(REGEX REPLACE "NOT ([A-Za-z_][A-Za-z0-9_]*)_NEG" "\\1" _var "${_neg}")
        if(${_var})
            set(${_var}_NEG OFF)
        else()
            set(${_var}_NEG ON)
        endif()
    endforeach()

    # Replace variable names with their values
    string(REGEX MATCHALL "[A-Za-z_][A-Za-z0-9_]*" _vars "${_cond}")
    set(_eval "${_cond}")
    foreach(_var IN LISTS _vars)
        if(${_var})
            string(REGEX REPLACE "${_var}" "ON" _eval "${_eval}")
        else()
            string(REGEX REPLACE "${_var}" "OFF" _eval "${_eval}")
        endif()
    endforeach()

    # Simple boolean logic: AND is implicit (all vars must be ON)
    if(_eval MATCHES "ON")
        set(${result} ON PARENT_SCOPE)
    else()
        set(${result} OFF PARENT_SCOPE)
    endif()
endfunction()

# ── Range validation ─────────────────────────────────────────────────────────

function(_kconfig_validate_range)
    foreach(_opt IN LISTS _KCONFIG_OPTIONS)
        get_property(_type GLOBAL PROPERTY "KCONFIG_TYPE_${_opt}")
        get_property(_range GLOBAL PROPERTY "KCONFIG_RANGE_${_opt}")

        if(NOT _type STREQUAL "int" OR _range STREQUAL "")
            continue()
        endif()

        string(REGEX MATCH "([0-9]+) ([0-9]+)" _match "${_range}")
        if(NOT _match)
            continue()
        endif()

        set(_min "${CMAKE_MATCH_1}")
        set(_max "${CMAKE_MATCH_2}")

        if(${_opt} LESS _min OR ${_opt} GREATER _max)
            message(WARNING "kconfig: ${_opt}=${${_opt}} outside range [${_min}, ${_max}] — clamping")
            if(${_opt} LESS _min)
                set(${_opt} ${_min} CACHE STRING "" FORCE)
            else()
                set(${_opt} ${_max} CACHE STRING "" FORCE)
            endif()
        endif()
    endforeach()
endfunction()

# ── config.hpp generation ────────────────────────────────────────────────────

function(_kconfig_generate_header output_path)
    set(_content "// Auto-generated by cmake/kconfig.cmake — DO NOT EDIT\n")
    set(_content "${_content}#pragma once\n\n")

    # Group by menu for readability
    set(_last_menu "")
    foreach(_opt IN LISTS _KCONFIG_OPTIONS)
        get_property(_menu GLOBAL PROPERTY "KCONFIG_MENU_${_opt}")
        get_property(_prompt GLOBAL PROPERTY "KCONFIG_PROMPT_${_opt}")

        if(NOT _menu STREQUAL _last_menu)
            string(APPEND _content "\n// ${_menu}\n")
            set(_last_menu "${_menu}")
        endif()

        # Bool → #define if ON
        get_property(_type GLOBAL PROPERTY "KCONFIG_TYPE_${_opt}")
        if(_type STREQUAL "bool")
            if(${_opt})
                string(APPEND _content "#define CONFIG_${_opt} 1\n")
            endif()
        elseif(_type STREQUAL "int")
            string(APPEND _content "#define CONFIG_${_opt} ${${_opt}}\n")
        elseif(_type STREQUAL "string")
            string(APPEND _content "#define CONFIG_${_opt} \"${${_opt}}\"\n")
        endif()
    endforeach()

    string(APPEND _content "\n")

    file(WRITE "${output_path}" "${_content}")
    message(STATUS "kconfig: generated ${output_path}")
endfunction()

# ── Public API ───────────────────────────────────────────────────────────────

function(kconfig_parse file_path)
    message(STATUS "kconfig: parsing ${file_path}")
    _kconfig_parse_file("${file_path}")
    _kconfig_create_cache()
    _kconfig_resolve_deps()
    _kconfig_validate_range()
endfunction()

function(kconfig_generate output_path)
    _kconfig_generate_header("${output_path}")
endfunction()

function(kconfig_dump)
    message(STATUS "===== Kconfig Configuration =====")

    set(_last_menu "")
    foreach(_opt IN LISTS _KCONFIG_OPTIONS)
        get_property(_menu GLOBAL PROPERTY "KCONFIG_MENU_${_opt}")
        get_property(_type GLOBAL PROPERTY "KCONFIG_TYPE_${_opt}")

        if(NOT _menu STREQUAL _last_menu)
            message(STATUS "  [${_menu}]")
            set(_last_menu "${_menu}")
        endif()

        message(STATUS "    ${_opt}=${${_opt}} (${_type})")
    endforeach()

    message(STATUS "==================================")
endfunction()

function(kconfig_validate)
    _kconfig_resolve_deps()
    _kconfig_validate_range()
    message(STATUS "kconfig: validation complete (${_KCONFIG_OPTIONS} options)")
endfunction()
