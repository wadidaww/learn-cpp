#!/usr/bin/env bash
# drill.sh <dayNN> [check]
# Without "check": copy the day's original skeleton (as first committed) and tests into drills/<dayNN>-<time>/.
# With "check": build the newest drill for that day with ASan+UBSan and run its tests.
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
day=${1:?usage: drill.sh dayNN [check]}
src=$(find "$root"/week? -maxdepth 1 -type d -name "${day}-*" | head -1)
[ -n "$src" ] || { echo "no folder for $day"; exit 1; }
[ -f "$src/test.cpp" ] || { echo "$day has no tests to drill"; exit 1; }
cflags="-std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Wconversion -march=native -pthread -fsanitize=address,undefined -fno-sanitize-recover=all -I$root/common"

if [ "${2:-}" = check ]; then
    dir=$(ls -d "$root"/drills/"$day"-* 2>/dev/null | sort | tail -1)
    [ -n "$dir" ] || { echo "no drill for $day yet"; exit 1; }
    echo "building $dir"
    g++ $cflags "$dir/test.cpp" -o "$dir/test" && "$dir/test"
    exit
fi

dir="$root/drills/$day-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$dir"
cp "$src/test.cpp" "$dir/"
rel=${src#"$(git -C "$root" rev-parse --show-toplevel)/"}
first=$(git -C "$root" log --diff-filter=A --format=%H -- "$src/solution.hpp" | tail -1)
if [ -n "$first" ]; then
    git -C "$root" show "$first:$rel/solution.hpp" > "$dir/solution.hpp"
else
    cp "$src/solution.hpp" "$dir/"
    echo "warning: $src/solution.hpp is not committed yet; copied the current file"
fi
echo "drill ready: $dir"
echo "timer starts now. When the tests should pass: $0 $day check"
