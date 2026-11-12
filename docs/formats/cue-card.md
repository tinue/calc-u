# Cue card block

A **cue card** is the card in the window above the top row of keys that
labels the TI-58/58C/59's user-defined keys for a program. **[59]** only.

## 1. Kinds

There are three kinds of cue card. The kind follows from where the block
stands; a block never names its kind.

| Kind | Defined in | Look |
|---|---|---|
| **Stand-alone cue card** (the blank non-magnetic card) | a preset's `cue-card:` section ([preset.md §4.4](preset.md#44-cue-card-59)); the constant memory file ([media.md §4](media.md#4-constant-memory-file)) | a plain card, written by hand |
| **Magnetic card** (TI-59) | a magnetic card file ([media.md §1](media.md#1-magnetic-card)); an in-file card ([preset.md §3.1](preset.md#31-magnetic-cards-59-ti-59)) | a magnetic strip; each corner shows the bank of that side |
| **Module cue card** | a Solid State ROM module's `cue-cards:` ([modules.md §2.3](modules.md#23-cue-cards)), one per library program | pre-printed, in gold |

## 2. Fitting

The window shows one cue card, or none. **Fit** means making a cue card
the one shown. Fitting changes nothing in the machine.

The window has two layers: the machine's **own card**, and above it,
while a library program is selected, that program's **module cue card**.

| Event | Effect |
|---|---|
| The preset's `cue-card:` section | Fits that stand-alone cue card as the own card |
| A magnetic card has passed through the reader (`card:`, or the user feeding an armed card) | Fits that magnetic card as the own card |
| A magnetic card is **armed** (`arm:`, or the frontend) | Fits it as the own card; the user can feed it from there by tapping a corner. Arming does nothing electrically and changes nothing in the machine |
| A TI-58C starts from its constant memory file | Fits the stand-alone card stored there, if any, as the own card |
| A library program of the module in the slot is selected (`Pgm nn`) | Fits that program's module cue card over the own card, if the module has one for nn |
| The library program is deselected | The module cue card goes; the own card shows again |
| The frontend fits a card | As the matching row above |
| A cold boot (every preset starts with one) | No card |

A new own card replaces the previous one. Which library program is
selected is the machine's state, read from the machine.

## 3. Keys

| Key | Kinds | Value |
|---|---|---|
| `title` | all | The headline |
| `id` | module | The module and program ID right of the title, e.g. `ML-02` |
| `row-1` | all | Text in place of the A′–E′ label row |
| `row-2`, `row-2-r` | all | Text in place of the A–E label row; `row-2-r` stands right of `row-2` on the same line |
| `row-1-align` | all | `left`, `center` (default) or `right` |
| `row-2-align`, `row-2-r-align` | all | `left` (default), `center` or `right` |
| `style` | all | `none` (default) or `button`: a box around the row text |
| `pencil-color` | stand-alone, magnetic | `black` (default), `pencil` or `sharpie`, for the whole card |
| `A` … `E` | all | Labels of the A–E row |
| `A'` … `E'` | all | Labels of the A′–E′ row. `A′` … `E′` are accepted too |

- A key that doesn't apply to the kind is an error.
- A label row shows only when its row text is empty.
- A label `\blank` is an empty cell that the label before it spans into.
  An empty label is an empty cell that nothing spans into.
- The label keys are key IDs, compared in any case. `A'` is quoted as a
  key: `"A'": text`.

## 4. Text

`title`, `id`, the rows and every label expand the tokens below. Tokens
are matched as written (case matters), in the order of the table, so a
longer token is matched before a shorter one it begins with.

| Group | Tokens |
|---|---|
| Greek | `\lambda` λ, `\Lambda` Λ, `\sigma` σ, `\Sigma` Σ, `\mu` μ, `\theta` θ, `\Theta` Θ, `\alpha` α, `\beta` β, `\epsilon` ε, `\Epsilon` Ε, `\Delta` Δ, `\pi` π, `\Pi` Π |
| Overlined | `\xbar` x̄, `\ybar` ȳ, `\zbar` z̄, `\nbar` n̄, `\mbar` m̄ |
| Arrows | `\leftrightarrows` ⇄, `\leftrightarrow` ↔, `\updownarrow` ↕, `\leftarrow` ←, `\to` → |
| Signs | `\sqrt` √, `\cbrt` ∛, `\fourthroot` ∜, `\triangle` △, `\angle` ∠, `\inf` ∞, `\sum` ∑, `\product` ∏, `\integral` ∫, `\approx` ≈, `\neq` ≠, `\leq` ≤, `\geq` ≥ |
| Layout | `\blank`: an empty cell (§3) |
| Superscript | `^{-1}` ⁻¹, `^{-2}` ⁻², `^{-3}` ⁻³, `^{-}` ⁻, `^{+}` ⁺, `^{*}` ˣ, `^{n}` ⁿ, `^{T}` ᵀ, `^{x}` / `^{X}` ˣ, `^{y}` / `^{Y}` ʸ, `^{z}` / `^{Z}` ᶻ, `^{0}` … `^{9}` ⁰ … ⁹ |
| Subscript | `_{-1}` ₋₁, `_{-}` ₋, `_{+}` ₊, `_{0}` … `_{9}` ₀ … ₉, `_{i}` ᵢ, `_{j}` ⱼ, `_{a}` ₐ, `_{e}` ₑ, `_{o}` ₒ, `_{x}` / `_{X}` ₓ, `_{y}` / `_{Y}` ᵧ, `_{n}` ₙ, `_{m}` ₘ, `_{k}` ₖ |

Quoted values have no escapes, so a text is written in quotes when it
contains `: ` or ` #`, or starts with `[`, `{`, `"` or `'`:

```yaml
cue-cards:
  - program: 2
    title: "DETERMINANT, MATRIX, & SIMUL. EQ."
    id: ML-02
    "A'": "i; \to x_{i}"
    "B'": \to A^{-1}
    "D'": \blank
    A: n
    B: "j: a_{i}_{j}"
```
