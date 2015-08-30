//
//  COMP1927 Assignment 1 - Vlad: the memory allocator
//  allocator.c ... implementation
//
//  Created by Liam O'Connor on 18/07/12.
//  Modified by John Shepherd in August 2014, August 2015
//  Copyright (c) 2012-2015 UNSW. All rights reserved.
//

#include "allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


#define HEADER_SIZE    sizeof(struct free_list_header)
#define MAGIC_FREE     0xDEADBEEF
#define MAGIC_ALLOC    0xBEEFDEAD

typedef unsigned char byte;
typedef u_int32_t vlink_t;
typedef u_int32_t vsize_t;
typedef u_int32_t vaddr_t;

typedef struct free_list_header {
   u_int32_t magic;  // ought to contain MAGIC_FREE
   vsize_t size;     // # bytes in this block (including header)
   vlink_t next;     // memory[] index of next free block
   vlink_t prev;     // memory[] index of previous free block
} free_header_t;

// Global data

static byte *memory = NULL;   // pointer to start of allocator memory
static vaddr_t free_list_ptr; // index in memory[] of first block in free list
static vsize_t memory_size;   // number of bytes malloc'd in memory[]


// Input: size - number of bytes to make available to the allocator
// Output: none
// Precondition: Size is a power of two.
// Postcondition: `size` bytes are now available to the allocator
//
// (If the allocator is already initialised, this function does nothing,
//  even if it was initialised with different size)

void vlad_init(u_int32_t size){
   free_list_ptr = (vaddr_t)0; //set the initial header pointer
   memory=NULL;//made to equal null as to detect if malloc fails later
   //Size determining logic
   if (size==0){ //if they ask for 0
      fprintf(stderr, "vlad_init: invalid size (0)\n");
      abort();
   }
   if (((size&(size-1))!=0)&&size>512){ //& operator used to determine if size already a power of 2, if not begin rounding if the size is large enough (spec'd)
      size--; //crazy bitshifting interpretation of log laws
      size |= size >> 1;
      size |= size >> 2;
      size |= size >> 4;
      size |= size >> 8;
      size |= size >> 16;
      size++;
   } else if (size<=512){
      size=512; //if below size threshold (given)
   }
   if (size<=0){ //due to bitshifting technique 0 is counted as power of 0, this catches negatives
      fprintf(stderr, "vlad_init: invalid size (Negative value)\n");
      abort();
   }

   memory=malloc(size); //allocate memory block
   if (memory==NULL){
      fprintf(stderr, "vlad_init: insufficient memory\n");
      abort();
   }
   memory_size=size; //store amount of size
   free_header_t *Head;
   Head=(free_header_t*)memory;
   Head->magic=MAGIC_FREE;
   Head->size=size;
   Head->next=0; //note these are indexs not actual pointers
   Head->prev=0; //first element should point to itself
}


// Input: n - number of bytes requested
// Output: p - a pointer, or NULL
// Precondition: n is < size of memory available to the allocator
// Postcondition: If a region of size n or greater cannot be found, p = NULL
//                Else, p points to a location immediately after a header block
//                      for a newly-allocated region of some size >=
//                      n + header size.

