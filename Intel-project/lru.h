#pragma once

#define LRU_age_increase(TYPE, WAYS, WAYS_INDEX, LINE_INDEX)\
foreach_way(i, WAYS)\
    if(cache_entry(TYPE, WAYS, LINE_INDEX, i)->age <= WAYS-1) cache_entry(TYPE, WAYS, LINE_INDEX, i)->age += 1;\
cache_entry(TYPE, WAYS, LINE_INDEX, WAYS_INDEX)->age=0;\


#define LRU_age_update(TYPE, WAYS,WAY_INDEX, LINE_INDEX) \
int max =(cache_entry(TYPE, WAYS, LINE_INDEX, WAY_INDEX)->age);\
foreach_way(i,WAYS)\
    if(cache_entry(TYPE, WAYS, LINE_INDEX, i)->age<max) cache_entry(TYPE, WAYS, LINE_INDEX, i)->age += 1;\
cache_entry(TYPE, WAYS, LINE_INDEX, WAY_INDEX)->age=0
