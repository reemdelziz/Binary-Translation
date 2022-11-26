#include <iostream>
#include <pthread.h>
#include <math.h>
#include <vector>

using namespace std;

struct info {
  vector<string> c;
  vector<int> val;
  vector<string>* bin;
  int* binLength;
  string* sequence;
  vector<string> seq;

  int* turn;
  pthread_mutex_t* m;
  pthread_mutex_t* m2;
  pthread_cond_t* cond;
  int idx;
};


//n threads Determines the binary representation of the symbol's code. Determines the frequency of the symbol in the compressed message
void* binAndFreq(void* character) {
    struct info currLetter = *(struct info*) character;
    pthread_mutex_unlock(currLetter.m);

    int currThread = currLetter.idx;
    int v = currLetter.val[currThread];
    int freq = 0;

    //find binary reps. for each val
    string binary = "";
    while (v > 0) {
      binary = to_string(v % 2) + binary;
      v = v / 2;
    }
    while (binary.length() < *currLetter.binLength) {
      binary = "0" + binary;
    }
    currLetter.bin->push_back(binary);


    //find frequency
    string line = *currLetter.sequence;
    while (line.length() > 0) {
      if (line.substr(0, *currLetter.binLength) == currLetter.bin->at(currThread)) freq++;
      line = line.substr(*currLetter.binLength);
    }

    //lock to wait and compare thread idx with turn
    pthread_mutex_lock(currLetter.m2);
    while(currLetter.idx != *currLetter.turn) {
      pthread_cond_wait(currLetter.cond, currLetter.m2);
    }
    pthread_mutex_unlock(currLetter.m2);
    
    cout << "Character: " << currLetter.c[currThread] << ", Code: " << currLetter.bin->at(currThread) << ", Frequency: " << freq << endl;

    
    //increment turn and broadcast to wait for other threads to continue 
    pthread_mutex_lock(currLetter.m2);
    *(currLetter.turn)=*(currLetter.turn)+1;
    pthread_cond_broadcast(currLetter.cond);
    pthread_mutex_unlock(currLetter.m2);

    return nullptr;
}



//m threads (length of message) print their decompressed character
void* decompress(void* character) {
  info charToDecode = *(info*) character;
  pthread_mutex_unlock(charToDecode.m2);

  int id = charToDecode.idx;

  pthread_mutex_lock(charToDecode.m2);
  while(charToDecode.idx != *charToDecode.turn) {
    pthread_cond_wait(charToDecode.cond, charToDecode.m2);
  }
  pthread_mutex_unlock(charToDecode.m2);

  for (int i = 0; i < charToDecode.c.size(); i++){
    if (charToDecode.seq[id] == charToDecode.bin->at(i)) {
      *charToDecode.sequence += charToDecode.c[i];
      break;
    }
  }

  pthread_mutex_lock(charToDecode.m2);
  *(charToDecode.turn)=*(charToDecode.turn)+1;
  pthread_cond_broadcast(charToDecode.cond);
  pthread_mutex_unlock(charToDecode.m2);

  return nullptr;
}



int main() {
  //declare and initialize semaphores
  static pthread_mutex_t mutex;
  static pthread_cond_t turn = PTHREAD_COND_INITIALIZER;
  pthread_mutex_init(&mutex, NULL);

  static pthread_mutex_t mutex2;
  static pthread_cond_t turn2 = PTHREAD_COND_INITIALIZER;
  pthread_mutex_init(&mutex2, NULL);

  int aSize;
  cin >> aSize;
  cin.ignore();
  
  info temp;
  string c;
  int num;
  int largest = -1;
  string line;

  //Recieve input, populate character vector and values vector
  for (int i = 0; i < aSize; i++) {
    getline(cin, line);
    c = line.substr(0, 1);
    num = stoi(line.substr(2));
    if (num > largest) largest = num;
    temp.c.push_back(c);
    temp.val.push_back(num);
  }

  //bits per char
  int binLength = ceil(log2(largest + 1));

  string sequence;
  cin >> sequence;

  cout << "Alphabet:" << endl;

  vector<string> listOfBinCodes;
  string decompressed;
  pthread_t tid[aSize];
  
  int tturn = 0;
  for(int i = 0; i < aSize; i++) 
  {
    pthread_mutex_lock(&mutex);
    temp.m = &mutex;
    temp.cond = &turn;
    temp.m2 = &mutex2;
    temp.bin = &listOfBinCodes;
    temp.sequence = &sequence;
    temp.turn = &tturn;
    temp.binLength = &binLength;
    
    temp.idx = i;
    if(pthread_create(&tid[i], nullptr, binAndFreq, &temp)) 
    {
      fprintf(stderr, "Error creating thread\n");
      return 1;
    }	
  }  

//SYNCH IN SEPERATE FOR LOOP (pthread_join())
  for (int i = 0; i < aSize; i++) {
    pthread_join(tid[i], NULL);
  }

  
  //calculate number of child threads
  int m = sequence.length()/binLength;

  //populate sequence array in struct
  for(int i = 0; i < m; i++) 
  {
    temp.seq.push_back(sequence.substr(0, binLength));
    sequence = sequence.substr(binLength);
  }

  tturn = 0;
  pthread_t tid2[m];

  temp.m2 = &mutex2;

  cout << endl << "Decompressed message: ";

  //create m threads, each thread decoding a part of the full sequence
  for (int i = 0; i < m; i++) {
    pthread_mutex_lock(&mutex2);
    
    temp.idx = i;
    if(pthread_create(&tid2[i], nullptr, decompress, &temp)) 
    {
      fprintf(stderr, "Error creating thread\n");
      return 1;
    }	
  }

  for (int i = 0; i < m; i++) {
    pthread_join(tid2[i], NULL);
  }

  cout << *temp.sequence;

}