void *vlad_malloc(u_int32_t n){ //need to add corruption testing, short circuiting
   if ((n==0)||(n>=memory_size)) return NULL;//attempt to get memory of invalid size
   u_int32_t offset=free_list_ptr;
   free_header_t *Head=(free_header_t*)memory+offset;
   do{
      if ((Head->magic!=MAGIC_FREE)&&(Head->magic!=MAGIC_ALLOC)){ //corruption check
         fprintf(stderr, "Memory corruption\n");
         abort();
      }
      if (Head->size>=HEADER_SIZE+n&&Head->magic==MAGIC_FREE){ //good block found
         while ((Head->size/2)>=HEADER_SIZE+n){ //while not perfect
            //div block by 2, create the div'd block
            free_header_t *HeaderDiv=Head+(Head->size/2); //unsure if right pointers here
            HeaderDiv->magic=MAGIC_FREE;
            HeaderDiv->prev=offset; //always moving to the left hence why constant
            HeaderDiv->next=Head->next;
            HeaderDiv->size=Head->size/2;

            //Insert div'd block into the free list (simplified from if statements, may take slightly more memory but less processing)
            free_header_t *HeadNext=(free_header_t*)memory+HeaderDiv->next;
            free_header_t *HeadPrev=(free_header_t*)memory+HeaderDiv->prev;
            HeadNext->prev=HeadPrev->next=((byte*)HeaderDiv-memory)/16;
            //cull primary head
            Head->size=Head->size/2;
            Head->next=offset+Head->size;
         }
         if (Head->next==offset){ //to fulfil specification requirement of there always been a free element
            return NULL;
         }
         //Perfect achieved, allocated and append out of free list
         Head->magic=MAGIC_ALLOC;
         free_header_t *HeadPrev=(free_header_t*)memory+Head->prev;
         free_header_t *HeadNext=(free_header_t*)memory+Head->next;
         HeadPrev->next=Head->next;
         HeadNext->prev=Head->prev;
         //free pointer logic, if the specification of always one element in the list was not present an extra if statement would be added to prevent inifinite loop (as did exist....)
         free_header_t *FreePoint=(free_header_t*)memory+free_list_ptr;
         while (FreePoint->magic!=MAGIC_FREE){ //ensure freepoint is pointing at a free element that is furthest left/highest
            FreePoint+=FreePoint->size;
            if ((((byte*)FreePoint-memory)/16)>=memory_size) { //iterated beyond end of block
               FreePoint=(free_header_t*)memory; //Reset to begining of memory
            }
         }
         free_list_ptr=((byte*)FreePoint-memory)/16;
         return((void*)Head+HEADER_SIZE);
      } else {
         offset=Head->next; //prepare for iteration
         Head=(free_header_t*)memory+offset; //iterate to next bloc
      }
   } while (offset!=free_list_ptr); //if the list has been looped, using post test prevents overlap calculation
   return NULL; //No possbile places to alloc so return NULL per specs
}

void vlad_merge(free_header_t *Head){ //recursive function
   //MERGE PART OF FREE FUNCTION
   free_header_t *HeadmergeNext=(free_header_t*)memory+Head->next;
   free_header_t *HeadmergePrev=(free_header_t*)memory+Head->prev;
   u_int32_t HeadOffset=((byte*)Head-memory)/16; //calculated here once to avoid multiple duplicate executions
   u_int32_t HeadmergePrevOffset=((byte*)HeadmergePrev-memory)/16; //same as above
   if ((Head->size==HeadmergeNext->size)&&((HeadOffset+Head->size)==Head->next)&&(HeadmergeNext->magic==MAGIC_FREE)&&(HeadOffset%(Head->size*2)==0)){ //first statement checks if of equal size, second checks if it is a neighbour
      Head->next=HeadmergeNext->next;//free next header, point head at headernext->next
      free_header_t *HeadmergeNextnext=(free_header_t*)memory+Head->next;//point headernext->next* at head
      HeadmergeNextnext->prev=HeadOffset; //make the header beyond the one been freed have its previous pointer point back at the now merged head
      Head->size=Head->size+HeadmergeNext->size; //actually merge sizes
      vlad_merge(Head); //call function again to merge the next head
   } else if ((Head->size==HeadmergePrev->size)&&((HeadmergePrevOffset+HeadmergePrev->size)==HeadOffset)&&(HeadmergePrev->magic==MAGIC_FREE)&&((HeadOffset%(Head->size*2)!=0)||(HeadmergePrevOffset==0))){
      HeadmergePrev->next=Head->next;//free current header, HeadPrev-> next skip to head->next
      HeadmergeNext->prev=HeadmergePrevOffset;//Headnext-> prev skip back to HeadPrev
      HeadmergePrev->size=HeadmergePrev->size+Head->size; //actually merge sizes
      Head=HeadmergePrev; //set header as the merged header for recursion and to prevent head pointing at a now non-existant head
      vlad_merge(Head); // as other if, i previously did this in a while loop but recursion is cooler
   } else {
      if (HeadOffset<free_list_ptr) free_list_ptr=HeadOffset; //set the freed head (always gets to this line even if no merge) as the new pointer, only if it is further left
      return; //no possible legal merges and no more code to execute so stop loop
   }
}


// Input: object, a pointer.
// Output: none
// Precondition: object points to a location immediately after a header block
//               within the allocator's memory.
// Postcondition: The region pointed to by object can be re-allocated by
//                vlad_malloc

