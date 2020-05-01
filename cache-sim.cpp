#include "cache-sim.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <climits>
#include <cmath>
using namespace std;

// Since all cache lines are 32 bytes, I used a macro to save some time
#define SIZE 32

// For LRU, we need a way to track least recently accessed. Using distance() is a good way to do so
#define TIMESTAMP distance(caches.begin(), c)

// For set associative caches, to index into the next row
# define ROWNUM rowLength / entries

int Cache::dmCache(int entries){
  
  /* DIRECT MAPPED CACHE
   *
   * Model cache at sizes 1KB, 4KB, 16KB, 32 KB
   *
   */
  int hits = 0;
  
  int **table = new int*[entries];
  for (int i = 0; i < entries; i++){
    table[i] = new int[2];
    for (int j = 0; j < 1; j++){
      table[i][j] = 0;
    }
  }

  int offset = log2(entries)+log2(SIZE);
  for (pair<char, int> c : caches){
    int address = floor(c.second / SIZE);
    int index = address % entries;
    int tag = c.second >> offset;

    if (table[index][0] == 1){         // If value at index is valid...
      if (table[index][1] == tag){     // and tag also matches..
	hits++;                        // we have a hit.
      }
      else{                            // Otherwise, we have to 
	table[index][1] = tag;         // update the tag and
	table[index][0] = 1;           // set the valid bit.
      }
    }
    else{
      table[index][1] = tag;           // Same as above ^ 
      table[index][0] = 1;
    }
  }
   
  return hits;
} 

int Cache::saCache(int associativity){

  /* SET ASSOCIATIVE CACHE
   *
   * Model cache with associativities of 2,4,8,16
   * Also implements LRU replacement strategy
   *
   */
  int hits = 0;
  int cacheEntries = 512 / associativity;        // 2^9 entries split into blocks
  int rowLength = 3 * associativity;             // Rows are [VALID][TAG][TIME].... 
  int offset = log2(cacheEntries) + log2(SIZE);  // Offset is calculated the same way.
 
  int **table = new int*[cacheEntries];
  for (int i = 0; i < cacheEntries; i++){
    table[i] = new int[rowLength];                
    for (int j = 0; j < rowLength; j++){         
      table[i][j] = 0;                           
    }
  }

  for (auto c = caches.begin(); c != caches.end(); c++){
    int address = floor(c->second / SIZE);
    int index = address % cacheEntries;
    int tag = c->second >> offset;
    
    int *cacheRow = table[index];
    bool found = false;                              
    int rowNum = rowLength / associativity;           
    
    for (int i = 0; i < rowLength; i += rowNum){
      if (cacheRow[i] == 1){                              // If valid bit is active, 
	if (cacheRow[i + 1] == tag){                      // tags match,
	  cacheRow[i + 2] = TIMESTAMP;                    // and distances match,
	  hits++;                                         // we have a hit.
	  found = true; 
	  break; 
	}
      }
    }

    if (!found){                                          // If there wasn't a match, we need to 
      bool LRU = false;                                   // add it to the cache and denote as
      for (int i = 0; i < rowLength; i += rowNum)    {    // least recently used.
	if (cacheRow[i] == 0){
	  cacheRow[i] = 1;
	  cacheRow[i + 1] = tag;
	  cacheRow[i + 2] = TIMESTAMP;
	  LRU = true;
	  break;
	}
      }

      // Overwrite the least recently used value
      if (!LRU){
       	int LRUIndex = 0;
	int minDistance = INT32_MAX;
	for (int i = 0; i < rowLength; i+= rowNum){
	  if (cacheRow[i + 2] < minDistance){
	    minDistance = cacheRow[i + 2];
	    LRUIndex = i;
	  }
	}
	cacheRow[LRUIndex + 1] = tag;
	cacheRow[LRUIndex + 2] = TIMESTAMP;
      }
    }
  }
  return hits;
}

