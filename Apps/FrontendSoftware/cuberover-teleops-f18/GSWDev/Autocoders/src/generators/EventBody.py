#!/bin/env python
#===============================================================================
# NAME: EventBody
#
# DESCRIPTION:  The EventBody class is the main entry point
#				for generation of the command class header.
#
# USAGE: Nominally the EventBody.__call__ is called by using the
#		 instance name.  The instance name is the function
#        called with a suitable argument object containing
#        all needed model information to generate the code.
#
# AUTHOR: tcanham
# EMAIL:  tcanham@jpl.nasa.gov
# DATE CREATED  : Feb. 25, 2015
#
# Copyright 2013, California Institute of Technology.
# ALL RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.
#===============================================================================
#
# Python standard modules
#
import logging
#import os
#import sys
#import time
#
# Python extention modules and custom interfaces
#
from generators.visitors import AbstractVisitor
#
# Universal globals used within module go here.
# (DO NOT USE MANY!)
#
# Global logger init. below.
PRINT = logging.getLogger('output')
DEBUG = logging.getLogger('debug')
#
# Module class or classes go here.

class EventBody:
    """
    Defines the interface concrete class implementation that drives code generation.
    """
    __visitor_list = []
    __obj          = None
    
    def __init__(self):
        """
        Constructor.
        """
        self.__visitor_list = list()
        
        
    def __call__(self, args):
        """
        Main execution point.
        Calls the accept method on each visitor to generate the code.
        """
        # Note that name handling for params goes
        # here so that the visitor in accept can
        # process all.
        self.__obj = args
        for v in self.__visitor_list:
            self.accept(v)


    def accept(self, visitor):
        """
        The operation in Visitor design pattern that takes a visitor as an argument
        and calls the visitor's method that corresponds to this element.
        @raise Exception: if the given visitor is not a subclass of AbstractVisitor
        """
        # visitor should be extended from the AbstractVisitor class
        if issubclass(visitor.__class__, AbstractVisitor.AbstractVisitor):
            visitor.eventBodyVisit(self.__obj)
        else:
            DEBUG.error('EventBody.accept() - the given visitor is not a subclass of AbstractVisitor!')
            raise 'EventBody.accept() - the given visitor is not a subclass of AbstractVisitor!'


    def addVisitor(self, visitor):
        """
        Add a visitor to the list of vistors.
        @param visitor: the visitor to add, must be derived from AbstractVisitor.
	    """
        if issubclass(visitor.__class__, AbstractVisitor.AbstractVisitor):
            self.__visitor_list.append(visitor)
        else:
            DEBUG.error('EventBody.addVisitor(v) - the given visitor is not a subclass of AbstractVisitor!')
            raise 'EventBody.addVisitor(v) - the given visitor is not a subclass of AbstractVisitor!'


    def getObj(self):
        """
        Return the object to the visitor.
        """
        return self.__obj


if __name__ == '__main__':
    pass
