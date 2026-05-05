# Simple Simon

`shlab::gopc::games::simple_simon::SimpleSimon` は、Simple Simon の状態を
`Card -> Position` の不変写像として表現します。

## 公開 API

- umbrella header: `shlab/gopc/games/simple_simon.hpp`
- namespace: `shlab::gopc::games::simple_simon`
- public types: `SimpleSimon`, `Position`, `Column`

## 状態モデル

- `SimpleSimon::state_type` は `std::unordered_map<Card, Position>`
- `Position` は `Foundation` と `Tableau` の `std::variant`
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

