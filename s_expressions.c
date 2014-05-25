#include <math.h>
#include <stdlib.h>
#include "mpc.h"
/* Here we are checking if the operating system in windows
 * and then we are making a fake readline function to serve
 * as readline for windows
 */

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) 
{
  // This puts a prompt and waits for input from stdin
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);

  // This part allocates memory for the string cpy into which
  // copy the contents of buffer. cpy is allocated the memory
  // of buffer + 1 because we need the null terminator '\0' 
  // to signify the end of the string.

  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* This function is left empty as this behavior of storing the
 * history is default in windows
 */

void add_history(char* unused) {}

#else

// In arch the library is histedit.h not history.h

#include <editline/readline.h>
#include <histedit.h>

#endif

/* Add SYM and SEXPR as possible lval types */
// This enum contains all possible lval (lisp value) types
// that our interpretter can handle. We use an enum to 
// easily enumurate the values which makes the code easier to
// read.

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

/* This is our main type with which our program handles expressions 
 * We are declaring a new datatype using typedef.
 */

typedef struct lval 
{
  // We store types using the enum defined above.
  int type;

  // This field is used to store numbers.
  double num;
  
  /* Error and Symbol types have some string data */
  char* err;
  char* sym;
  
  /* Count and Pointer to a list of "lval*" */
  // We use lval** as it is a pointer to a list 
  // of pointers. These cointain expressions.

  int count;
  struct lval** cell;
  
} lval;

// All these are functions which return the type lval*
// which is a pointer to the struc of type lval.
// These basically act as contructors.

/* Construct a pointer to a new Number lval */ 
lval* lval_num(long x) 
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* Construct a pointer to a new Error lval */ 
lval* lval_err(char* m) 
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

/* Construct a pointer to a new Symbol lval */ 
lval* lval_sym(char* s) 
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

/* A pointer to a new empty Sexpr lval */
lval* lval_sexpr(void) 
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

/* This is a resursive function acts as the destructor for our lval type structs.
 * It checks what type of the passed lval is and frees up memory of
 * the strings err and sym if the types are LVAL_ERR and LVAL_SYM.
 * If the type is LVAL_SEXPR then it iterates over all the cells 
 * freeing up the memory and frees up the memo allocated to contain 
 * the pointers.
 */

void lval_del(lval* v) 
{

  switch (v->type) 
  {
    /* Do nothing special for number type */
    case LVAL_NUM: break;
    
    /* For Err or Sym free the string data */
    case LVAL_ERR:
      free(v->err); break;
    case LVAL_SYM:
      free(v->sym); break;
    
    /* If Sexpr then delete all elements inside */
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) 
      {
        lval_del(v->cell[i]);
      }
      /* Also free the memory allocated to contain the pointers */
      free(v->cell);
    break;
  }
  
  /* Finally free the memory allocated for the "lval" struct itself */
  free(v);
}

/* This function extracts the lval type expression at the passed 
 * position and moves the remaining pointers back and returns the 
 * extracted expression, acting like popping a value out of a stack
 * and rearranging the remaining values.
 */

