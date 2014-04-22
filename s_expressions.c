#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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


/* Create an Enumeration of possible lval types */
// Adding SYM and SEXPR as possible types
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR  };

/* Creating a new lval struct */
typedef struct lval
{
  int type;
  long num;

  // Error and symbold types declared, these contain string data
  char* err;
  char* sym;

  // Count and pointer to a list of 'lval*' declared
  int count;
  struct lval** cell;

}lval;

void lval_print(lval* v);

// These are constructors for our lval datatype.
/* Construct a pointer to a new num type lval */
lval* lval_num(long x)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* Construct a new pointer to a erorr type lval */
lval* lval_err(char* m)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

/* Contruct a new pointer to a sym type eval */
lval* lval_sym(char* s)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

/* Contruct a new pointer to a sexpr type lval */
lval* lval_sxepr (void)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

/* This function is a destructor*/
void lval_del(lval* v)
{
  switch(v->type)
  {
    /* Do nothing for number type */
    case LVAL_NUM:
      break;

    /* If err or sym, free the string data allocated on the heap*/
    case LVAL_ERR:
      free(v->err);
      break;

    case LVAL_SYM:
      free(v->sym);
      break;

   /* If the type is sexpr then delete all the elements inside, by
    * iterating over the list of lists 
    */
    case LVAL_SEXPR:
      for(int i = 0; i < v->count; i++)
      {
	lval_del(v->cell[i]);
      }
      // also freeing the memory allocated to contain pointers
      free(v->cell);
    break;
  }

  // Freeing the memory allocated to lval struct itself
  free(v);
}

/* Function to add another list to the cell in the lval structure*/
lval* lval_add(lval* v, lval* x)
{
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count - 1] = x;
  return v;
}

/* This function is used to read in a number, convert it to long and 
 * check for any errors
 */
lval* lval_read_num(mpc_ast_t* t)
{
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("Invalid number");
}

lval* lval_read(mpc_ast_t* t)
{
  if(strstr(t->tag, "number"))
    return lval_read_num(t);
  if(strstr(t->tag, "symbol"))
    return lval_sym(t->contents);

  lval* x = NULL;
  if(strcmp(t->tag, ">") == 0)
  {
    x = lval_sxepr();
  }

  if(strstr(t->tag, "sexpr"))
  {
    x = lval_sxepr();
  }
  
  for(int i = 0; i < t->children_num; i++)
  {
    if(strcmp(t->children[i]->contents, "(") == 0)
      continue;
    if(strcmp(t->children[i]->contents, ")") == 0)
      continue;
    if(strcmp(t->children[i]->contents, "{") == 0)
      continue;
    if(strcmp(t->children[i]->contents, "}") == 0) 
      continue;
    if(strcmp(t->children[i]->contents, "regex") == 0)
     continue;
    x = lval_add(x, lval_read(t->children[i])); 
  }

  return x;
}

/* Function to print lval */
// This is because we have two types of possible lval types.
void lval_expr_print(lval* v, char open, char close)
{
  putchar(open);
  for(int i = 0; i < v->count; i++)
  {
    lval_print(v->cell[i]);

    if(i != (v->count - 1))
      putchar(' ');

  }

  putchar(close);
}

void lval_print(lval* v)
{
  switch(v->type)
  {
    case LVAL_NUM:
      printf("%li", v->num);
      break;
    case LVAL_ERR:
      printf("ERROR: %s", v->err);
      break;
    case LVAL_SYM:
      printf("%s", v->sym);
      break;
    case LVAL_SEXPR:
      lval_expr_print(v, '(', ')');
      break;
  }
}

/* Adding a new line to the optput, by printing the lval followed by the new line */
void lval_println(lval* v)
{
  lval_print(v);
  putchar('\n');
}

/* This function is to evaluate the operator now that, we have a structure
 * to ensure error handling 
 
lval eval_op(lval x, char* op, lval y)
{
  // This is similar to the previous version, only difference being that 
  // we are checking whether lval is of the type error.
  // If either of the values is an error, return It
  if(x.type == LVAL_ERR)
    return x;
  if(y.type == LVAL_ERR)
    return y;

  // Otherwise compute the operation and return that
  if(strcmp(op, "+") == 0)
    return lval_num(x.num + y.num);
  if(strcmp(op, "-") == 0)
    return lval_num(x.num - y.num);
  if(strcmp(op, "*") == 0)
    return lval_num(x.num * y.num);
  if(strcmp(op, "/") == 0)
    return y.num ==0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
  if(strcmp(op, "%") == 0)
    return lval_num(pow(x.num, y.num));
}



/* This is the main recursive evaluation function. It takes in a structure which is
 * the representation of trees and checks if the tag of the structure contains a particular
 * string. If the string is 'number' then that is a leaf node and the recursion can be 
 * stopped. If the string is 'expr' then the evaluation has to continue. It uses
 * the eval_op function to return the result 
 *
 * Now that we have an error handling mechanism, we use this to ensure that we are not 
 * erronous values.
 *
 * We use a special variable called errono, wehich is used to find the type of the error
 * is ERANGE which means whether the result is too large. This is done to ensure that the
 * conversion of the number field in children struct of the tree to float is done properly.
 *

lval eval(mpc_ast_t* t)
{
  if(strstr(t->tag, "number"))
  {
    // Check whether was an error in the conversion
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  char* op = t->children[1]->contents;
  lval x = eval(t->children[2]);

  int i = 3;
  while(strstr(t->children[i]->tag, "expr"))
  {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}
*
*/


/* This is a function which returns the number of children a tree has */
int number_of_children(mpc_ast_t* t)
{
  if(t->children_num == 0)
    return 1;
  if(t->children_num == 1)
  {
    int total = 1;
    for(int i = 0; i< t->children_num; i++)
    {
      total = total + number_of_children(t->children[i]);
    }
    return total;
  }
}

/* This is a function to count the number of branches of a tree */
int number_of_branches(mpc_ast_t* t)
{
  // This the base case for our recursion. It checks if the fist has number,
  // if it does then it means that there are no more branches.
  if(strstr(t->tag, "number"))
    return 1;

  // Initializing the accumulators and the loop variables
  int total = 1;
  int i = 3;

  // Checking if the tag contains expr
  while(strstr(t->children[i]->tag, "expr"))
  {
    total  = total + number_of_branches(t->children[i]);
    i ++;
  }
  return total;
}

/*Declare a static buffer for user input of maximun size 2048*/

int main(int arc, char** argv)
{

  /* Create parsers using the mpc library */
  // Creating individual parts of the language
  
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  // Define the parsers using the following language
  mpca_lang(MPCA_LANG_DEFAULT,
      "									\
        number		: /-?[0-9]+/ ; 					\
  	symbol		: '+' | '-' | '*' | '/'	;			\
	sexpr		: '(' <expr>* ')' ;				\
	expr		: <number> | <symbol>| <sexpr> ;		\	
	lispy		: /^/ <expr>* /$/ ;				\      
      ",
      Number,Symbol, Sexpr, Expr,  Lispy);

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
      //On succes print the AST
      lval* x = lval_read(r.output);
      int total = number_of_children(r.output);
      int branches = number_of_branches(r.output);


      lval_println(x);
      lval_del(x);
      printf("This tree has %d children\n", total);
      printf("This tree has %d branches\n", branches);

      mpc_ast_print(r.output);
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
  mpc_cleanup(5, Number,Symbol,Sexpr, Expr, Lispy);

  return 0;
}
