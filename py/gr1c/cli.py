"""
Command-line interface to gr1c Python wrapper package
"""
from __future__ import print_function
from __future__ import absolute_import
import sys
import argparse


def main(args=None):
    parser = argparse.ArgumentParser(prog='gr1c')
    
    if args is None:
        args = parser.parse_args()
    else:
        args = parser.parse_args(args)

    return 0
