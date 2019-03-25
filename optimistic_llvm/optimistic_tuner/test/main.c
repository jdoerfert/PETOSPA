int bar(int *a) {
  return *a;
}
void foo(int *a) {
  *a = 1;
}

int main(int argc, const char** argv) {
  int a = 0;
  foo(&a);
  return a + bar(&a) - 1;
}
static const char *bar_OptimisticChoices = "#c11#c:1#c<2#c>2#cB1#cE13";
const char **KeepAlive_bar_main = &bar_OptimisticChoices;
static const char *foo_OptimisticChoices = "#c11#c:1#c<2#c>2#cD3#cE13";
const char **KeepAlive_foo_main = &foo_OptimisticChoices;
