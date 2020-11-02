# Makefile to run global make.
DEPLOYMENT := CubeRover
BUILD_ROOT ?= $(PWD)/..

MODULES = Top ComLogger

export BUILD_ROOT

default_build: all dict_install

include $(BUILD_ROOT)/mk/makefiles/deployment_makefile.mk
