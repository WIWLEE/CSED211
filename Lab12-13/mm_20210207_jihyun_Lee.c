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
* malloc�̶� c����� ���� �޸� �Ҵ��̴�
* ���� �޸� �Ҵ��� heap�̶�� ���� �޸� ������ �̷������.
* 
* �����Ҵ��� explicit or implicit free list�� �����ѵ�,
* ���⼭�� LAB���� ������ implicit free list�� �̿��ؼ� �����Ͽ���.
* 
* implicit ����Ʈ�� header - payload - padding - footer�� �̷�����ִ�.
* -header�� block size + ����� �Ҵ� ����
* -payload�� �Ҵ�� ��Ͽ� ���� ����ִ� �κ�
* -padding�� double word alignment�� ���� optional�� �κ�
* -footer�� boundary tag��, header�� ���� ����Ǿ� �ִ�.
* 
* 
* (1) mm_init
* : ���ѷα� ��ϰ� ���ʷα� ����� �ʱ�ȭ�ϰ� �Ҵ�
* : double word alignment�� ���� �е� ����� �� �տ� ���δ�.
* : heap�� Ȯ��� �� ���ʷα� ����� Ȯ��� ���� �������� ��ġ�ϵ��� �Ѵ�.
* 
* (2) extend_heap
* : ���� �ʱ�ȭ�ɶ�, Ȥ�� ����� ������ ã�� ������ �� ȣ��ȴ�
* 
* (3) mm_free
* : ����� �Ҵ� �����ϰ� ��ȯ�Ѵ�
* 
* (4) coalesce
* : header, footer��� boundary�� �̿��� ������ free block���� �����Ѵ�.
* 
* (5) mm_malloc
* : ��û�� size��ŭ�� ������ �ִ� block�� �Ҵ��Ѵ�.
* : ���� header�� footer�� ������� ������ ���� 16����Ʈ�� ũ�⸦ �����ؾ��Ѵ�
* : double word alignement�� ���ؼ��� 8�� ����� �ݿø��� �޸� ũ�⸦ �Ҵ��ؾ��Ѵ�.
* : �޸� �Ҵ� ũ�⸦ ����������, search�� ���� ������ ����� ã�ƾ� �ϴµ� �̴� find_fit�Լ��̴�
* : ã�� ����� ��ġ�ϰ� ���� ������ �����ϴ� ���� place �Լ��̴�
* 
* (6) find_fit
* : best-fit ����� �̿��ؼ� ���� ������ �޸� ���� ã�´�.
* : �� ��, bset-fit�� ó������ ��� ����� Ž���� ��, ���� ������ ����� �����ϴ� ���� Ž���̴�.
* 
* (7) place
* : ã�� block�� ��ġ�� �� ���������� �ִٸ� �����Ѵ�.
* : ��, ���� ����� size�� �Ҵ��� �Ŀ� ���� size�� �ּ� ���(header�� footer�� ������ 4����)���� ũ�ų� ������ �����Ѵ�
* : �ƴ϶�� size ������ �����ϸ� �ȴ�.
* 
* (8) mm_realloc
* : ������ �Ҵ�Ǿ� �ִ� ����� ���ο� ������� ���Ҵ��Ѵ�
* : ���� �Ҵ��Ϸ��� size�� ���� size���� ���� ������ ���� ������ �Ϻθ� �����ϵ��� �����Ѵ�.
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
#define CHUNKSIZE (1<<12) //heap�� �ѹ� �ø� �� �ʿ��� ������ 

#define MAX(x,y) ((x)>(y)? (x):(y)) // max ���� -> ū ���� return
#define PACK(size, alloc) ((size)|(alloc)) // block header = size + �Ҵ翩��

#define GET(p) (*(unsigned int*)(p)) //block�� �� return
#define PUT(p, val) (*(unsigned int*)(p)=(val)) //block�� �� �ֱ�

#define GET_SIZE(p) (GET(p) & ~0x7) //block size (���� 3��Ʈ ���� ~00000111 -> 11111000)
#define GET_ALLOC(p) (GET(p) & 0x1) //�Ҵ� ���� (������ ���� 1��Ʈ)

#define HDRP(bp) ((char*)(bp)-WSIZE) //��� header ��ġ
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) -DSIZE) //��� footer ��ġ
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE))) //���� ����� ���� ��ġ�� �̵�
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE))) //���� ����� ���� ��ġ�� �̵�

static char* heap_listp; //heap list

//function prototype
static void* coalesce(void* bp);
static void* extend_heap(size_t words);
static void* find_fit(size_t asize);
static void place(void* bp, size_t asize);

/*
 * mm_init - initialize the malloc package.
 */
 // mm malloc �Ǵ� mm realloc �Ǵ� mm free�� call�ϱ� ���� ȣ���Ͽ� �ʱ� �� ���� �Ҵ�, �ʱ�ȭ
 // => ����ִ� heap�� ����� ���� ����
 //�ʱ�ȭ ���� �� -1, �ƴ϶�� 0
