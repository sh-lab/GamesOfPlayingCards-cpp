# Calculation

`shlab::gopc::games::calculation::Calculation` は、Calculation の状態を
`Card -> Position` の不変写像で表現します。

## 公開 API

- umbrella header: `shlab/gopc/games/calculation.hpp`
- namespace: `shlab::gopc::games::calculation`
- public types: `Calculation`, `Position`, `FoundationColumn`, `TableauColumn`

## 状態モデル

- `Calculation::state_type` は `std::unordered_map<Card, Position>`
- `Position` は `Foundation`, `Stock`, `WastePile`, `Tableau` の `std::variant`
- `Foundation` は `FoundationColumn` を持つ
- `Tableau` は `TableauColumn` と `number` を持つ
- `Stock` は `number` を持ち、`WastePile` は単一位置として表現する

## 主要操作

- `deal(std::span<const Card>)`
- `next_rank(FoundationColumn)`
- `draw(const Card&)`
- `move_to_tableau(const Card&, TableauColumn)`
- `move_to_foundation(const Card&, FoundationColumn)`

すべての遷移 API は既存状態を変更せず、新しい `Calculation` を返します。

## 現在の実装範囲

- この実装では A / 2 / 3 / 4 を foundation の起点に置く
- `next_rank()` で各 foundation に次に置ける rank を取得できる
- `WastePile` は常に 0 枚または 1 枚だけ存在できる
