cmake_minimum_required(VERSION 3.2.2)
project(Emojicode)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -DDEBUG=1 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -fcolor-diagnostics")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
add_compile_options(-fcolor-diagnostics)
add_compile_options(-fcolor-diagnostics)
endif()

if(heapSize)
  add_definitions(-DheapSize=${heapSize})
endif()

if(defaultPackagesDirectory)
  add_definitions(-DdefaultPackagesDirectory="${defaultPackagesDirectory}")
endif()

file(GLOB_RECURSE RUNTIME "Runtime/*")
add_library(runtime STATIC ${RUNTIME})
target_compile_options(runtime PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)
target_link_libraries(runtime dl pthread)

link_directories(/usr/local/Cellar/llvm/4.0.1/lib)
file(GLOB_RECURSE EMOJICODEC_SOURCES "EmojicodeCompiler/*")
add_executable(emojicodec ${EMOJICODEC_SOURCES})
target_compile_options(emojicodec PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)
include_directories(SYSTEM /usr/local/Cellar/llvm/4.0.1/include)
target_link_libraries(emojicodec LLVMLTO LLVMPasses LLVMObjCARCOpts LLVMMIRParser LLVMSymbolize LLVMDebugInfoPDB LLVMDebugInfoDWARF LLVMCoverage LLVMTableGen LLVMOrcJIT LLVMXCoreDisassembler LLVMXCoreCodeGen LLVMXCoreDesc LLVMXCoreInfo LLVMXCoreAsmPrinter LLVMSystemZDisassembler LLVMSystemZCodeGen LLVMSystemZAsmParser LLVMSystemZDesc LLVMSystemZInfo LLVMSystemZAsmPrinter LLVMSparcDisassembler LLVMSparcCodeGen LLVMSparcAsmParser LLVMSparcDesc LLVMSparcInfo LLVMSparcAsmPrinter LLVMRISCVDesc LLVMRISCVCodeGen LLVMRISCVInfo LLVMPowerPCDisassembler LLVMPowerPCCodeGen LLVMPowerPCAsmParser LLVMPowerPCDesc LLVMPowerPCInfo LLVMPowerPCAsmPrinter LLVMNVPTXCodeGen LLVMNVPTXDesc LLVMNVPTXInfo LLVMNVPTXAsmPrinter LLVMMSP430CodeGen LLVMMSP430Desc LLVMMSP430Info LLVMMSP430AsmPrinter LLVMMipsDisassembler LLVMMipsCodeGen LLVMMipsAsmParser LLVMMipsDesc LLVMMipsInfo LLVMMipsAsmPrinter LLVMLanaiDisassembler LLVMLanaiCodeGen LLVMLanaiAsmParser LLVMLanaiDesc LLVMLanaiInstPrinter LLVMLanaiInfo LLVMHexagonDisassembler LLVMHexagonCodeGen LLVMHexagonAsmParser LLVMHexagonDesc LLVMHexagonInfo LLVMBPFDisassembler LLVMBPFCodeGen LLVMBPFDesc LLVMBPFInfo LLVMBPFAsmPrinter LLVMARMDisassembler LLVMARMCodeGen LLVMARMAsmParser LLVMARMDesc LLVMARMInfo LLVMARMAsmPrinter LLVMAMDGPUDisassembler LLVMAMDGPUCodeGen LLVMAMDGPUAsmParser LLVMAMDGPUDesc LLVMAMDGPUInfo LLVMAMDGPUAsmPrinter LLVMAMDGPUUtils LLVMAArch64Disassembler LLVMAArch64CodeGen LLVMAArch64AsmParser LLVMAArch64Desc LLVMAArch64Info LLVMAArch64AsmPrinter LLVMAArch64Utils LLVMObjectYAML LLVMLibDriver LLVMOption LLVMX86Disassembler LLVMX86AsmParser LLVMX86CodeGen LLVMGlobalISel LLVMSelectionDAG LLVMAsmPrinter LLVMDebugInfoCodeView LLVMDebugInfoMSF LLVMX86Desc LLVMMCDisassembler LLVMX86Info LLVMX86AsmPrinter LLVMX86Utils LLVMMCJIT LLVMLineEditor LLVMInterpreter LLVMExecutionEngine LLVMRuntimeDyld LLVMCodeGen LLVMTarget LLVMCoroutines LLVMipo LLVMInstrumentation LLVMVectorize LLVMScalarOpts LLVMLinker LLVMIRReader LLVMAsmParser LLVMInstCombine LLVMTransformUtils LLVMBitWriter LLVMAnalysis LLVMObject LLVMMCParser LLVMMC LLVMBitReader LLVMProfileData LLVMCore LLVMSupport LLVMDemangle curses m z)

file(GLOB EMOJICODEMIG_SOURCES "Migrator/*")
add_executable(emojicodemig ${EMOJICODEMIG_SOURCES})
target_compile_options(emojicodemig PUBLIC -Wall -Wno-unused-result -Wno-missing-braces -pedantic)

if(APPLE)
  set(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -undefined dynamic_lookup")
endif()

function(package name)
  file(GLOB FILES_SOURCES "DefaultPackages/${name}/*")
  add_library(${name} MODULE ${FILES_SOURCES})
  set_target_properties(${name} PROPERTIES PREFIX "")
  set_target_properties(${name} PROPERTIES SUFFIX ".so")
endfunction()

# package(files)
# package(sockets)
# package(allegro allegro)

# find_library(SYSTEM_ALLEGRO NAMES allegro)
# target_link_libraries(allegro ${SYSTEM_ALLEGRO} allegro_color allegro_primitives allegro_image allegro_ttf allegro_audio allegro_acodec)

add_custom_target(dist python3 ${PROJECT_SOURCE_DIR}/dist.py ${PROJECT_SOURCE_DIR})
add_custom_target(tests python3 ${PROJECT_SOURCE_DIR}/tests.py ${PROJECT_SOURCE_DIR} DEPENDS dist)
add_custom_target(magicinstall python3 ${PROJECT_SOURCE_DIR}/dist.py ${PROJECT_SOURCE_DIR} install)
