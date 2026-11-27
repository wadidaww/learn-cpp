# Day 23 · Probability for trading interviews

**Why trading firms care:** Jane Street, Optiver, IMC and SIG ask probability in every loop, and Citadel Securities often asks some. They test whether you reason cleanly under uncertainty, out loud, without a calculator: set up, find the trick (symmetry, linearity, conditioning, a recursion), check the answer is sane.

## 1. The toolkit (45 min)

| Tool | Use it when | One-line example |
|---|---|---|
| **Linearity of expectation** | Counting things; works even when events are dependent | Expected fixed points of a random permutation: n × 1/n = 1 |
| **Indicator variables** | "Expected number of ..." | Expected runs in n flips: 1 + (n − 1) × P(two neighbours differ) |
| **Conditioning on the first step** | Waiting times, games | E[flips to HH]: write equations for "just saw H" and "start" |
| **Symmetry** | Anything that looks hard but has no preferred direction | 100 passengers, lost seat: the last seat is the first passenger's or the last one's, equally likely |
| **Bayes** | "Given that ..." | P(A given B) = P(B given A) P(A) / P(B). Draw the 2 × 2 table with counts out of 1,000 |
| **Complement** | "At least one" | 1 − P(none) |
| **Backward induction** | Stopping games, "may reroll" | Value of the last roll first, then work back |
| **Geometric distribution** | "Until the first success" | Mean 1/p |
| **Variance of sums** | Market making, risk | Var(X + Y) = Var X + Var Y + 2 Cov; independent dice add variances |

Habits interviewers look for: state assumptions ("fair coin, independent"), give a quick estimate first, then the exact answer, then a sanity check (limits, small cases, symmetry). Say fractions, not long decimals.

**Kelly criterion** (they ask it at market-making shops): with win probability p on an even-money bet, bet the fraction f = p − q = 2p − 1 of your bankroll. In general f = (bp − q) / b for odds b to 1. Betting more than twice Kelly has negative expected log growth: you go broke slowly even with an edge.

## 2. Exercise (45 min code, then problems)

In `solution.hpp`, write the two optimal-stopping games exactly with dynamic programming: `dice_stop_value(rolls)` and `red_black_value(red, black)`. The test prints the value of the full-deck red/black game; it is a classic interview question, and now you know the number and how to derive it.
```bash
cmake --build build --target day23_test && ./build/day23_test
```

Then **10 problems a day** for the rest of the week: the list below, then the Green Book (Xinfeng Zhou, *A Practical Guide to Quantitative Finance Interviews*), chapters 2 to 4. Time each one; aim for under 3 minutes out loud. Cover the answers.

## 3. Problems

<details><summary>1. Expected number of rolls of a fair die until the first 6?</summary>6 (geometric with p = 1/6).</details>

<details><summary>2. Expected number of rolls until you have seen all six faces?</summary>6 × (1 + 1/2 + 1/3 + 1/4 + 1/5 + 1/6) = 14.7. After k distinct faces, a new one takes 6/(6 − k) rolls on average.</details>

<details><summary>3. You roll a die and are paid the face value. You may reroll once and must take the second roll. Fair price?</summary>Reroll on 1, 2, 3. Value = (4 + 5 + 6)/6 + (3/6) × 3.5 = 4.25.</details>

<details><summary>4. Same game with up to three rolls?</summary>Two rolls left is worth 4.25, so on the first roll keep only 5 or 6: (5 + 6)/6 + (4/6) × 4.25 = 14/3 ≈ 4.67.</details>

<details><summary>5. A family has two children, at least one is a boy. P(both boys)?</summary>1/3 (BB, BG, GB equally likely). If instead "the older one is a boy": 1/2.</details>

<details><summary>6. A disease affects 1% of people. The test catches 99% of cases and gives 5% false positives. You test positive. P(sick)?</summary>Out of 10,000: 99 true positives, 495 false positives. 99/594 = 1/6 ≈ 16.7%.</details>

