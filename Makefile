#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := smart_master

EXTRA_COMPONENT_DIRS += ./components ./main


include $(IDF_PATH)/make/project.mk

