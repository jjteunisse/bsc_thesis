/*
 * hearts.cc
 * Source code for a program that plays Hearts using several strategies
 * Part of a bachelor thesis by Joris Teunisse, supervised by Walter Kosters
 * Compile using g++ -O2 -o hearts hearts.cc or the included run.sh file
 * Date of last edit: Jul 9, 2017
 */

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>

class Hearts{
  public:
    Hearts();
    ~Hearts();
    enum P_Type{PT_RD, PT_MC, PT_CV, PT_HM, PT_RB};
    struct Player{
      P_Type type;
      bool known[52];
      bool noneOfSuit[4];
      int hand[13];
      int validIndexes[13];
      int played;
      int points;
      int startPoints;
      int place;
      int playouts;
      int threshold;
    };
    bool justInvalids(int pNr, bool queen);
    int compareSituation(int pNr, Hearts O);
    int playRandomCard(int pNr);
    int playHumanCard(int pNr);
    int playCard(int pNr, int cardNr);
    int playMCCard(int pNr);
    int playRBCard(int pNr);
    int storeValidIndexes(int pNr);
    int getTotalPoints(int pNr){return totalPoints[pNr];}
    void playTrick();
    void playRound();
    void playGame();
    void shuffle(int *deck, int maxSize);
    void passCards();
    void printHand(int pNr);
    void evaluateTrick();
    void writeStats(std::ofstream &out);
    void setPT(int pNr, P_Type type){P[pNr].type = type;}
    void setPlayouts(int pNr, int amount){P[pNr].playouts = amount;}
    void setThreshold(int pNr, int amount){P[pNr].threshold = amount;}
    void setSuitOwners(int pNr);
    void debugMode(){debug = true;}
    void printCard(int card);
    void updateStandings();
    void randomPlayout(int pNr);
    void determinize(int pNr);
    void caseTest(); // TODO
  private:
    Player P[4];
    bool debug;
    bool gameWon;
    bool heartsBroken;
    int deck[52];
    int totalPoints[4];
    int ownerOfSuit[4];
    int first;
    int roundNr;
    int trickNr;
    int trump;
};

// Constructor
Hearts::Hearts(){
  for(int i = 0; i < 52; i++){
    deck[i] = i;
  }
  for(int i = 0; i < 4; i++){
    P[i].type = PT_RD;
  }
  debug = false;
  memset(totalPoints, 0, sizeof(totalPoints));
}

// Destructor
Hearts::~Hearts(){

}

// Prints a card integer in an easier format
void Hearts::printCard(int card){
  if(debug && card != -1){
    std::cout << "(" << card/13 << " " << card%13 << ") ";
  }
}

// Shuffle a deck into a random order, while making sure
// it does not go out of bounds
void Hearts::shuffle(int *deck, int maxSize){
  int size = 0, r, temp;
  while(size < maxSize && deck[size] != -1){
    size++;
  }
  for(int i = size-1; i > 0; i--){
    r = rand() % (i+1);
    temp = deck[r];
    deck[r] = deck[i];
    deck[i] = temp;
  }
}

// Every player passes and receives 3 cards
// TODO: RB/MC passing
void Hearts::passCards(){
  int passedCards[4][3];
  bool human = false;
  if(roundNr%4 != 0){
    for(int i = 0; i < 4; i++){
      if(P[i].type == PT_HM){
        human = true;
        printHand(i);
        std::cout << "Please enter three cards to pass to player ";
        std::cout  << (i+roundNr)%4 << " [0-12]" << std::endl;
      }
      for(int j = 0; j < 3; j++){
        int toPass;
        if(P[i].type == PT_HM){
          std::cin >> toPass;
          std::cout << "You pass ";
          printCard(P[i].hand[toPass]);
          std::cout << std::endl;
        }
        else{
          toPass = rand() % 13;
          while(P[i].hand[toPass] == -1){
            toPass = rand() % 13;
          }
        }
        if(P[i].type == PT_MC){
          P[i].known[P[i].hand[toPass]] = true;
        }
        passedCards[i][j] = P[i].hand[toPass];
        P[i].hand[toPass] = -1;
      }
    }
    for(int i = 0; i < 4; i++){
      int amtReceived = 0;
      for(int j = 0; j < 13; j++){
        if(P[i].hand[j] == -1){
          P[i].hand[j] = passedCards[(i+roundNr)%4][amtReceived];
          amtReceived++;
          if(P[i].hand[j] == 0){
            first = i;
          }
        }
      }
      if(!human){
        printHand(i);
      }
    }
  }
}

