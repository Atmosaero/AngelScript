
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/AngelScript/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/AngelScript/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Check to see if building the Gem Modules are supported for the current platform
if(NOT PAL_TRAIT_ANGELSCRIPT_SUPPORTED)
    return()
endif()

set(ANGELSCRIPT_LEGACY_SDK_ROOT "${gem_path}/3rdParty/AngelScript")
set(ANGELSCRIPT_DEFAULT_SDK_ROOT "${gem_path}/sdk")
set(ANGELSCRIPT_SDK_ROOT "${ANGELSCRIPT_DEFAULT_SDK_ROOT}" CACHE PATH "Path to a local AngelScript SDK root.")
if(ANGELSCRIPT_SDK_ROOT STREQUAL ANGELSCRIPT_LEGACY_SDK_ROOT AND EXISTS "${ANGELSCRIPT_DEFAULT_SDK_ROOT}/angelscript/include/angelscript.h")
    set(ANGELSCRIPT_SDK_ROOT "${ANGELSCRIPT_DEFAULT_SDK_ROOT}" CACHE PATH "Path to a local AngelScript SDK root." FORCE)
endif()

set(ANGELSCRIPT_RUNTIME_BUILD_DEPENDENCIES)
set(ANGELSCRIPT_RUNTIME_COMPILE_DEFINITIONS)
set(ANGELSCRIPT_SDK_CMAKE_DIR "${ANGELSCRIPT_SDK_ROOT}/angelscript/projects/cmake")

find_path(ANGELSCRIPT_SDK_INCLUDE_DIR
    NAMES
        angelscript.h
    PATHS
        "${ANGELSCRIPT_SDK_ROOT}/angelscript/include"
        "${ANGELSCRIPT_SDK_ROOT}/include"
    NO_DEFAULT_PATH
)

find_library(ANGELSCRIPT_SDK_LIBRARY
    NAMES
        angelscript
        angelscript64
    PATHS
        "${ANGELSCRIPT_SDK_ROOT}/lib"
        "${ANGELSCRIPT_SDK_ROOT}/lib/${PAL_PLATFORM_NAME}"
        "${ANGELSCRIPT_SDK_ROOT}/lib/windows"
        "${ANGELSCRIPT_SDK_ROOT}/lib/Windows"
        "${ANGELSCRIPT_SDK_ROOT}/build/lib"
    NO_DEFAULT_PATH
)

if(ANGELSCRIPT_SDK_INCLUDE_DIR AND EXISTS "${ANGELSCRIPT_SDK_CMAKE_DIR}/CMakeLists.txt")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    set(AS_NO_EXCEPTIONS ON CACHE BOOL "Build AngelScript without C++ exception handling." FORCE)
    add_subdirectory("${ANGELSCRIPT_SDK_CMAKE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/AngelScriptSdk")
    set_target_properties(angelscript PROPERTIES
        OUTPUT_NAME AngelScriptSDK
    )
    target_compile_definitions(angelscript
        PUBLIC
            AS_MAX_PORTABILITY
            AS_NO_EXCEPTIONS
    )
    if(MSVC)
        target_compile_options(angelscript PRIVATE
            $<$<COMPILE_LANGUAGE:C,CXX>:/W0>
            $<$<COMPILE_LANGUAGE:C,CXX>:/WX->
        )
    endif()
    set(ANGELSCRIPT_RUNTIME_BUILD_DEPENDENCIES angelscript)
    set(ANGELSCRIPT_RUNTIME_COMPILE_DEFINITIONS ANGELSCRIPT_RUNTIME_HAS_SDK)
    message(STATUS "AngelScript Gem: building SDK from '${ANGELSCRIPT_SDK_ROOT}'.")
elseif(ANGELSCRIPT_SDK_INCLUDE_DIR AND ANGELSCRIPT_SDK_LIBRARY)
    add_library(AngelScript.SDK UNKNOWN IMPORTED GLOBAL)
    set_target_properties(AngelScript.SDK PROPERTIES
        IMPORTED_LOCATION "${ANGELSCRIPT_SDK_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ANGELSCRIPT_SDK_INCLUDE_DIR}"
    )
    set(ANGELSCRIPT_RUNTIME_BUILD_DEPENDENCIES AngelScript.SDK)
    set(ANGELSCRIPT_RUNTIME_COMPILE_DEFINITIONS ANGELSCRIPT_RUNTIME_HAS_SDK)
    message(STATUS "AngelScript Gem: using SDK include '${ANGELSCRIPT_SDK_INCLUDE_DIR}' and library '${ANGELSCRIPT_SDK_LIBRARY}'.")
else()
    message(VERBOSE "AngelScript Gem: SDK not found under '${ANGELSCRIPT_SDK_ROOT}'. Runtime will build with script compilation disabled.")
endif()

# The ${gem_name}.API target declares the common interface that users of this gem should depend on in their targets
ly_add_target(
    NAME ${gem_name}.API INTERFACE
    NAMESPACE Gem
    FILES_CMAKE
        angelscript_api_files.cmake
        ${pal_dir}/angelscript_api_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
           AZ::AzCore
)

