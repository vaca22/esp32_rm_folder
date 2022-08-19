set(srcs  
        sqlite3secure.c esp32.c shox96_0_2.c
    )

set(include_dirs "." "private_include")
#set(priv_include_dirs "private_include")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       PRIV_INCLUDE_DIRS "${priv_include_dirs}"
                       PRIV_REQUIRES spi_flash esp_hw_support heap)

target_compile_options(${COMPONENT_LIB} PRIVATE  -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unused-function -Wno-implicit-int -Wpointer-arith -Wno-error=unused-function -Wno-error=unused-but-set-variable -Wno-error=unused-variable -Wno-error=deprecated-declarations -Wno-unused-parameter -Wno-sign-compare -Wno-old-style-declaration -Wno-error=maybe-uninitialized -Wno-error=char-subscripts -Wno-error=unused-label -Wno-error=unused-value -DCORE_DEBUG_LEVEL=0 -DNDEBUG -DSQLITE_DEBUG)

target_compile_options(${COMPONENT_LIB} PRIVATE  "-fno-profile-arcs" "-fno-test-coverage" "-fno-stack-protector" "-ffunction-sections"  "-fdata-sections" "-fstrict-volatile-bitfields" "-mlongcalls"  "-nostdlib")

target_compile_definitions(${COMPONENT_LIB} PRIVATE -DSQLITE_HAS_CODEC -DCODEC_TYPE=CODEC_TYPE_AES256 )