#include "cache-sim.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <climits>
#include <cmath>

using namespace std;

#define SIZE 32

int Cache::LFU(int entries){
  /* LFU CACHE IMPLEMENTATION
   *
   * SOURCE: https://ieftimov.com/post/when-why-least-frequently-used-cache-implementation-golang/
   * Idea: Add another dimension to LRU cache by adding "frequencies". The general idea is to
   * improve on LRU's weakness by adding an element of how often a specific element is used; if
   * it's used often, its likely to be used more in the future. To do this, I create a separate
   * array to keep track of the frequency of each element
   *
   */

  int hits = 0;
  int cacheEntries = 512 / entries;
  int offset = log2(cacheEntries) + log2(SIZE);

  int **table = new int*[cacheEntries];
  for (int i = 0; i < cacheEntries; i++){
    table[i] = new int[entries];
    for (int j = 0; j < entries; j++){
      table[i][j] = 0;
    }
  }

  vector<int> tags;
  vector<int> frequencies;

  for (auto c = caches.begin(); c!= caches.end(); c++){
    int address = floor(c->second / SIZE);
    int index = address % cacheEntries;
    int tag = c->second >> offset;

    bool found = false;
    for (int i = 0; i < entries; i++){
      if (table[index][i] == tag){
	hits++;
	found = true;
	for (int j = 0; j < tags.size(); j++){
	  if (tags[j] == tag){
	    frequencies[j]++;
	    break;
	  }
	}
	break;
      }
    }
    if (!found){
      int freq[entries];
      for (int i = 0; i < entries; i++) freq[i] = 0;
      for (int i = 0; i < tags.size(); i++){
	for (int j = 0; j < entries; j++){
	  if (tags[i] == table[index][j]){
	    freq[j] = frequencies[i];
	  }
	}
      }
      int max_index = 0;
      int max_value = table[index][0];
      for (int i = 0; i < entries; i++){
	if (freq[i] < max_value){
	  max_index = i;
	  max_value = freq[i];
	}
      }

      table[index][max_index] = tag;
      bool found = false;
      for (int i =0; i < tags.size(); i++){
	if (tags[i] == tag){
	  found = true;
	}
      }
      if (!found){
	tags.push_back(tag);
	frequencies.push_back(1);
      }
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

  vector<int> associativity = {2,4,8,16};
  
  for (int i = 0; i < associativity.size(); i++){
    retVal = caches.LFU(associativity.at(i));
    outputFile << retVal << "," << caches.getSize();
    outputFile << ((i == associativity.size() - 1) ? ";" : "; ");
  }

  outputFile.close();
}
