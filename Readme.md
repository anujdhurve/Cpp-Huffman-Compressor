# Cpp-Huffman-Compressor
A simple and efficient command-line utility for lossless file compression and decompression. This project implements the classic Huffman coding algorithm to encode data using variable-length codes, resulting in significant file size reduction for text-based content.

Features
Lossless Compression: Reconstructs the original data perfectly.

Custom Bit-level I/O: Implements a custom BitStream class to read and write data one bit at a time, a fundamental requirement for Huffman coding.

Header-based Decoding: The compressed file includes a header with the character count and codebook, allowing for self-contained decompression.

Command-line Interface: Simple to compile and run.

How to Get Started
Prerequisites
A C++ compiler (e.g., g++, clang)

Compiling and Running
Save the code as huffman_coder.cpp.

Compile the program using your compiler. For example:

g++ huffman_coder.cpp -o huffman_coder

Place the input.txt file you want to compress in the same directory.

Run the program:

./huffman_coder

Usage
The program will:

Read the contents of input.txt to calculate character frequencies.

Generate a huffman_codes.bin file containing the compressed data and codebook.

Decompress the huffman_codes.bin file and save the output as decompressed.txt.

Technologies Used
C++

Standard Template Library (STL)