Developed & Designed together by: Umang (111815046), Tanmay (111815043)
Coded by: Umang (because we wanted to use LINUX(ubuntu) and only umang had that OS)

To compile & use the library, follow the instructions:
  To compile, type in the following commands in bash:
    1) gcc -c -o selrepeat.o selrepeat.c 
    2) ar rcs libselrepeat.a selrepeat.o
  
  To use the shared library in your program(assuming the name of program is udp_server.c):
    gcc -o udp_server.c {output_file_name_here} -L{PATH to the directory where the library is stored} -lselrepeat -pthread
  
  To see the demo:
  Run {server} file in one terminal, {client} in the other and type ./test.txt as arguement for the {client} program
  Content received will be written in a file "received_file.txt"


    