// Prints the hand of a player
void Hearts::printHand(int pNr){
  if(debug) std::cout << "Hand of player " << pNr << ":" << std::endl;
  for(int i = 0; i < 13; i++){
    printCard(P[pNr].hand[i]);
  }
  if(debug) std::cout << std::endl;
}

// Checks whether a player is forced to play a normally invalid card:
// either this consists of only Hearts, or Hearts and the Queen of Spades
bool Hearts::justInvalids(int pNr, bool queen){
  for(int i = 0; i < 13; i++){
    int card = P[pNr].hand[i];
    if(card != -1 && (card/13 != 2 || (queen && card != 49))){
      return false;
    }
  }
  return true;
}

// Stores which card indexes are valid for a player, and returns the amount
int Hearts::storeValidIndexes(int pNr){
  int amtValid = 0;
  if(trump == -1){
    if(trickNr == 0){
      for(int i = 0; i < 13; i++){
        if(P[pNr].hand[i] == 0){
          P[pNr].validIndexes[0] = i;
          return 1;
        }
      }
    }
    else{
      for(int i = 0; i < 13; i++){
        int card = P[pNr].hand[i];
        if(card != -1 && (!(!heartsBroken && card/13 == 2) || justInvalids(pNr, false))){
          P[pNr].validIndexes[amtValid] = i;
          amtValid++;
        }
      }
    }
  }
  else{
    for(int i = 0; i < 13; i++){
      if(P[pNr].hand[i] != -1 && P[pNr].hand[i]/13 == trump){
        P[pNr].validIndexes[amtValid] = i;
        amtValid++;
      }
    }
    if(amtValid == 0){
      P[pNr].noneOfSuit[trump] = true;
      for(int i = 0; i < 13; i++){
        int card = P[pNr].hand[i];
        if(card != -1 && (!(trickNr == 0 && (card/13 == 2 || card == 49)) || justInvalids(pNr, true))){
          P[pNr].validIndexes[amtValid] = i;
          amtValid++;
        }
      }
    }
  }
  return amtValid;
}

// Returns a random valid card belonging to the player in question
int Hearts::playRandomCard(int pNr){
  return playCard(pNr, P[pNr].validIndexes[rand() % storeValidIndexes(pNr)]);
}

// Lets a human choose a card to play for to the player in question
// TODO: Reject wrong input (spare time)
int Hearts::playHumanCard(int pNr){
  int amtValid = storeValidIndexes(pNr), cardNr;
  std::cout << std::endl;
  printHand(pNr);
  std::cout << "Your valid cards are: ";
  for(int i = 0; i < amtValid; i++){
    printCard(P[pNr].hand[P[pNr].validIndexes[i]]);
  }
  std::cout << std::endl;
  std::cout << "Which one do you want to play? [0-" << amtValid-1 << "]" << std::endl;
  std::cin >> cardNr;
  return playCard(pNr, P[pNr].validIndexes[cardNr]);
}

// Plays a specific card for a player
int Hearts::playCard(int pNr, int cardNr){
  int card = P[pNr].hand[cardNr];
  P[pNr].hand[cardNr] = -1;
  if(trump == -1){
    trump = card/13;
  }
  return card;
}

// Plays a card according to a few simple rules
// Always leads with the lowest card available
// If not leading, try to get rid of the penalty cards or play
// the highest card that can be freely discarded
// Also aims to shoot the moon if the player is the only one with
// penalty points, given the points are above a certain threshold
int Hearts::playRBCard(int pNr){
  int amtValid = storeValidIndexes(pNr), cardNr = -1, bestScore;
  bool shootTheMoon = (P[pNr].points - P[pNr].startPoints) >= P[pNr].threshold;
  for(int i = 0; i < 4; i++){
    if(i != pNr && (P[i].points - P[i].startPoints) > 0){
      shootTheMoon = false;
    }
  }
  bestScore = shootTheMoon ? 15 : -1;
  for(int i = 0; i < amtValid; i++){
    int card = P[pNr].hand[P[pNr].validIndexes[i]], score = 0;
    if(trump == -1){
      score = 13 - card%13;
    }
    else{
      if(card/13 == trump){
        for(int j = 0; j < 4; j++){
          if(P[j].played != -1 && P[j].played/13 == trump && P[j].played%13 > card%13){
            score = card%13;
          }
        }
      }
      else if(card/13 == 2){
        score = 13;
      }
      else if(card == 49){
        score = 14;
      }
    }
    if((!shootTheMoon && score > bestScore)
      || (shootTheMoon && score < bestScore)
      || (score == bestScore && rand()%2 == 0)){
      bestScore = score;
      cardNr = i;
    }
  }
  return playCard(pNr, P[pNr].validIndexes[cardNr]);
}