<details><summary>7. One fair coin and one double-headed coin. Pick one at random, flip: heads. P(double-headed)? And after a second heads?</summary>2/3, then 4/5. Prior odds 1:1, each heads multiplies the odds by 2.</details>

<details><summary>8. Expected flips of a fair coin to get HH? To get HT?</summary>HH: 6. HT: 4. For HT, once you have an H you only wait for a T (2 more on average); for HH a T sends you back to the start.</details>

<details><summary>9. A and B take turns flipping a fair coin, A first. The first to flip heads wins. P(A wins)?</summary>p = 1/2 + (1/4) p, so p = 2/3.</details>

<details><summary>10. You have $k and bet $1 on fair coin flips until you reach $N or $0. P(reach N)?</summary>k/N. Your wealth is a martingale, so E[final] = k = N × P(win).</details>

<details><summary>11. Break a stick at two uniform random points. P(the three pieces form a triangle)?</summary>1/4. Every piece must be shorter than 1/2.</details>

<details><summary>12. Expected maximum of three independent uniform(0, 1)? Expected minimum?</summary>n/(n + 1) = 3/4 and 1/(n + 1) = 1/4.</details>

<details><summary>13. Expected distance between two uniform random points on [0, 1]?</summary>1/3.</details>

<details><summary>14. Expected number of fixed points of a random permutation of n items?</summary>1, for every n (linearity: n items, each fixed with probability 1/n).</details>

<details><summary>15. 100 passengers board in order; the first sits in a random seat, everyone else takes their own seat if free, otherwise a random one. P(the last passenger gets their own seat)?</summary>1/2. The last free seat is either the first passenger's or the last passenger's, and by symmetry each is equally likely.</details>

<details><summary>16. Shuffle a deck and deal until the first ace. Expected number of cards dealt (including the ace)?</summary>The 4 aces split the 48 other cards into 5 gaps of expected size 48/5 = 9.6, so 10.6.</details>

<details><summary>17. Expected value of the maximum of two dice?</summary>P(max = k) = (2k − 1)/36, so E = (1 + 6 + 15 + 28 + 45 + 66)/36 = 161/36 ≈ 4.47.</details>

<details><summary>18. Expected product of two dice?</summary>Independent, so 3.5 × 3.5 = 12.25.</details>

<details><summary>19. How many people for a better-than-even chance that two share a birthday?</summary>23 (about 50.7%).</details>

<details><summary>20. Three ants on the corners of a triangle each walk along a random edge. P(no two meet)?</summary>Only if all go clockwise or all anticlockwise: 2/8 = 1/4.</details>

<details><summary>21. Which is more likely: at least one 6 in 4 rolls of a die, or at least one double 6 in 24 rolls of two dice?</summary>The first: 1 − (5/6)^4 ≈ 0.518 vs 1 − (35/36)^24 ≈ 0.491.</details>

<details><summary>22. You win an even-money bet with probability 0.6. What fraction of your bankroll should you bet each time?</summary>Kelly: 2p − 1 = 0.2.</details>

<details><summary>23. Expected number of runs in 10 fair coin flips (HHTH has 3 runs)?</summary>1 + 9 × 1/2 = 5.5.</details>

<details><summary>24. Mean and standard deviation of the sum of three dice?</summary>Mean 10.5. Variance 3 × 35/12 = 8.75, standard deviation ≈ 2.96. You need this for tomorrow's market-making games.</details>

<details><summary>25. A shuffled deck, 26 red and 26 black. Turn cards one at a time: +1 for red, −1 for black. Stop whenever you like. Value of the game?</summary>About 2.62. Your `red_black_value(26, 26)` gives the exact figure. The key idea: when the remaining deck has more red than black you never stop, and the recursion V(r, b) = max(0, r/(r + b) × (1 + V(r − 1, b)) + b/(r + b) × (−1 + V(r, b − 1))) needs only 27 × 27 states.</details>
