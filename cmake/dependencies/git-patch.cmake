# In variables: patch_file, with_reset

function(patch)
    execute_process(
        COMMAND git apply ${patch_file}
        RESULT_VARIABLE ret
        ERROR_QUIET
    )
    set(ret ${ret} PARENT_SCOPE)
endfunction()

function(check_patch)
    execute_process(
        COMMAND git apply --reverse --check ${patch_file}
        RESULT_VARIABLE ret
        ERROR_QUIET
    )
    set(ret ${ret} PARENT_SCOPE)
endfunction()

# Applies the patch or checks if it has already been applied successfully previously. Will error otherwise.
function(patch_if_needed)
    patch()
    if(NOT ret EQUAL 0)
        check_patch()
    endif()
    set(ret ${ret} PARENT_SCOPE)
endfunction()

# Resets code and reapply patch, if old (potentially incompatible) patch applied
function(patch_if_needed_with_reset)
    patch_if_needed()
    if(NOT ret EQUAL 0)
        message(STATUS "Failed to patch in current state, clearing changes to reapply")
        execute_process(
            COMMAND git status --porcelain
            RESULT_VARIABLE is_changed
        )
        if(NOT is_changed EQUAL 0)
            message(WARNING "Patch inapplyable in clean state")
            set(ret 1)
        else()
            execute_process(COMMAND git reset --hard)
            patch_if_needed()
        endif()
    endif()
    set(ret ${ret} PARENT_SCOPE)
endfunction()

message(STATUS "Trying to apply patch ${patch_file}")
if(with_reset)
    patch_if_needed_with_reset()
else()
    patch_if_needed()
endif()

if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to apply patch ${patch_file}")
else()
    message(STATUS "Successfully patched with ${patch_file}")
endif()
