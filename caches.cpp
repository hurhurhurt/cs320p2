#include "caches.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>

using namespace std;

#define SIZE 32

int Cache::getSize(){
  return this->caches.size();
}

int Cache::dmCache(int entries){
  int hits = 0;
  
  // allocate array
  int **table = new int*[entries];
  for (int i = 0; i < entries; i++){
    table[i] = new int[2];
  }

  int offset = (int)(log2(entries)+log2(SIZE));
  for (auto c : caches){
    int address = floor(c.second / SIZE);
    int index = address % entries;
    int tag = c.second >> offset;

    if (table[index][0] == 1){
      if (table[index][1] == tag){
	hits++;
      }
      else{
	table[index][1] = tag;
	table[index][0] = 1;
      }
    }
    else{
      table[index][1] = tag;
      table[index][0] = 1;
    }
  }

  for (int i = 0; i < entries; i++){
    delete table[i];
  }
  delete table;
  
  return hits;
}

int Cache::saCache(int entries){
  const int rowEntries = 512 / entries;
  const int rowLength = 3 * entries;
  int hits = 0;

  int ** table = new int*[rowEntries];
  for (int i = 0; i < rowEntries; i++){
    table[i] = new int[rowLength];
  }
  for (int i = 0; i < rowEntries; i++){
    for (int j = 0; j < rowLength; j++){
      table[i][j] = 0;
    }
  }
  int offset = (int)(log2(entries)+log2(SIZE));
  for (auto c = caches.begin(); c != caches.end(); c++){
    int address = floor(c->second / SIZE);
    int index = address % rowEntries;
    int tag = c->second >> offset;

    int * pageRow = table[index];
    bool found = false;
    for (int i = 0; i < rowLength; i+= rowLength / entries){
      if (pageRow[i] == 1){
	if (pageRow[i+1] == tag){
	  hits++;
	  pageRow[i+2] = distance(caches.begin(), c);
	  found = true;
	  break;
	}
      }
    }
    if (!found){
      bool inputted = false;
      for (int i = 0; i < rowLength; i+= rowLength / entries){
	if (pageRow[i] == 0){
	  pageRow[i] = 1;
	  pageRow[i+1] = tag;
	  pageRow[i+2] = distance(caches.begin(), c);
	  inputted = true;
	  break;
	}
      }
      if (!inputted){
	int lruIndex = 0;
	int min = INT32_MAX;
	for (int i = 0; i < rowLength; i += rowLength / entries){
	  if (pageRow[i+2] < min){
	    min = pageRow[i+2];
	    lruIndex = i;
	  }
	}
	pageRow[lruIndex+1] = tag;
	pageRow[lruIndex+2] = distance(caches.begin(), c);
      }
    }
  }
  for (int i = 0; i < rowEntries; i++){
    delete table[i];
  }
  delete table;
  return hits;
}

int Cache::faLRU(){
  return saCache(512);
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
  int counter = 0;
  for (auto c : cacheSizes){
    retVal = caches.dmCache(c);
    outputFile << retVal << "," << caches.getSize();
    outputFile << ((counter == cacheSizes.size() - 1) ? ";" : "; ");
    counter++;
  }
  outputFile << endl;
  
  //Q2: Set-Associatie Cache
  vector<int> associativity = {2,4,8,16};
  for (auto a : associativity){
    retVal = caches.saCache(a);
    outputFile << retVal << "," << caches.getSize();
    outputFile << ((counter == cacheSizes.size() - 1) ? ";" : "; ");
    counter++;
  }
  outputFile << endl;
  
  // Q3a: Fully Associative Cache LRU
  retVal = caches.faLRU();
  outputFile << retVal << "," << caches.getSize() << endl;
  
  outputFile.close();
  
}
