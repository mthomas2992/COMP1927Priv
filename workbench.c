#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(int argc, char *argv[]){
  int size = 500;
  scanf("%d",&size); //gate for efficiancy, is that efficiancy though?
  printf("size %d \n",size);
  //size--; //why did I put this here again it works fine without as far as testing goes
  printf("size after minus %d\n",size);
  size |= size >> 1; //bit shift over till there are a bunch of ones
  printf("bit shift of 1 %d\n",size);
  size |= size >> 2;
  printf("bit shift of 2 %d\n",size);
  size |= size >> 4;
  printf("bit shift of 4 %d\n",size);
  size |= size >> 8; //if at any stage the number is already reached, doesn't matter because the or is with a number with a bunch of 1s in the binary so all 1s stay
  printf("bit shift of 8 %d\n",size);
  size |= size >> 16; //stop at 16 as only a 32 bit unsigned int
  printf("bit shift of 16 %d\n",size);
  size++; //add one because binary should be 11111111 etc so one less then power of 2
  printf("size plus plus end result %d\n",size);
  return EXIT_SUCCESS;
}
