/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */

/*
* 
* malloc이란 c언어의 동적 메모리 할당이다
* 동적 메모리 할당은 heap이라는 가상 메모리 영역에 이루어진다.
* 
* 동적할당은 explicit or implicit free list가 가능한데,
* 여기서는 LAB에서 설명한 implicit free list을 이용해서 구현하였다.
* 
* implicit 리스트는 header - payload - padding - footer로 이루어져있다.
* -header는 block size + 블록의 할당 여부
* -payload는 할당된 블록에 값이 들어있는 부분
* -padding은 double word alignment을 위한 optional한 부분
* -footer는 boundary tag로, header의 값이 복사되어 있다.
* 
* 
* (1) mm_init
* : 프롤로그 블록과 에필로그 블록을 초기화하고 할당
* : double word alignment를 위해 패딩 블록을 맨 앞에 붙인다.
* : heap이 확장될 때 에필로그 블록은 확장된 힙의 마지막에 위치하도록 한다.
* 
* (2) extend_heap
* : 힙이 초기화될때, 혹은 충분한 공간을 찾지 못했을 때 호출된다
* 
* (3) mm_free
* : 블록을 할당 해제하고 반환한다
* 
* (4) coalesce
* : header, footer라는 boundary를 이용해 인접한 free block들을 병합한다.
* 
* (5) mm_malloc
* : 요청한 size만큼의 공간이 있는 block을 할당한다.
* : 보통 header와 footer의 오버헤드 공간을 위해 16바이트의 크기를 유지해야한다
* : double word alignement를 위해서는 8의 배수로 반올림한 메모리 크기를 할당해야한다.
* : 메모리 할당 크기를 조절했으면, search를 통해 적합한 블록을 찾아야 하는데 이는 find_fit함수이다
* : 찾은 블록을 배치하고 남는 공간을 분할하는 것이 place 함수이다
* 
* (6) find_fit
* : best-fit 방식을 이용해서 가장 적합한 메모리 블럭을 찾는다.
* : 이 때, bset-fit은 처음부터 모든 블록을 탐색한 뒤, 가장 적절한 블록을 선택하는 완전 탐색이다.
* 
* (7) place
* : 찾은 block을 배치한 후 여유공간이 있다면 분할한다.
* : 즉, 현재 블록의 size를 할당한 후에 남은 size가 최소 블록(header와 footer를 포함한 4워드)보다 크거나 같으면 분할한다
* : 아니라면 size 정보만 변경하면 된다.
* 
* (8) mm_realloc
* : 기존에 할당되어 있던 블록을 새로운 사이즈로 재할당한다
* : 새로 할당하려는 size가 기존 size보다 작을 때에는 기존 정보의 일부만 복사하도록 설정한다.
* */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

 /* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


#define WSIZE 4 //single-word size 4
#define DSIZE 8 //double-word size 8
#define CHUNKSIZE (1<<12) //heap을 한번 늘릴 때 필요한 사이즈 

#define MAX(x,y) ((x)>(y)? (x):(y)) // max 구현 -> 큰 값을 return
#define PACK(size, alloc) ((size)|(alloc)) // block header = size + 할당여부

#define GET(p) (*(unsigned int*)(p)) //block의 값 return
#define PUT(p, val) (*(unsigned int*)(p)=(val)) //block에 값 넣기

#define GET_SIZE(p) (GET(p) & ~0x7) //block size (하위 3비트 제외 ~00000111 -> 11111000)
#define GET_ALLOC(p) (GET(p) & 0x1) //할당 여부 (마지막 하위 1비트)

#define HDRP(bp) ((char*)(bp)-WSIZE) //블록 header 위치
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) -DSIZE) //블록 footer 위치
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE))) //현재 블록의 다음 위치로 이동
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE))) //현재 블록의 이전 위치로 이동

static char* heap_listp; //heap list

//function prototype
static void* coalesce(void* bp);
static void* extend_heap(size_t words);
static void* find_fit(size_t asize);
static void place(void* bp, size_t asize);

/*
 * mm_init - initialize the malloc package.
 */
 // mm malloc 또는 mm realloc 또는 mm free를 call하기 전에 호출하여 초기 힙 영역 할당, 초기화
 // => 비어있는 heap을 만드는 것이 목적
 //초기화 오류 시 -1, 아니라면 0
int mm_init(void)
{
    //mem_sbrk 함수를 통해 할당된 힙 영역의 바이트를 가져옴 -> 에러라면 -1이 리턴될 것
    //mem_sbrk가 -1이라면 초기화 오류 -> -1
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)-1)
        return -1;

    PUT(heap_listp, 0); //alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); //prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); //prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); //epilogue header
    heap_listp += (2 * WSIZE);

    //mm_malloc이 적당한 fit을 찾지 못했을 때도 호출된다
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}