# The ${gem_name}.Private.Object target is an internal target
# It should not be used outside of this Gems CMakeLists.txt
ly_add_target(
    NAME ${gem_name}.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        angelscript_private_files.cmake
        ${pal_dir}/angelscript_private_files.cmake
    TARGET_PROPERTIES
        O3DE_PRIVATE_TARGET TRUE
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
            "${ANGELSCRIPT_SDK_ROOT}/add_on/scriptarray"
            "${ANGELSCRIPT_SDK_ROOT}/add_on/scriptstdstring"
    COMPILE_DEFINITIONS
        PRIVATE
            ${ANGELSCRIPT_RUNTIME_COMPILE_DEFINITIONS}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
        PRIVATE
            ${ANGELSCRIPT_RUNTIME_BUILD_DEPENDENCIES}
)
if(MSVC)
    target_compile_options(${gem_name}.Private.Object PRIVATE
        /wd4296
        /wd4774
    )
endif()

# Here add ${gem_name} target, it depends on the Private Object library and Public API interface
ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        angelscript_shared_files.cmake
        ${pal_dir}/angelscript_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::${gem_name}.API
        PRIVATE
            Gem::${gem_name}.Private.Object
)

# Include the gem name into the Client Module source file
# for use with the AZ_DECLARE_MODULE_CLASS macro
# This is to allow renaming of the gem to also cause
# the CreateModuleClass_Gem_<gem-name> function which
# is used to bootstrap the gem in monolithic builds to link to the new gem name
ly_add_source_properties(
SOURCES
    Source/Clients/AngelScriptModule.cpp
PROPERTY COMPILE_DEFINITIONS
    VALUES
        O3DE_GEM_NAME=${gem_name}
        O3DE_GEM_VERSION=${gem_version})

# By default, we will specify that the above target ${gem_name} would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})

# For the Client and Server variants of ${gem_name} Gem, an alias to the ${gem_name}.API target will be made
ly_create_alias(NAME ${gem_name}.Clients.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)
ly_create_alias(NAME ${gem_name}.Servers.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)
ly_create_alias(NAME ${gem_name}.Unified.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)

# Add in CMake dependencies for each gem dependency listed in this gem's gem.json file
# for the Clients, Servers, Unified gem variants
o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Clients Servers Unified)

# If we are on a host platform, we want to add the host tools targets like the ${gem_name}.Editor MODULE target
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    # The ${gem_name}.Editor.API target can be used by other gems that want to interact with the ${gem_name}.Editor module
    ly_add_target(
        NAME ${gem_name}.Editor.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            angelscript_editor_api_files.cmake
            ${pal_dir}/angelscript_editor_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
    )

    # The ${gem_name}.Editor.Private.Object target is an internal target
    # which is only to be used by this gems CMakeLists.txt and any subdirectories
    # Other gems should not use this target
    ly_add_target(
        NAME ${gem_name}.Editor.Private.Object STATIC
        NAMESPACE Gem
        FILES_CMAKE
            angelscript_editor_private_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                ${gem_name}.Private.Object
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            angelscript_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::${gem_name}.Editor.API
            PRIVATE
                Gem::${gem_name}.Editor.Private.Object
    )

    # Include the gem name into the Editor Module source file
    # for use with the AZ_DECLARE_MODULE_CLASS macro
    # This is to allow renaming of the gem to also cause
    # the CreateModuleClass_Gem_<gem-name> function which
    # is used to bootstrap the gem in monolithic builds to link to the new gem name
    ly_add_source_properties(
    SOURCES
        Source/Tools/AngelScriptEditorModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

    # By default, we will specify that the above target ${gem_name} would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME ${gem_name}.Tools NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)

    ly_add_target(
        NAME ${gem_name}.Builders GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            angelscript_builder_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AssetBuilderSDK
                Gem::${gem_name}.Editor.Private.Object
    )

    ly_add_source_properties(
    SOURCES
        Source/Tools/AngelScriptBuildersModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

    # For the Tools and Builders variants of ${gem_name} Gem, an alias to the ${gem_name}.Editor API target will be made
    ly_create_alias(NAME ${gem_name}.Tools.API NAMESPACE Gem TARGETS Gem::${gem_name}.Editor.API)
    ly_create_alias(NAME ${gem_name}.Builders.API NAMESPACE Gem TARGETS Gem::${gem_name}.Editor.API)

    # Add in CMake dependencies for each gem dependency listed in this gem's gem.json file
    # for the Tools and Builders gem variants
    o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Tools Builders)
endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for ${gem_name}.Tests
    if(PAL_TRAIT_ANGELSCRIPT_TEST_SUPPORTED)
        # We support ${gem_name}.Tests on this platform, add dependency on the Private Object target
        ly_add_target(
            NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                angelscript_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
                    Include
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::${gem_name}.Private.Object
        )

        # Add ${gem_name}.Tests to googletest
        ly_add_googletest(
            NAME Gem::${gem_name}.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_ANGELSCRIPT_EDITOR_TEST_SUPPORTED)
            # We support ${gem_name}.Editor.Tests on this platform, add ${gem_name}.Editor.Tests target which depends on
            # private ${gem_name}.Editor.Private.Object target
            ly_add_target(
                NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    angelscript_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                        Include
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::${gem_name}.Editor.Private.Object
            )

            # Add ${gem_name}.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::${gem_name}.Editor.Tests
            )
        endif()
    endif()
endif()