int mm_init(void)
{
    //mem_sbrk �Լ��� ���� �Ҵ�� �� ������ ����Ʈ�� ������ -> ������� -1�� ���ϵ� ��
    //mem_sbrk�� -1�̶�� �ʱ�ȭ ���� -> -1
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)-1)
        return -1;

    PUT(heap_listp, 0); //alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); //prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); //prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); //epilogue header
    heap_listp += (2 * WSIZE);

    //mm_malloc�� ������ fit�� ã�� ������ ���� ȣ��ȴ�
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}

//���� ũ�⸦ Ȯ������ִ� �Լ�
static void* extend_heap(size_t words) {

    size_t size;
    char* bp;

    //��û�� ũ�⸦ 8����Ʈ�� ����� �ݿø�
    if (words % 2) {
        size = (words + 1) * WSIZE;
    }
    else {
        size = (words)*WSIZE;
    }
    //�߰� �� ���� ��û
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0)); //free�� header
    PUT(FTRP(bp), PACK(size, 0)); //free�� footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //new epilogue header

    return coalesce(bp);

}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
 //�Ҵ�� block payload�� ���� pointer�� ��ȯ�Ѵ�.(�ּ��� size bytes)
 //�Ҵ�� ��ü ����� heap �ȿ� �־�� �ϸ�, ��ġ�� �ʾƾ� �Ѵ�.
void* mm_malloc(size_t size)
{
    char* bp;
    size_t asize;
    size_t extendsize;

    if (size == 0) return NULL;

    //alignment 8����Ʈ, header 4����Ʈ, footer 4����Ʈ�� Ȯ��
    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    }
    else {
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    }

    //��� ������ block�� find -> ������ ��ġ
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    //fit�� block�� ������ Ȯ�� �� ��ġ
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;

}

//��� ������ block�� �ּҸ� best-fit���� ã�� return�ϴ� �Լ�
static void* find_fit(size_t asize) {

    char* best = NULL;
    char* bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp))> 0; bp = NEXT_BLKP(bp)) {
        //�䱸 ���ǿ� ������ ������
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            //best�� �ƹ��͵� �ȵ����� �ٷ� bp �ֱ�
            if (best == NULL)
                best = bp;
            //bp�� size���� best�� ����� �� ũ�� bp�� best�� �ǵ��� �Ѵ�.
            if (GET_SIZE(HDRP(bp)) < GET_SIZE(HDRP(best)))
                best = bp;
        }

    }
    return best;
}

//��� ������ ���� ã����, block�� ��ġ�ϰ� ���� �κ��� ������.
static void place(void* bp, size_t asize) {


    size_t csize = GET_SIZE(HDRP(bp));

    //double word * 2���� (��� ������ block ������ - ��û block size)�� ũ�� ����
    if ((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }
    else {
        //������ �����͸� ���� �� ���⿡ �������� �ʰ� �׳� ��ġ
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }

};

//ptr�� ����Ű�� �ִ� �Ҵ� ����� �����Ѵ�. ��ȯ���� ����.
//������ mm malloc Ȥ�� mm realloc���� �Ҵ��� ���� �������� �ʾ��� �� ���
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

//������ �����ϸ� �����ϱ�
static void* coalesce(void* bp) {

    size_t size = GET_SIZE(HDRP(bp));
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    //�յ� �Ҵ� �ڵ� �Ҵ��̸� ��ĥ �ʿ� ����.
    if (prev_alloc && next_alloc) {
        return bp;
    }
    //���� ����� free��� ��ģ��.
    if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return bp;
    }
    //���� ����� free��� ��ģ��.
    if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        return bp;
    }
    //�Ѵ� free��� �Ѵ� ��ģ��.
    if (!prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        return bp;
    }

}
// �ּ� size�� �Ҵ�� region������ pointer�� ��ȯ�Ѵ�.
//���� ptr�� NULL�̸� mm malloc(size)�� ���� ����
//���� size�� 0�̸� mm free(ptr)�� ���� ����
//���� ptr�� not NUL�̸�, ptr�� �ռ� �ҷ��� malloc�� realloc�� ���� ���ϵ� ���̾�� �Ѵ�.
//���� ptr�� ����Ű�� �޸� ���� ũ�⸦ �����Ѵ�. -> ���ο� size�� ���ο� block�� �ּҸ� ��ȯ�Ѵ�.
//�ּ��� �� �ּҴ� ���� �ּҿ� ���� ���� �ְ� �ٸ� ���� �ִ�.
//�� ����� ������ ���� ptr block�� ����� �����ϰ�, �ʱ�ȭ���� �ʴ´�.
//
/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void* mm_realloc(void* ptr, size_t size)
{
    void* oldptr = ptr;
    void* newptr = mm_malloc(size);
    size_t copySize;

    //�̹� NULL �����Ϳ����� �׳� malloc
    if (ptr == NULL) {
        return newptr;
    }
    //size�� 0�̸� free�ϰ� NULL �Ѱ��ֱ�
    else if (size == 0) {
        mm_free(oldptr);
        return NULL;
    }
    else {
        //oldsize�� �� ũ�� ���̱�
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











