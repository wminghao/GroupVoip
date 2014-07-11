#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *doubleToRawString(double x) {
  // Assumes sizeof(long long) == 8.
  char *buffer = (char*)malloc(32);
  sprintf(buffer, "%llx", *(unsigned long long *)&x);  // Evil!
  return buffer;
}

double rawStringToDouble(const char *s) {
  // Assumes sizeof(long long) == 8.

  double ret;
  sscanf(s, "%llx", (unsigned long long *)&ret);  // Evil!
  return ret;
}

int main(int argc, char** argv) {
  double d = rawStringToDouble("401c000000000000");
  printf("d=%f\r\n",d);
  char* eightResult = doubleToRawString(7.0);
  printf("eightResult=%s\r\n",eightResult);
  free(eightResult);
  return 0;
}
