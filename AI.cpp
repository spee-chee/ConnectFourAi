using namespace std;
#include "board.h"

#include <algorithm>
#include <array>
#include <random>
#include <unordered_map>
#include <vector>

#define EXPLORATIONCONST 1.414214
struct Node
{

  // TODO
  // change ChildNodes to a set or map rather than a vector
  vector<Node *> ChildNodes = {};  // Pointers to each child node
  vector<Node *> ParentNodes = {}; // Pointers to all parent nodes
  unsigned int Hash;

  short int wins = 0;
  short int visits = 0;
  bool expanded = false;
  short int depth;
  char move;
  PlayerPiece PlayerTurn;

  Node(Node *parent, int move)
  {
    ParentNodes.push_back(parent);
    depth = parent->depth + 1;
    PlayerTurn = ((parent->PlayerTurn) == PlayerOne) ? PlayerTwo : PlayerOne;
  }
  /*
  Node* deep_copy()
  {
      Node copy = Node(ParentNodes[0]);

      for(int i = 1; i != ParentNodes.size(); ++i)
          copy.ParentNodes.push_back(ParentNodes[i]);

      for(int i = 0; i != ChildNodes.size(); ++i)
          copy.ChildNodes.push_back(ChildNodes[i]);

      copy.wins = wins;
      copy.visits = visits;
      copy.Hash = Hash;

  }
  */
};

struct TranspositionTable // contains map of all nodes which have been explored
{
  unordered_map<unsigned int, Node *> table = {};

  // Creates new node if hasn't been explored.
  Node *create_node(unsigned int hash, Node *parent, char move)
  {
    if(table.find(hash) == table.end())
    {
      table[hash] = new Node(parent, move);
    }
    return table[hash];
  }
};

class ZorbistHashing
{

  array<array<unsigned int, 2>, 42> ZorbistTable = {};

public:
  ZorbistHashing()
  {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distrib(1, 4294967295);

    // generates a random number for each piece in each position
    for(int i = 0; i != 42; ++i)
    {
      ZorbistTable[i][0] = distrib(gen);
      ZorbistTable[i][1] = distrib(gen);
    }
  }
  unsigned int get_zorbist_hash(PlayerPiece player, int x, int y, unsigned int hash)
  {

    return ZorbistTable[x + y * 7][player] ^ hash;
  }
};

class MCTS
{
  Node RootNode = Node(nullptr, 0);
  Board RootBoard = Board();

  Node CurrentNode = RootNode;
  Board CurrentBoard = *RootBoard.deep_copy();

  TranspositionTable TransTable = TranspositionTable();

  ZorbistHashing HashGen = ZorbistHashing();
  MCTS()
  {
    RootNode.Hash = 0;
  }

  vector<int> get_moves()
  {
    vector<int> ValidMoves = {};
    for(int i = 0; i != 7; ++i)
    {
      if(CurrentBoard.BoardTop[i] != -1)
      {
        ValidMoves.push_back(i);
      }
    }
    return ValidMoves;
  }

  double calc_UCT(Node node)
  {
    /*  UCT normally only works for trees where nodes have a single parent,
        so I'm not sure if totally all of the parent visits up will work    */

    int ParentVisits = 0;
    for(int i = 0; i != node.ParentNodes.size(); ++i)
    {
      ParentVisits += node.ParentNodes[i]->visits;
    }

    double UCT = node.wins / node.visits + EXPLORATIONCONST * sqrt(log(ParentVisits) / node.visits);

    return UCT;
  }

  void selection()
  {
    double MaxUCT;
    double uct;
    Node *NextNode;

    while(CurrentNode.expanded = true)
    {
      MaxUCT = 0;

      for(Node *pNode: CurrentNode.ChildNodes)
      {
        uct = calc_UCT(*pNode);
        if(uct > MaxUCT)
        {
          NextNode = pNode;
          MaxUCT = uct;
        }
      }

      CurrentBoard.drop_piece(NextNode->move, NextNode->PlayerTurn);
      CurrentNode = *NextNode;
    }

    /*
   double max_uct = 0;
   double uct;
   Node* node;
   for(pair<unsigned int, Node*> NodePair: TransTable.table)
   {
       uct = calc_UCT((*NodePair.second));
       if ( uct > max_uct)
       {
           node = NodePair.second;
           max_uct = uct;
       }
   }

   return node;
    */
  }

  void explansion()
  {

    vector<int> ValidMoves = get_moves();
    unsigned int NewHash;
    Node *pNewChild;

    // Loops through each valid move and adds it to ChildNodes attribute
    for(int move: ValidMoves)
    {
      NewHash = HashGen.get_zorbist_hash((CurrentNode.PlayerTurn == PlayerOne) ? PlayerTwo : PlayerOne, move, CurrentBoard.BoardTop[move],
                                         CurrentNode.Hash);
      pNewChild = TransTable.create_node(NewHash, &CurrentNode, (char) move);

      // Check if the node already exists as a child node before adding.
      if(find(CurrentNode.ChildNodes.begin(), CurrentNode.ChildNodes.end(), pNewChild) == CurrentNode.ChildNodes.end())
      {
        CurrentNode.ChildNodes.push_back(pNewChild);
      }
    }

    CurrentNode.expanded = true;
  }

  PlayerPiece simulation()
  {

    bool complete;
    int move;
    unsigned int NewHash;
    Node *pNewChild;
    Board SelectedNodeBoard = *CurrentBoard.deep_copy();

    // TODO
    // Make it so it changes the board state after changing current node
    // Implement backpropagation after each child finishes exploring
    for(Node *pChildNode: CurrentNode.ChildNodes) // note sure if this will work.
    {
      CurrentNode = *pChildNode;
      complete = false;
      while(!complete)
      {
        vector<int> ValidMoves = get_moves();

        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> distrib(0, ValidMoves.size() - 1);
        move = distrib(gen);

        NewHash = HashGen.get_zorbist_hash((CurrentNode.PlayerTurn == PlayerOne) ? PlayerTwo : PlayerOne, move, CurrentBoard.BoardTop[move],
                                           CurrentNode.Hash);
        pNewChild = TransTable.create_node(NewHash, &CurrentNode, (char) move);

        if(find(CurrentNode.ChildNodes.begin(), CurrentNode.ChildNodes.end(), pNewChild) == CurrentNode.ChildNodes.end())
        {
          CurrentNode.ChildNodes.push_back(pNewChild);
        }

        CurrentNode = *pNewChild;
        CurrentBoard.drop_piece(CurrentNode.move, CurrentNode.PlayerTurn);

        if(CurrentBoard.check_win(CurrentNode.move, CurrentNode.PlayerTurn) ||
           equal(CurrentBoard.BoardTop.begin() + 1, CurrentBoard.BoardTop.end(), CurrentBoard.BoardTop.begin()))
          complete = true;
      }

      // will randomly play until game is complete
    }
  }
  bool backpropagation()
  {
  }

  void play_move()
  {
    // change root node
    // playesr turn
  }
};