lval* lval_pop(lval* v, int i) 
{
  /* Find the item at "i" */
  lval* x = v->cell[i];
  
  /* Shift the memory following the item at "i" over the top of it */
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
  
  /* Decrease the count of items in the list */
  v->count--;
  
  /* Reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

/* This is similar to lval_pop but instead of arrangin the remaining
 * expressions it deletes them.
 */

lval* lval_take(lval* v, int i) 
{
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

// This is a prototype to resolve inter-dependencies.
void lval_print(lval* v);

/* This function loops through all the cells in the passed
 * lval and prints them out with a space if it's the last element
 */

void lval_expr_print(lval* v, char open, char close) 
{
  putchar(open);
  for (int i = 0; i < v->count; i++) 
  {
    
    /* Print Value contained within */
    lval_print(v->cell[i]);
    
    /* Don't print trailing space if last element */
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

/*This function checks the type of the lval and then 
 * prints the appropriate value
 */

void lval_print(lval* v) 
{
  switch (v->type) 
  {
    case LVAL_NUM:
      printf("%f", v->num);
      break;
    case LVAL_ERR:
      printf("Error: %s", v->err);
      break;
    case LVAL_SYM:
      printf("%s", v->sym);
      break;
    case LVAL_SEXPR:
      lval_expr_print(v, '(', ')');
      break;
  }
}

/* This is the function which is called to print lval values.*/
void lval_println(lval* v) 
{
  lval_print(v);
  putchar('\n');
}

/* This function is used to evaluate expressions. It takes an pointer 
 * to an lval and a pointer to an string which represents an operator.
 * It chekcs if all the experssions in the cells of the passed lval
 * are numbers. If it finds an error then it deletes the passed lval
 * and then returns a new lval of the type LVAL_ERR by calling the 
 * lval_err() function.
 *
 * It then pops the contents of the first cell and then checks if it has 
 * and argumets and if the operator passed is "-" then performs unary 
 * negation.
 *
 * Then as long as there are elements in the cells remaining, it pops 
 * out the next element and then performs arithematic operations on 
 * the previously popped and the element popped in the while loop.
 *
 * It then deletes the expression passed to it and returns the result
 * of the in the new expression.  
 */

lval* builtin_op(lval* a, char* op) 
{
  
  /* Ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) 
  {
    if (a->cell[i]->type != LVAL_NUM) 
    {
      lval_del(a);
      return lval_err("Cannot operator on non number!");
    }
  }
  
  /* Pop the first element */
  lval* x = lval_pop(a, 0);
  
  /* If no arguments and sub then perform unary negation */
  if ((strcmp(op, "-") == 0) && a->count == 0) 
  {
    x->num = -x->num; 
  }
  
  /* While there are still elements remaining */
  while (a->count > 0) 
  {
  
    /* Pop the next element */
    lval* y = lval_pop(a, 0);
    
    /* Perform operation */
    if (strcmp(op, "+") == 0) 
    {
      x->num += y->num; 
    }
    if (strcmp(op, "-") == 0)
    {
      x->num -= y->num; 
    }
    if (strcmp(op, "*") == 0) 
    {
      x->num *= y->num; 
    }
    if (strcmp(op, "/") == 0) 
    {
      if (y->num == 0) 
      {
        lval_del(x); lval_del(y);
        x = lval_err("Division By Zero.");
        break;
      }
      x->num /= y->num;
    }
    if (strcmp(op, "%") == 0)
    {
      x->num = fmod(x->num, y->num);
    }
    if (strcmp(op, "^") == 0)
    {
      x->num = pow(x->num, y->num);
    }
    
    /* Delete element now finished with */
    lval_del(y);
  }
  
  /* Delete input expression and return result */
  lval_del(a);
  return x;
}

/* This again is a function prototype to resolve interdependencies.*/
lval* lval_eval(lval* v);

/* This function is called by lval_eval() to evaluate s-expressions. 
 * The two functions recursively call each other, this function first 
 * evaluates the last expression and recursively moves on to the First.
 * Once it gets the last expression in the cell field, it then ckecks if 
 * any of the the expressions are of the type LVAL_ERR and if they are 
 * it calls lval_take() with the value of i at that iteration and returns
 * the value that lval_take() returns. 
 *
 * If no errors are present, it then checks if it's an empty expression, 
 * if so it returns the Expression
 *
 * Then it checks if it's a single expression, and calls lval_take() and
 * returns the value.
 *
 * Lastly it checks if the first element (expression in the first cell) is
 * of the type LVAL_SYM, if it isn't it deletes the popped lval, the passed
 * lval and returns an calls lval_err() with an error message.
 *
 * It calls builtin_op() with the Symbol and the passed expression and 
 * assigns that value to a new lval called result and returns this pointer
 * to the lval result after deleting the temporary lval.
 */

lval* lval_eval_sexpr(lval* v) 
{
  
  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) 
  {
    v->cell[i] = lval_eval(v->cell[i]);
  }
  
  /* Error Checking */
  for (int i = 0; i < v->count; i++) 
  {
    if (v->cell[i]->type == LVAL_ERR) 
    {
      return lval_take(v, i); 
    }
  }
  
  /* Empty Expression */
  if (v->count == 0) 
  {
    return v; 
  }
  
  /* Single Expression */
  if (v->count == 1) 
  {
    return lval_take(v, 0); 
  }
  
  /* Ensure First Element is Symbol */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) 
  {
    lval_del(f);
    lval_del(v);
    return lval_err("S-expression Does not start with symbol.");
  }
  
  /* Call builtin with operator */
  lval* result = builtin_op(v, f->sym);
  lval_del(f);
  return result;
}

/* This function takes in an lval of type v and checks if the 
 * type is LVAL_SEXPR, if it is it passes it to the function
 * lval_eval_sexpr() and returns the other types as it is.
 */

lval* lval_eval(lval* v) 
{
  /* Evaluate Sexpressions */
  if (v->type == LVAL_SEXPR) 
  {
    return lval_eval_sexpr(v); 
  }
  /* All other lval types remain the same */
  return v;
}

/* This function is called from lval_read() to convert a number which is
 * of the string datatype into a float. It checks if there was any error 
 * during the conversion, and according to that it checks 
 */

lval* lval_read_num(mpc_ast_t* t) 
{
  errno = 0;
  double x = atof(t->contents);
  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}


/* this function adds one more additional cell, or in other words it
 * adds another pointer to the list of pointers and stores the Pointer
 * to the passed lval in it.
 */

lval* lval_add(lval* v, lval* x) 
{
  // Increasing the count 
  v->count++;
  
  // Reallocating memory with the new count size 
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);

  // assigning the last cell to the new lval type.
  v->cell[v->count-1] = x;
  return v;
}

/* This function takes in an abstract syntax tree and converts it into s-expressions
 * by ckecking the tag of the branch. It then determines the type and constructs 
 * lvals related to those tags, if the tag contains ">" or "sexpr" then it creates 
 * an empty sexpr type lval. It then loops through all the children of the baranch
 * and adds them to the sexpr by using the lval_add() function. It returns an lval 
 * of the type x which effectively contains all the inpupt that the user typed.
 */
lval* lval_read(mpc_ast_t* t) 
{
  
  /* If Symbol or Number return conversion to that type */
  if (strstr(t->tag, "number"))
  {
    return lval_read_num(t); 
  }
  if (strstr(t->tag, "symbol"))
  {
    return lval_sym(t->contents); 
  }
  
  /* If root (>) or sexpr then create empty list */
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0)
  {
    x = lval_sexpr(); 
  } 
  if (strstr(t->tag, "sexpr"))
  {
    x = lval_sexpr(); 
  }
  
  /* Fill this list with any valid expression contained within */
  for (int i = 0; i < t->children_num; i++) 
  {
    if (strcmp(t->children[i]->contents, "(") == 0)
    {
      continue; 
    }
    if (strcmp(t->children[i]->contents, ")") == 0)
    {
      continue; 
    }
    if (strcmp(t->children[i]->contents, "}") == 0) 
    {
      continue; 
    }
    if (strcmp(t->children[i]->contents, "{") == 0)
    {
      continue; 
    }
    if (strcmp(t->children[i]->tag,  "regex") == 0)
    {
      continue; 
    }
    x = lval_add(x, lval_read(t->children[i]));
  }
  
  return x;
}

int main(int argc, char** argv) 
{
  
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr  = mpc_new("sexpr");
  mpc_parser_t* Expr   = mpc_new("expr");
  mpc_parser_t* Lispy  = mpc_new("lispy");
  
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                          	    \
      number : /-?[0-9]+.[0-9]+/ ;                    	    \
      symbol : '+' | '-' | '*' | '/' | '%' | '^';   \
      sexpr  : '(' <expr>* ')' ;                    \
      expr   : <number> | <symbol> | <sexpr> ;      \
      lispy  : /^/ <expr>* /$/ ;                    \
    ",
    Number, Symbol, Sexpr, Expr, Lispy);
  
  puts("Lispy Version 0.0.0.0.5");
  puts("Press Ctrl+c to Exit\n");
  
  while (1) 
  {
  
    char* input = readline("lispy> ");
    add_history(input);
    
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) 
    {
      // We pass the ast to lval_read() which returns an lval* 
      // which is passed to lval_eval().

      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);
      
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    }
    else 
    {    
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    free(input);
    
  }
  
  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);
  
  return 0;
}

