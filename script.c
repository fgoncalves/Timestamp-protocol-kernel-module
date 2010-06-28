#include <stdio.h>
#include <unistd.h>

int main(){
  unsigned int i = 0;

  while(i < 65507){
    write(1,"a",1);
    i++;
  }
  return 0;
}
