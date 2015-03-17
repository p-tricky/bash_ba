# Shell Assignment
Spring 2015, CS 224
Due Tuesday, March 24
You will implement a 'shell', a Command Line Interface
which will run commands. 
Your shell is expected to:
     a) allow arbitrary number of pipes 
     b) allow redirection (< and >) when syntactically appropriate
     c) allow 'backgrounding' using an & as the last token on the command line
     d) implement exit,  cd, and kill as builtin commands. Kill requires only a process id.  

You will push to the a3 repo i) your source code ii) a makefile which creates 
executable called MyOwnShell

You will have to push any headers and source (e.g. error.c) so that the makefile will work when we use it. 
