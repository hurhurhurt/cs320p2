#ifndef CACHES_H
#define CACHES_H

#include <iostream>
#include <vector>

class Cache{
 private:
  std::vector<std::pair<char, int>> caches;
 public:
  Cache(std::vector<std::pair<char,int>> caches){
    this->caches = caches;
  }
  int getSize(){
    return this->caches.size();
  }
  int dmCache(int);
  int saCache(int);
  int faLRU();
  int faHCR(int);
  int saNoAlloc(int);
  int saNextLine(int);
  int prefetchMiss(int);
  int LFU(int);
};
#endif
