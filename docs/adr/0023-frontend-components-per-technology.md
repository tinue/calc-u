# Frontend components are shared per technology

Status: accepted (2026-11-09)

The library holds one Qt6 component set and one React component set,
built around the capabilities (ADR 0007): media pickers, the printer's
paper panel, the debug panel, the settings dialog, the control bar and the
menus. Each product composes its window from them plus its product
material (faceplate, hit map, layout); layouts stay per product, because
the portrait TI-59 and the landscape PC-1600 differ by nature. The Swift
frontend follows the same concepts natively and is the component set a
later Calc-U-1600 Swift frontend would use.

## Considered options

- Shared concepts only, separate frontend code per product: rejected;
  the same widgets would be written twice per technology.
- One generic frontend per technology driven by machine descriptors
  (MAME-like): the most sharing, but the two products' layouts differ too
  much for it today.