int Cache::faLRU(){

  /* FULLY ASSOCIATIVE CACHE: LRU REPLACEMENT
   *
   * We don't have to actually implement this because we
   * implemented it above. The only difference is that because
   * we are fully associative, we only have 1 set with 2^9 entries
   * since 16KB / 32 line size = 2^9.
   *
   */
  
  return saCache(512);
}      

int Cache::faHCR(int entries){
  
  /* FULLY ASSOCIATIVE CACHE: HOT-COLD REPLACEMENT
   *
   * Utilize hot-cold "way" replacement strategy in place of LRU.
   * Hot cold bits are initialized to "0", representing the fact that
   * the left child, way 0, is "hot" and the right child is "cold".
   *
   */
  int hits = 0;
  
  int *wayTable = new int[entries];        // holds the "ways"
  int *hcTable = new int[entries];         // holds the hold/cold bits

  for (int i = 0; i < entries; i++){       // zero out array
    wayTable[i] = 0;
    hcTable[i] = 0;
  }
  
  for (auto c: caches){
    int tag = c.second >> (int)log2(SIZE); // Organization: [Tag][Offset]... no index in FAcache
    bool valid = false;                    // check if way is active
    for (int i = 0; i < entries; i++){
      if (wayTable[i] == tag){ 
	hits++;
	int parent = 0;
	for (int j = 0; j< log2(entries); j++){
	  int referenceBit = (i/(entries/(2 << j))) % 2; //check the old value
	  if (referenceBit == 0){
	    hcTable[parent] = 1;
	    parent = 2 * parent + 1; 
	  }
	  else{
	    hcTable[parent] = 0; 
	    parent = 2 * parent + 2;
	  }
	}
	valid = true;                      // we had a hit!
	break; 
      }
    }

    if (!valid){
      int replaceIndex = 0;
      for (int i = 0; i < log2(entries); i++){
	if (hcTable[replaceIndex] == 0){
	  hcTable[replaceIndex] = 1;
	  replaceIndex = 2 * replaceIndex + 1;
	}
	else{
	  hcTable[replaceIndex] = 0;
	  replaceIndex = 2 * replaceIndex + 2;
	}
      }

      replaceIndex = replaceIndex + 1 - entries; //cold start miss
      wayTable[replaceIndex] = tag;
    }
  }
  return hits;
}

int Cache::saNoAlloc(int entries){
  /* SET ASSOCIATIVE CACHE: NO ALLOCATION ON A WRITE MISS
   *
   * Same as set-associative cache above, but the only difference is
   * that if a store instruction misses, the missing line is written into
   * memory instead of into the cache
   *
   */
  int hits = 0;
  int cacheEntries = 512 / entries;              // 2^9 entries split into blocks
  int rowLength = 3 * entries;                   // Rows are [VALID][TAG][TIME]....
  int offset = log2(cacheEntries) + log2(SIZE);  // Offset is calculated the same way.

  int **table = new int*[cacheEntries];
  for (int i = 0; i < cacheEntries; i++){
    table[i] = new int[rowLength];
    for (int j = 0; j < rowLength; j++){
      table[i][j] = 0;
    }
  }
  for (auto c = caches.begin(); c!= caches.end(); c++){
    bool found = false;
    int address = floor(c->second / SIZE);
    int index = address % cacheEntries;
    int tag = c->second >> offset;
    int *cacheRow = table[index];
    for (int i = 0; i < rowLength; i++){
      if (cacheRow[i] == 1 && cacheRow[i+1] == tag){
	hits++;
	cacheRow[i+2] = TIMESTAMP;
	found = true;
	break;
      }
    }
    
    if (!found){                                  
      bool interrupt = false;       // We want to write "stores" directly to memory
      if (c->first == 'S') interrupt = true;
      for (int i = 0; i < rowLength && !interrupt; i+= ROWNUM){
	if (cacheRow[i] == 0){
	  cacheRow[i] = 1;
	  cacheRow[i+1] = tag;
	  cacheRow[i+2] = TIMESTAMP;
	  interrupt = true;
	  break;
	}
      }
      
      if (!interrupt){              // Proceed normally with LRU if not store instruction
	int LRUIndex = 0;
	int min = INT_MAX;
	for (int i = 0; i < rowLength; i+= ROWNUM){
	  if (cacheRow[i+2] < min){
	    min = cacheRow[i+2];
	    LRUIndex = i;
	  }
	}
	cacheRow[LRUIndex + 1] = tag;
	cacheRow[LRUIndex + 2] = TIMESTAMP;
      }
    }
  }
  return hits;
}

