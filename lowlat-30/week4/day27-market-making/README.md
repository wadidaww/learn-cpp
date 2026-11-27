# Day 27 · Market-making games and probability drills

**Why trading firms care:** Optiver, IMC, SIG, Jane Street and Citadel Securities run "make me a market" games. They are not testing trivia. They watch whether you centre on an expected value, size the width to your uncertainty, update when someone trades with you, and manage your position, all while talking.

## 1. How to play (30 min)

**"Make me a market on X"** means: say a bid and an ask, e.g. "45 at 55" (you buy at 45, sell at 55), and a size if asked. Then the interviewer buys from you, sells to you, or passes, and may reveal information.

1. **Centre on the expected value.** Compute it, or estimate it quickly and say so.
2. **Width from uncertainty.** Wider when you know less or the payoff varies more; a standard deviation either side is a reasonable first width for a one-shot game. Tighten as you learn.
3. **Every trade is information.** If they buy at your ask, they may know it is worth more: move your market **up**. That is adverse selection (day 20) in one sentence.
4. **Skew for inventory.** Long already? Lower both prices so you are more likely to sell and less likely to buy more. Today's exercise is exactly this rule.
5. **Know your limits.** Say the maximum you are willing to hold, and stop quoting that side when you reach it.
6. **Talk.** "My fair value is 10.5, the standard deviation is about 3, so I'll make 8 at 13. You bought, so I think you have information: I'm moving to 10 at 14."

**Estimation markets** ("make a market on the population of Sydney", "on the number of piano keys in this building") are the same game: give a wide first market, break the estimate into factors, tighten as you reason, and never be ashamed of a wide market when you genuinely do not know.

## 2. Exercise (45 min)

Implement `make_quote` in `solution.hpp`: fair value plus inventory skew, half-spread, tick rounding toward the safe side, position limits. It is the core of a real quoting engine, just without the fair-value model. Note the integer rounding for negative numbers: C++ integer division truncates toward zero, which is the wrong direction for one side.

```bash
cmake --build build --target day27_test && ./build/day27_test
```

## 3. Drills (one hour: play each out loud, then check)

<details><summary>1. Make a market on the sum of two dice.</summary>Fair 7, standard deviation about 2.4 (variance 35/6). A first market like 5 at 9, then tighten. If they buy at 9 repeatedly with no information, they are just testing whether you hold your fair value: tighten toward 6.5 at 7.5 and keep it centred.</details>

<details><summary>2. Same game, but I tell you the first die is a 5.</summary>Fair 5 + 3.5 = 8.5, standard deviation about 1.7. Market around 7.5 at 9.5, then tighter.</details>

<details><summary>3. Make a market on the maximum of two dice.</summary>Fair 161/36 ≈ 4.47. Skewed distribution (P(max = 6) = 11/36). Market about 3.5 at 5.5.</details>

<details><summary>4. Market on the sum of three dice. I buy 5 from you at your ask, then 5 more. What do you do?</summary>Fair 10.5, standard deviation about 3. After the first buy, either they know something or they are testing you. Move up (say from 9 at 12 to 10 at 13) and reduce size. After the second, move up again and say your position limit. Explain the reasoning: two buys at the ask is information.</details>

<details><summary>5. I have a card from a standard deck. Make a market on its rank (A = 1 ... K = 13).</summary>Fair 7, uniform on 1..13, standard deviation about 3.7. Market like 5 at 9. If I say "it's a face card", fair is 12, market 11 at 13.</details>

<details><summary>6. Make a market on the number of heads in 10 fair coin flips. I flip three and they are all heads.</summary>Before: fair 5, standard deviation about 1.6. After: 3 + 7 × 0.5 = 6.5, standard deviation about 1.3.</details>

<details><summary>7. A game pays the square of one die roll. Fair price?</summary>E[X²] = 91/6 ≈ 15.17. Not 3.5² = 12.25: E[X²] = Var X + (E X)². Common trap.</details>

<details><summary>8. You are long 20 contracts at the limit of your position size. A customer wants to buy 10 from you. What do you quote?</summary>Gladly sell: it reduces your risk. Quote a tighter, lower ask than usual to make sure you get the trade, and keep your bid off (you are at your limit).</details>

<details><summary>9. You are making a market and the true value suddenly jumps. Who fills you first?</summary>The fastest informed traders, at your stale price. This is why market makers race to cancel and why quote latency matters (day 20, latency arbitrage).</details>

<details><summary>10. Expected value of a bet: you pay 1 to roll a die; you win 3 on a 6, otherwise nothing. Would you play 100 times? What if you have 10 to your name?</summary>EV per game = 3/6 − 1 = −0.5. Do not play. If it were positive, bankroll and variance would decide how much to bet (Kelly, day 23).</details>

<details><summary>11. A coin is either fair or lands heads 75% of the time, equally likely. It lands heads. Make a market on the probability that the next flip is heads.</summary>P(biased given H) = 0.75 / (0.75 + 0.5) = 0.6. P(next H) = 0.6 × 0.75 + 0.4 × 0.5 = 0.65. Market around 0.6 at 0.7.</details>

<details><summary>12. Make a market on the number of times I need to roll a die to get a 6.</summary>Fair 6, but a long right tail (geometric, standard deviation about 5.5). Market wider on the upside: 4 at 9, for example. Say why the market is asymmetric.</details>

Finish with 10 problems from the Green Book you have not done yet, and 15 minutes of Zetamac. Log the score: it should be higher than on day 1.
