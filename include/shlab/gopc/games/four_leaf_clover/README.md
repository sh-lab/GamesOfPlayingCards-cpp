# Four Leaf Clover

`shlab::gopc::games::four_leaf_clover::FourLeafClover` は、
Four Leaf Clover の状態を `Card -> Position` の不変写像として表現します。

## 公開 API

- umbrella header: `shlab/gopc/games/four_leaf_clover.hpp`
- namespace: `shlab::gopc::games::four_leaf_clover`
- public types: `FourLeafClover`, `Position`

## 状態モデル

- `FourLeafClover::state_type` は `std::unordered_map<Card, Position>`
- `Position` は `Field`, `Stock`, `Removed` の `std::variant`
- `Field` は 4x4 の場札スロットを row-major の `number` で表す
- `Stock` は補充順の `number` を持つ

## 主要操作

- `deal(std::span<const Card>)`
- `can_remove(std::span<const Card>)`
- `remove(std::span<const Card>)`
- `stock_count()`
- `is_win()`

すべての遷移 API は既存状態を変更せず、新しい `FourLeafClover` を返します。

## 現在の実装範囲

- 使用札は 10 と joker を除いた 48 枚
- 初期配置は 4x4 の場札 16 枚、残りは `Stock`
- 同一スートで合計 15 になる数札集合をまとめて除去できる
- 同一スートの J / Q / K が揃った場合に 3 枚まとめて除去できる
- 除去後の空きスロットは `Stock` から左上順に自動補充される