// Plays out the rest of the game randomly, starting from player pNr
void Hearts::randomPlayout(int pNr){
  int _trickNr = trickNr;
  debug = false;
  for(int i = 0; i < 4; i++){
    P[i].type = PT_RD;
  }
  while(P[pNr].played == -1){
    P[pNr].played = playRandomCard(pNr);
    if(P[pNr].played/13 == 2 && heartsBroken == false){
      heartsBroken = true;
    }
    pNr = (pNr+1)%4;
  }
  evaluateTrick();
  trickNr++;
  while(trickNr < 13 && trickNr - _trickNr != 7){
    playTrick();
    trickNr++;
  }
}

// Gets which player can own what suit based on available information
// The player the search is queried for is exempted from it
void Hearts::setSuitOwners(int pNr){
  memset(ownerOfSuit, -1, sizeof(ownerOfSuit));
  for(int i = 0; i < 4; i++){
    int owner = 0, singleSuit = -1, pCount = 0, sCount = 4;
    for(int j = 0; j < 4; j++){
      if(i != pNr){
        P[i].noneOfSuit[j] ? sCount-- : singleSuit = j;
      }
      if(j != pNr){
        P[j].noneOfSuit[i] ? pCount++ : owner = j;
      }
    }
    if(ownerOfSuit[i] == -1 && pCount == 2){
      ownerOfSuit[i] = owner;
    }
    if(ownerOfSuit[singleSuit] == -1 && sCount == 1){
      ownerOfSuit[singleSuit] = i;
    }
  }
}

// Applies determinization for a player: the cards are re-arranged for the
// current player such that all current knowledge is used, but there is no need
// to access hidden information.
// TODO: Predictive determinization, cleanup
void Hearts::determinize(int pNr){
  /*
    // Random version
    for(int k = 0; k < 100000; k++){
      int size = 0, toDeal[39];
      bool error = false;
      for(int i = 0; i < 4; i++){
        if(i != pNr){
          for(int j = 0; j < 13;  j++){
            if(P[i].hand[j] != -1){
              toDeal[size] = P[i].hand[j];
              P[i].hand[j] = -2;
              size++;
            }
          }
        }
      }
      shuffle(toDeal, size);
      for(int i = 0; i < 4; i++){
        if(i != pNr){
          for(int j = 0; j < 13;  j++){
            if(size > 0 && P[i].hand[j] == -2){
              P[i].hand[j] = toDeal[size-1];
              size--;
            }
          }
        }
      }
      for(int i = 0; i < 4; i++){
        if(i != pNr){
          for(int j = 0; j < 13;  j++){
            if(P[i].hand[j] != -1 && P[i].noneOfSuit[P[i].hand[j]/13]){
              error = true;
              break;
            }
          }
        }
      }
      if(error){
        continue;
      }
      break;
    }
  */
  int size = 0, amtOfSpots[4] = {0}, amtOfSuit[4] = {0}, unknown[39], spot[39];
  for(int i = 0; i < 4; i++){
    if(i != pNr){
      for(int j = 0; j < 13; j++){
        int card = P[i].hand[j];
        if(card != -1 && !P[pNr].known[card] && ownerOfSuit[card/13] != i){
          unknown[size] = P[i].hand[j];
          spot[size] = i*100+j;
          amtOfSuit[card/13]++;
          amtOfSpots[i]++;
          size++;
        }
      }
    }
  }
  for(int i = 0; i < 4; i++){
    if(i != pNr && amtOfSpots[i] > 0){
      int invalids = 0;
      for(int j = 0; j < 4; j++){
        if(P[i].noneOfSuit[j]){
          invalids += amtOfSuit[j];
        }
      }
      if(size - invalids == amtOfSpots[i]){
        for(int j = size-1; j >= 0; j--){
          if(!P[i].noneOfSuit[unknown[j]/13]){
            for(int k = 0; k < size; k++){
              if(spot[k]/100 == i){
                P[i].hand[spot[k]%100] = unknown[j];
                unknown[j] = unknown[size-1];
                spot[k] = spot[size-1];
                size--;
                break;
              }
            }
          }
        }
      }
    }
  }
  for(int i = 0; i < 100; i++){
    bool error = false;
    int currUnknown[size];
    int currSize = size;
    for(int j = 0; j < size; j++){
      currUnknown[j] = unknown[j];
    }
    shuffle(currUnknown, size);
    shuffle(spot, size);
    while(currSize > 0){
      int receiver = spot[currSize-1]/100, toDeal = currSize-1;
      while(P[receiver].noneOfSuit[currUnknown[toDeal]/13]){
        if(toDeal == 0){
          error = true;
          break;
        }
        toDeal--;
      }
      P[receiver].hand[spot[currSize-1]%100] = currUnknown[toDeal];
      currUnknown[toDeal] = currUnknown[currSize-1];
      currSize--;
    }
    if(!error){
      return;
    }
  }
}