int Cache::saNextLine(int entries){
  /* SET-ASSOCIATIVE NEXT LINE PREFETCHING
   *
   * With each cache access, the next cache line is also brought in.
   * Prefetched blocks will update the LRU order, and will consequently be the
   * most recently used block in the set
   *
   */
  
  int hits = 0;
  int cacheEntries = 512 / entries;              // 2^9 entries split into blocks
  int rowLength = 2 * entries;                   // Rows are [VALID][TIME]....
  int offset = log2(cacheEntries) + log2(SIZE);  // Offset is calculated the same way.

  int **table = new int*[cacheEntries];

  auto c = caches.begin();
  for (int i = 0; i < cacheEntries; i++){
    table[i] = new int[rowLength];
    for (int j = 0; j < rowLength; j+= ROWNUM){
      table[i][j] = c->second;
      table[i][j+1] = -1;
      c++;
    }
  }
  
  for (auto c = caches.begin(); c!= caches.end(); c++){
    int address = floor(c->second / SIZE);
    int index = address % cacheEntries;
    int tag = c->second >> offset;

    int * cacheRow = table[index];

    int pfIndex = (address + 1) % cacheEntries;
    int pfTag = (c->second + SIZE) >> offset;
    int * pfRow = table[pfIndex];

    bool found = false;
    bool pfFound = false;

    for (int i = 0; i < rowLength; i+= ROWNUM){
      if (cacheRow[i] == tag){
	hits++;
	cacheRow[i+1] = TIMESTAMP;
	found = true;
	break;
      }
    }

    for (int i = 0; i < rowLength; i+= ROWNUM){
      if (pfRow[i] == tag){
	pfRow[i+1] = TIMESTAMP;
	pfFound = true;
	break;
      }
    }

    if (!found){
      int replacePage = cacheRow[1]; //timestamp 
      int replaceIndex = 0;

      for (int i = 0; i < rowLength; i+= ROWNUM){
	if (cacheRow[i+1] < replacePage){
	  replacePage = cacheRow[i+1];
	  replaceIndex = i;
	}
      }

      cacheRow[replaceIndex] = tag;
      cacheRow[replaceIndex + 1] = TIMESTAMP;
    }
    
    if (!pfFound){
      int replacePage = pfRow[1];
      int replaceIndex = 0;

      for (int i = 0; i < rowLength; i+= ROWNUM){
	if (pfRow[i+1] < replacePage){
	  replacePage = pfRow[i+1];
	  replaceIndex = i;
	}
      }

      pfRow[replaceIndex] = pfTag;
      pfRow[replaceIndex+1] = TIMESTAMP;
    }
  }
  return hits;    
}

