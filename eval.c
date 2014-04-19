#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

/* If we are compiling on Windows, compile these functions */

#ifdef _WIN32

#include <string.h>

/* Fake readline function */
char* readline(char* prompt)
{

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

/* Evaluates the operation by comparing the operator string with given operators */
long eval_op(long x, char* op, long y)
{
  if(strcmp(op, "+") == 0)
    return x + y;
  if(strcmp(op, "-") == 0)
    return x - y;
  if(strcmp(op, "*") == 0)
    return x * y;
  if(strcmp(op, "/") == 0)
    return x / y;
  return 0;  
}

/* This is the main recursive evaluation function. It takes in a structure which is
 * the representation of trees and checks if the tag of the structure contains a particular
 * string. If the string is 'number' then that is a leaf node and the recursion can be 
 * stopped. If the string is 'expression' then the evaluation has to continue. It uses
 * the eval_op function to return the result 
 */

long eval(mpc_ast_t* t)
{
  /* If the tag is number then it is immediately returned */
  if(strstr(t->tag, "number"))
  {
    return atoi(t->contents);
  }

  /* Otherwise continue */

  // The Operator is always the second child
  char* op = t->children[1]->contents;

  // We store the third child in the variable 'x' by recursively going through 
  // the tree until we get a number
  long x = eval(t->children[2]);
 

  // Iterate over the remaining children, combining them using our Operator
  int i = 3;
  while(strstr(t->children[i]->tag, "expression"))
  {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}



/*Declare a static buffer for user input of maximun size 2048*/

int main(int arc, char** argv)
{

  /* Create parsers using the mpc library */
  // Creating individual parts of the language
  
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expression = mpc_new("expression");
  mpc_parser_t* Lispy = mpc_new("lispy");

  // Define the parsers using the following language
  mpca_lang(MPCA_LANG_DEFAULT,
      "									\
      	number		: /-?[0-9]+/	;				\
	operator	: '+' | '-' | '*' | '/'	;			\
	expression	: <number> | '(' <operator> <expression>+ ')' ;	\
	lispy		: /^/ <operator> <expression>+ /$/ ;		\      
      ",
      Number, Operator, Expression, Lispy);

  /*Print versiona nd exit information */
  puts("Lispy Version 0.0.0.2");
  puts("Press Ctrl+c to Exit\n");

  /* Infinite loop */
  while(1)
  {

    /* Output our prompt and get input */
    char* input = readline("lispy>");

    /* Add history to the input */
    add_history(input);

    /* Attempt to parse the user input*/
    mpc_result_t r;

    if(mpc_parse("<stdin>", input, Lispy, &r))
    {
      // On succes print the AST

      long result = eval(r.output);
      printf("%li\n", result);
      mpc_ast_delete(r.output);
    }
    else
    {
      // Print error
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    /* Free retrieved input */
    free(input);

  }

  /* Undefine and delete the parsers */
  mpc_cleanup(4, Number, Operator, Expression, Lispy);

  return 0;
}