// Compares the current situation for the player relative to another one
// and assigns points to it
// TODO: Avoid really bad moves
int Hearts::compareSituation(int pNr, Hearts O){
  return P[pNr].points - O.P[pNr].points;
}

// Plays a card for the player according to the Monte Carlo strategy
// Can play either clairvoyant or according to current knowledge
// TODO: Inspect individual cases for errors and improvement
//       Also, do borderline shoot-the-moon cases get stuck between
//       two options and choose a bad path? Maybe count cases and choose
//       most occurring one
int Hearts::playMCCard(int pNr){
  int amtValid = storeValidIndexes(pNr), bestCardNr = -1;
  int lowestScore = 100*P[pNr].playouts, score;
  setSuitOwners(pNr);
  for(int i = 0; i < amtValid; i++){
    Hearts C = *this;
    int cardNr = C.P[pNr].validIndexes[i];
    score = 0;
    C.P[pNr].played = C.playCard(pNr, cardNr);
    for(int j = 0; j < P[pNr].playouts; j++){
      Hearts T = C;
      if(P[pNr].type == PT_MC){
        T.determinize(pNr);
      }
      T.randomPlayout((pNr+1)%4);
      score += T.compareSituation(pNr, C);
    }
    /*if(score - lowestScore < P[pNr].playouts){
      // Per playout 1 point, check bounds
    }*/
    if(score < lowestScore || (score == lowestScore && rand()%2 == 0)){
      bestCardNr = cardNr;
      lowestScore = score;
    }
  }
  return playCard(pNr, bestCardNr);
}

// Updates the ranks of all the players
void Hearts::updateStandings(){
  int lowest = -1;
  for(int i = 0; i < 4; i++){
    P[i].place = 0;
  }
  for(int i = 0; i < 4; i++){
    int temp_lowest = 1000;
    for(int j = 0; j < 4; j++){
      if(P[j].points > lowest){
        P[j].place++;
        if(P[j].points < temp_lowest){
          temp_lowest = P[j].points;
        }
      }
    }
    lowest = temp_lowest;
  }
}

// Evaluates a trick by calculating the points and which player is next
// TODO: Different points to test Shooting the Moon
void Hearts::evaluateTrick(){
  int highest = 0, next = -1, trickValue = 0;
  for(int i = 0; i < 4; i++){
    if(P[i].played/13 == trump && P[i].played > highest){
      highest = P[i].played;
      next = i;
    }
    if(P[i].played/13 == 2){
      trickValue++;
    }
    else if(P[i].played == 49){
      trickValue += 13;
    }
  }
  P[next].points += trickValue;
  if(P[next].points - P[next].startPoints == 26){
    P[next].points -= 26;
    for(int i = next+1; i < next+4; i++){
      P[i%4].points += 26;
      if(P[i%4].points >= 100 && !gameWon){
        gameWon = true;
      }
    }
  }
  else if(P[next].points >= 100 && !gameWon){
    gameWon = true;
  }
  first = next;
}

