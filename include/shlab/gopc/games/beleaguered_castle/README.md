# Beleaguered Castle

`shlab::gopc::games::beleaguered_castle::BeleagueredCastle` は、
Beleaguered Castle の状態を `Card -> Position` の不変写像で表現します。

## 公開 API

- umbrella header: `shlab/gopc/games/beleaguered_castle.hpp`
- namespace: `shlab::gopc::games::beleaguered_castle`
- public types: `BeleagueredCastle`, `Position`, `Column`

## 状態モデル

- `BeleagueredCastle::state_type` は `std::unordered_map<Card, Position>`
- `Position` は `Foundation` と `Tableau` の `std::variant`
- `Tableau` は `column` と `number` を持つ

## 主要操作

- `deal(std::span<const Card>)`
- `move_to_foundation(const Card&)`
- `move_to_tableau(const Card&, Column)`

すべての遷移 API は既存状態を変更せず、新しい `BeleagueredCastle` を返します。

## 現在の実装範囲

- 4 aces を foundation に置く構成を前提とする
- 8 tableau columns を rank 降順・スート無関係で構築する
- 空列には任意のカードを置ける

