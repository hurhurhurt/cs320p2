#include "cache-sim.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <climits>
#include <cmath>
using namespace std;

#define SIZE 32

int Cache::getSize(){
  return this->caches.size();
}

int Cache::dmCache(int entries){
  int hits = 0;

  /* ALLOCATION OF ARRAY */
  // 2D Array of size entries containing 2 blocks
  // Blocks are denoted as [VALID | TAG]
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

  /* DEALLOCATING ARRAY */
  for (int i = 0; i < entries; i++){   // I considered using a smart pointer to avoid
    delete table[i];                   // allocating/deallocating arrays, but thought was
  }                                    // more complicated than necessary, since we know size.
  delete table;
  
  return hits;
}

int Cache::saCache(int associativity){
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
    bool found = false;                                   // Used for checking if value is there.
    int rowEntries = rowLength / associativity;           
    
    for (int i = 0; i < rowLength; i += rowEntries){
      if (cacheRow[i] == 1){                              // If valid bit is active, 
	if (cacheRow[i + 1] == tag){                      // tags match,
	  cacheRow[i + 2] = distance(caches.begin(), c);  // and distances match,
	  hits++;                                         // we have a hit.
	  found = true; 
	  break; 
	}
      }
    }

    if (!found){                                          // If there wasn't a match, we need to 
      bool LRU = false;                                   // add it to the cache and denote as
      for (int i = 0; i < rowLength; i += rowEntries){    // least recently used.
	if (cacheRow[i] == 0){
	  cacheRow[i] = 1;
	  cacheRow[i + 1] = tag;
	  cacheRow[i + 2] = distance(caches.begin(), c);
	  LRU = true;
	  break;
	}
      }

      if (!LRU){
       	int LRUIndex = 0;
	int minDistance = INT32_MAX;
	for (int i = 0; i < rowLength; i+= rowEntries){
	  if (cacheRow[i + 2] < minDistance){
	    minDistance = cacheRow[i + 2];
	    LRUIndex = i;
	  }
	}
	cacheRow[LRUIndex + 1] = tag;
	cacheRow[LRUIndex + 2] = distance(caches.begin(), c);
      }
    }
  }

  return hits;
}

int Cache::faLRU(){
  return saCache(512);
}      
      
int Cache::faHCR(int entries){
  int hits = 0;
  
  int *wayTable = new int[entries];        // holds the "ways"
  int *hcTable = new int[entries];         // holds the hold/cold bits

  for (int i = 0; i < entries; i++){       // zero out array
    wayTable[i] = 0;
    hcTable[i] = 0;
  }
  
  for (auto c: caches){
    int tag = c.second >> (int)(log2(SIZE));
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

      replaceIndex = replaceIndex + 1 - entries;//cold start miss
      wayTable[replaceIndex] = tag;
    }
  }
  return hits;
}

int Cache::saNoAlloc(int entries){
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
	cacheRow[i+2] = distance(caches.begin(), c);
	found = true;
	break;
      }
    }
    if (!found){
      bool interrupt = false;
      if (c->first == 'S') interrupt = true;
      for (int i = 0; i < rowLength && !interrupt; i+= rowLength/entries){
	if (cacheRow[i] == 0){
	  cacheRow[i] = 1;
	  cacheRow[i+1] = tag;
	  cacheRow[i+2] = distance(caches.begin(), c);
	  interrupt = true;
	  break;
	}
      }
      if (!interrupt){
	int LRUIndex = 0;
	int min = INT_MAX;
	for (int i = 0; i < rowLength; i+= rowLength / entries){
	  if (cacheRow[i+2] < min){
	    min = cacheRow[i+2];
	    LRUIndex = i;
	  }
	}
	cacheRow[LRUIndex + 1] = tag;
	cacheRow[LRUIndex + 2] = distance(caches.begin(), c);
      }
    }
  }
  return hits;
}

int Cache::saNextLine(int entries){
  int hits = 0;
  int cacheEntries = 512 / entries;              // 2^9 entries split into blocks
  int rowLength = 2 * entries;                   // Rows are [VALID][TIME]....
  int offset = log2(cacheEntries) + log2(SIZE);  // Offset is calculated the same way.

  auto c = caches.begin();
  int **table = new int*[cacheEntries];
  for (int i = 0; i < cacheEntries; i++){
    table[i] = new int[cacheEntries];
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

    int rowEntries = rowLength / entries;
    for (int i = 0; i < rowLength; i+= rowEntries){
      if (cacheRow[i] == tag){
	hits++;
	cacheRow[i+1] = distance(caches.begin(), c);
	found = true;
	break;
      }
    }

    for (int i = 0; i < rowLength; i+=rowEntries){
      if (pfRow[i] == tag){
	pfRow[i+1] = distance(caches.begin(), c);
	pfFound = true;
	break;
      }
    }

    if (!found){
      int replacePage = cacheRow[1];
      int replaceIndex = 0;

      for (int i = 0; i < rowLength; i+= rowEntries){
	if (cacheRow[i+1] < replacePage){
	  replacePage = cacheRow[i+1];
	  replaceIndex = i;
	}
      }

      cacheRow[replaceIndex] = tag;
      cacheRow[replaceIndex + 1] = distance(caches.begin(), c);
    }
    
    if (!pfFound){
      int replacePage = pfRow[1];
      int replaceIndex = 0;

      for (int i = 0; i < rowLength; i+= rowEntries){
	if (pfRow[i+1] < replacePage){
	  replacePage = pfRow[i+1];
	  replaceIndex = i;
	}
      }

      pfRow[replaceIndex] = pfTag;
      pfRow[replaceIndex+1] = distance(caches.begin(), c);
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
  outputFile << retVal << "," << caches.getSize() << endl;

  // Q3b: Full Associative Cache HCR
  retVal = caches.faHCR(512);
  outputFile << retVal << "," << caches.getSize() << endl;

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
    
  outputFile.close();

}
