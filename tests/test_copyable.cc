#include "../convey/copyable.h"
#include "../convey/noncopyable.h"

class A : public copyable
{
};

class B : public noncopyable
{
};

int main()
{
  A a;
  B b;
  A x = a;
  A y(a);
  (void) a;
  (void) b;
  (void) x;
  (void) y;
  // B z = b;  // Call to implicitly-deleted copy constructor of 'B'
  return 0;
}
