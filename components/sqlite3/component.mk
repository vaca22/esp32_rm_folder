#
# Component Makefile
#
COMPONENT_PRIV_INCLUDEDIRS=private_include
CFLAGS = -std=gnu99 -g3 -fno-stack-protector -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -mlongcalls -nostdlib -Wpointer-arith -Wno-error=unused-function -Wno-error=unused-but-set-variable -Wno-error=unused-variable -Wno-error=deprecated-declarations -Wno-unused-parameter -Wno-sign-compare -Wno-old-style-declaration -DCORE_DEBUG_LEVEL=0 -DNDEBUG

