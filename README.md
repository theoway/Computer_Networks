To compile & use the library, follow the instructions:
  To compile, type in the following commands in bash:
    gcc -c -o selrepeat.o selrepeat.c
    ar rcs libselrepeat.a selrepeat.o
  
  To use the shared library in your program(assuming the name of program is udp_server.c):
    gcc -o udp_server.c <output_file_name_here> -L<PATH to the directory where the library is stored> -lselrepeat -pthread


    
