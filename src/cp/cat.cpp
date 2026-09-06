// g++ -std=c++23 -Wl,--stack,268435456 cat.cpp -o cat.exe
// -fsanitize=address,undefined g++ -std=c++23 cat.cpp -o cat.exe
// -fsanitize=address,undefined

#include <algorithm>
#include <bits/stdc++.h>
#pragma GCC optimize("Ofast")
#pragma GCC optimize("unroll-loops")
#pragma GCC target("sse4")
using namespace std;
typedef long double ld;
typedef long long ll;
typedef unsigned long long ull;
#define pii pair<int, int>
#define pll pair<ll, ll>
#define pld pair<ld, ld>
#define pb push_back
#define fi first
#define se second
#define UP(a, b, c) for (ll(a) = (b); (a) < (c); ++(a))
#define UU(a, b, c) for (ll(a) = (b); (a) <= (c); ++(a))
#define DN(a, b, c) for (ll(a) = (b); (a) > (c); --(a))
#define DU(a, b, c) for (ll(a) = (b); (a) >= (c); --(a))
#define lc(i) i + 1
#define rc(i) i + (m - l + 1) * 2
#define debug(x) cout << #x << " = " << x << endl;

template <class A, class B>
ostream &operator<<(ostream &os, const pair<A, B> &p) {
  os << '(' << p.first << ',' << p.second << ')';
  return os;
}
template <class T> ostream &operator<<(ostream &os, const vector<T> &v) {
  bool fs = 1;
  os << '{';
  for (auto &i : v) {
    if (!fs)
      os << ", ";
    os << i;
    fs = 0;
  }
  os << '}';
  return os;
}

#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
using namespace __gnu_pbds;

#define ordered_set                                                            \
  tree<int, null_type, less_equal<int>, rb_tree_tag,                           \
       tree_order_statistics_node_update>

mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());

ll expo(ll a, ll b, ll mod) {
  if (b == 0)
    return 1;
  if (b == 1)
    return a % mod;
  ll ret = expo(a, b >> 1, mod);
  ret *= ret;
  if (ret >= mod)
    ret %= mod;
  if (b & 1) {
    ret *= a;
    if (ret >= mod)
      ret %= mod;
  }
  return ret;
}

ll inv(ll v, ll mod) { return expo(v, mod - 2, mod); }

void reset() {}

void input() {}

void solve() {}

void Ahoy() { solve(); }

struct FutuOpenApiResult {
  bool success{false};
  std::string payload;
};

using FutuOpenApiInvoker =
    std::function<FutuOpenApiResult(int command, const std::string &payload)>;

int main() {
  ios_base::sync_with_stdio(false);
  cin.tie(NULL);
  FutuOpenApiInvoker apiInvoker;
  apiInvoker = [](int command,
                  const std::string &payload) -> FutuOpenApiResult {
    // Simulate API response based on command and payload
    FutuOpenApiResult result;
    if (command == 1) {
      result.success = true;
      result.payload = "Command 1 executed with payload: " + payload;
    } else if (command == 2) {
      result.success = true;
      result.payload = "Command 2 executed with payload: " + payload;
    } else {
      result.success = false;
      result.payload = "Unknown command";
    }
    return result;
  };
  apiInvoker(1, "Test payload for command 1");

  int TC = 1;
  // cin >> TC;
  UU(t, 1, TC) {
    // cout << "Case #" << t << ": ";
    reset();
    input();
    Ahoy();
  }

  return 0;
}