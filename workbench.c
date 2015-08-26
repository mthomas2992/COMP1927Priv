#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct Node {
   int value;
   struct Node *next;
} Node;
typedef Node *List;
//not d because prints [0] when empty list
/*void putListD(List L) {
    Node *cur;
    if (L == NULL) {
        printf("[]");
    }
    else {
        printf("[");
        for (cur = L; cur->next != NULL; cur = cur->next) {
            printf("%d,",cur->value);
        }
        printf("%d]",cur->value);
    }
}*/
//C segment faults
/*void putListC(List L) {
    Node *cur;
    printf("[");
    for (cur = L; cur != NULL; cur = cur->next) {
        printf("%d",cur->value);
        if (cur->next == NULL) printf(",");
    }
    printf("%d]",cur->value);
}*/
//prints [0] for null
/*void putListB(List L) {
    Node *cur;
    printf("[");
    for (cur = L; cur->next != NULL; cur = cur->next) {
        printf("%d,",cur->value);
    }
    printf("%d]",cur->value);
}*/
//A INCORRECT
/*void putListA(List L) {
    Node *cur;
    printf("[");
    for (cur = L; cur != NULL; cur = cur->next) {
        printf("%d,",cur->value);
    }
    printf("]");
}*/

int main(int argc, char *argv[]){
  List a=malloc(sizeof(List));
  /*a->value=1;
  a->next=malloc(sizeof(Node));
  a->next->value=2;*/
  putListA(a);
  a->value=1;
  a->next=malloc(sizeof(Node));
  a->next->value=2;
  putListA(a);
  return EXIT_SUCCESS;
}
