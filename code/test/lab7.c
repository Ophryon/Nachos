/* lab7.c
 *	Simple program to test whether the systemcall interface works.
 *
 *	Just do a add/sub/div/mul/pow  syscall that adds two values and returns the result.
 *
 */
int main()
{
  int result;

  result = Add(42, 23);
  result = Sub(12,6);
  result = Div(12,6);
  result = Pow(2,4);
  result = Mul(2,4);

  Halt();
  /* not reached */
}
