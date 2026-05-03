# Pyramid

`shlab::gopc::games::pyramid::Pyramid` は、Pyramid の状態を
`Card -> Position` の不変写像として表現します。

## 公開 API

- umbrella header: `shlab/gopc/games/pyramid.hpp`
- namespace: `shlab::gopc::games::pyramid`
- public types: `Pyramid`, `Position`

## 状態モデル

- `Pyramid::state_type` は `std::unordered_map<Card, Position>`
- `Position` は `Deck`, `Discard`, `Field`, `Hand`, `FreeSpace`, `Outside` の `std::variant`
- `Deck` と `Discard` は `number` で順序を表現する
- `Field` は `row` と `number` を持つ
- `Hand` は現在選択中のカード、`FreeSpace` / `Outside` は joker 関連位置として使う

## 主要操作

- `deal(std::span<const Card>)`
- `can_select_for_remove(const Card&)`
- `remove(const Card&)`
- `remove(const Card&, const Card&)`
- `redeal()`
- `draw(const Card&)`

すべての遷移 API は既存状態を変更せず、新しい `Pyramid` を返します。

## 現在の実装範囲

- この実装では 52 枚の標準カードに joker 2 枚を加える
- joker は `FreeSpace` または `Outside` にのみ存在できる
- 単独除去と 2 枚組除去の両方を公開 API として提供する
