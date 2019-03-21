#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
COMPONENT_ADD_INCLUDEDIRS += . 
COMPONENT_SRCDIRS += .

CFLAGS += -Wno-error=unused-value -Wno-error=format=  -Wno-error=char-subscripts