# Makefile to run global make.
DEPLOYMENT := CmuApp
BUILD_ROOT ?= $(subst /$(DEPLOYMENT),,$(CURDIR))

export BUILD_ROOT

default_build: all dict_install

include $(BUILD_ROOT)/mk/makefiles/deployment_makefile.mk

