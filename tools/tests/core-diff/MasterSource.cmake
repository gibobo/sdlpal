# Helpers that pull upstream sources out of git at configure time, so the
# differential tests compile the reference code without vendoring a copy of it.

# Read <ref>:<path> from the repository into <out_var>.
function(pal_git_show ref path out_var)
    execute_process(
        COMMAND git -C "${PAL_REPO_ROOT}" show "${ref}:${path}"
        OUTPUT_VARIABLE content
        ERROR_VARIABLE err
        RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "core-diff: cannot read ${ref}:${path}: ${err}"
            "Fetch the upstream branch (git fetch origin master) or set PAL_MASTER_REF.")
    endif()
    set(${out_var} "${content}" PARENT_SCOPE)
endfunction()

# Write <ref>:<path> to <out_file> unchanged.
function(pal_git_export ref path out_file)
    pal_git_show("${ref}" "${path}" content)
    file(WRITE "${out_file}" "${content}")
endfunction()

# Append the definitions of the named functions found in <text_var> to <out_var>.
# A definition is a line starting with its return type (or the name itself on the
# line after the return type) whose parameter list is followed by the function
# header comment or the body; prototypes and calls never match.
function(pal_slice_functions text_var out_var)
    set(text "${${text_var}}")
    set(result "${${out_var}}")
    foreach(name IN LISTS ARGN)
        string(REGEX MATCH
            "\n[A-Za-z_][A-Za-z0-9_ \t]*[ \t\r\n*]+${name}[ \t]*\\([^;{)]*\\)[ \t\r\n]*(/\\*\\+\\+|{)"
            match "${text}")
        if(match STREQUAL "")
            message(FATAL_ERROR "core-diff: definition of ${name} not found")
        endif()
        string(FIND "${text}" "${match}" begin)
        math(EXPR begin "${begin} + 1")
        string(SUBSTRING "${text}" ${begin} -1 tail)
        string(FIND "${tail}" "\n}" end)
        if(end EQUAL -1)
            message(FATAL_ERROR "core-diff: end of ${name} not found")
        endif()
        math(EXPR end "${end} + 2")
        string(SUBSTRING "${tail}" 0 ${end} body)
        string(APPEND result "${body}\n\n")
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# Append the single line of <text_var> that starts with <prefix> to <out_var>.
function(pal_slice_line text_var prefix out_var)
    set(text "${${text_var}}")
    string(FIND "${text}" "\n${prefix}" begin)
    if(begin EQUAL -1)
        message(FATAL_ERROR "core-diff: line '${prefix}' not found")
    endif()
    math(EXPR begin "${begin} + 1")
    string(SUBSTRING "${text}" ${begin} -1 tail)
    string(FIND "${tail}" "\n" end)
    string(SUBSTRING "${tail}" 0 ${end} line)
    set(${out_var} "${${out_var}}${line}\n\n" PARENT_SCOPE)
endfunction()
