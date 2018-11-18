#include "cachelab.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#define MAGIC_LRU_NUM 999
typedef struct{
    int valid;       //有效位
    int tag;          //标识位
    int LruNumber;   //LRU算法需要位
} Line;

typedef struct{
    Line* lines;    //一组中所有的行
} Set;

typedef struct {
    int set_num;    //组数
    int line_num;   //行数
    Set* sets;      //模拟cache
} Cache;
                                                                      //打印Help帮助文档                                                         //检查参数是否非法，初步检测是否是‘-’开头参数
int get_Opt(int argc,char **argv,int *s,int *E,int *b,char *tracefileName,int *isVerbose);  //读取用户输入参数
void init_Cache(int s,int E,int b,Cache *cache);                                     //初始化模拟缓存
int getSet(int addr,int s,int b);                                                           //得到组号
int getTag(int addr,int s,int b);                                                           //得到标记

void loadData(Cache *cache,int addr,int size,int setBits,int tagBits ,int isVerbose);                                                          //得到标记号
void storeData(Cache *cache,int addr,int size,int setBits,int tagBits ,int isVerbose);                                                          //得到标记号
void modifyData(Cache *cache,int addr,int size,int setBits,int tagBits ,int isVerbose);                                                          //得到标记号

int misses;
int hits;
int evictions;
int main(int argc,char **argv){
    int s,E,b,isVerbose=0;
    char tracefileName[100],opt[10];

    int addr,size;
    misses = hits = evictions =0;

    Cache cache;

    get_Opt(argc,argv,&s,&E,&b,tracefileName,&isVerbose);
    init_Cache(s,E,b,&cache);
    FILE *tracefile = fopen(tracefileName,"r");

    while(fscanf(tracefile,"%s %x,%d",opt,&addr,&size) != EOF){
        if(strcmp(opt,"I")==0)continue;
        int setBits = getSet(addr,s,b);
        int tagBits = getTag(addr,s,b);
//        printf("----------------------------------\n");
//        printf("setBits:%x  tagBits:%x\n",setBits,tagBits);
        if(isVerbose == 1) printf("%s %x,%d ",opt,addr,size);
        if(strcmp(opt,"S")==0) {
            storeData(&cache,addr,size,setBits,tagBits,isVerbose);
        }
        if(strcmp(opt,"M")==0) {
            modifyData(&cache,addr,size,setBits,tagBits,isVerbose);
        }
        if(strcmp(opt,"L")==0) {
            loadData(&cache,addr,size,setBits,tagBits,isVerbose);
        }
        if(isVerbose == 1) printf("\n");
    }
    printSummary(hits,misses,evictions);
    return 0;
}

int findMinLruNumber(Cache *cache,int setBits){
    int i;
    int minIndex=0;
    int minLru = MAGIC_LRU_NUM;
    for(i=0;i<cache->line_num;i++){
        if(cache->sets[setBits].lines[i].LruNumber < minLru){
            minIndex = i;
            minLru = cache->sets[setBits].lines[i].LruNumber;
        }
    }
    return minIndex;
}

void updateLruNumber(Cache *cache,int setBits,int hitIndex){
        cache->sets[setBits].lines[hitIndex].LruNumber = MAGIC_LRU_NUM;
        int j;
        for(j=0;j<cache->line_num;j++){//更新其他行的LruNumber
            if(j!=hitIndex) cache->sets[setBits].lines[j].LruNumber--;
        }
}

int updateCache(Cache *cache,int setBits,int tagBits){
    int i;
    int isfull = 1;
    for(i=0;i<cache->line_num;i++){
        if(cache->sets[setBits].lines[i].valid == 0){
            isfull = 0; //该组未满
            break;
        }
    }
    if(isfull == 0){
        cache->sets[setBits].lines[i].valid = 1;
        cache->sets[setBits].lines[i].tag = tagBits;
        updateLruNumber(cache,setBits,i);
    }else{
        //组已经满，需要牺牲行
        int evictionIndex = findMinLruNumber(cache,setBits);
        cache->sets[setBits].lines[evictionIndex].valid = 1;
        cache->sets[setBits].lines[evictionIndex].tag = tagBits;
        updateLruNumber(cache,setBits,evictionIndex);
    }
    return isfull;
}

int isMiss(Cache *cache,int setBits,int tagBits){
    int i;
    int isMiss = 1;
    for(i=0;i<cache->line_num;i++){
        if(cache->sets[setBits].lines[i].valid == 1 && cache->sets[setBits].lines[i].tag == tagBits){
            isMiss = 0;
            updateLruNumber(cache,setBits,i);
        }
    }
    return isMiss;
}



/*获取地址中的组号*/
int getSet(int addr,int s,int b){
    addr = addr >> b;
    int mask =  (1<<s)-1;
    return addr &mask;
}

/*获取地址中的标识号*/
int getTag(int addr,int s,int b){
    int mask = s+b;
    return addr >> mask;
}


void loadData(Cache *cache,int addr,int size,int setBits,int tagBits ,int isVerbose){

    if(isMiss(cache,setBits,tagBits)==1){ //没有命中
        misses++;
        if(isVerbose == 1) printf("miss ");
        if(updateCache(cache,setBits,tagBits) == 1){//该组已满，需要牺牲行
            evictions++;
            if(isVerbose==1) printf("eviction ");
        }
    }else{ //命中
       hits++;
       if(isVerbose == 1) printf("hit ");
    }
}

void storeData(Cache *cache,int addr,int size,int setBits,int tagBits ,int isVerbose){
    loadData(cache,addr,size,setBits,tagBits,isVerbose);
}

void modifyData(Cache *cache,int addr,int size,int setBits,int tagBits ,int isVerbose){
    loadData(cache,addr,size,setBits,tagBits,isVerbose);
    storeData(cache,addr,size,setBits,tagBits,isVerbose);
}
/*初始化模拟内存*/
void init_Cache(int s,int E,int b,Cache *cache){
    if(s < 0){
        printf("invaild cache sets number\n!");
        exit(0);
    }
    cache->set_num = 2 << s; //2^s 组
    cache->line_num = E;
    cache->sets = (Set *)malloc(cache->set_num * sizeof(Set));
    if(!cache->sets){
        printf("Set Memory error\n");
        exit(0);
    }
    int i ,j;
    for(i=0; i< cache->set_num; i++)
    {
        cache->sets[i].lines = (Line *)malloc(E*sizeof(Line));
        if(!cache->sets){
            printf("Line Memory error\n");
            exit(0);
        }
        for(j=0; j < E; j++){
            cache->sets[i].lines[j].valid = 0;
            cache->sets[i].lines[j].LruNumber = 0;
        }
    }
    return ;
}

int get_Opt(int argc,char **argv,int *s,int *E,int *b,char *tracefileName,int *isVerbose){
    int c;
    while((c = getopt(argc,argv,"hvs:E:b:t:"))!=-1)
    {
        switch(c)
        {
        case 'v':
            *isVerbose = 1;
            break;
        case 's':
            *s = atoi(optarg);
            break;
        case 'E':
            *E = atoi(optarg);
            break;
        case 'b':
            *b = atoi(optarg);
            break;
        case 't':
            strcpy(tracefileName,optarg);
            break;
        case 'h':
        default:
            exit(0);
        }
    }
    return 1;
}
