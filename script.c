#include <stdio.h>

int main(){
  unsigned int ip = 0;
  
  while(1){
    if(ip >= (unsigned int) 0xFFFF)
      break;
    printf("%04X %02X 1 3\n",ip, 57843);
    ip++;
  }
  return 0;
}
