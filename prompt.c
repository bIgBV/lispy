#include <stdio.h>
#include <stdlib.h>

/* If we are compiling on Windows, compile these functions */

#ifdef _WIN32

#include <string.h>

/* Fake readline function */
char* readline(char* prompt){

  /* gets a prompt and prints it out waiting for input */
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);

  /* Copy the input into a buffer called cpy and add \0 (Null terminator) to the end of the string */
  char* cpy = malloc(strlen(buffer)+1);
  cpy[strlen(cpy)-1] = '\0';


  return cpy;
}

/* Otherwise include editline */

#else

#include<editline/readline.h>
#include<histedit.h>

#endif

/*Declare a static buffer for user input of maximun size 2048*/

int main(int arc, char** argv){
  
  /*Print versiona nd exit information */
  puts("Lispy Version 0.0.0.1");
  puts("Press Ctrl+c to Exit\n");

  /* Infinite loop */
  while(1){

    /* Output our prompt and get input */
    char* input = readline("lispy>");

    /* Add history to the input */
    add_history(input);

    /* Echo the entered text */
    printf("No you are a %s\n", input);

    /* Free retrieved input */
    free(input);

  }

  return 0;
}