// Plays a trick of Hearts
void Hearts::playTrick(){
  if(debug) std::cout << "Player " << first << " is first." << std::endl;
  if(debug) std::cout << "Cards on table: ";
  for(int i = 0; i < 4; i++){
    memset(P[i].validIndexes, -1, sizeof(P[i].validIndexes));
    P[i].played = -1;
  }
  trump = -1;
  for(int i = first; i < first+4; i++){
    int pNr = i%4;
    if(P[pNr].type == PT_HM){
      P[pNr].played = playHumanCard(pNr);
    }
    else if(P[pNr].type == PT_MC || P[pNr].type == PT_CV){
      P[pNr].played = playMCCard(pNr);
    }
    else if(P[pNr].type == PT_RB){
      P[pNr].played = playRBCard(pNr);
    }
    else{
      P[pNr].played = playRandomCard(pNr);
    }
    if(P[pNr].played/13 == 2 && heartsBroken == false){
      heartsBroken = true;
    }
    printCard(P[pNr].played);
  }
  if(debug) std::cout << std::endl << std::endl;
  evaluateTrick();
}

// Writes statistics about the games to a file
// TODO: Include file in class?
void Hearts::writeStats(std::ofstream &out){
  for(int i = 0; i < 4; i++){
    out << "p" << i << "_" << P[i].place << '\n';
  }
  out << '\n';
}

// Plays a round of Hearts
void Hearts::playRound(){
  heartsBroken = false;
  roundNr++;
  for(int i = 0; i < 4; i++){
    memset(P[i].noneOfSuit, false, sizeof(P[i].noneOfSuit));
    memset(P[i].known, false, sizeof(P[i].known));
    P[i].startPoints = P[i].points;
  }
  shuffle(deck, 52);
  for(int i = 0; i < 4; i++){
    for(int j = 0; j < 13; j++){
      P[i].hand[j] = deck[i*13+j];
      if(P[i].hand[j] == 0){
        first = i;
      }
    }
  }
  passCards();
  if(debug) std::cout << std::endl;
  for(trickNr = 0; trickNr < 13; trickNr++){
    playTrick();
  }
  updateStandings();
  if(debug) std::cout << "Current points:" << std::endl;
  for(int i = 0; i < 4; i++){
    if(debug) std::cout << i << ": " << P[i].points << std::endl;
  }
  if(debug){
    if(gameWon){
      std::cout << "End of this game." << std::endl << std::endl;
    }
    else{
      std::cout << "End of round " << roundNr << "." << std::endl << std::endl;
    }
  }
}

// Plays a game of Hearts
void Hearts::playGame(){
  for(int i = 0; i < 4; i++){
    P[i].points = 0;
  }
  gameWon = false;
  roundNr = 0;
  while(!gameWon){
    playRound();
  }
  for(int i = 0; i < 4; i++){
    totalPoints[i] += P[i].points;
  }
}

