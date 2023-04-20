// 20210207 이지현
// ID : io0818
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include "cachelab.h"

//캐시 구조체
typedef struct cache_line {
    char valid;
    unsigned long long int tag;
    unsigned long long int LRU;
} cache_line;
typedef cache_line* cache_set;
typedef cache_set* cache;
cache CACHE;

//캐시 변수들
typedef struct arguments {
    int s;
    int S;
    int b;
    int B;
    int E;
} argument;
argument args;

// 이외의 전역 변수들
int verbosity = 0; 
char* verbose;
char command;
char* trace_name = NULL;
unsigned long long int LRU_counter = 1;

//결과 변수들
int miss_num = 0;
int hit_num = 0;
int eviction_num = 0;

//함수 프로토타입
void accessCache(unsigned long long int addr);



int main(int argc, char* argv[]) {
   
    char opt;
    while ((opt = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
        switch (opt) {

        case 's':
            args.s = atoi(optarg);
            args.S = (unsigned int)(1 << args.s);
            break;
        case 'E':
            args.E = atoi(optarg);
            break;
        case 'b':
            args.b = atoi(optarg);
            args.B = (unsigned int)(1 << args.b);
            break;
        case 't':
            trace_name = optarg;
            break;
        case 'v':
            verbosity = 1;
            break;
        case 'h':
            printf("Options:\n");
            printf(" -h            : Optional help flag that prints usage info\n");
            printf(" -v            : Optional verbose flag that displays trace info\n");
            printf(" -s <s>        : Number of set index bits (S = 2^s is the number of sets)\n");
            printf(" -E <E>        : Associativity (number of lines per set)\n");
            printf(" -b <b>        : Number of block bits (B = 2^b is the block size)\n");
            printf(" -t <tracefile>: Name of the valgrind trace to replay\n");
            exit(0);
        default:
            return 0;
        };
    }

// 위에서 받은 매개변수를 토대로 CACHE를 동적 할당한다.

    int i, j;
    CACHE = (cache_set*)malloc(sizeof(cache_set) * args.S);
    for (i = 0; i < args.S; i++) {
        CACHE[i] = (cache_line*)malloc(sizeof(cache_line) * args.E);
        for (j = 0; j < args.E; j++) {
            CACHE[i][j].valid = 0;
            CACHE[i][j].tag = 0;
            CACHE[i][j].LRU = 0;
        }
    }

 //CACHE 안의 값을 TRACE 파일 안에서 읽어온다.

    FILE* trace = fopen(trace_name, "r");
    unsigned long long int address;
    int size;

    while (fscanf(trace, " %c %llx,%d", &command, &address, &size)!=EOF) {
        switch (command) {
        case 'L': accessCache(address); break;
        case 'S': accessCache(address); break;
        case 'M': accessCache(address); accessCache(address); break;
        default: break;
        }
        //만약 V 옵션이 주어지면 자세한 창을 PRINT하기
        if (verbosity != 0) {
            printf("%c %llx,%d %s\n", command, address, size, verbose);
        }
    }

 //HIT, MISS, EVICTION 된 NUM을 PRINT한다.
    printSummary(hit_num, miss_num, eviction_num);
 
 //CACHE 동적 할당 해제
    for (i = 0; i < args.S; i++) {
        free(CACHE[i]);
    }
    free(CACHE);

 //TRACE 파일 닫기
    fclose(trace);
    return 0;
}


void accessCache(unsigned long long int address) {

    int i;
    unsigned long long int set = (0x7fffffff >> (31 - args.s)) & (address >> args.b);
    unsigned long long int tag = 0x7ffffffff & (address >> args.s >> args.b);
    
    cache_set CACHE_SET = CACHE[set];

    int local_hit = 0;
    int local_eviction = 0;
    
    
    //hit
    for (i = 0; i < args.E; i++) {
         if (CACHE_SET[i].tag == tag && CACHE_SET[i].valid == 1) {
             CACHE_SET[i].LRU = LRU_counter++;
             hit_num++;
             local_hit = 1;
             return;
         }      
    }

    //miss
    miss_num++;
    

    //LRU 만족하는 가장 최근에 사용되지 않았던 block 찾기
    unsigned long long int eviction_LRU = ULONG_MAX;
    unsigned int eviction_line = 0;

    for (i = 0; i < args.E; ++i) {
        if (eviction_LRU > CACHE_SET[i].LRU) {
            eviction_line = i;
            eviction_LRU = CACHE_SET[i].LRU;
        }
    }
 

    //eviction
    if (CACHE_SET[eviction_line].valid) {
        eviction_num++;
        local_eviction = 1;
    }

    CACHE_SET[eviction_line].valid = 1;
    CACHE_SET[eviction_line].tag = tag;
    CACHE_SET[eviction_line].LRU = LRU_counter++;

    //v 옵션일 때 verbose 설정
    if (command == 'L' || command == 'S') {

        if (local_hit) verbose = "hit";
        else if (local_eviction) verbose = "miss evction";
        else verbose = "miss";

    }
    else if (command == 'M') {

        if (local_hit) verbose = "hit hit";
        else if (local_eviction) verbose = "miss evction hit";
        else verbose = "miss hit";

    }

}