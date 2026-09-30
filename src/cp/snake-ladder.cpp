#include <bits/stdc++.h>
using namespace std;

class SnakeLadder {
  struct Coord {
    size_t x_, y_;
    Coord(const size_t &&x, const size_t &&y) : x_(x), y_(x){};

    Coord operator++() { return *this; }
  };

public:
  SnakeLadder(const vector<vector<int>> &&board)
      : R_(board.size()), C_(board[0].size()), player_(0, 0) {
    grid_ = board;
  }

  bool roll(const int dice) {}

  ~SnakeLadder() {}

private:
  const size_t R_;
  const size_t C_;
  vector<vector<int>> grid_;
  Coord player_;
};