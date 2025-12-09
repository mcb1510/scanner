# CS 552 - Homework 5: Driversity

**Author:** Jim Buffenbarger, Miguel Carrasco Belmar 
**Date:** December 09, 2025  
**Course:** CS 552  
**Assignment:** HW5

## Overview
This project is an implementation of a Linux module that implements a device driver. This assignment asks you to develop a Linux module that implements a device
driver. The driver will provide a software abstraction called a scanner. A scanner helps an application split a sequence of characters into a sequence of tokens,
based on the positions of separators. For this project, a separator is a single character from a set of characters (e.g., space, tab, newline, and colon).

## Files Included

- `scanner.c` - Implementation of a character device that scans input data into tokens based on configurable separators.
- `TryScanner` - Header file with program interface hw1

## How to Run
make
./TryScanner.c 


## Resources
-Starter code provided by Professor Jim Buffenbarger.
-Operating Systems: Three Easy Pieces book
-GitHub Copilot
-Chat GPT5