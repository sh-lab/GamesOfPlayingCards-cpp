# Klondike

`shlab::gopc::games::klondike::Klondike` は、Klondike の状態を
`Card -> Position` の不変写像として表現する公開 API です。

## 公開 API

- umbrella header: `shlab/gopc/games/klondike.hpp`
- namespace: `shlab::gopc::games::klondike`
- public types: `Klondike`, `Position`, `Column`

## 状態モデル

- `Klondike::state_type` は `std::unordered_map<Card, Position>`
- `Position` は `Foundation`, `Stock`, `WastePile`, `Tableau` の `std::variant`
- `Stock` と `WastePile` は `number` で順序を表現
- `Tableau` は `column`, `number`, `open` を持つ

## 主要操作

- `deal(std::span<const Card>)`
- `open(const Card&)`
- `draw(const Card&)`
- `move_to_foundation(const Card&)`
- `redeal()`
- `move_to_tableau(const Card&, Column)`

すべての遷移 API は既存状態を変更せず、新しい `Klondike` を返します。

## 現在の実装範囲

- 基本 Klondike を対象とする
- `stock_count()`, `is_pre_win()`, `is_win()` を公開している
- tableau の開閉状態は `Tableau::open` で保持する

