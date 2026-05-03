# Canfield

`shlab::gopc::games::canfield::Canfield` は、Canfield の状態を
`Card -> Position` の不変写像として保持し、foundation の base rank も公開します。

## 公開 API

- umbrella header: `shlab/gopc/games/canfield.hpp`
- namespace: `shlab::gopc::games::canfield`
- public types: `Canfield`, `Position`, `Column`

## 状態モデル

- `Canfield::state_type` は `std::unordered_map<Card, Position>`
- `Position` は `Foundation`, `Stock`, `WastePile`, `Reserve`, `Tableau` の `std::variant`
- `Stock`, `WastePile`, `Reserve`, `Tableau` は `number` を持つ
- `Tableau` は `column` で列を区別する
- `foundation_base_rank()` で foundation の起点 rank を取得できる

## 主要操作

- `deal(std::span<const Card>)`
- `draw()`
- `redeal()`
- `move_to_foundation(const Card&)`
- `move_to_tableau(const Card&, Column)`

すべての遷移 API は既存状態を変更せず、新しい `Canfield` を返します。

## 現在の実装範囲

- クラシック寄りの Canfield を対象とする
- reserve 13 枚、draw 3、waste 再循環を扱う
- foundation の base rank を状態として保持する
- empty tableau の reserve 補充を扱う

