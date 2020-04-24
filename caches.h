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
  int getSize();
  int dmCache(int);
  int saCache(int);
  int faLRU();
  int faHCR();
  int saNoAlloc(int);
  int saNextLine(int);
  int prefetchMiss(int);
};
#endif