void vlad_free(void *object){ //NOTE THAT OBJECT IS THE POINTER OF HEADER + N
   object-=HEADER_SIZE; //Find header pointer
   free_header_t *Head=(free_header_t*)object;//specify head
   free_header_t *HeadSearch=(free_header_t*)object;//Create search header for list iteration
   if (Head->magic!=MAGIC_ALLOC){ //check not somehow been fed already freed memory
      fprintf(stderr, "Attempt to free non-allocated memory");
      abort();
   }
   while (HeadSearch->magic!=MAGIC_FREE){ //Iterate through memory to find next closest free element
      if ((HeadSearch->magic!=MAGIC_FREE)&&(HeadSearch->magic!=MAGIC_ALLOC)){ //corruption check
         fprintf(stderr, "Memory corruption\n");
         abort();
      }
      HeadSearch+=HeadSearch->size;//iterate forward by size
      if ((((byte*)HeadSearch-memory)/16)>=memory_size) { //iterated beyond end of block
         HeadSearch=(free_header_t*)memory; //Reset to begining of memory
      }
      if (HeadSearch==Head) break; //no other free headers so stop the loop
   }
   Head->magic=MAGIC_FREE; //free the header
   Head->next=((byte*)HeadSearch-memory)/16;//link header into the free list, start with making header point to list
   Head->prev=HeadSearch->prev;
   free_header_t *HeadPrev=(free_header_t*)memory+Head->prev;
   HeadSearch->prev=HeadPrev->next=((byte*)Head-memory)/16;//now that header points to list modify list to include it
   //call vlad merge function as per spec its in another function
   vlad_merge(Head);
}


// Stop the allocator, so that it can be init'ed again:
// Precondition: allocator memory was once allocated by vlad_init()
// Postcondition: allocator is unusable until vlad_int() executed again

void vlad_end(void){
   free(memory); //end it all, global variables reset in init function
}


// Precondition: allocator has been vlad_init()'d
// Postcondition: allocator stats displayed on stdout

void vlad_stats(void){
   /*printf("Memory init at %p\n",memory);
   printf("Magic free is %u\n",MAGIC_FREE);
   printf("Magic Allocated is %u\n",MAGIC_ALLOC);
   printf("Free List pointer thing is %u\n\n",free_list_ptr);
   int offset=0;
   //lets write a loop that prints all current free blocks
   while (offset<memory_size){
      free_header_t *Head=(free_header_t*)memory+offset;
      printf("Block at offset %d\n",offset);
      printf("Block memory Location %p\n",Head);
      printf("Magic %u",Head->magic);
      if (Head->magic==MAGIC_FREE) printf(" (free)\n");
      else if (Head->magic==MAGIC_ALLOC) printf(" (alloc)\n");
      else printf("UNKNOWN\n");
      printf("Size %u\n",Head->size);
      printf("Previous index %u\n",Head->prev);
      printf("Next index %u\n\n",Head->next);
      offset+=Head->size; //move on to next block
   }
   return;*/
}


//
// All of the code below here was written by Alen Bou-Haidar, COMP1927 14s2
//

//
// Fancy allocator stats
// 2D diagram for your allocator.c ... implementation
//
// Copyright (C) 2014 Alen Bou-Haidar <alencool@gmail.com>
//
// FancyStat is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// FancyStat is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>


#include <string.h>

#define STAT_WIDTH  32
#define STAT_HEIGHT 16
#define BG_FREE      "\x1b[48;5;35m"
#define BG_ALLOC     "\x1b[48;5;39m"
#define FG_FREE      "\x1b[38;5;35m"
#define FG_ALLOC     "\x1b[38;5;39m"
#define CL_RESET     "\x1b[0m"


typedef struct point {int x, y;} point;

static point offset_to_point(int offset,  int size, int is_end);
static void fill_block(char graph[STAT_HEIGHT][STAT_WIDTH][20],
                        int offset, char * label);



