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
   // dummy statements to keep compiler happy
   //memory = NULL;
   free_list_ptr = (vaddr_t)0; //set the initial header pointer
   //memory_size = 0;
   // remove the above when you implement your code

   //NEED TO ADD LOGIC TO DETERMINE SIZE IF IT IS NOT A POWER OF 2^n
   memory=malloc(size); //allocate memory block
   memory_size=size; //store amount of size
   free_header_t *Head;
   Head=(free_header_t*)memory;
   Head->magic=MAGIC_FREE;
   Head->size=size;
   Head->next=0; //note these are indexs not actual pointers
   Head->prev=0;
}


// Input: n - number of bytes requested
// Output: p - a pointer, or NULL
// Precondition: n is < size of memory available to the allocator
// Postcondition: If a region of size n or greater cannot be found, p = NULL
//                Else, p points to a location immediately after a header block
//                      for a newly-allocated region of some size >=
//                      n + header size.

void *vlad_malloc(u_int32_t n){ //need to add corruption testing, short circuiting
   //First we need to iterate through the current free blocks in an efficient manner stuff that middle line big screen master race
   //note that index is not a pointer, it is an u32int
   int offset=free_list_ptr;
   int x=1;
   while (x==1){
      free_header_t *Head=(free_header_t*)memory+offset;
      if (Head->size>=HEADER_SIZE+n&&Head->magic==MAGIC_FREE){ //good block found
         while ((Head->size/2)>=HEADER_SIZE+n){ //while not perfect
            //div block by 2, create the div'd block
            free_header_t *HeaderDiv=Head+(Head->size/2); //unsure if right pointers here
            HeaderDiv->magic=MAGIC_FREE;
            HeaderDiv->prev=offset; //always moving to the left hence why constant
            HeaderDiv->next=Head->next;
            HeaderDiv->size=Head->size/2;
            if (Head->next>=HeaderDiv->size){
               free_header_t *HeadNext=(free_header_t*)memory+HeaderDiv->next;
               HeadNext->prev=HeaderDiv->size-HeadNext->prev;
            }

            //Need to make this run only on the first split
            if (((byte*)Head==memory)&&(Head->size==memory_size/2))/*&&((byte*)HeaderDiv==(memory+(memory_size/2))))*/{ //if head is at offset 0 and first cut
               Head->prev=memory_size-Head->size;
               printf("executed\n");
               printf("head size %u\n",Head->size);
               printf("Head prev calculated %u\n",Head->prev);
            }

            Head->size=Head->size/2;
            Head->next=offset+Head->size;
            if (Head->next==memory_size) Head->next=0;
            if (Head->prev==memory_size) Head->prev=0;
            if (HeaderDiv->prev==memory_size) HeaderDiv->prev=0;
            if (HeaderDiv->next==memory_size) HeaderDiv->next=0;
            printf("Head prev end of loop %u\n",Head->prev);
         }
         //now that the perfect head has been achieved
         Head->magic=MAGIC_ALLOC;
         //need to append block out of list
         printf("Head prev after loop %u\n",Head->prev);
         free_header_t *HeadPrev=(free_header_t*)memory+Head->prev;
         free_header_t *HeadNext=(free_header_t*)memory+Head->next;
         HeadPrev->next=Head->next;
         HeadNext->prev=Head->prev;
         if (HeadNext->prev==memory_size) HeadNext->prev=0;
         //return ((void*)(memory + offset + HEADER_SIZE));
         return((void*)Head+HEADER_SIZE);
      } else {
         offset=Head->next; //iterate to next block
      }
      if (offset==free_list_ptr) return NULL; //Entire list iterated with no success therefore null
   }
   return NULL; // temporarily
}


// Input: object, a pointer.
// Output: none
// Precondition: object points to a location immediately after a header block
//               within the allocator's memory.
// Postcondition: The region pointed to by object can be re-allocated by
//                vlad_malloc

void vlad_free(void *object){ //NOTE THAT OBJECT IS THE POINTER OF HEADER + N
   object-=HEADER_SIZE; //Find header pointer
   printf("Object pointer after minus %p\n",object);
   free_header_t *Head=(free_header_t*)object;//find head
   free_header_t *HeadSearch=(free_header_t*)object;
   printf("Head pointer %p\n",Head);
   printf("I got to here\n");
   Head->magic=MAGIC_FREE; //set magic
   printf("new head magic %u\n",Head->magic);
   //Find closest region
   int found=0;
   int looped=0;
   while (found==0){ //need case for when its the only free object
      printf("looping\n");
      HeadSearch+=HeadSearch->size;//iterate forward
      if (HeadSearch->magic==MAGIC_FREE){ //found a free one
         printf("HeadSearch header found at %p\n",HeadSearch);
         found=1;
      }  else if ((((byte*)HeadSearch-memory)/16)>=memory_size) { //iterated beyond end of block
         printf("iterated beyond\n");
         HeadSearch=(free_header_t*)memory;
         looped=1;
      } else if (looped==1&&(void*)HeadSearch>=(void*)Head) {//no other free blocks
         Head->next=Head->prev=((byte*)HeadSearch-memory)/16; //should be offset
         printf("No other blocks free\n");
         found=1;
         return;
      }
   }
   u_int32_t HeadOffset =((byte*)Head-memory)/16;
   Head->next=((byte*)HeadSearch-memory)/16;//should be offset
   printf("Next calculate offset %u\n",Head->next);
   Head->prev=HeadSearch->prev;
   printf("Previous obtained offset %u\n",Head->prev);
   HeadSearch->prev=HeadOffset; //should be head offset
   printf("Heads calculated offset for next prev %u\n",HeadSearch->prev);
   free_header_t *HeadPrev=(free_header_t*)memory+Head->prev; //right now this pointer overlaps as Head->prev==0
   HeadPrev->next=HeadOffset;
   printf("Recalculated heads offset for prev->next %u\n",HeadPrev->next);
   printf("end of free function %u\n",Head->next);
}


// Stop the allocator, so that it can be init'ed again:
// Precondition: allocator memory was once allocated by vlad_init()
// Postcondition: allocator is unusable until vlad_int() executed again

void vlad_end(void){
   free(memory); //end it all
}


// Precondition: allocator has been vlad_init()'d
// Postcondition: allocator stats displayed on stdout

void vlad_stats(void){
   printf("Memory init at %p\n",memory);
   printf("Magic free is %u\n",MAGIC_FREE);
   printf("Magic Allocated is %u\n\n",MAGIC_ALLOC);
   /*free_header_t *Head=(free_header_t*)memory;
   printf("Memory read initial block %u\n",Head->magic);*/
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
      //if (Head->next==0) x=0; else offset+=Head->next;
      offset+=Head->size; //move on to next block
   }
   return;
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
