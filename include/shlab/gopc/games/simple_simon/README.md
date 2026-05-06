# Simple Simon

`shlab::gopc::games::simple_simon::SimpleSimon` は、Simple Simon の状態を
`Card -> Position` の不変写像として表現します。

## 公開 API

- umbrella header: `shlab/gopc/games/simple_simon.hpp`
- namespace: `shlab::gopc::games::simple_simon`
- public types: `SimpleSimon`, `Position`, `Column`

## 状態モデル

- `SimpleSimon::state_type` は `shlab::gopc::utility::CardState<Position, 52>`
- `Position` は `Foundation` と `Tableau` を保持する tagged union 型で、`kind()` / `is<T>()` / `get<T>()` / `get_if<T>()` / `visit(...)` で扱える
- `Tableau` は `column` と `number` を持つ

## 主要操作

- `deal(std::span<const Card>)`
- `move_to_foundation(const Card&)`
- `move_to_tableau(const Card&, Column)`

すべての遷移 API は既存状態を変更せず、新しい `SimpleSimon` を返します。

## 現在の実装範囲

- 10 tableau columns に `8, 8, 8, 7, 6, 5, 4, 3, 2, 1` で配る
- tableau は rank 降順・スート不問で受ける
- 一手で動かせるのは同一スート降順の tail のみとする
