# Games of Playing Cards (C++ Core)

このリポジトリは、トランプを用いたカードゲームの**コアロジックのみ**を提供する
C++20 ライブラリです。

UI、描画、入出力、プラットフォーム依存コードは含みません。

## 設計方針

- **ゲーム状態は不変**: 各ゲーム状態は `Card -> Position` の写像で表現し、状態遷移は常に新しい状態を返します
- **`Card` は value object**: 同一性は `CardId` に基づく値として扱い、可変状態は持ちません
- **ゲームごとの差異は `Position` に閉じ込める**: core 層は個別ゲームに依存せず、ゲーム固有の配置と制約を `Position` とルールで表現します
- **公開 API は `include/` に限定**: 安定したインタフェースのみを `include/` に置き、実装詳細は `src/` に配置します

## 公開 API

基礎となる公開 API は次の名前空間にあります。

- `shlab::gopc::playing_cards`
- `shlab::gopc::games`
- `shlab::gopc::games::<game>`

`Card` は `CardId` を正規の識別子として保持し、そこから `suit`、`rank`、joker 判定を導出します。
通常カードは `Card::of(suit, rank)`、joker 系は `Card::from_id(CardId::Joker)` で生成できます。

利用時は、基礎型を `playing_cards` から、各ゲームを `games/<game>.hpp` から参照します。

```cpp
#include "shlab/gopc/playing_cards/card.hpp"
#include "shlab/gopc/games/klondike.hpp"
```

## 現在の実装範囲

Playing Cards の基礎型に加えて、次のゲームを公開しています。詳細は各ゲーム README を参照してください。

| Game | 概要 | 詳細 |
| --- | --- | --- |
| Klondike | stock / waste / tableau / foundation を持つ基本 Klondike 実装 | [include/shlab/gopc/games/klondike/README.md](include/shlab/gopc/games/klondike/README.md) |
| Beleaguered Castle | foundation と 8 tableau columns を持つ Beleaguered Castle 実装 | [include/shlab/gopc/games/beleaguered_castle/README.md](include/shlab/gopc/games/beleaguered_castle/README.md) |
| Canfield | reserve、stock、waste、foundation base rank を扱う Canfield 実装 | [include/shlab/gopc/games/canfield/README.md](include/shlab/gopc/games/canfield/README.md) |
| Calculation | A / 2 / 3 / 4 を起点とする foundation を持つ Calculation 実装 | [include/shlab/gopc/games/calculation/README.md](include/shlab/gopc/games/calculation/README.md) |
| FreeCell | cells、foundations、tableau を持つ FreeCell 実装 | [include/shlab/gopc/games/freecell/README.md](include/shlab/gopc/games/freecell/README.md) |
| Four Leaf Clover | 4x4 の場札から同一スート合計 15 または同一スート JQK を除去する実装 | [include/shlab/gopc/games/four_leaf_clover/README.md](include/shlab/gopc/games/four_leaf_clover/README.md) |
| Pyramid | field、deck、discard、hand に加えて joker 用の位置を持つ Pyramid 実装 | [include/shlab/gopc/games/pyramid/README.md](include/shlab/gopc/games/pyramid/README.md) |
| Simple Simon | 10 tableau columns と foundation を持つ Simple Simon 実装 | [include/shlab/gopc/games/simple_simon/README.md](include/shlab/gopc/games/simple_simon/README.md) |
| Yukon | stock / waste を持たず、open 状態を持つ tableau で表現する Yukon 実装 | [include/shlab/gopc/games/yukon/README.md](include/shlab/gopc/games/yukon/README.md) |
| Golf | hand / deck / field を持ち、`can_loop` を切り替えられる Golf 実装 | [include/shlab/gopc/games/golf/README.md](include/shlab/gopc/games/golf/README.md) |

## ディレクトリ構成

```text
include/   公開インタフェース
src/       実装詳細
tests/     テストコード
```

## ビルド

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

`cmake` が未導入の環境では、最小確認として次のように直接コンパイルできます。

```bash
clang++ -std=c++20 -Wall -Wextra -Werror -Iinclude \
  src/shlab/gopc/playing_cards/card.cpp \
  tests/shlab/gopc/playing_cards/card_test.cpp \
  -o build/card_tests
./build/card_tests
```
