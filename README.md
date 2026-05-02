# Games of Playing Cards (C++ Core)

このリポジトリは、トランプを用いたカードゲームの**コアロジックのみ**を提供する
C++21 ライブラリです。

UI、入出力、プラットフォーム依存コードは含みません。

## 設計方針

- **ゲーム状態は不変**: 状態遷移は既存状態を変更せず、新しい状態を返す
- **Card は value object**: 公開 API では安定した値型として扱う
- **ゲーム差分は Position に閉じ込める**: core は個別ゲームに依存しない
- **公開 API は `include/` に限定**: 実装詳細は `src/` に配置する

## 現在の実装範囲

現在は、Playing Cards の基礎、基本 Klondike、FreeCell、Golf を実装しています。

- `Suit`
- `Rank`
- `CardId`
- `Card`
- `games::klondike::Klondike`
- `games::klondike::Position` (`Foundation`, `Stock`, `WastePile`, `Tableau`, `Column`)
- `games::freecell::FreeCell`
- `games::freecell::Position` (`Cell`, `Foundation`, `Tableau`, `Column`)
- `games::golf::Golf`
- `games::golf::Position` (`Hand`, `Deck`, `Field`, `Lane`)

公開 API は次の階層を前提にしています。

- `shlab::gopc::playing_cards`
- `shlab::gopc::games`
- `shlab::gopc::games::freecell`
- `shlab::gopc::games::golf`
- `shlab::gopc::games::klondike`

`Card` は `CardId` を正規の識別子として保持し、そこから `suit`, `rank`, joker 判定を導出します。
通常カードは `Card::of(suit, rank)`、joker 系は `Card::from_id(CardId::Joker)` で生成できます。
個別カードは `CardId` を唯一の識別子として扱い、必要に応じて `Card::from_id(...)` から生成します。

`Klondike` は `Card -> Position` の写像を不変状態として保持し、`open`, `draw`, `move_to_foundation`, `redeal`, `move_to_tableau` は既存状態を変更せず、新しい状態を返します。

`FreeCell` も同様に `Card -> Position` の不変状態として保持し、`move_to_cell`, `move_to_foundation`, `move_to_tableau` は既存状態を変更せず、新しい状態を返します。

`Golf` も同様に `Card -> Position` の不変状態として保持し、`move_to_hand` は deck / field から hand への遷移後の新しい状態を返します。`can_loop` ルールと joker あり/なしの deck を扱えます。

## ディレクトリ構成

```text
include/   公開インタフェース
src/       実装詳細
tests/     テストコード
refcs/     設計意図を確認するための C# 参照実装
```

## ビルド

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

`cmake` が未導入の環境では、最小確認として次のように直接コンパイルできます。

```bash
clang++ -std=c++2b -Wall -Wextra -Werror -Iinclude \
  src/shlab/gopc/playing_cards/card.cpp \
  tests/shlab/gopc/playing_cards/card_test.cpp \
  -o build/card_tests
./build/card_tests
```