int Cache::prefetchMiss(int entries){
  int hits = 0;
  int cacheEntries = 512 / entries;              // 2^9 entries split into blocks
  int rowLength = 2 * entries;                   // Rows are [VALID][TIME]....
  int offset = log2(cacheEntries) + log2(SIZE);  // Offset is calculated the same way.

  auto c = caches.begin();
  int **table = new int*[cacheEntries];
  for (int i = 0; i < cacheEntries; i++){
    table[i] = new int[rowLength];
    for (int j = 0; j < rowLength; j+= 2){
      table[i][j] = c->second;
      table[i][j+1] = -1;
      c++;
    }
  }

  for (auto c = caches.begin(); c!= caches.end(); c++){
    int address = floor(c->second / SIZE);
    int index = address % cacheEntries;
    int tag = c->second >> offset;

    int * cacheRow = table[index];

    int pfIndex = (address + 1) % cacheEntries;
    int pfTag = (c->second + SIZE) >> offset;
    int * pfRow = table[pfIndex];

    bool found = false;
    bool pfFound = false;

    for (int i = 0; i < rowLength; i+= ROWNUM){
      if (cacheRow[i] == tag){
	hits++;
	cacheRow[i+1] = TIMESTAMP;
	found = true;
	break;
      }
    }

    if (!found){
      for (int i = 0; i < rowLength; i+= ROWNUM){
	if (pfRow[i] == pfTag){
	  pfRow[i+1] = TIMESTAMP;
	  pfFound = true;
	  break;
	}
      }

      int replacePage = cacheRow[1];
      int replaceIndex = 0;

      for (int i = 0; i < rowLength; i+= ROWNUM){
	if (cacheRow[i+1] < replacePage){
	  replacePage = cacheRow[i+1];
	  replaceIndex = i;
	}
      }
      
      cacheRow[replaceIndex] = tag;
      cacheRow[replaceIndex + 1] = TIMESTAMP;
    }

    if (!pfFound && !found){
      int replacePage = pfRow[1];
      int replaceIndex = 0;

      for (int i = 0; i < rowLength; i+= ROWNUM){
	if (pfRow[i+1] < replacePage){
	  replacePage = pfRow[i+1];
	  replaceIndex = i;
	}
      }

      pfRow[replaceIndex] = pfTag;
      pfRow[replaceIndex + 1] = TIMESTAMP;
    }
  }
  return hits;
}

int main(int argc, char *argv[]){
  vector<pair<char, int>> inputVector;
  string trace = "traces/";
  ifstream inputFile(trace+argv[1]);
  string line;
  if (inputFile.is_open()){
    while (getline(inputFile, line)){
	char action = line.at(0);
	string str = line.substr(2);
	unsigned int hexAddr = stoul(str, nullptr, 16);
	inputVector.push_back(make_pair(action, hexAddr));
      }
  }
  inputFile.close();
  Cache caches(inputVector);

  int retVal;
  ofstream outputFile(argv[2]);
  
  // Q1: Direct-Mapped Cache
  vector<int> cacheSizes = {32, 128, 512, 1024};
  for (int i = 0; i < cacheSizes.size(); i++){
    retVal = caches.dmCache(cacheSizes.at(i));
    outputFile << retVal << "," << caches.getSize();
    outputFile << ((i == cacheSizes.size() - 1) ? ";" : "; ");
  }
  outputFile << endl;
  
  //Q2: Set-Associative Cache
  vector<int> associativity = {2,4,8,16};
  for (int i = 0; i < associativity.size(); i++){
    retVal = caches.saCache(associativity.at(i));
    outputFile << retVal << "," << caches.getSize();
    outputFile << ((i == associativity.size() - 1) ? ";" : "; ");
  }
  outputFile << endl;
  
  // Q3a: Fully Associative Cache LRU
  retVal = caches.faLRU();
  outputFile << retVal << "," << caches.getSize() << ";" << endl;

  // Q3b: Full Associative Cache HCR
  retVal = caches.faHCR(512);
  outputFile << retVal << "," << caches.getSize() << ";" << endl;

  // Q4: Set-Associative Cache, no Alloc on a Miss
  for (int i = 0; i < associativity.size(); i++){
    retVal = caches.saNoAlloc(associativity.at(i));
    outputFile << retVal << "," << caches.getSize();
    outputFile << ((i == associativity.size() - 1) ? ";" : "; ");
  }
  outputFile << endl;

  // Q5: Set-Associative Cache, Next Line Prefetching
  for (int i = 0; i < associativity.size(); i++){
    retVal = caches.saNextLine(associativity.at(i));
    outputFile << retVal << "," << caches.getSize();
    outputFile << ((i == associativity.size() - 1) ? ";" : "; ");
  }
  outputFile << endl;

  // Q6: Prefetch on Miss
  for (int i = 0; i < associativity.size(); i++){
    retVal = caches.prefetchMiss(associativity.at(i));
    outputFile << retVal << "," << caches.getSize();
    outputFile << ((i == associativity.size() - 1) ? ";" : "; ");
  }
  outputFile << endl;

  outputFile.close();
}
