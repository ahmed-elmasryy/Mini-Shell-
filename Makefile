## you have command.cc command.h tokenizer.cc tokenizer.h you need compile them and link
## them to one object file called  : "myshell" 
## so we should run make , that will create our object myshell ,
## ./myshell should run your application 

compiler = g++

# generate debug info and show all warnings
flags = -g -Wall -Wextra 

# link command.o and tokenizer.o to create myshell
myshell: command.o tokenizer.o
	$(compiler) $(flags) -o myshell command.o tokenizer.o 

# compile command.cc to command.o
command.o: command.cc command.h
	$(compiler) $(flags) -c command.cc 

# compile tokenizer.cc to tokenizer.o
tokenizer.o: tokenizer.cc tokenizer.h
	$(compiler) $(flags) -c tokenizer.cc 

# force remove myshell, command.o, and tokenizer.o
clean:
	rm -f myshell command.o tokenizer.o # force remove myshell, command.o, and tokenizer.o