// Print fancy 2D view of memory
// Note, This is limited to memory_sizes of under 16MB
void vlad_reveal(void *alpha[26])
{
    int i, j;
    vlink_t offset;
    char graph[STAT_HEIGHT][STAT_WIDTH][20];
    char free_sizes[26][32];
    char alloc_sizes[26][32];
    char label[3]; // letters for used memory, numbers for free memory
    int free_count, alloc_count, max_count;
    free_header_t * block;

	// TODO
	// REMOVE these statements when your vlad_malloc() is done
    //printf("vlad_reveal() won't work until vlad_malloc() works\n");
    //return;

    // initilise size lists
    for (i=0; i<26; i++) {
        free_sizes[i][0]= '\0';
        alloc_sizes[i][0]= '\0';
    }

    // Fill graph with free memory
    offset = 0;
    i = 1;
    free_count = 0;
    while (offset < memory_size){
        block = (free_header_t *)memory + offset; //changed this line from casting the entire addition to just memory and it made it work
        if (block->magic == MAGIC_FREE) {
            snprintf(free_sizes[free_count++], 32,
                "%d) %d bytes", i, block->size);
            snprintf(label, 3, "%d", i++);
            fill_block(graph, offset,label);
        }
        offset += block->size;
    }

    // Fill graph with allocated memory
    alloc_count = 0;
    for (i=0; i<26; i++) {
        if (alpha[i] != NULL) {
            offset = ((byte *) alpha[i] - (byte *) memory) - HEADER_SIZE;
            block = (free_header_t *)(memory + offset);
            snprintf(alloc_sizes[alloc_count++], 32,
                "%c) %d bytes", 'a' + i, block->size);
            snprintf(label, 3, "%c", 'a' + i);
            fill_block(graph, offset,label);
        }
    }

    // Print all the memory!
    for (i=0; i<STAT_HEIGHT; i++) {
        for (j=0; j<STAT_WIDTH; j++) {
            printf("%s", graph[i][j]);
        }
        printf("\n");
    }

    //Print table of sizes
    max_count = (free_count > alloc_count)? free_count: alloc_count;
    printf(FG_FREE"%-32s"CL_RESET, "Free");
    if (alloc_count > 0){
        printf(FG_ALLOC"%s\n"CL_RESET, "Allocated");
    } else {
        printf("\n");
    }
    for (i=0; i<max_count;i++) {
        printf("%-32s%s\n", free_sizes[i], alloc_sizes[i]);
    }

}

// Fill block area
static void fill_block(char graph[STAT_HEIGHT][STAT_WIDTH][20],
                        int offset, char * label)
{
    point start, end;
    free_header_t * block;
    char * color;
    char text[3];
    block = (free_header_t *)(memory + offset);
    start = offset_to_point(offset, memory_size, 0);
    end = offset_to_point(offset + block->size, memory_size, 1);
    color = (block->magic == MAGIC_FREE) ? BG_FREE: BG_ALLOC;

    int x, y;
    for (y=start.y; y < end.y; y++) {
        for (x=start.x; x < end.x; x++) {
            if (x == start.x && y == start.y) {
                // draw top left corner
                snprintf(text, 3, "|%s", label);
            } else if (x == start.x && y == end.y - 1) {
                // draw bottom left corner
                snprintf(text, 3, "|_");
            } else if (y == end.y - 1) {
                // draw bottom border
                snprintf(text, 3, "__");
            } else if (x == start.x) {
                // draw left border
                snprintf(text, 3, "| ");
            } else {
                snprintf(text, 3, "  ");
            }
            sprintf(graph[y][x], "%s%s"CL_RESET, color, text);
        }
    }
}

// Converts offset to coordinate
static point offset_to_point(int offset,  int size, int is_end)
{
    int pot[2] = {STAT_WIDTH, STAT_HEIGHT}; // potential XY
    int crd[2] = {0};                       // coordinates
    int sign = 1;                           // Adding/Subtracting
    int inY = 0;                            // which axis context
    int curr = size >> 1;                   // first bit to check
    point c;                                // final coordinate
    if (is_end) {
        offset = size - offset;
        crd[0] = STAT_WIDTH;
        crd[1] = STAT_HEIGHT;
        sign = -1;
    }
    while (curr) {
        pot[inY] >>= 1;
        if (curr & offset) {
            crd[inY] += pot[inY]*sign;
        }
        inY = !inY; // flip which axis to look at
        curr >>= 1; // shift to the right to advance
    }
    c.x = crd[0];
    c.y = crd[1];
    return c;
}