//힙의 크기를 확장시켜주는 함수
static void* extend_heap(size_t words) {

    size_t size;
    char* bp;

    //요청한 크기를 8바이트의 배수로 반올림
    if (words % 2) {
        size = (words + 1) * WSIZE;
    }
    else {
        size = (words)*WSIZE;
    }
    //추가 힙 공간 요청
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0)); //free의 header
    PUT(FTRP(bp), PACK(size, 0)); //free의 footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //new epilogue header

    return coalesce(bp);

}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
 //할당된 block payload에 대한 pointer를 반환한다.(최소한 size bytes)
 //할당된 전체 블록은 heap 안에 있어야 하며, 겹치지 않아야 한다.
void* mm_malloc(size_t size)
{
    char* bp;
    size_t asize;
    size_t extendsize;

    if (size == 0) return NULL;

    //alignment 8바이트, header 4바이트, footer 4바이트를 확보
    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    }
    else {
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    }

    //사용 가능한 block을 find -> 있으면 배치
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    //fit한 block이 없으면 확장 및 배치
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;

}

//사용 가능한 block의 주소를 best-fit으로 찾고 return하는 함수
static void* find_fit(size_t asize) {

    char* best = NULL;
    char* bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp))> 0; bp = NEXT_BLKP(bp)) {
        //요구 조건에 맞으면 돌리기
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            //best에 아무것도 안들어갔으면 바로 bp 넣기
            if (best == NULL)
                best = bp;
            //bp의 size보다 best의 사이즈가 더 크면 bp가 best가 되도록 한다.
            if (GET_SIZE(HDRP(bp)) < GET_SIZE(HDRP(best)))
                best = bp;
        }

    }
    return best;
}

//사용 가능한 블럭을 찾으면, block을 배치하고 남는 부분을 나눈다.
static void place(void* bp, size_t asize) {


    size_t csize = GET_SIZE(HDRP(bp));

    //double word * 2보다 (사용 가능한 block 사이즈 - 요청 block size)가 크면 분할
    if ((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }
    else {
        //작으면 데이터를 담을 수 없기에 분할하지 않고 그냥 배치
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }

};

//ptr이 가르키고 있는 할당 블록을 해제한다. 반환값은 없다.
//이전에 mm malloc 혹은 mm realloc으로 할당한 값이 해제되지 않았을 때 사용
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void* ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

//병합이 가능하면 병합하기
static void* coalesce(void* bp) {

    size_t size = GET_SIZE(HDRP(bp));
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    //앞도 할당 뒤도 할당이면 합칠 필요 없다.
    if (prev_alloc && next_alloc) {
        return bp;
    }
    //다음 블록이 free라면 합친다.
    if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return bp;
    }
    //이전 블록이 free라면 합친다.
    if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        return bp;
    }
    //둘다 free라면 둘다 합친다.
    if (!prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        return bp;
    }

}
// 최소 size의 할당된 region으로의 pointer를 반환한다.
//만약 ptr이 NULL이면 mm malloc(size)과 같은 동작
//만약 size가 0이면 mm free(ptr)과 같은 동작
//만약 ptr이 not NUL이면, ptr은 앞서 불렀던 malloc과 realloc에 의해 리턴된 것이어야 한다.
//이후 ptr이 가르키는 메모리 블럭의 크기를 변경한다. -> 새로운 size와 새로운 block의 주소를 반환한다.
//주소의 새 주소는 기존 주소와 같을 수도 있고 다를 수도 있다.
//새 블록의 내용은 이전 ptr block의 내용과 동일하고, 초기화되지 않는다.
//
/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void* mm_realloc(void* ptr, size_t size)
{
    void* oldptr = ptr;
    void* newptr = mm_malloc(size);
    size_t copySize;

    //이미 NULL 포인터였으면 그냥 malloc
    if (ptr == NULL) {
        return newptr;
    }
    //size가 0이면 free하고 NULL 넘겨주기
    else if (size == 0) {
        mm_free(oldptr);
        return NULL;
    }
    else {
        //oldsize가 더 크면 줄이기
        copySize = GET_SIZE(HDRP(oldptr));
        if (size <= copySize) {
            copySize = size;
        }
        //copy
        memcpy(newptr, oldptr, copySize);
        //old free
        mm_free(oldptr);
        return newptr;

    }

}