void Hearts::caseTest(){
  memset(P[0].known, 0, sizeof(P[0].known));
  memset(ownerOfSuit, 0, sizeof(ownerOfSuit));
  for(int i = 1; i < 4; i++){
    memset(P[i].hand, -1, sizeof(P[i].hand));
  }
  P[2].noneOfSuit[0] = true;
  P[3].noneOfSuit[1] = true;
  P[3].noneOfSuit[3] = true;
  // std::ofstream o("det.txt");
  // std::string comp;
  long long int dists[2352];
  int distcts[2352];
  memset(dists, 0, sizeof(dists));
  memset(distcts, 0, sizeof(distcts));
  int arr[16] = {0, 1, 13, 14, 15, 16, 17, 26, 27, 28, 29, 39, 40, 41, 42, 43};
  int sp[16] = {10, 11, 12, 13, 14, 15, 20, 21, 22, 23, 24, 30, 31, 32, 33, 34};
  for(int i = 0; i < 23520000; i++){
    long long int currdist = 0;
    // std::string str = "_suits ";
    P[1].hand[0] = arr[0];
    P[1].hand[1] = arr[1];
    P[1].hand[2] = arr[2];
    P[1].hand[3] = arr[3];
    P[1].hand[4] = arr[4];
    P[1].hand[5] = arr[5];
    P[2].hand[0] = arr[6];
    P[2].hand[1] = arr[7];
    P[2].hand[2] = arr[8];
    P[2].hand[3] = arr[9];
    P[2].hand[4] = arr[10];
    P[3].hand[0] = arr[11];
    P[3].hand[1] = arr[12];
    P[3].hand[2] = arr[13];
    P[3].hand[3] = arr[14];
    P[3].hand[4] = arr[15];
    determinize(0);
    // comp += "_dist";
    for(int j = 0; j < 16; j++){
      for(int k = 0; k < 16; k++){
        if(P[sp[k]/10].hand[sp[k]%10] == arr[j]){
          long long int player = sp[k]/10;
          // comp += " ";
          // comp += player;
          for(int l = 0; l < j; l++){
            player *= 10;
          }
          currdist += player;
          /*
          if(sp[k]/10 == 1 && arr[j]/13 == 0){
            str += "WC ";
          }
          else if(sp[k]/10 == 1 && arr[j]/13 == 2){
            str += "WH ";
          }
          else if(sp[k]/10 == 2 && arr[j]/13 == 2){
            str += "NH ";
          }
          */
          break;
        }
      }
    }
    for(int j = 0; j < 2352; j++){
      if(currdist == dists[j]){
        distcts[j]++;
        break;
      }
      else if(dists[j] == 0){
        dists[j] = currdist;
        distcts[j]++;
        break;
      }
    }
    // o << comp << '\n' << str << '\n';
  }
  std::cout << "WC:" << std::endl;
  for(int j = 0; j < 2352; j++){
    if((dists[j]%10)/1 == 1 || (dists[j]%100)/10 == 1){
      std::cout << distcts[j] << " ";
    }
  }
  std::cout << std::endl << std::endl << "WH:" << std::endl;
  for(int j = 0; j < 2352; j++){
    if((dists[j]%100000000)/10000000 == 1 || (dists[j]%1000000000)/100000000 == 1
    || (dists[j]%10000000000)/1000000000 == 1 || (dists[j]%100000000000)/10000000000 == 1){
      std::cout << distcts[j] << " ";
    }
  }
  std::cout << std::endl << std::endl << "NH:" << std::endl;
  for(int j = 0; j < 2352; j++){
    if((dists[j]%100000000)/10000000 == 2 || (dists[j]%1000000000)/100000000 == 2
    || (dists[j]%10000000000)/1000000000 == 2 || (dists[j]%100000000000)/10000000000 == 2){
      std::cout << distcts[j] << " ";
    }
  }
  // o.close();
  std::cout << std::endl;
}

int main(int argc, char *argv[]){
  srand(time(NULL));
  clock_t start = clock();
  std::ofstream out;
  out.open("stats.txt");
  Hearts *H = new Hearts();
  int amtOfGames = 100;
  int progress = 0;
  for(int i = 1; i < argc; i++){
    if(strcmp(argv[i], "-mc") == 0 && i+2 < argc){
      H->setPT(atoi(argv[++i]), H->PT_MC);
      H->setPlayouts(atoi(argv[i]), atoi(argv[i+1]));
      i++;
    }
    else if(strcmp(argv[i], "-cv") == 0 && i+2 < argc){
      H->setPT(atoi(argv[++i]), H->PT_CV);
      H->setPlayouts(atoi(argv[i]), atoi(argv[i+1]));
      i++;
    }
    else if(strcmp(argv[i], "-hm") == 0 && i+1 < argc){
      H->setPT(atoi(argv[++i]), H->PT_HM);
      H->debugMode();
      progress = -1;
    }
    else if(strcmp(argv[i], "-rb") == 0 && i+2 < argc){
      H->setPT(atoi(argv[++i]), H->PT_RB);
      H->setThreshold(atoi(argv[i]), atoi(argv[i+1]));
      i++;
    }
    else if(strcmp(argv[i], "-d") == 0){
      H->debugMode();
      progress = -1;
    }
    else if(argv[i] != NULL){
      amtOfGames = atoi(argv[i]);
    }
  }
  // H->caseTest();
  if(progress == 0) std::cout << "Progress: " << std::endl;
  for(int i = 0; i < amtOfGames; i++){
    H->playGame();
    H->writeStats(out);
    if(progress >= 0){
      if(i >= progress*(float)amtOfGames/100.0){
        progress += amtOfGames > 100 ? 1 : 100/amtOfGames;
        std::cout << progress << "%" << '\r' << std::flush;
      }
    }
  }
  std::cout << "100%" << std::endl << std::endl;
  out.close();
  std::cout << "Average points per game session: " << std::endl;
  for(int i = 0; i < 4; i++){
    std::cout << "Player " << i << ": " << H->getTotalPoints(i) / (float)amtOfGames << std::endl;
  }
  delete H;
  std::cout << "Time required: " << (clock() - start) / (double) CLOCKS_PER_SEC << "s" << std::endl;
  return 0;
}
